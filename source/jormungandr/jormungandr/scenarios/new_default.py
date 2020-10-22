# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

from copy import deepcopy
import itertools
import logging
from flask_restful import abort
from flask import g
from jormungandr.scenarios import simple, journey_filter, helpers
from jormungandr.scenarios.utils import (
    journey_sorter,
    change_ids,
    updated_request_with_default,
    get_or_default,
    fill_uris,
    gen_all_combin,
    get_pseudo_duration,
    mode_weight,
    switch_back_to_ridesharing,
    nCr,
    updated_common_journey_request_with_default,
)
from navitiacommon import type_pb2, response_pb2, request_pb2
from jormungandr.scenarios.qualifier import (
    min_from_criteria,
    arrival_crit,
    departure_crit,
    duration_crit,
    transfers_crit,
    nonTC_crit,
    trip_carac,
    has_no_car,
    has_car,
    has_pt,
    has_no_bike,
    has_bike,
    has_no_bss,
    has_bss,
    non_pt_journey,
    has_walk,
    and_filters,
    get_ASAP_journey,
    comfort_crit,
)
import numpy as np
import collections
from jormungandr.utils import date_to_timestamp, copy_flask_request_context, copy_context_in_greenlet_stack
from jormungandr.scenarios.simple import get_pb_data_freshness
import gevent, gevent.pool
from jormungandr import app
from jormungandr.autocomplete.geocodejson import GeocodeJson
from jormungandr import global_autocomplete
from jormungandr.new_relic import record_custom_parameter
from jormungandr import fallback_modes

from six.moves import filter
from six.moves import range
from six.moves import zip
from functools import cmp_to_key

SECTION_TYPES_TO_RETAIN = {response_pb2.PUBLIC_TRANSPORT, response_pb2.STREET_NETWORK}
JOURNEY_TYPES_TO_RETAIN = ['best', 'comfort', 'non_pt_walk', 'non_pt_bike', 'non_pt_bss']
STREET_NETWORK_MODE_TO_RETAIN = {response_pb2.Ridesharing, response_pb2.Car, response_pb2.Bike, response_pb2.Bss}


def get_kraken_calls(request):
    """
    return a list of tuple (departure fallback mode, arrival fallback mode, direct_path_type)
    from the request dict.

    for the moment it's a simple stuff:
    if there is only one elt in {first|last}_section_mode we take those
    else we do the cartesian product and remove forbidden association.

    The good thing to do would be to have API param to be able to better handle this
    """
    dep_modes = request['origin_mode']
    arr_modes = request['destination_mode']
    direct_path_type = request.get('direct_path', 'indifferent')
    direct_path_mode = request.get('direct_path_mode', [])

    res = set()
    if direct_path_type != "none":
        for mode in direct_path_mode:
            # to avoid duplicate tuples (mode1, mode2, "indifferent") and (mode1, mode2, "only")
            # which will trigger 2 exact same computations
            if mode not in dep_modes:
                res.add((mode, mode, "only"))

    if len(dep_modes) == len(arr_modes) == 1:
        res.add((dep_modes[0], arr_modes[0], direct_path_type))
        return res

    # this allowed_combinations is temporary, it does not handle all the use cases at all
    # We don't want to do ridesharing - ridesharing journeys
    combination_mode = {
        c for c in fallback_modes.allowed_combinations if c in itertools.product(dep_modes, arr_modes)
    }

    if not combination_mode:
        abort(
            404,
            message='the asked first_section_mode[] ({}) and last_section_mode[] '
            '({}) combination is not yet supported'.format(dep_modes, arr_modes),
        )

    for dep_mode, arr_mode in combination_mode:
        res.add((dep_mode, arr_mode, direct_path_type))

    return res


def create_pb_request(requested_type, request, dep_mode, arr_mode, direct_path_type):
    """Parse the request dict and create the protobuf version"""
    # TODO: bench if the creation of the request each time is expensive
    req = request_pb2.Request()
    req.requested_api = requested_type
    req._current_datetime = date_to_timestamp(request['_current_datetime'])

    if "origin" in request and request["origin"]:
        if requested_type != type_pb2.NMPLANNER:
            origins, durations = ([request["origin"]], [0])
        else:
            # in the n-m query, we have several origin points, with their corresponding access duration
            origins, durations = (request["origin"], request["origin_access_duration"])
        for place, duration in zip(origins, durations):
            location = req.journeys.origin.add()
            location.place = place
            location.access_duration = duration
    if "destination" in request and request["destination"]:
        if requested_type != type_pb2.NMPLANNER:
            destinations, durations = ([request["destination"]], [0])
        else:
            destinations, durations = (request["destination"], request["destination_access_duration"])
        for place, duration in zip(destinations, durations):
            location = req.journeys.destination.add()
            location.place = place
            location.access_duration = duration

    req.journeys.datetimes.append(request["datetime"])  # TODO remove this datetime list completly in another PR

    req.journeys.clockwise = request["clockwise"]
    sn_params = req.journeys.streetnetwork_params
    sn_params.max_walking_duration_to_pt = request["max_walking_duration_to_pt"]
    sn_params.max_bike_duration_to_pt = request["max_bike_duration_to_pt"]
    sn_params.max_bss_duration_to_pt = request["max_bss_duration_to_pt"]
    sn_params.max_car_duration_to_pt = request["max_car_duration_to_pt"]
    # So far, ridesharing is the only mode using 'car_no_park' in new_default
    sn_params.max_car_no_park_duration_to_pt = request["max_ridesharing_duration_to_pt"]
    sn_params.walking_speed = request["walking_speed"]
    sn_params.bike_speed = request["bike_speed"]
    sn_params.car_speed = request["car_speed"]
    sn_params.bss_speed = request["bss_speed"]
    sn_params.car_no_park_speed = request["ridesharing_speed"]
    sn_params.origin_filter = request.get("origin_filter", "")
    sn_params.destination_filter = request.get("destination_filter", "")
    # we always want direct path, even for car
    sn_params.enable_direct_path = True

    # settings fallback modes
    sn_params.origin_mode = dep_mode
    sn_params.destination_mode = arr_mode

    # If we only want direct_paths, max_duration(time to pass in pt) is zero
    if direct_path_type == "only":
        req.journeys.max_duration = 0
    else:
        req.journeys.max_duration = request["max_duration"]

    req.journeys.max_transfers = request["max_transfers"]
    if request["max_extra_second_pass"]:
        req.journeys.max_extra_second_pass = request["max_extra_second_pass"]
    req.journeys.wheelchair = request["wheelchair"] or False  # default value is no wheelchair
    req.journeys.realtime_level = get_pb_data_freshness(request)

    if "details" in request and request["details"]:
        req.journeys.details = request["details"]

    req.journeys.walking_transfer_penalty = request['_walking_transfer_penalty']

    for forbidden_uri in get_or_default(request, "forbidden_uris[]", []):
        req.journeys.forbidden_uris.append(forbidden_uri)
    for allowed_id in get_or_default(request, "allowed_id[]", []):
        req.journeys.allowed_id.append(allowed_id)

    req.journeys.bike_in_pt = (dep_mode == 'bike') and (arr_mode == 'bike')

    if request["free_radius_from"]:
        req.journeys.free_radius_from = request["free_radius_from"]
    if request["free_radius_to"]:
        req.journeys.free_radius_to = request["free_radius_to"]

    if request["min_nb_journeys"]:
        req.journeys.min_nb_journeys = request["min_nb_journeys"]

    req.journeys.night_bus_filter_max_factor = request['_night_bus_filter_max_factor']
    req.journeys.night_bus_filter_base_factor = request['_night_bus_filter_base_factor']

    if request['timeframe_duration']:
        req.journeys.timeframe_duration = int(request['timeframe_duration'])

    req.journeys.depth = request['depth']

    return req


