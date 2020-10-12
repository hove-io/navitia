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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import

from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.utils import (
    PeriodExtremity,
    SectionSorter,
    get_pt_object_coord,
    generate_id,
    get_overriding_mode,
)
from jormungandr.street_network.utils import crowfly_distance_between
from jormungandr.fallback_modes import FallbackModes, all_fallback_modes
from .helper_exceptions import *
import copy
import logging
import six
from functools import cmp_to_key
from contextlib import contextmanager
import time


CAR_PARK_DURATION = 300  # secs


def _create_crowfly(pt_journey, crowfly_origin, crowfly_destination, begin, end, mode):
    section = response_pb2.Section()
    section.type = response_pb2.CROW_FLY
    section.origin.CopyFrom(crowfly_origin)
    section.destination.CopyFrom(crowfly_destination)
    section.duration = end - begin
    pt_journey.durations.total += section.duration
    pt_journey.duration += section.duration
    section.begin_date_time = begin
    section.end_date_time = end
    if section.duration > 0:
        section.street_network.mode = FallbackModes[mode].value
    # mode is always walking for a teleportation crow_fly
    else:
        section.street_network.mode = response_pb2.Walking

    # Calculate section length
    from_coord = get_pt_object_coord(section.origin)
    to_coord = get_pt_object_coord(section.destination)
    section.length = int(crowfly_distance_between(from_coord, to_coord))

    section.id = six.text_type(generate_id())
    return section


def _is_crowfly_needed(uri, fallback_durations, crowfly_sps, fallback_direct_path):
    # Unable to project
    f = fallback_durations.get(uri, None)
    is_unknown_projection = f.status == response_pb2.unknown if f else False

    is_crowfly_sp = uri in crowfly_sps

    # At this point, theoretically, fallback_dp should be found since the isochrone has already given a
    # valid value BUT, in some cases(due to the bad projection, etc), fallback_dp may not exist even
    # though the isochrone said "OK". In these cases, we create a crowfly section
    is_empty_fallback_dp = not fallback_direct_path or not fallback_direct_path.journeys
    return is_unknown_projection or is_crowfly_sp or is_empty_fallback_dp


def _align_fallback_direct_path_datetime(fallback_direct_path, fallback_extremity):
    """
    :param fallback_extremity: is a PeriodExtremity (a datetime and its meaning on the fallback period)
    in this function, we retrieve from streetnetwork_path_pool the direct path regarding to the given
    parameters(mode, orig_uri, etc...) then we recompute the datetimes of the found direct path,
    since the request datetime might no longer be the same (but we consider the same fallback duration).
    """

    # align datetimes to requested ones (as we consider fallback duration are the same no matter when)
    journey = fallback_direct_path.journeys[0]
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
    return fallback_direct_path


def _rename_fallback_sections_ids(sections):
    for s in sections:
        s.id = six.text_type(generate_id())


def _extend_pt_sections_with_fallback_sections(pt_journey, dp_journey):
    if getattr(dp_journey, 'journeys', []) and hasattr(dp_journey.journeys[0], 'sections'):
        _rename_fallback_sections_ids(dp_journey.journeys[0].sections)
        pt_journey.sections.extend(dp_journey.journeys[0].sections)


def _make_beginning_car_park_sections(
    car_park_section,
    car_park_to_sp_section,
    dp_extremity,
    extremity_date_time,
    pt_journey_extremity,
    car_park,
    car_park_duration,
    car_park_crowfly_duration,
):
    # car_park section
    car_park_section.type = response_pb2.PARK
    car_park_section.origin.CopyFrom(dp_extremity)
    car_park_section.destination.CopyFrom(car_park)
    car_park_section.begin_date_time = extremity_date_time
    car_park_section.end_date_time = car_park_section.begin_date_time + car_park_duration
    car_park_section.duration = car_park_duration

    # crowfly_section
    car_park_to_sp_section.type = response_pb2.CROW_FLY
    car_park_to_sp_section.street_network.CopyFrom(
        response_pb2.StreetNetwork(duration=car_park_crowfly_duration, mode=response_pb2.Walking)
    )
    car_park_to_sp_section.origin.CopyFrom(car_park)
    car_park_to_sp_section.destination.CopyFrom(pt_journey_extremity)
    car_park_to_sp_section.begin_date_time = car_park_section.end_date_time
    car_park_to_sp_section.end_date_time = car_park_to_sp_section.begin_date_time + car_park_crowfly_duration
    car_park_to_sp_section.duration = car_park_crowfly_duration


