# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.utils import PeriodExtremity, SectionSorter
from navitiacommon import response_pb2
from helper_exceptions import *
import uuid
import copy
import logging

MODE_TO_PB_MODE = {'walking': response_pb2.Walking,
                  'bike': response_pb2.Bike,
                  'bss': response_pb2.Bss,
                  'car': response_pb2.Car}

def _create_crowfly(pt_journey, crowfly_origin, crowfly_destination, begin, end, mode):
    section = response_pb2.Section()
    section.type = response_pb2.CROW_FLY
    section.origin.CopyFrom(crowfly_origin)
    section.destination.CopyFrom(crowfly_destination)
    section.duration = end - begin
    pt_journey.durations.walking += section.duration
    pt_journey.durations.total += section.duration
    pt_journey.duration += section.duration
    section.begin_date_time = begin
    section.end_date_time = end
    section.street_network.mode = MODE_TO_PB_MODE.get(mode)
    section.id = unicode(uuid.uuid4())
    return section


def _is_crowfly_needed(uri, fallback_durations, crowfly_sps, fallback_direct_path):
    # Unable to project
    f = fallback_durations.get(uri, None)
    is_unknown_projection = (f.status == response_pb2.unknown if f else False)

    is_crowfly_sp = (uri in crowfly_sps)

    # At this point, theoretically, fallback_dp should be found since the isochrone has already given a
    # valid value BUT, in some cases(due to the bad projection, etc), fallback_dp may not exist even
    # though the isochrone said "OK". In these cases, we create a crowfly section
    is_empty_fallback_dp = (not fallback_direct_path or not fallback_direct_path.journeys)
    return is_unknown_projection or is_crowfly_sp or is_empty_fallback_dp


def _align_fallback_direct_path_datetime(fallback_direct_path, fallback_extremity):
    """
    :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
    in this function, we retrieve from streetnetwork_path_pool the direct path regarding to the given
    parameters(mode, orig_uri, etc...) then we recompute the datetimes of the found direct path,
    since the request datetime might no longer be the same (but we consider the same fallback duration).
    """

    # align datetimes to requested ones (as we consider fallback duration are the same no matter when)
    fallback_copy = copy.deepcopy(fallback_direct_path)
    journey = fallback_copy.journeys[0]
    datetime, represents_start_fallback = fallback_extremity
    if represents_start_fallback:
        journey.departure_date_time = datetime
        journey.arrival_date_time = datetime + journey.duration
    else:
        journey.departure_date_time = datetime - journey.duration
        journey.arrival_date_time = datetime

    delta = journey.departure_date_time - journey.sections[0].begin_date_time
    if delta != 0:
        for s in journey.sections:
            s.begin_date_time += delta
            s.end_date_time += delta
    return fallback_copy


def _rename_journey_sections_ids(start_idx, sections):
    for s in sections:
        s.id = "dp_section_{}".format(start_idx)
        start_idx += 1


def _extend_pt_sections_with_direct_path(pt_journey, dp_journey):
    if getattr(dp_journey, 'journeys', []) and hasattr(dp_journey.journeys[0], 'sections'):
        _rename_journey_sections_ids(len(pt_journey.sections), dp_journey.journeys[0].sections)
        pt_journey.sections.extend(dp_journey.journeys[0].sections)


def _extend_journey(pt_journey, fallback_dp, fallback_extremity):
    '''
    :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
    '''
    aligned_fallback = _align_fallback_direct_path_datetime(fallback_dp, fallback_extremity)
    pt_journey.duration += aligned_fallback.journeys[0].duration
    pt_journey.durations.total = pt_journey.duration
    pt_journey.durations.walking += aligned_fallback.journeys[0].durations.walking
    _extend_pt_sections_with_direct_path(pt_journey, aligned_fallback)