def _has_pt(j):
    return any(s.type == response_pb2.PUBLIC_TRANSPORT for s in j.sections)


def sort_journeys(resp, journey_order, clockwise):
    if resp.journeys:
        resp.journeys.sort(key=cmp_to_key(journey_sorter[journey_order](clockwise=clockwise)))


def compute_car_co2_emission(pb_resp, api_request, instance, request_id):
    if not pb_resp.journeys:
        return
    car = next((j for j in pb_resp.journeys if helpers.is_car_direct_path(j)), None)
    if car is None or not car.HasField('co2_emission'):
        # if there is no car journey found, we request kraken to give us an estimation of
        # co2 emission
        co2_estimation = instance.georef.get_car_co2_emission_on_crow_fly(
            api_request['origin'], api_request['destination'], request_id
        )
        if co2_estimation:
            # Assign car_co2_emission into the resp, these value will be exposed in the final result
            pb_resp.car_co2_emission.value = co2_estimation.value
            pb_resp.car_co2_emission.unit = co2_estimation.unit
    else:
        # Assign car_co2_emission into the resp, these value will be exposed in the final result
        pb_resp.car_co2_emission.value = car.co2_emission.value
        pb_resp.car_co2_emission.unit = car.co2_emission.unit


def tag_ecologic(resp):
    # if there is no available car_co2_emission in resp, no tag will be assigned
    if resp.car_co2_emission.value and resp.car_co2_emission.unit:
        for j in resp.journeys:
            if not j.HasField('co2_emission'):
                j.tags.append('ecologic')
                continue
            if j.co2_emission.unit != resp.car_co2_emission.unit:
                continue
            if j.co2_emission.value < resp.car_co2_emission.value * 0.5:
                j.tags.append('ecologic')


def _tag_direct_path(responses):
    street_network_mode_tag_map = {
        response_pb2.Walking: ['non_pt_walking'],
        response_pb2.Bike: ['non_pt_bike'],
        response_pb2.Taxi: ['non_pt_taxi'],
        response_pb2.Car: ['non_pt_car'],
        response_pb2.CarNoPark: ['non_pt_car_no_park'],
        response_pb2.Ridesharing: ['non_pt_ridesharing'],
    }

    for j in itertools.chain.from_iterable(r.journeys for r in responses):
        if all(s.type != response_pb2.PUBLIC_TRANSPORT for s in j.sections):
            j.tags.extend(['non_pt'])

        # TODO: remove that (and street_network_mode_tag_map) when NMP stops using it
        # if there is only one section
        if 'non_pt' in j.tags:
            if j.sections[0].type == response_pb2.STREET_NETWORK and hasattr(j.sections[0], 'street_network'):
                tag = street_network_mode_tag_map.get(j.sections[0].street_network.mode)
                if tag:
                    j.tags.extend(tag)


def _is_bike_section(s):
    return (
        s.type == response_pb2.CROW_FLY or s.type == response_pb2.STREET_NETWORK
    ) and s.street_network.mode == response_pb2.Bike


def _is_pt_bike_accepted_section(s):
    bike_ok = type_pb2.hasEquipments.has_bike_accepted
    return (
        s.type == response_pb2.PUBLIC_TRANSPORT
        and bike_ok in s.pt_display_informations.has_equipments.has_equipments
        and bike_ok in s.origin.stop_point.has_equipments.has_equipments
        and bike_ok in s.destination.stop_point.has_equipments.has_equipments
    )


def _is_bike_in_pt_journey(j):
    bike_indifferent = [
        response_pb2.boarding,
        response_pb2.landing,
        response_pb2.WAITING,
        response_pb2.TRANSFER,
        response_pb2.ALIGHTING,
    ]
    return _has_pt(j) and all(
        _is_bike_section(s) or _is_pt_bike_accepted_section(s) or s.type in bike_indifferent for s in j.sections
    )


def _tag_bike_in_pt(responses):
    """
    we tag as 'bike _in_pt' journeys that are using bike as start AND end fallback AND
    that allow carrying bike in transport (and journey has to include PT)
    """
    for j in itertools.chain.from_iterable(r.journeys for r in responses):
        if _is_bike_in_pt_journey(j):
            j.tags.extend(['bike_in_pt'])


def tag_journeys(resp):
    """
    tag the journeys
    """
    tag_ecologic(resp)


def update_durations(pb_resp):
    """
    update journey.duration and journey.durations.total
    """
    if not pb_resp.journeys:
        return
    for j in pb_resp.journeys:
        j.duration = sum(s.duration for s in j.sections)
        j.durations.total = j.duration


def update_total_co2_emission(pb_resp):
    """
    update journey.co2_emission.value
    """
    if not pb_resp.journeys:
        return
    for j in pb_resp.journeys:
        j.co2_emission.value = sum(s.co2_emission.value for s in j.sections)


