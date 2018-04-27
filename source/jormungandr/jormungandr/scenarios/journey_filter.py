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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import, print_function, unicode_literals, division
import logging
import itertools
import functools
import datetime
from jormungandr.scenarios.utils import compare, get_pseudo_duration, get_or_default, mode_weight
from navitiacommon import response_pb2
from jormungandr.utils import pb_del_if, compose


def delete_journeys(responses, request):

    if request.get('debug', False):
        return

    nb_deleted = 0
    for r in responses:
        nb_deleted += pb_del_if(r.journeys, lambda j: to_be_deleted(j))

    if nb_deleted:
        logging.getLogger(__name__).info('filtering {} journeys'.format(nb_deleted))


def filter_journeys(responses, instance, request):
    """
    Filter by side effect the list of pb responses's journeys

    first draft, we only remove the journeys with the same vjs
    """
    #DEBUG
    if request['debug']:
        [_debug_journey(j) for j in get_qualified_journeys(responses)]

    return compose(functools.partial(_filter_too_short_heavy_journeys, request=request),
                   functools.partial(_filter_too_long_waiting, request=request),
                   functools.partial(_filter_max_successive_physical_mode, instance=instance, request=request),
                   functools.partial(_filter_min_transfers, instance=instance, request=request),
                   functools.partial(_filter_direct_path, instance=instance, request=request),
                   )(get_qualified_journeys(responses))

def final_filter_journeys(response_list, instance, request):
    """
    Filter by side effect the list of pb responses's journeys
    Final pass : we remove similar journeys (same lines and stop_points of change)
    """

    # for clarity purpose we build a temporary list

    final_line_filter = get_or_default(request, '_final_line_filter', False)
    if final_line_filter:
        journeys = get_qualified_journeys(response_list)
        journeys_pool = itertools.combinations(journeys, 2)
        _filter_similar_line_journeys(journeys_pool, request)

    journeys = get_qualified_journeys(response_list)
    _filter_too_much_connections(journeys, instance, request)




def _get_worst_similar(j1, j2, request):
    """
    Decide which is the worst journey between 2 similar journeys.

    The choice is made on:
     - asap
     - duration
     - more fallback
     - more connection
     - smaller value among min waiting duration
     - more constrained fallback mode : all else being equal,
            it's better to know that you can do it by bike for 'bike_in_pt' tag
            (and traveler presumes he can do it walking too, as the practical case is 0s fallback)
    """
    if request.get('clockwise', True):
        if j1.arrival_date_time != j2.arrival_date_time:
            return j1 if j1.arrival_date_time > j2.arrival_date_time else j2
    else:
        if j1.departure_date_time != j2.departure_date_time:
            return j1 if j1.departure_date_time < j2.departure_date_time else j2

    if j1.duration != j2.duration:
        return j1 if j1.duration > j2.duration else j2

    if fallback_duration(j1) != fallback_duration(j2):
        return j1 if fallback_duration(j1) > fallback_duration(j2) else j2

    if get_nb_connections(j1) != get_nb_connections(j2):
        return j1 if get_nb_connections(j1) > get_nb_connections(j2) else j2

    if get_min_waiting(j1) != get_min_waiting(j2):
        return j1 if get_min_waiting(j1) < get_min_waiting(j2) else j2


    def get_mode_rank(section):
        mode_rank = {response_pb2.Car: 0,
                     response_pb2.Bike: 1,
                     response_pb2.Walking: 2}
        return mode_rank.get(section.street_network.mode)

    def is_fallback(section):
        return section.type == response_pb2.CROW_FLY or section.type != response_pb2.STREET_NETWORK

    s1 = j1.sections[0]
    s2 = j2.sections[0]
    if is_fallback(s1) and is_fallback(s2) and s1.street_network.mode != s2.street_network.mode:
        return j1 if get_mode_rank(s1) > get_mode_rank(s2) else j2

    s1 = j1.sections[-1]
    s2 = j2.sections[-1]
    if is_fallback(s1) and is_fallback(s2) and s1.street_network.mode != s2.street_network.mode:
        return j1 if get_mode_rank(s1) > get_mode_rank(s2) else j2

    return j2


def to_be_deleted(journey):
    return 'to_delete' in journey.tags


def mark_as_dead(journey, request, *reasons):
    journey.tags.append('to_delete')
    if request.get('debug'):
        journey.tags.extend(('deleted_because_' + reason for reason in reasons))