def _make_ending_car_park_sections(
    car_park_section,
    car_park_to_sp_section,
    dp_extremity,
    extremity_date_time,
    pt_journey_extremity,
    car_park,
    car_park_duration,
    car_park_crowfly_duration,
):

    # car_park section
    car_park_section.type = response_pb2.LEAVE_PARKING
    car_park_section.origin.CopyFrom(car_park)
    car_park_section.destination.CopyFrom(dp_extremity)
    car_park_section.end_date_time = extremity_date_time
    car_park_section.begin_date_time = car_park_section.end_date_time - car_park_duration
    car_park_section.duration = car_park_duration

    # crowfly_section
    car_park_to_sp_section.type = response_pb2.CROW_FLY
    car_park_to_sp_section.street_network.CopyFrom(
        response_pb2.StreetNetwork(duration=car_park_crowfly_duration, mode=response_pb2.Walking)
    )
    car_park_to_sp_section.origin.CopyFrom(pt_journey_extremity)
    car_park_to_sp_section.destination.CopyFrom(car_park)
    car_park_to_sp_section.end_date_time = car_park_section.begin_date_time
    car_park_to_sp_section.begin_date_time = car_park_to_sp_section.end_date_time - car_park_crowfly_duration
    car_park_to_sp_section.duration = car_park_crowfly_duration


def _extend_with_car_park(
    fallback_dp, pt_journey, fallback_type, walking_speed, car_park, car_park_duration, car_park_crowfly_duration
):
    dp_journey = fallback_dp.journeys[0]

    if fallback_type == StreetNetworkPathType.BEGINNING_FALLBACK:
        dp_journey.sections[-1].destination.CopyFrom(car_park)
        dp_extremity = dp_journey.sections[-1].destination
        # the first section of pt_journey is a fake crowfly section,
        # the origin of the second section is the real start of the public transport journey
        pt_journey_extrimity = pt_journey.sections[1].origin
        car_park_section = dp_journey.sections.add()
        car_park_to_sp_section = dp_journey.sections.add()

        _make_beginning_car_park_sections(
            car_park_section,
            car_park_to_sp_section,
            dp_extremity,
            dp_journey.arrival_date_time,
            pt_journey_extrimity,
            car_park,
            car_park_duration,
            car_park_crowfly_duration,
        )

        dp_journey.arrival_date_time = car_park_to_sp_section.end_date_time

    elif fallback_type == StreetNetworkPathType.ENDING_FALLBACK:
        dp_journey.sections[0].origin.CopyFrom(car_park)
        dp_extremity = dp_journey.sections[0].origin
        # the last section of pt_journey is a fake crowfly section,
        # the destination of the second section before the last is the real end of the public transport journey
        pt_journey_extrimity = pt_journey.sections[-2].destination

        car_park_section = dp_journey.sections.add()
        car_park_to_sp_section = dp_journey.sections.add()

        _make_ending_car_park_sections(
            car_park_section,
            car_park_to_sp_section,
            dp_extremity,
            dp_journey.departure_date_time,
            pt_journey_extrimity,
            car_park,
            car_park_duration,
            car_park_crowfly_duration,
        )

        dp_journey.departure_date_time = car_park_to_sp_section.begin_date_time

    dp_journey.sections.sort(key=cmp_to_key(SectionSorter()))
    dp_journey.duration += CAR_PARK_DURATION + car_park_crowfly_duration
    dp_journey.durations.walking += car_park_crowfly_duration
    dp_journey.distances.walking += int(walking_speed * car_park_crowfly_duration)