def _get_section_id(section):
    street_network_mode = None
    if section.type in SECTION_TYPES_TO_RETAIN:
        if getattr(section.street_network, 'mode', None) in STREET_NETWORK_MODE_TO_RETAIN:
            street_network_mode = section.street_network.mode
    return section.uris.line, street_network_mode, section.type


def _build_candidate_pool_and_sections_set(journeys):
    sections_set = set()
    candidates_pool = list()
    idx_of_jrny_must_keep = list()

    for (i, jrny) in enumerate(journeys):
        if jrny.type in JOURNEY_TYPES_TO_RETAIN:
            idx_of_jrny_must_keep.append(i)
        sections_set |= set([_get_section_id(s) for s in jrny.sections if s.type in SECTION_TYPES_TO_RETAIN])
        candidates_pool.append(jrny)

    return np.array(candidates_pool), sections_set, idx_of_jrny_must_keep


def _build_selected_sections_matrix(sections_set, candidates_pool):
    sections_2_index_dict = dict()
    for index, value in enumerate(sections_set):
        sections_2_index_dict[value] = index
    selected_sections_matrix = list()
    nb_sections = len(sections_set)

    for j in candidates_pool:
        section_select = np.zeros(nb_sections, dtype=int)

        def _gen_section_ind():
            for s in j.sections:
                ind = sections_2_index_dict.get(_get_section_id(s))
                if ind is not None:
                    yield ind

        selected_indexes = list(_gen_section_ind())
        section_select[selected_indexes] = 1
        selected_sections_matrix.append(section_select)
    return np.array(selected_sections_matrix)


def _get_sorted_solutions_indexes(selected_sections_matrix, nb_journeys_to_find, idx_of_jrny_must_keep):
    """
    The entry is a 2D array where its lines are journeys, its columns are (non) chosen sections
    """
    logger = logging.getLogger(__name__)
    """
    Get a matrix which illustrate all possible non ordered combinations of t elements from n elements where n >= t with
    their indexes.

    Let's say we'd like to find all combinations of 2 elements from a set of 3.

    selected_journeys_matrix will be like:

    [[0,1]
     [0,2]
     [1,2]]
    """
    # Allocation of memory
    shape = (nCr(selected_sections_matrix.shape[0], nb_journeys_to_find), nb_journeys_to_find)
    selected_journeys_matrix = np.empty(shape, dtype=np.uint16)

    def f(pair):
        i, c = pair
        selected_journeys_matrix[i] = c

    collections.deque(
        map(f, zip(range(shape[0]), gen_all_combin(selected_sections_matrix.shape[0], nb_journeys_to_find))),
        maxlen=0,
    )
    """
    We should cut out those combinations that don't contain must-keep journeys
    """

    def _contains(idx_selected_jrny):
        return set(idx_selected_jrny).issuperset(idx_of_jrny_must_keep)

    selected_journeys_matrix = selected_journeys_matrix[
        np.apply_along_axis(_contains, 1, selected_journeys_matrix)
    ]

    selection_matrix = np.zeros((selected_journeys_matrix.shape[0], selected_sections_matrix.shape[0]))

    """
    Given  selected_journeys_matrix:
    [[0,1]
     [0,2]
     [1,2]]

    selection_matrix will be like:
    [[1,1,0], -> the first one and the second are chosen, etc...
     [1,0,1],
     [0,1,1]]
    """
    # http://stackoverflow.com/questions/20103779/index-2d-numpy-array-by-a-2d-array-of-indices-without-loops
    selection_matrix[np.arange(selected_journeys_matrix.shape[0])[:, None], selected_journeys_matrix] = 1

    res_pool = np.dot(selection_matrix, selected_sections_matrix)

    """
    Get number of sections(or tranfers) for every solution
    """
    nb_sections = np.sum(res_pool, axis=1)

    """
    integrity shows at which point a solution(chosen journeys) covers sections.
    0 means a solution covers all sections
    1 means a solutions covers almost all sections but 1 is missing
    2 means 2 sections are missing
    .... etc
    """
    integrity = res_pool.shape[1] - np.array([np.count_nonzero(r) for r in res_pool])

    """
    We want to find the solutions covers as many sections as possible which has less sections(less transfer to do)
    """
    the_best_idx = np.lexsort((nb_sections, integrity))[0]  # sort by integrity then by nb_sections

    best_indexes = np.where(
        np.logical_and(nb_sections == nb_sections[the_best_idx], integrity == integrity[the_best_idx])
    )[0]

    logger.debug("Best Itegrity: {0}".format(integrity[the_best_idx]))
    logger.debug("Best Nb sections: {0}".format(nb_sections[the_best_idx]))

    return best_indexes, selection_matrix