def filter_similar_vj_journeys(journeys, request):
    _filter_similar_journeys(journeys, request, similar_journeys_vj_generator)


def _filter_similar_line_journeys(journeys, request):
    _filter_similar_journeys(journeys, request, similar_journeys_line_generator)


def _filter_similar_journeys(journeys_pool, request, similar_journey_generator):
    """
    we filter similar journeys
    The given generator tells which part of journeys are compared

    in case of similar journeys we let _get_worst_similar_vjs decide which one to delete
    """

    logger = logging.getLogger(__name__)
    for j1, j2 in journeys_pool:
        if to_be_deleted(j1) or to_be_deleted(j2):
            continue
        if compare(j1, j2, similar_journey_generator):
            #chose the best
            worst = _get_worst_similar(j1, j2, request)
            logger.debug("the journeys {}, {} are similar, we delete {}".format(j1.internal_id,
                                                                                j2.internal_id,
                                                                                worst.internal_id))

            mark_as_dead(worst, request, 'duplicate_journey', 'similar_to_{other}'
                          .format(other=j1.internal_id if worst == j2 else j2.internal_id))



def exceed_min_duration(current_section, journey, request,  min_duration):
    if current_section == journey.sections[0]:
        return 'origin_mode' in request and 'walking' in request['origin_mode'] \
               and current_section.duration < min_duration
    return 'destination_mode' in request and 'walking' in request['destination_mode'] \
           and current_section.duration < min_duration


def _filter_too_short_heavy_journeys(journeys, request):
    """
    we filter the journeys with use an "heavy" fallback mode if it's use only for a few minutes
    Heavy fallback mode are Bike and Car, bss is not considered as one.
    Typically you don't take your car for only 2 minutes
    """
    logger = logging.getLogger(__name__)

    for journey in journeys:
        on_bss = False
        is_dead = False
        for s in journey.sections:
            if s.type == response_pb2.BSS_RENT:
                on_bss = True
            if s.type == response_pb2.BSS_PUT_BACK:
                on_bss = False
            if s.type != response_pb2.STREET_NETWORK:
                continue

            if s.street_network.mode == response_pb2.Car \
                    and exceed_min_duration(s, journey, request, request['_min_car']):
                logger.debug("the journey {} has not enough car, we delete it".format(journey.internal_id))
                mark_as_dead(journey, request, "not_enough_car")
                is_dead = True
                break
            if not on_bss \
                    and s.street_network.mode == response_pb2.Bike \
                    and exceed_min_duration(s, journey, request, request['_min_bike']):
                logger.debug("the journey {} has not enough bike, we delete it".format(journey.internal_id))
                mark_as_dead(journey, request, "not_enough_bike")
                is_dead = True
                break
        if not is_dead:
            yield journey
        elif request.get('debug'):
            yield journey

def _filter_too_long_waiting(journeys, request):
    """
    filter journeys with a too long section of type waiting
    """
    logger = logging.getLogger(__name__)

    for j in journeys:
        is_dead = False
        for s in j.sections:
            if s.type != response_pb2.WAITING:
                continue
            if s.duration < 4 * 60 * 60:
                continue
            logger.debug("the journey {} has a too long waiting, we delete it".format(j.internal_id))
            mark_as_dead(j, request, "too_long_waiting")
            is_dead = True
            break
        if not is_dead:
            yield j
        elif request.get('debug'):
            yield j

def _filter_max_successive_physical_mode(journeys, instance, request):
    """
    eliminates journeys with specified public_transport.physical_mode more than
    _max_successive_physical_mode (used for STIF buses)
    """
    logger = logging.getLogger(__name__)
    max_successive_physical_mode = get_or_default(request, '_max_successive_physical_mode', 0)

    if max_successive_physical_mode == 0:
        for j in journeys:
            yield j
        return
    for j in journeys:
        is_dead = False
        bus_count = 0
        for s in j.sections:
            if s.type != response_pb2.PUBLIC_TRANSPORT:
                continue
            if s.pt_display_informations.uris.physical_mode == instance.successive_physical_mode_to_limit_id:
                bus_count += 1
            else:
                if bus_count <= max_successive_physical_mode:
                    bus_count = 0

        if bus_count > max_successive_physical_mode:
            logger.debug("the journey {} has a too much successive {}, we delete it".
                format(j.internal_id, instance.successive_physical_mode_to_limit_id))
            mark_as_dead(j, request, "too_much_successive_physical_mode")
            is_dead = True
        if not is_dead:
            yield j
        elif request.get('debug'):
            yield j