def _update_fallback_sections(journey, fallback_dp, fallback_period_extremity, fallback_type):
    """
    Replace journey's fallback sections with the given fallback_dp.

    Note: the replacement is done in place of the journey
    """
    aligned_fallback = _align_fallback_direct_path_datetime(fallback_dp, fallback_period_extremity)
    fallback_sections = aligned_fallback.journeys[0].sections

    # update the 'id' which isn't set
    _rename_fallback_sections_ids(fallback_sections)

    if fallback_type == StreetNetworkPathType.BEGINNING_FALLBACK:
        section_to_replace = journey.sections[0]
    else:
        section_to_replace = journey.sections[-1]

    journey.sections.remove(section_to_replace)

    # We have to create the link between the fallback and the pt part manually here
    if fallback_type == StreetNetworkPathType.BEGINNING_FALLBACK:
        fallback_sections[-1].destination.CopyFrom(journey.sections[0].origin)
    else:
        fallback_sections[0].origin.CopyFrom(journey.sections[-1].destination)

    journey.sections.extend(fallback_sections)
    journey.sections.sort(key=cmp_to_key(SectionSorter()))


def _get_fallback_logic(fallback_type):
    if fallback_type == StreetNetworkPathType.BEGINNING_FALLBACK:
        return BeginningFallback()

    if fallback_type == StreetNetworkPathType.ENDING_FALLBACK:
        return EndingFallback()

    raise EntryPointException(
        "Unknown fallback type : {}".format(fallback_type), response_pb2.Error.unknown_object
    )


class BeginningFallback(object):
    def get_first_non_crowfly_section(self, journey):
        return next(s for s in journey.sections if s.type != response_pb2.CROW_FLY)

    def get_pt_boundaries(self, journey):
        return self.get_first_non_crowfly_section(journey).origin

    def get_journey_bound_datetime(self, journey):
        return journey.departure_date_time

    def is_after_pt_sections(self):
        return False

    def get_pt_section_datetime(self, journey):
        return self.get_first_non_crowfly_section(journey).begin_date_time

    def get_fallback_datetime(self, pt_datetime, fallback_duration):
        return pt_datetime - fallback_duration

    def route_params(self, origin, dest):
        return origin, dest

    def set_journey_bound_datetime(self, journey):
        journey.departure_date_time = journey.sections[0].begin_date_time


class EndingFallback(object):
    def get_first_non_crowfly_section(self, journey):
        return next(s for s in reversed(journey.sections) if s.type != response_pb2.CROW_FLY)

    def get_pt_boundaries(self, journey):
        return self.get_first_non_crowfly_section(journey).destination

    def get_journey_bound_datetime(self, journey):
        return journey.arrival_date_time

    def is_after_pt_sections(self):
        return True

    def get_pt_section_datetime(self, journey):
        return self.get_first_non_crowfly_section(journey).end_date_time

    def get_fallback_datetime(self, pt_datetime, fallback_duration):
        return pt_datetime + fallback_duration

    def route_params(self, origin, dest):
        return dest, origin

    def set_journey_bound_datetime(self, journey):
        journey.arrival_date_time = journey.sections[-1].end_date_time


def _build_crowflies(pt_journeys, orig, dest):
    """
    Update all journeys in pt_journeys with a crowfly as origin and destination fallback
    """
    if pt_journeys is None:
        return

    for pt_journey in pt_journeys.journeys:
        crowflies = [_build_crowfly(pt_journey, **orig), _build_crowfly(pt_journey, **dest)]

        # Filter out empty crowflies
        crowflies = [c for c in crowflies if c]

        pt_journey.sections.extend(crowflies)
        pt_journey.sections.sort(key=cmp_to_key(SectionSorter()))
        pt_journey.departure_date_time = pt_journey.sections[0].begin_date_time
        pt_journey.arrival_date_time = pt_journey.sections[-1].end_date_time