def culling_journeys(resp, request):
    """
    Remove some journeys if there are too many of them to have max_nb_journeys journeys.

    resp.journeys should be sorted before this function is called

    The goal is to choose a bunch of journeys(max_nb_journeys) that covers as many as possible sections
    but have as few as possible sum(sections)

    Ex:

    From:

    Journey_1 : Line 1 -> Line 8 -> Bus 172
    Journey_2 : Line 14 -> Line 6 -> Bus 165
    Journey_3 : Line 14 -> Line 6 ->Line 8 -> Bus 165
    Journey_4 : Line 1 -> Line 8 -> Bus 172 (this may happen when timeframe_duration or same_journey_schedule is used)

    We'd like to choose two journeys. The algo will return Journey_1 and Journey2.
    Note that Journey_4 is similar to the Journey_1 and will be ignored when max_nb_journeys<=3

    Because
    With Journey_1 and Journey_3, they cover all lines but have 5 transfers in all
    With Journey_2 and Journey_3, they don't cover all lines(Line 1 is missing) and have 5 transfers in all

    With Journey_1 and Journey_2, they cover all lines and have only 4 transfers in all -> OK

    No removing done in debug
    """
    logger = logging.getLogger(__name__)

    max_nb_journeys = request["max_nb_journeys"]
    if max_nb_journeys is None or max_nb_journeys >= len(resp.journeys):
        logger.debug('No need to cull journeys')
        return

    """
    Why aggregating journeys before culling journeys?
    We have encountered severe slowness when combining max_nb_journeys(ex: 20) and a big timeframe_duration(ex: 86400s).
    It turned out that, with this configuration, kraken will return a lot of journeys(ex: 100 journeys) and the
    algorithm was trying to figure out the best solution over 5.35E+20 possible combinations
    ( 5.35E+20=Combination(100,20) )!!

    aggregated_journeys will group journeys that are similar('similar' is defined by 'Journeys that have the same sequence
    of sections are similar'), which reduces the number of possible combinations considerably
    """
    aggregated_journeys, remaining_journeys = aggregate_journeys(resp.journeys)
    logger.debug(
        'aggregated_journeys: {} remaining_journeys: {}'.format(
            len(aggregated_journeys), len(remaining_journeys)
        )
    )
    is_debug = request.get('debug')

    if max_nb_journeys >= len(aggregated_journeys):
        """
        In this case, we return all aggregated_journeys plus earliest/latest journeys in remaining journeys
        """
        for j in remaining_journeys[max(0, max_nb_journeys - len(aggregated_journeys)) :]:
            journey_filter.mark_as_dead(
                j, is_debug, 'max_nb_journeys >= len(aggregated_journeys), ' 'Filtered by max_nb_journeys'
            )
        journey_filter.delete_journeys((resp,), request)
        return

    """
    When max_nb_journeys < len(aggregated_journeys), we first remove all remaining journeys from final response because
    those journeys already have a similar journey in aggregated_journeys
    """
    for j in remaining_journeys:
        journey_filter.mark_as_dead(
            j, is_debug, 'Filtered by max_nb_journeys, ' 'max_nb_journeys < len(aggregated_journeys)'
        )

    logger.debug('Trying to culling the journeys')

    """
    To create a candidates pool, we choose only journeys that are NOT tagged as 'comfort' and 'best' and we create a
    section set from that pool

    Ex:
    Journey_1 (Best): Line 14 -> Line 8 -> Bus 172
    Journey_2 : Line 14 -> Line 6 -> Bus 165
    Journey_3 : Line 14 -> Line 8 -> Bus 165

    The candidate pool will be like [Journey_2, Journey_3]
    The sections set will be like set([Line 14, Line 6, Line 8, Bus 165])
    """
    candidates_pool, sections_set, idx_of_jrnys_must_keep = _build_candidate_pool_and_sections_set(
        aggregated_journeys
    )

    nb_journeys_must_have = len(idx_of_jrnys_must_keep)
    logger.debug("There are {0} journeys we must keep".format(nb_journeys_must_have))

    if max_nb_journeys <= nb_journeys_must_have:
        # At this point, max_nb_journeys is smaller than nb_journeys_must_have, we have to make choices

        def _inverse_selection(d, indexes):
            select = np.in1d(list(range(d.shape[0])), indexes)
            return d[~select]

        # Here we mark all journeys as dead that are not must-have
        for jrny in _inverse_selection(candidates_pool, idx_of_jrnys_must_keep):
            journey_filter.mark_as_dead(jrny, is_debug, 'Filtered by max_nb_journeys')

        if max_nb_journeys == nb_journeys_must_have:
            logger.debug('max_nb_journeys equals to nb_journeys_must_have')
            journey_filter.delete_journeys((resp,), request)
            return

        logger.debug(
            'max_nb_journeys:{0} is smaller than nb_journeys_must_have:{1}'.format(
                request["max_nb_journeys"], nb_journeys_must_have
            )
        )

        # At this point, resp.journeys should contain only must-have journeys
        list_dict = collections.defaultdict(list)
        for jrny in resp.journeys:
            if not journey_filter.to_be_deleted(jrny):
                list_dict[jrny.type].append(jrny)

        sorted_by_type_journeys = []
        for t in JOURNEY_TYPES_TO_RETAIN:
            sorted_by_type_journeys.extend(list_dict.get(t, []))

        for jrny in sorted_by_type_journeys[max_nb_journeys:]:
            journey_filter.mark_as_dead(jrny, is_debug, 'Filtered by max_nb_journeys')

        journey_filter.delete_journeys((resp,), request)
        return

    logger.debug('Trying to find {0} journeys from {1}'.format(max_nb_journeys, candidates_pool.shape[0]))

    """
    Ex:
    Journey_2 : Line 14 -> Line 6 -> Bus 165
    Journey_3 : Line 14 -> Line 8 -> Bus 165

    The candidate pool will be like [Journey_2, Journey_3]
    The sections set will be like set([Line 14, Line 6, Line 8, Bus 165])

    selected_sections_matrix:
    [[1,1,0,1] -> journey_2
     [1,0,1,1] -> journey_3
    ]
    """
    selected_sections_matrix = _build_selected_sections_matrix(sections_set, candidates_pool)

    best_indexes, selection_matrix = _get_sorted_solutions_indexes(
        selected_sections_matrix, max_nb_journeys, idx_of_jrnys_must_keep
    )

    logger.debug("Nb best solutions: {0}".format(best_indexes.shape[0]))

    the_best_index = best_indexes[0]

    logger.debug("Trying to find the best of best")
    """
    Let's find the best of best :)
    """
    # If there're several solutions which have the same score of integrity and nb_sections
    if best_indexes.shape[0] != 1:
        requested_dt = request['datetime']
        is_clockwise = request.get('clockwise', True)

        def combinations_sorter(v):
            # Hoping to find We sort the solution by the sum of journeys' pseudo duration
            return np.sum(
                (
                    get_pseudo_duration(jrny, requested_dt, is_clockwise)
                    for jrny in np.array(candidates_pool)[np.where(selection_matrix[v, :])]
                )
            )

        the_best_index = min(best_indexes, key=combinations_sorter)

    logger.debug('Removing non selected journeys')
    for jrny in candidates_pool[np.where(selection_matrix[the_best_index, :] == 0)]:
        journey_filter.mark_as_dead(jrny, is_debug, 'Filtered by max_nb_journeys')

    journey_filter.delete_journeys((resp,), request)