def _filter_too_much_connections(journeys, instance, request):
    """
    eliminates journeys with number of connections more then minimum connections among journeys
    in the result + _max_additional_connections
    """
    logger = logging.getLogger(__name__)
    max_additional_connections = get_or_default(request, '_max_additional_connections',
                                                instance.max_additional_connections)
    import itertools
    it1, it2 = itertools.tee(journeys, 2)
    min_connections = get_min_connections(it1)
    if min_connections is not None:
        max_connections_allowed = max_additional_connections + min_connections
        for j in it2:
            if get_nb_connections(j) > max_connections_allowed:
                logger.debug("the journey {} has a too much connections, we delete it".format(j.internal_id))
                mark_as_dead(j, request, "too_much_connections")


def _filter_min_transfers(journeys, instance, request):
    """
    eliminates journeys with number of connections less then min_nb_transfers among journeys
    """
    logger = logging.getLogger(__name__)
    min_nb_transfers = get_or_default(request, 'min_nb_transfers', 0)

    for j in journeys:
        is_dead = False
        if get_nb_connections(j) < min_nb_transfers:
            logger.debug("the journey {} has not enough connections, we delete it".format(j.internal_id))
            mark_as_dead(j, request, "not_enough_connections")
            is_dead = True
        if not is_dead:
            yield j
        elif request.get('debug'):
            yield j

def _filter_direct_path(journeys, instance, request):
    """
    eliminates journeys that are not matching direct path parameter (none, only or indifferent)
    """
    logger = logging.getLogger(__name__)
    dp = request.get('direct_path') or 'indifferent'

    if dp == 'indifferent':
        for j in journeys:
            yield j
        return
    for j in journeys:
        is_dead = False
        if dp == 'none' and 'non_pt' in j.tags:
            logger.debug("the journey {} is direct, we delete it as param direct_path=none"
                         .format(j.internal_id))
            mark_as_dead(j, request, "direct_path_none")
            is_dead = True
        elif dp == 'only' and 'non_pt' not in j.tags:
            logger.debug("the journey {} uses pt, we delete it as param direct_path=only"
                         .format(j.internal_id))
            mark_as_dead(j, request, "direct_path_only")
            is_dead = True
        if not is_dead:
            yield j
        elif request.get('debug'):
            yield j

def get_min_connections(journeys):
    """
    Returns min connection count among journeys
    Returns None if journeys empty
    """
    if not journeys:
        return None

    return min((get_nb_connections(j) for j in journeys if not to_be_deleted(j)), 0)


def get_nb_connections(journey):
    """
    Returns connections count in a journey
    """
    return journey.nb_transfers


def _get_mode_weight(journey):
    if not journey.tags:
        return 1
    return max(mode_weight.get(mode, 1) for mode in journey.tags)


def get_min_waiting(journey):
    """
    Returns min waiting time in a journey
    """
    return min((s.duration for s in journey.sections if s.type == response_pb2.WAITING), 0)


def way_later(request, journey1, journey2):
    """to check if a journey is way later than another journey

    First, this rule doesn't apply if journey1 has a strictly better
    mode than journey2.

    Then, we check for each journey the difference between the
    requested datetime and the arrival datetime (the other way around
    for non clockwise)

    requested dt
    *
                   |=============>
                          journey2

                                           |=============>
                                                 journey1

    -------------------------------
             journey2 pseudo duration

    ------------------------------------------------------
                       journey1 pseudo duration

    """
    if _get_mode_weight(journey1) < _get_mode_weight(journey2):
        return False

    requested_dt = request['datetime']
    is_clockwise = request.get('clockwise', True)

    pseudo_j2_duration = get_pseudo_duration(journey2, requested_dt, is_clockwise)
    pseudo_j1_duration = get_pseudo_duration(journey1, requested_dt, is_clockwise)

    max_value = pseudo_j2_duration * request['_night_bus_filter_max_factor'] + request['_night_bus_filter_base_factor']
    return pseudo_j1_duration > max_value