def _build_crowfly(pt_journey, entry_point, mode, places_free_access, fallback_durations, fallback_type):
    """
    Return a new crowfly section for the current pt_journey.

    If the pt section's boundary is the same as the entry_point or within places_free_access, we won't do anything
    """
    fallback_logic = _get_fallback_logic(fallback_type)
    pt_obj = fallback_logic.get_pt_boundaries(pt_journey)

    if pt_obj.uri == entry_point.uri:
        # No need for a crowfly if the pt section starts from the requested object
        return None

    if pt_obj.uri in places_free_access.odt:
        pt_obj.CopyFrom(entry_point)
        return None

    section_datetime = fallback_logic.get_pt_section_datetime(pt_journey)
    pt_datetime = fallback_logic.get_journey_bound_datetime(pt_journey)
    fallback_duration = fallback_durations[pt_obj.uri]

    crowfly_dt = fallback_logic.get_fallback_datetime(pt_datetime, fallback_duration)

    crowfly_origin, crowfly_destination = fallback_logic.route_params(entry_point, pt_obj)
    begin, end = fallback_logic.route_params(crowfly_dt, section_datetime)

    return _create_crowfly(pt_journey, crowfly_origin, crowfly_destination, begin, end, mode)


def _build_fallback(
    requested_obj,
    pt_journey,
    mode,
    streetnetwork_path_pool,
    obj_accessible_by_crowfly,
    fallback_durations_pool,
    request,
    fallback_type,
):
    accessibles_by_crowfly = obj_accessible_by_crowfly.wait_and_get()
    fallback_durations = fallback_durations_pool.wait_and_get(mode)
    fallback_logic = _get_fallback_logic(fallback_type)

    pt_obj = fallback_logic.get_pt_boundaries(pt_journey)
    car_park = None
    car_park_crowfly_duration = None

    if mode == 'car':
        _, _, car_park, car_park_crowfly_duration = fallback_durations[pt_obj.uri]

    if requested_obj.uri != pt_obj.uri:
        if pt_obj.uri in accessibles_by_crowfly.odt:
            pt_obj.CopyFrom(requested_obj)
        else:
            # extend the journey with the fallback routing path
            is_after_pt_sections = fallback_logic.is_after_pt_sections()
            pt_datetime = fallback_logic.get_pt_section_datetime(pt_journey)
            fallback_period_extremity = PeriodExtremity(pt_datetime, is_after_pt_sections)
            orig, dest = fallback_logic.route_params(requested_obj, car_park or pt_obj)

            real_mode = fallback_durations_pool.get_real_mode(mode, pt_obj.uri)
            fallback_dp = streetnetwork_path_pool.wait_and_get(
                orig, dest, real_mode, fallback_period_extremity, fallback_type, request=request
            )

            if not _is_crowfly_needed(
                pt_obj.uri, fallback_durations, accessibles_by_crowfly.crowfly, fallback_dp
            ):
                fallback_dp_copy = copy.deepcopy(fallback_dp)

                if mode == 'car':
                    _extend_with_car_park(
                        fallback_dp_copy,
                        pt_journey,
                        fallback_type,
                        request['walking_speed'],
                        car_park,
                        request['_car_park_duration'],
                        car_park_crowfly_duration,
                    )
                _update_fallback_sections(pt_journey, fallback_dp_copy, fallback_period_extremity, fallback_type)

                # update distances and durations by mode if it's a proper computed streetnetwork fallback
                if fallback_dp_copy and fallback_dp_copy.journeys:
                    for m in all_fallback_modes - {FallbackModes.bss.name, FallbackModes.car_no_park.name}:
                        fb_distance = getattr(fallback_dp_copy.journeys[0].distances, m)
                        main_distance = getattr(pt_journey.distances, m)
                        setattr(pt_journey.distances, m, fb_distance + main_distance)

                        fb_duration = getattr(fallback_dp_copy.journeys[0].durations, m)
                        main_duration = getattr(pt_journey.durations, m)
                        setattr(pt_journey.durations, m, fb_duration + main_duration)
            # if it's only a non-teleport crowfly fallback, update distances and durations by mode
            else:
                if fallback_type == StreetNetworkPathType.BEGINNING_FALLBACK:
                    crowfly_section = pt_journey.sections[0]
                else:
                    crowfly_section = pt_journey.sections[-1]
                if crowfly_section.duration:
                    if hasattr(pt_journey.distances, mode):
                        setattr(
                            pt_journey.distances,
                            mode,
                            (getattr(pt_journey.distances, mode) + crowfly_section.length),
                        )
                    if hasattr(pt_journey.durations, mode):
                        setattr(
                            pt_journey.durations,
                            mode,
                            (getattr(pt_journey.durations, mode) + crowfly_section.duration),
                        )

    fallback_logic.set_journey_bound_datetime(pt_journey)

    return pt_journey