def _tag_journey_by_mode(journey):
    fm = fallback_modes
    mode = fm.FallbackModes.walking
    for i, section in enumerate(journey.sections):
        cur_mode = mode
        if (section.type == response_pb2.BSS_RENT) or (
            section.type == response_pb2.CROW_FLY and section.street_network.mode == response_pb2.Bss
        ):
            cur_mode = fm.FallbackModes.bss
        elif (
            (section.type == response_pb2.STREET_NETWORK or section.type == response_pb2.CROW_FLY)
            and section.street_network.mode == response_pb2.Bike
            and journey.sections[i - 1].type != response_pb2.BSS_RENT
        ):
            cur_mode = fm.FallbackModes.bike
        elif (
            section.type == response_pb2.STREET_NETWORK or section.type == response_pb2.CROW_FLY
        ) and section.street_network.mode in (response_pb2.Car, response_pb2.CarNoPark):
            if section.street_network.mode == response_pb2.Car:
                cur_mode = fm.FallbackModes.car
            else:
                cur_mode = fm.FallbackModes.car_no_park
        elif (
            section.type == response_pb2.STREET_NETWORK or section.type == response_pb2.CROW_FLY
        ) and section.street_network.mode == response_pb2.Ridesharing:
            # When the street network data is missing, the section maybe a crow_fly
            cur_mode = fm.FallbackModes.ridesharing
        elif (
            section.type == response_pb2.STREET_NETWORK or section.type == response_pb2.CROW_FLY
        ) and section.street_network.mode == response_pb2.Taxi:
            # When the street network data is missing, the section maybe a crow_fly
            cur_mode = fm.FallbackModes.taxi

        if mode_weight[mode] < mode_weight[cur_mode]:
            mode = cur_mode

    journey.tags.append(mode.name)


def _tag_by_mode(responses):
    for r in responses:
        for j in r.journeys:
            _tag_journey_by_mode(j)


def type_journeys(resp, req):
    """
    Set the type of the journeys
    """
    best_crit = arrival_crit if req["clockwise"] else departure_crit
    # first, we want a type for every journey. Just pick "rapid" by default.
    for j in resp.journeys:
        j.type = "rapid"

    # Then, we want something like the old types
    trip_caracs = [
        # comfort tends to limit the number of transfers and fallback
        ("comfort", trip_carac([has_no_car], [comfort_crit, best_crit, duration_crit])),
        # for car we want at most one journey, the earliest one
        (
            "car",
            trip_carac(
                [has_car, has_pt],  # We don't want car only solution, we MUST have PT
                [best_crit, transfers_crit, nonTC_crit, duration_crit],
            ),
        ),
        # less_fallback tends to limit the fallback while walking
        (
            "less_fallback_walk",
            trip_carac([has_no_car, has_no_bike], [nonTC_crit, transfers_crit, duration_crit, best_crit]),
        ),
        # less_fallback tends to limit the fallback for biking and bss
        (
            "less_fallback_bike",
            trip_carac(
                [has_no_car, has_bike, has_no_bss], [nonTC_crit, transfers_crit, duration_crit, best_crit]
            ),
        ),
        # less_fallback tends to limit the fallback for biking and bss
        (
            "less_fallback_bss",
            trip_carac([has_no_car, has_bss], [nonTC_crit, transfers_crit, duration_crit, best_crit]),
        ),
        # the fastest is quite explicit
        ("fastest", trip_carac([has_no_car], [duration_crit, transfers_crit, nonTC_crit, best_crit])),
        # the non_pt journeys is the earliest journey without any public transport
        ("non_pt_walk", trip_carac([non_pt_journey, has_no_car, has_walk], [best_crit])),
        # the non_pt journey is the earliest journey without any public transport
        # only walking, biking or driving
        ("non_pt_bike", trip_carac([non_pt_journey, has_no_car, has_bike], [best_crit])),
        ("non_pt_bss", trip_carac([non_pt_journey, has_no_car, has_bss], [best_crit])),
        ("non_pt_car", trip_carac([non_pt_journey, has_car], [best_crit])),
    ]

    for name, carac in trip_caracs:
        sublist = list(filter(and_filters(carac.constraints), resp.journeys))
        best = min_from_criteria(sublist, carac.criteria)
        if best is not None:
            best.type = name

    # Finally, we want exactly one best, the ASAP one
    best = get_ASAP_journey(resp.journeys, req)
    if best is not None:
        best.type = "best"


def merge_responses(responses, debug):
    """
    Merge all responses in one protobuf response
    """
    merged_response = response_pb2.Response()

    for response_index, r in enumerate(responses):
        if r.HasField(str('error')) or not r.journeys:
            # we do not take responses with error, but if all responses have errors, we'll aggregate them
            continue

        change_ids(r, response_index)

        # we don't want to add a journey already there
        merged_response.journeys.extend(r.journeys)

        # we have to add the additional fares too
        # if at least one journey has the ticket we add it
        # and if the journey is not to be deleted in debug mode
        tickets_to_add = set(
            t
            for j in r.journeys
            if not (journey_filter.to_be_deleted(j) and not debug)
            for t in j.fare.ticket_id
        )
        merged_response.tickets.extend((t for t in r.tickets if t.id in tickets_to_add))

        initial_feed_publishers = {}
        for fp in merged_response.feed_publishers:
            initial_feed_publishers[fp.id] = fp

        # Add feed publishers from the qualified journeys only
        # Note : For BSS, it can happen that one journey in the response has returned a walking fallback.
        # If all other journeys in the response are to delete, the feed publisher will still be added
        # TODO: link feed publisher to a journey instead of a response with several journeys
        merged_response.feed_publishers.extend(
            fp
            for fp in r.feed_publishers
            if fp.id not in initial_feed_publishers
            and (debug or any('to_delete' not in j.tags for j in r.journeys))
        )

        # handle impacts
        for i in r.impacts:
            if any(other.uri == i.uri for other in merged_response.impacts):
                continue
            merged_response.impacts.extend([i])

    if not merged_response.journeys:
        # we aggregate the errors found
        errors = {r.error.id: r.error for r in responses if r.HasField(str('error'))}

        # Only one errors field
        if len(errors) == 1:
            merged_response.error.id = list(errors.values())[0].id
            merged_response.error.message = list(errors.values())[0].message
        # we need to merge the errors
        elif len(errors) > 1:
            merged_response.error.id = response_pb2.Error.no_solution
            merged_response.error.message = "several errors occured: \n * {}".format(
                "\n * ".join([m.message for m in errors.values()])
            )

    return merged_response


def get_kraken_id(entrypoint_detail):
    """
    returns a usable id for kraken from the entrypoint detail
    returns None if the original ID needs to be kept
    """
    if not entrypoint_detail:
        # impossible to find the object
        return None

    emb_type = entrypoint_detail.get('embedded_type')
    if emb_type in ('stop_point', 'stop_area', 'administrative_region'):
        # for those object, we need to keep the original id, as there are specific treatment to be done
        return None

    # for the other objects the id is the object coordinated
    coord = entrypoint_detail.get(emb_type, {}).get('coord')

    if not coord:
        # no coordinate, we keep the original id
        return None

    return '{};{}'.format(coord['lon'], coord['lat'])