def _filter_too_long_journeys(responses, request):
    """
    Filter not coherent journeys

    The aim is to keep that as simple as possible
    """
    # for clarity purpose we build a temporary iterator
    journeys = (j for r in responses for j in r.journeys if 'non_pt' not in j.tags
                and not to_be_deleted(j))

    logger = logging.getLogger(__name__)
    for (j1, j2) in itertools.permutations(journeys, 2):
        if to_be_deleted(j1) or to_be_deleted(j2):
            continue
        if way_later(request, j1, j2):
            logger.debug("the journey {} is too long compared to {}, we delete it"
                         .format(j1.internal_id, j2.internal_id))
            mark_as_dead(j1, request, 'too_long', 'too_long_compared_to_{}'.format(j2.internal_id))

def is_walk_after_parking(journey, idx_section):
    '''
    True if section at given index is a walking after/before parking car/bss, False otherwise
    '''
    is_park_section = lambda section: section.type in {response_pb2.PARK,
                                                       response_pb2.LEAVE_PARKING,
                                                       response_pb2.BSS_PUT_BACK,
                                                       response_pb2.BSS_RENT}

    s = journey.sections[idx_section]
    if s.type == response_pb2.STREET_NETWORK and s.street_network.mode == response_pb2.Walking and \
        ((idx_section - 1 >= 0 and is_park_section(journey.sections[idx_section - 1])) or
         (idx_section + 1 < len(journey.sections) and is_park_section(journey.sections[idx_section + 1]))):
        return True
    return False


def similar_journeys_vj_generator(journey):
    for idx, s in enumerate(journey.sections):
        # special case, we don't want to consider the walking section after/before parking a car
        # so CAR / PARK / WALK / PT is equivalent to CAR / PARK / PT
        if is_walk_after_parking(journey, idx):
            continue

        if s.type == response_pb2.PUBLIC_TRANSPORT:
            yield "pt:{}".format(s.pt_display_informations.uris.vehicle_journey)
        elif s.type == response_pb2.STREET_NETWORK:
            yield "sn:{}".format(s.street_network.mode)


def similar_journeys_line_generator(journey):
    for idx, s in enumerate(journey.sections):
        # special case, we don't want to consider the walking section after/before parking a car
        # so CAR / PARK / WALK / PT is equivalent to CAR / PARK / PT
        if is_walk_after_parking(journey, idx):
            continue

        if s.type == response_pb2.PUBLIC_TRANSPORT:
            yield "pt:{}".format(s.pt_display_informations.uris.line)
        elif s.type == response_pb2.STREET_NETWORK:
            yield "sn:{}".format(s.street_network.mode)


def fallback_duration(journey):
    duration = 0
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY):
            duration += section.duration

    return duration


def _debug_journey(journey):
    """
    For debug purpose print the journey
    """
    logger = logging.getLogger(__name__)

    __shorten = {
        'Walking': 'W',
        'Bike': 'B',
        'Car': 'C',
        'BSS_RENT': '/',
        'BSS_PUT_BACK': '\\',
        'PARK': '/',
        'LEAVE_PARKING': '\\',
        'TRANSFER': 'T',
        'WAITING': '.',
    }

    def shorten(name):
        return __shorten.get(name, name)

    sections = []
    for s in journey.sections:
        if s.type == response_pb2.PUBLIC_TRANSPORT:
            sections.append(u"{line} ({vj})".format(line=s.pt_display_informations.uris.line,
                                                   vj=s.pt_display_informations.uris.vehicle_journey))
        elif s.type == response_pb2.STREET_NETWORK:
            sections.append(shorten(response_pb2.StreetNetworkMode.Name(s.street_network.mode)))
        else:
            sections.append(shorten(response_pb2.SectionType.Name(s.type)))

    def _datetime_to_str(ts):
        #DEBUG, no tz handling
        dt = datetime.datetime.utcfromtimestamp(ts)
        return dt.strftime("%dT%H:%M:%S")

    logger.debug(u"journey {id}: {dep} -> {arr} - {duration} ({fallback} map) | ({sec})".format(
        id=journey.internal_id,
        dep=_datetime_to_str(journey.departure_date_time),
        arr=_datetime_to_str(journey.arrival_date_time),
        duration=datetime.timedelta(seconds=journey.duration),
        fallback=datetime.timedelta(seconds=fallback_duration(journey)),
        sec=" - ".join(sections)))


def get_qualified_journeys(responses):
    """

    :param responses: protobuf
    :return: iterator of journeys
    """
    return (j for r in responses for j in r.journeys if not to_be_deleted(j))