def get_max_fallback_duration(request, mode, dp_future):
    """
    By knowing the duration of direct path, we can limit the max duration for proximities by crowfly and fallback
    durations
    :param request:
    :param mode:
    :param dp: direct_path future
    :return:  max_fallback_duration
    """
    # 30 minutes by default
    max_duration = request.get('max_{}_duration_to_pt'.format(mode), 1800)
    dp = dp_future.wait_and_get() if dp_future else None
    dp_duration = dp.journeys[0].durations.total if getattr(dp, 'journeys', None) else max_duration
    return min(max_duration, dp_duration)


def compute_fallback(
    from_obj,
    to_obj,
    streetnetwork_path_pool,
    orig_places_free_access,
    dest_places_free_access,
    orig_fallback_durations_pool,
    dest_fallback_durations_pool,
    request,
    pt_journeys,
    request_id,
):
    """
    Launching fallback computation asynchronously once the pt_journey is finished
    """
    logger = logging.getLogger(__name__)

    places_free_access = orig_places_free_access.wait_and_get()
    orig_all_free_access = places_free_access.odt | places_free_access.crowfly | places_free_access.free_radius
    places_free_access = dest_places_free_access.wait_and_get()
    dest_all_free_access = places_free_access.odt | places_free_access.crowfly | places_free_access.free_radius

    for (i, (dep_mode, arr_mode, journey)) in enumerate(pt_journeys):
        logger.debug("completing pt journey that starts with %s and ends with %s", dep_mode, arr_mode)

        # from
        direct_path_type = StreetNetworkPathType.BEGINNING_FALLBACK
        fallback = _get_fallback_logic(direct_path_type)
        pt_orig = fallback.get_pt_boundaries(journey)
        pt_departure = fallback.get_pt_section_datetime(journey)
        fallback_extremity_dep = PeriodExtremity(pt_departure, False)
        from_sub_request_id = "{}_{}_from".format(request_id, i)
        if from_obj.uri != pt_orig.uri and pt_orig.uri not in orig_all_free_access:
            orig_obj = pt_orig
            # here, if the mode is car, we have to find from which car park the stop_point is accessed
            if dep_mode == 'car':
                orig_obj = orig_fallback_durations_pool.wait_and_get(dep_mode)[pt_orig.uri].car_park

            real_mode = orig_fallback_durations_pool.get_real_mode(dep_mode, orig_obj.uri)
            streetnetwork_path_pool.add_async_request(
                from_obj,
                orig_obj,
                real_mode,
                fallback_extremity_dep,
                request,
                direct_path_type,
                from_sub_request_id,
            )

        # to
        direct_path_type = StreetNetworkPathType.ENDING_FALLBACK
        fallback = _get_fallback_logic(direct_path_type)
        pt_dest = fallback.get_pt_boundaries(journey)
        pt_arrival = fallback.get_pt_section_datetime(journey)
        fallback_extremity_arr = PeriodExtremity(pt_arrival, True)
        to_sub_request_id = "{}_{}_to".format(request_id, i)
        if to_obj.uri != pt_dest.uri and pt_dest.uri not in dest_all_free_access:
            dest_obj = pt_dest
            if arr_mode == 'car':
                dest_obj = dest_fallback_durations_pool.wait_and_get(arr_mode)[pt_dest.uri].car_park

            real_mode = dest_fallback_durations_pool.get_real_mode(arr_mode, dest_obj.uri)
            streetnetwork_path_pool.add_async_request(
                dest_obj, to_obj, real_mode, fallback_extremity_arr, request, direct_path_type, to_sub_request_id
            )