def aggregate_journeys(journeys):
    """
    when building candidates_pool, we should take into count the similarity of journeys, which means, we add a journey
    into the pool only when there are no other "similar" journey already existing in the pool.

    the similarity is defined by a tuple of journeys sections.
    """
    added_sections_ids = set()
    aggregated_journeys = list()
    remaining_journeys = list()

    # we pick out all journeys that must be kept:
    for j in (j for j in journeys if j.type in JOURNEY_TYPES_TO_RETAIN):
        section_id = tuple(_get_section_id(s) for s in j.sections if s.type in SECTION_TYPES_TO_RETAIN)
        aggregated_journeys.append(j)
        added_sections_ids.add(section_id)

    for j in (j for j in journeys if j.type not in JOURNEY_TYPES_TO_RETAIN):
        section_id = tuple(_get_section_id(s) for s in j.sections if s.type in SECTION_TYPES_TO_RETAIN)

        if section_id in added_sections_ids:
            remaining_journeys.append(j)
        else:
            aggregated_journeys.append(j)
            added_sections_ids.add(section_id)
    return aggregated_journeys, remaining_journeys


def filter_journeys(responses, new_resp, instance, api_request):
    # we filter unwanted journeys in the new response
    # note that filter_journeys returns a generator which will be evaluated later
    filtered_new_resp = journey_filter.filter_journeys(new_resp, instance, api_request)

    # duplicate the generator
    tmp1, tmp2 = itertools.tee(filtered_new_resp)
    qualified_journeys = journey_filter.get_qualified_journeys(responses)

    # now we want to filter similar journeys in the new response which is done in 2 steps
    # In the first step, we compare journeys from the new response only , 2 by 2
    # hopefully, it may lead to some early return for the second step to improve the perf a little
    # In the second step, we compare the journeys from the new response with those that have been qualified
    # already in the former iterations
    # note that the journey_pairs_pool is a list of 2-element tuple of journeys
    journey_pairs_pool = itertools.chain(
        # First step: compare journeys from the new response only
        itertools.combinations(tmp1, 2),
        # Second step:
        # we use the itertools.product to create combinations between qualified journeys and new journeys
        # Ex:
        # new_journeys = [n_1, n_2]
        # qualified_journeys = [q_1, q_2, q_3]
        # itertools.product(new_journeys, qualified_journeys) gives combinations as follows:
        # (n_1, q_1), (n_1, q_2),(n_1, q_3),(n_2, q_1),(n_2, q_2),(n_2, q_3)
        itertools.product(tmp2, qualified_journeys),
    )

    journey_filter.filter_similar_vj_journeys(journey_pairs_pool, api_request)


def isochrone_common(isochrone, request, instance, journey_req):

    if request.get("origin"):
        origin = journey_req.origin.add()
        origin.place = request["origin"]
        origin.access_duration = 0
        journey_req.clockwise = True
    if request.get("destination"):
        destination = journey_req.destination.add()
        destination.place = request["destination"]
        destination.access_duration = 0
        journey_req.clockwise = False
    journey_req.datetimes.append(request["datetime"])
    journey_req.wheelchair = request["wheelchair"] or False
    journey_req.realtime_level = get_pb_data_freshness(request)
    updated_common_journey_request_with_default(request, instance)
    sn_params = journey_req.streetnetwork_params
    sn_params.max_walking_duration_to_pt = request["max_walking_duration_to_pt"]
    sn_params.max_bike_duration_to_pt = request["max_bike_duration_to_pt"]
    sn_params.max_bss_duration_to_pt = request["max_bss_duration_to_pt"]
    sn_params.max_car_duration_to_pt = request["max_car_duration_to_pt"]
    sn_params.walking_speed = request["walking_speed"]
    sn_params.bike_speed = request["bike_speed"]
    sn_params.car_speed = request["car_speed"]
    sn_params.bss_speed = request["bss_speed"]

    journey_req.max_transfers = request["max_transfers"]
    isochrone.origin_modes = request["origin_mode"]
    isochrone.destination_modes = request["destination_mode"]
    if "forbidden_uris[]" in request and request["forbidden_uris[]"]:
        for forbidden_uri in request["forbidden_uris[]"]:
            journey_req.forbidden_uris.append(forbidden_uri)
    if "allowed_id[]" in request and request["allowed_id[]"]:
        for allowed_id in request["allowed_id[]"]:
            journey_req.allowed_id.append(allowed_id)

    journey_req.streetnetwork_params.origin_mode = isochrone.origin_modes[0]
    journey_req.streetnetwork_params.destination_mode = isochrone.destination_modes[0]