def _build_from(requested_orig_obj, pt_journeys, dep_mode, streetnetwork_path_pool, orig_accessible_by_crowfly,
                orig_fallback_durations_pool):

    accessibles_by_crowfly = orig_accessible_by_crowfly.wait_and_get()
    fallback_durations = orig_fallback_durations_pool.wait_and_get(dep_mode)

    for pt_journey in pt_journeys.journeys:
        pt_origin = pt_journey.sections[0].origin

        fallback_extremity = PeriodExtremity(pt_journey.departure_date_time, False)
        # extend the journey with the fallback routing path
        direct_path_type = StreetNetworkPathType.BEGINNING_FALLBACK
        fallback_dp = streetnetwork_path_pool.wait_and_get(requested_orig_obj, pt_origin,
                                                           dep_mode, fallback_extremity, direct_path_type)

        if requested_orig_obj.uri != pt_origin.uri:
            if pt_origin.uri in accessibles_by_crowfly.odt:
                pt_journey.sections[0].origin.CopyFrom(requested_orig_obj)
            elif _is_crowfly_needed(pt_origin.uri, fallback_durations, accessibles_by_crowfly.crowfly, fallback_dp):
                crowfly_departure_dt = pt_journey.departure_date_time - fallback_durations[pt_origin.uri].duration
                pt_journey.sections.extend([_create_crowfly(pt_journey, requested_orig_obj, pt_origin,
                                                            crowfly_departure_dt,
                                                            pt_journey.sections[0].begin_date_time, dep_mode)])
            else:
                # extend the journey with the fallback routing path
                _extend_journey(pt_journey, fallback_dp, fallback_extremity)

        pt_journey.sections.sort(SectionSorter())
        pt_journey.departure_date_time = pt_journey.sections[0].begin_date_time

    return pt_journeys


def _build_to(requested_dest_obj, pt_journeys, arr_mode, streetnetwork_path_pool, dest_accessible_by_crowfly,
              dest_fallback_durations_pool):

    accessibles_by_crowfly = dest_accessible_by_crowfly.wait_and_get()
    fallback_durations = dest_fallback_durations_pool.wait_and_get(arr_mode)

    for pt_journey in pt_journeys.journeys:
        pt_destination = pt_journey.sections[-1].destination
        last_section_end = pt_journey.sections[-1].end_date_time

        if requested_dest_obj.uri != pt_destination.uri:
            fallback_extremity = PeriodExtremity(pt_journey.arrival_date_time, True)
            # extend the journey with the fallback routing path
            direct_path_type = StreetNetworkPathType.ENDING_FALLBACK
            fallback_dp = streetnetwork_path_pool.wait_and_get(pt_destination, requested_dest_obj,
                                                               arr_mode, fallback_extremity, direct_path_type)
            if pt_destination.uri in accessibles_by_crowfly.odt:
                pt_journey.sections[-1].destination.CopyFrom(requested_dest_obj)
            elif _is_crowfly_needed(pt_destination.uri, fallback_durations, accessibles_by_crowfly.crowfly, fallback_dp):
                crowfly_arrival_dt = pt_journey.arrival_date_time + fallback_durations[pt_destination.uri].duration
                pt_journey.sections.extend([_create_crowfly(pt_journey, pt_destination, requested_dest_obj,
                                                            last_section_end,
                                                            crowfly_arrival_dt, arr_mode)])
            else:
                _extend_journey(pt_journey, fallback_dp, fallback_extremity)

        pt_journey.sections.sort(SectionSorter())
        pt_journey.arrival_date_time = pt_journey.sections[-1].end_date_time
    return pt_journeys


def _clean_pt_journey_error_or_raise(pt_journeys, has_valid_direct_path_no_pt):
    if pt_journeys and pt_journeys.HasField(b"error"):
        if pt_journeys.error.id == response_pb2.Error.error_id.Value('no_solution') and has_valid_direct_path_no_pt:
            pt_journeys.ClearField(b"error")
        else:
            raise PtException(pt_journeys)


def get_max_fallback_duration(request, mode, dp_future):
    """
    By knowing the duration of direct path, we can limit the max duration for proximities by crowfly and fallback
    durations
    :param request:
    :param mode:
    :param dp_future:
    :return:  max_fallback_duration
    """
    # 30 minutes by default
    max_duration = request.get('max_{}_duration_to_pt'.format(mode), 1800)
    dp = dp_future.wait_and_get() if dp_future else None
    dp_duration = dp.journeys[0].durations.total if getattr(dp, 'journeys', None) else max_duration
    return min(max_duration, dp_duration)