def complete_pt_journey(
    requested_orig_obj,
    requested_dest_obj,
    dep_mode,
    arr_mode,
    pt_journey,
    streetnetwork_path_pool,
    orig_places_free_access,
    dest_places_free_access,
    orig_fallback_durations_pool,
    dest_fallback_durations_pool,
    request,
):
    """
    We complete the pt_journey by adding the beginning fallback and the ending fallback

    :return: pt_journeys: a deeply-copied list of pt_journeys with beginning and ending fallbacks
    """
    logger = logging.getLogger(__name__)
    logger.debug("building pt journey starts with %s and ends with %s", dep_mode, arr_mode)

    pt_journey = _build_fallback(
        requested_orig_obj,
        pt_journey,
        dep_mode,
        streetnetwork_path_pool,
        orig_places_free_access,
        orig_fallback_durations_pool,
        request=request,
        fallback_type=StreetNetworkPathType.BEGINNING_FALLBACK,
    )

    pt_journey = _build_fallback(
        requested_dest_obj,
        pt_journey,
        arr_mode,
        streetnetwork_path_pool,
        dest_places_free_access,
        dest_fallback_durations_pool,
        request=request,
        fallback_type=StreetNetworkPathType.ENDING_FALLBACK,
    )

    logger.debug("finish building pt journey starts with %s and ends with %s", dep_mode, arr_mode)

    return pt_journey


def get_entry_point_or_raise(entry_point_obj_future, requested_uri):
    entry_point_obj = entry_point_obj_future.wait_and_get()
    if not entry_point_obj:
        raise EntryPointException(
            error_message="The entry point: {} is not valid".format(requested_uri),
            error_id=response_pb2.Error.unknown_object,
        )
    return entry_point_obj


def check_final_results_or_raise(final_results, orig_fallback_durations_pool, dest_fallback_durations_pool):
    if final_results:
        return
    orig_fallback_durations_is_empty = orig_fallback_durations_pool.is_empty()
    dest_fallback_durations_is_empty = dest_fallback_durations_pool.is_empty()
    if orig_fallback_durations_is_empty and dest_fallback_durations_is_empty:
        raise EntryPointException(
            error_message="no origin point nor destination point",
            error_id=response_pb2.Error.no_origin_nor_destination,
        )
    if orig_fallback_durations_is_empty:
        raise EntryPointException(error_message="no origin point", error_id=response_pb2.Error.no_origin)
    if dest_fallback_durations_is_empty:
        raise EntryPointException(
            error_message="no destination point", error_id=response_pb2.Error.no_destination
        )


@contextmanager
def timed_logger(logger, task_name, request_id):
    start = time.time()
    try:
        yield logger
    finally:
        end = time.time()
        elapsed_time = end - start
        start_in_ms = int(start * 1000)
        end_in_ms = int(end * 1000)
        logger.info(
            "Task : {}, request : {},  start : {}, end : {}".format(
                task_name, request_id, start_in_ms, end_in_ms
            )
        )
        if elapsed_time > 1e-5:
            logger.info('time  in {}: {}s'.format(task_name, '%.2e' % elapsed_time))