class Scenario(simple.Scenario):
    """
    TODO: a bit of explanation about the new scenario
    """

    def __init__(self):
        super(Scenario, self).__init__()
        self.nb_kraken_calls = 0

    def get_context(self):
        return None

    def finalise_journeys(self, request, pt_journey_elements, context, instance, is_debug, request_id):
        pass

    def fill_journeys(self, request_type, api_request, instance):
        logger = logging.getLogger(__name__)
        request_id = api_request.get("request_id", None)

        if api_request['max_nb_journeys'] is not None and api_request['max_nb_journeys'] <= 0:
            return response_pb2.Response()

        # sometimes we need to change the entrypoint id (eg if the id is from another autocomplete system)
        origin_detail = self.get_entrypoint_detail(
            api_request.get('origin'), instance, request_id="{}_origin_detail".format(request_id)
        )
        destination_detail = self.get_entrypoint_detail(
            api_request.get('destination'), instance, request_id="{}_dest_detail".format(request_id)
        )
        # we store the origin/destination detail in g to be able to use them after the marshall
        g.origin_detail = origin_detail
        g.destination_detail = destination_detail

        api_request['origin'] = get_kraken_id(origin_detail) or api_request.get('origin')
        api_request['destination'] = get_kraken_id(destination_detail) or api_request.get('destination')

        # building ridesharing request from "original" request
        ridesharing_req = deepcopy(api_request)

        if not api_request['origin_mode']:
            api_request['origin_mode'] = ['walking']
        if not api_request['destination_mode']:
            api_request['destination_mode'] = ['walking']

        # Return the possible combinations (origin_mode ,destination_mode, direct_path_type)
        krakens_call = get_kraken_calls(api_request)

        # We need the original request (api_request) for filtering, but request
        # is modified by create_next_kraken_request function.
        request = deepcopy(api_request)

        # min_nb_journeys option
        if request['min_nb_journeys']:
            min_nb_journeys = request['min_nb_journeys']
        else:
            min_nb_journeys = 1

        responses = []
        nb_try = 0
        nb_qualified_journeys = 0
        nb_previously_qualified_journeys = 0
        last_chance_retry = False

        min_journeys_calls = request.get('_min_journeys_calls', 1)
        max_journeys_calls = app.config.get('MAX_JOURNEYS_CALLS', 20)
        max_nb_calls = min(min_nb_journeys, max_journeys_calls)

        # Initialize a context for distributed
        distributed_context = self.get_context()

        while request is not None and (
            (nb_qualified_journeys < min_nb_journeys and nb_try < max_nb_calls) or nb_try < min_journeys_calls
        ):

            nb_try = nb_try + 1

            # The parameter 'min_nb_journeys' isn't used in the following cases:
            # - If there's more than one single origin_mode and destination_mode couple.
            # - If there was no journey qualified in the previous response, the last chance request is limited
            if len(krakens_call) > 1 or last_chance_retry:
                request['min_nb_journeys'] = 0
            elif api_request['min_nb_journeys']:
                min_nb_journeys_left = min_nb_journeys - nb_qualified_journeys
                request['min_nb_journeys'] = max(0, min_nb_journeys_left)

            new_resp = self.call_kraken(
                request_type,
                request,
                instance,
                krakens_call,
                "{}_try_{}".format(request_id, nb_try),
                distributed_context,
            )

            _tag_by_mode(new_resp)
            _tag_direct_path(new_resp)
            _tag_bike_in_pt(new_resp)

            if journey_filter.nb_qualifed_journeys(new_resp) == 0:
                # no new journeys found, we stop
                # we still append the new_resp because there are journeys that a tagged as dead probably
                responses.extend(new_resp)
                break

            request = self.create_next_kraken_request(request, new_resp)

            filter_journeys(responses, new_resp, instance, api_request)

            responses.extend(new_resp)  # we keep the error for building the response

            if api_request['timeframe_duration']:
                # If timeframe_duration is active, it is useless to recall Kraken,
                # it has already sent back what he could
                break

            nb_qualified_journeys = journey_filter.nb_qualifed_journeys(responses)
            if nb_previously_qualified_journeys == nb_qualified_journeys:
                # If there is no additional qualified journey in the kraken response,
                # another request is sent to try to find more journeys, just in case...
                if last_chance_retry:
                    break
                last_chance_retry = True
            nb_previously_qualified_journeys = nb_qualified_journeys

            if nb_qualified_journeys == 0:
                min_journeys_calls = max(min_journeys_calls, 2)

        logger.debug('nb of call kraken: %i', nb_try)

        journey_filter.apply_final_journey_filters(responses, instance, api_request)

        self.finalise_journeys(
            api_request, responses, distributed_context, instance, api_request['debug'], request_id
        )

        pb_resp = merge_responses(responses, api_request['debug'])

        sort_journeys(pb_resp, instance.journey_order, api_request['clockwise'])
        compute_car_co2_emission(pb_resp, api_request, instance, "{}_car_co2".format(request_id))
        tag_journeys(pb_resp)

        # Handle ridesharing service
        self.handle_ridesharing_services(logger, instance, ridesharing_req, pb_resp)

        journey_filter.delete_journeys((pb_resp,), api_request)
        type_journeys(pb_resp, api_request)
        culling_journeys(pb_resp, api_request)

        # We have to update total duration as some sections could be updated in distributed.
        update_durations(pb_resp)
        # We have to update total co2_emission as some sections could be updated in distributed.
        update_total_co2_emission(pb_resp)

        # need to clean extra tickets after culling journeys
        journey_filter.remove_excess_tickets(pb_resp)

        self._compute_pagination_links(pb_resp, instance, api_request['clockwise'])
        return pb_resp

    def call_kraken(self, request_type, request, instance, krakens_call, request_id, context=None):
        """
        For all krakens_call, call the kraken and aggregate the responses

        return the list of all responses
        """
        # TODO: handle min_alternative_journeys
        # TODO: call first bss|bss and do not call walking|walking if no bss in first results
        record_custom_parameter('scenario', 'new_default')
        resp = []
        logger = logging.getLogger(__name__)
        futures = []
        reqctx = copy_flask_request_context()

        def worker(dep_mode, arr_mode, instance, request, _request_id):
            with copy_context_in_greenlet_stack(reqctx):
                return (dep_mode, arr_mode, instance.send_and_receive(request, request_id=_request_id))

        pool = gevent.pool.Pool(app.config.get('GREENLET_POOL_SIZE', 3))
        for dep_mode, arr_mode, direct_path_type in krakens_call:
            pb_request = create_pb_request(request_type, request, dep_mode, arr_mode, direct_path_type)
            # we spawn a new greenlet, it won't have access to our thread local request object so we pass the request_id
            futures.append(pool.spawn(worker, dep_mode, arr_mode, instance, pb_request, _request_id=request_id))

        for future in gevent.iwait(futures):
            dep_mode, arr_mode, local_resp = future.get()
            # for log purpose we put and id in each journeys
            self.nb_kraken_calls += 1
            for idx, j in enumerate(local_resp.journeys):
                j.internal_id = "{resp}-{j}".format(resp=self.nb_kraken_calls, j=idx)

            if dep_mode == 'ridesharing':
                switch_back_to_ridesharing(local_resp, True)
            if arr_mode == 'ridesharing':
                switch_back_to_ridesharing(local_resp, False)

            fill_uris(local_resp)
            resp.append(local_resp)
            logger.debug(
                "for mode %s|%s we have found %s journeys", dep_mode, arr_mode, len(local_resp.journeys)
            )

        return resp

    def __on_journeys(self, requested_type, request, instance):
        updated_request_with_default(request, instance)

        # call to kraken
        resp = self.fill_journeys(requested_type, request, instance)
        return resp

    def journeys(self, request, instance):
        return self.__on_journeys(type_pb2.PLANNER, request, instance)

    def isochrone(self, request, instance):
        updated_request_with_default(request, instance)
        # we don't want to filter anything!
        krakens_call = get_kraken_calls(request)
        # Initialize a context for distributed
        distributed_context = self.get_context()
        request_id = request.get("request_id", None)
        resp = merge_responses(
            self.call_kraken(
                type_pb2.ISOCHRONE, request, instance, krakens_call, request_id, distributed_context
            ),
            request['debug'],
        )
        if not request['debug']:
            # on isochrone we can filter the number of max journeys
            if request["max_nb_journeys"] and len(resp.journeys) > request["max_nb_journeys"]:
                del resp.journeys[request["max_nb_journeys"] :]

        return resp

    def handle_ridesharing_services(self, logger, instance, request, pb_response):

        if instance.ridesharing_services_manager.ridesharing_services_activated() and (
            'ridesharing' in request['origin_mode'] or 'ridesharing' in request['destination_mode']
        ):

            def is_asynchronous_ridesharing(request, instance):
                if request.get("_asynchronous_ridesharing", None) is None:
                    # when the param is not set in the request, use the instance config
                    return instance.asynchronous_ridesharing
                else:
                    return request.get("_asynchronous_ridesharing")

            def is_ridesharing_partner_service(request):
                partner_services = request.get('partner_services', None)
                return partner_services is not None and 'ridesharing' in partner_services

            if not is_asynchronous_ridesharing(request, instance) or is_ridesharing_partner_service(request):
                logger.debug('trying to add ridesharing journeys')
                try:
                    instance.ridesharing_services_manager.decorate_journeys_with_ridesharing_offers(
                        pb_response, request
                    )
                except Exception:
                    logger.exception('Error while retrieving ridesharing ads')
                if is_ridesharing_partner_service(request):
                    for j in pb_response.journeys:
                        if not 'ridesharing' in j.tags:
                            journey_filter.mark_as_dead(j, request.get('debug'), 'only_ridesharing_option')
            else:
                for j in pb_response.journeys:
                    if 'ridesharing' in j.tags:
                        journey_filter.mark_as_dead(j, request.get('debug'), 'asynchronous_ridesharing_mode')
            self._add_ridesharing_link(pb_response, request, instance)
        else:
            for j in pb_response.journeys:
                if 'ridesharing' in j.tags:
                    journey_filter.mark_as_dead(j, request.get('debug'), 'no_matching_ridesharing_found')

    def create_next_kraken_request(self, request, responses):
        """
        modify the request to call the next (resp previous for non clockwise search) journeys in kraken

        to do that we find ask the next (resp previous) query datetime
        """

        # We always calculate the 'next_request_date_time' from current journeys in the response of kraken
        request['datetime'] = self.get_next_datetime(responses)

        if request['datetime'] is None:
            logger = logging.getLogger(__name__)
            logger.warning("Impossible to calculate new date_time from journeys in response for next call")
            return None

        # TODO forbid ODTs
        return request

    @staticmethod
    def __get_best_for_criteria(journeys, criteria):
        return min_from_criteria(filter(has_pt, journeys), [criteria, duration_crit, transfers_crit, nonTC_crit])

    def get_best(self, journeys, clockwise):
        if clockwise:
            return self.__get_best_for_criteria(journeys, arrival_crit)
        else:
            return self.__get_best_for_criteria(journeys, departure_crit)

    def next_journey_datetime(self, journeys, clockwise):
        """
        to get the next journey, we add one second to the departure of the 'best found' journey
        """
        best = self.get_best(journeys, clockwise)

        if best is None:
            return None

        one_second = 1
        return best.departure_date_time + one_second

    def previous_journey_datetime(self, journeys, clockwise):
        """
        to get the next journey, we add one second to the arrival of the 'best found' journey
        """
        best = self.get_best(journeys, clockwise)

        if best is None:
            return None

        one_second = 1
        return best.arrival_date_time - one_second

    def get_entrypoint_detail(self, entrypoint, instance, request_id):
        logging.debug("calling autocomplete {} for {}".format(instance.autocomplete, entrypoint))
        detail = instance.autocomplete.get_object_by_uri(entrypoint, instances=[instance], request_id=request_id)

        if detail:
            return detail

        if not isinstance(instance.autocomplete, GeocodeJson):
            bragi = global_autocomplete.get('bragi')
            if bragi:
                # if the instance's autocomplete is not a geocodejson autocomplete, we also check in the
                # global autocomplete instance
                return bragi.get_object_by_uri(entrypoint, instances=[instance], request_id=request_id)

        return None

    def get_next_datetime(self, responses):
        request_datetime_list = []
        for r in responses:
            if r.HasField('next_request_date_time'):
                request_datetime_list.append(r.next_request_date_time)
        return min(request_datetime_list) if request_datetime_list else None

    def graphical_isochrones(self, request, instance):
        if "taxi" in request["origin_mode"] or "taxi" in request["destination_mode"]:
            abort(400, message="taxi is not available with new_default scenario")

        req = request_pb2.Request()
        req._current_datetime = date_to_timestamp(request["_current_datetime"])
        journey_req = req.isochrone.journeys_request
        isochrone_common(self, request, instance, journey_req)
        req.requested_api = type_pb2.graphical_isochrone
        journey_req = req.isochrone.journeys_request

        if request.get("max_duration"):
            journey_req.max_duration = request["max_duration"]
        else:
            journey_req.max_duration = max(request["boundary_duration[]"], key=int)
        if request.get("boundary_duration[]"):
            for duration in sorted(request["boundary_duration[]"], key=int, reverse=True):
                if request["min_duration"] < duration < journey_req.max_duration:
                    req.isochrone.boundary_duration.append(duration)
        req.isochrone.boundary_duration.insert(0, journey_req.max_duration)
        req.isochrone.boundary_duration.append(request["min_duration"])
        resp = instance.send_and_receive(req)
        return resp

    def heat_maps(self, request, instance):
        req = request_pb2.Request()
        req._current_datetime = date_to_timestamp(request["_current_datetime"])
        journey_req = req.heat_map.journeys_request
        isochrone_common(self, request, instance, journey_req)
        req.requested_api = type_pb2.heat_map
        max_resolution = 1000
        req.heat_map.resolution = min(request["resolution"], max_resolution)
        req.heat_map.journeys_request.max_duration = request["max_duration"]
        resp = instance.send_and_receive(req)
        return resp