def compute_fallback(from_obj,
                     to_obj,
                     pt_journey_pool,
                     streetnetwork_path_pool,
                     orig_places_free_access,
                     dest_places_free_access,
                     request):
    """
    Launching fallback computation asynchronously once the pt_journey is finished
    """
    logger = logging.getLogger(__name__)

    has_valid_direct_paths = streetnetwork_path_pool.has_valid_direct_paths()
    for (dep_mode, arr_mode, pt_journey_f) in pt_journey_pool:
        logger.debug("waiting for pt journey starts with %s and ends with %s", dep_mode, arr_mode)

        pt_journeys = pt_journey_f.wait_and_get()

        _clean_pt_journey_error_or_raise(pt_journeys, has_valid_direct_paths)

        if not getattr(pt_journeys, "journeys", None):
            continue

        places_free_access = orig_places_free_access.wait_and_get()
        orig_all_free_access = places_free_access.odt | places_free_access.crowfly
        places_free_access = dest_places_free_access.wait_and_get()
        dest_all_free_access = places_free_access.odt | places_free_access.crowfly

        for journey in pt_journeys.journeys:
            # from
            pt_orig = journey.sections[0].origin
            direct_path_type = StreetNetworkPathType.BEGINNING_FALLBACK
            fallback_extremity_dep = PeriodExtremity(journey.departure_date_time, False)
            if from_obj.uri != pt_orig.uri and pt_orig.uri not in orig_all_free_access:
                streetnetwork_path_pool.add_async_request(from_obj, pt_orig, dep_mode, fallback_extremity_dep,
                                                          request, direct_path_type)

            # to
            pt_dest = journey.sections[-1].destination
            direct_path_type = StreetNetworkPathType.ENDING_FALLBACK
            fallback_extremity_arr = PeriodExtremity(journey.arrival_date_time, True)
            if to_obj.uri != pt_dest.uri and pt_dest.uri not in dest_all_free_access:
                streetnetwork_path_pool.add_async_request(pt_dest, to_obj, arr_mode, fallback_extremity_arr,
                                                          request, direct_path_type)


def complete_pt_journey(requested_orig_obj,
                        requested_dest_obj,
                        pt_journey_pool_elem,
                        streetnetwork_path_pool,
                        orig_places_free_access,
                        dest_places_free_access,
                        orig_fallback_durations_pool,
                        dest_fallback_durations_pool):
    """
    We complete the pt journey by adding the beginning fallback and the ending fallback

    :return: pt_journeys: a deeply-copied list of pt_journeys with beginning and ending fallbacks
    """
    logger = logging.getLogger(__name__)

    pt_journeys = copy.deepcopy(pt_journey_pool_elem.pt_journey.wait_and_get())
    if not getattr(pt_journeys, "journeys", None):
        return pt_journeys
    dep_mode = pt_journey_pool_elem.dep_mode
    arr_mode = pt_journey_pool_elem.arr_mode

    logger.debug("building pt journey starts with %s and ends with %s", dep_mode, arr_mode)

    pt_journeys = _build_from(requested_orig_obj,
                              pt_journeys,
                              dep_mode,
                              streetnetwork_path_pool,
                              orig_places_free_access,
                              orig_fallback_durations_pool)

    pt_journeys = _build_to(requested_dest_obj,
                            pt_journeys,
                            arr_mode,
                            streetnetwork_path_pool,
                            dest_places_free_access,
                            dest_fallback_durations_pool)

    logger.debug("finish building pt journey starts with %s and ends with %s", dep_mode, arr_mode)

    return pt_journeys


def get_entry_point_or_raise(entry_point_obj_future, requested_uri):
    entry_point_obj = entry_point_obj_future.wait_and_get()
    if not entry_point_obj:
        raise EntryPointException("The entry point: {} is not valid".format(requested_uri),
                                  response_pb2.Error.unknown_object)
    return entry_point_obj


def check_final_results_or_raise(final_results, orig_fallback_durations_pool, dest_fallback_durations_pool):
    if final_results:
        return
    orig_fallback_durations_is_empty = orig_fallback_durations_pool.is_empty()
    dest_fallback_durations_is_empty = dest_fallback_durations_pool.is_empty()
    if orig_fallback_durations_is_empty and dest_fallback_durations_is_empty:
        raise EntryPointException("no origin point nor destination point",
                                  response_pb2.Error.no_origin_nor_destination)
    if orig_fallback_durations_is_empty:
        raise EntryPointException("no origin point", response_pb2.Error.no_origin)
    if dest_fallback_durations_is_empty:
        raise EntryPointException("no destination point", response_pb2.Error.no_destination)
