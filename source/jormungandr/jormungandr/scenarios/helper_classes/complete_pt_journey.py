# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
import logging
from .helper_utils import (
    complete_pt_journey,
    compute_fallback,
    _build_crowflies,
    timed_logger,
    complete_transfer,
)
from .helper_exceptions import InvalidDateBoundException
from jormungandr.street_network.street_network import StreetNetworkPathType
from collections import namedtuple
from navitiacommon import response_pb2

Pt_element = namedtuple("Pt_element", "dep_mode, arr_mode, pt_journeys")


def wait_and_get_pt_journeys(future_pt_journey, has_valid_direct_paths):
    """
    block until the pt_journeys are computed

    :return: The list of journeys computed by the planner
    """
    pt_journeys = future_pt_journey.wait_and_get()

    if (
        pt_journeys
        and pt_journeys.HasField(str("error"))
        and pt_journeys.error.id != response_pb2.Error.error_id.Value('bad_format')
    ):
        if pt_journeys.error.id == response_pb2.Error.error_id.Value('no_solution') and has_valid_direct_paths:
            pt_journeys.ClearField(str("error"))
        elif pt_journeys.error.id == response_pb2.Error.error_id.Value('date_out_of_bounds'):
            raise InvalidDateBoundException(pt_journeys)
        else:
            return None

    return pt_journeys


def wait_and_build_crowflies(
    requested_orig_obj,
    requested_dest_obj,
    pt_journey_pool,
    has_valid_direct_paths,
    orig_places_free_access,
    dest_places_free_acces,
    orig_fallback_durations_pool,
    dest_fallback_durations_pool,
):
    """
    block until pt_journeys are computed and attach crow-flies as fallback to all the journeys

    :return: A list of pt journeys with crowfly as a fallback
    """
    logger = logging.getLogger(__name__)
    res = []
    for (dep_mode, arr_mode, future_pt_journey) in pt_journey_pool:
        logger.debug("waiting for pt journey starts with %s and ends with %s", dep_mode, arr_mode)
        pt_journeys = wait_and_get_pt_journeys(future_pt_journey, has_valid_direct_paths)

        if pt_journeys and pt_journeys.journeys:
            orig_fallback_durations = orig_fallback_durations_pool.get_best_fallback_durations(dep_mode)
            origin_crowfly = {
                "entry_point": requested_orig_obj,
                "mode": dep_mode,
                "places_free_access": orig_places_free_access.wait_and_get(),
                "fallback_durations": orig_fallback_durations,
                "fallback_type": StreetNetworkPathType.BEGINNING_FALLBACK,
            }

            dest_fallback_durations = dest_fallback_durations_pool.get_best_fallback_durations(arr_mode)
            dest_crowfly = {
                "entry_point": requested_dest_obj,
                "mode": arr_mode,
                "places_free_access": dest_places_free_acces.wait_and_get(),
                "fallback_durations": dest_fallback_durations,
                "fallback_type": StreetNetworkPathType.ENDING_FALLBACK,
            }

            _build_crowflies(pt_journeys, origin_crowfly, dest_crowfly)

            res.append(Pt_element(dep_mode, arr_mode, pt_journeys))

    return res


def tag_LEZ(journey):
    tmp_list = [
        not section.low_emission_zone.on_path
        for section in journey.sections
        if section.HasField('low_emission_zone')
    ]
    # all([]) == True it's not what we want :(
    if tmp_list and all(tmp_list):
        journey.low_emission_zone.on_path = False
    elif any(
        section.low_emission_zone.on_path
        for section in journey.sections
        if section.HasField('low_emission_zone')
    ):
        journey.low_emission_zone.on_path = True


def get_journeys_to_complete(responses, context, is_debug):
    """
    Prepare a list of journeys from the response that will be use to compute street-network as a fallback.
    In order to compute the street network, we retrieve the fallback modes from the 'context'.

    If a journey is tagged as 'to_delete' we ignore it, thus leaving its fallback as crow fly

    :return: A list of Pt_element (journey+modes) that requires street-network
    """
    journeys_to_complete = []
    for r in responses:
        if r is None:
            continue
        for j in r.journeys:
            if is_debug is False and "to_delete" in j.tags:
                continue
            if j.internal_id in context.journeys_to_modes:
                journey_modes = context.journeys_to_modes[j.internal_id]
                pt_elem = Pt_element(journey_modes[0], journey_modes[1], j)
                journeys_to_complete.append(pt_elem)

    return journeys_to_complete


def wait_and_complete_pt_journey(
    requested_orig_obj,
    requested_dest_obj,
    streetnetwork_path_pool,
    orig_places_free_access,
    dest_places_free_access,
    orig_fallback_durations_pool,
    dest_fallback_durations_pool,
    transfer_pool,
    request,
    journeys,
    request_id,
):
    """
    In this function, we compute all fallback path once the pt journey is finished, then we build the
    whole pt journey by adding the fallback path to the beginning and the ending section of pt journey
    """
    if len(journeys) == 0:
        # Early return if no journey has to be finished
        return

    logger = logging.getLogger(__name__)
    # launch fallback direct path asynchronously
    sub_request_id = "{}_fallback".format(request_id)
    with timed_logger(logger, 'compute_fallback', sub_request_id):
        compute_fallback(
            from_obj=requested_orig_obj,
            to_obj=requested_dest_obj,
            streetnetwork_path_pool=streetnetwork_path_pool,
            orig_places_free_access=orig_places_free_access,
            dest_places_free_access=dest_places_free_access,
            orig_fallback_durations_pool=orig_fallback_durations_pool,
            dest_fallback_durations_pool=dest_fallback_durations_pool,
            request=request,
            pt_journeys=journeys,
            request_id=sub_request_id,
        )

    with timed_logger(logger, 'complete_pt_journeys', request_id):
        for pt_element in journeys:
            complete_pt_journey(
                requested_orig_obj=requested_orig_obj,
                requested_dest_obj=requested_dest_obj,
                dep_mode=pt_element.dep_mode,
                arr_mode=pt_element.arr_mode,
                pt_journey=pt_element.pt_journeys,
                streetnetwork_path_pool=streetnetwork_path_pool,
                orig_places_free_access=orig_places_free_access,
                dest_places_free_access=dest_places_free_access,
                orig_fallback_durations_pool=orig_fallback_durations_pool,
                dest_fallback_durations_pool=dest_fallback_durations_pool,
                request=request,
            )
            if {pt_element.dep_mode, pt_element.arr_mode} & {'car', 'car_no_park'}:
                tag_LEZ(pt_element.pt_journeys)

    if request['_transfer_path'] is True:
        with timed_logger(logger, 'complete_transfer', request_id):
            for pt_element in journeys:
                complete_transfer(
                    pt_journey=pt_element.pt_journeys,
                    transfer_pool=transfer_pool,
                )


def wait_and_complete_pt_journey_fare(pt_elements, pt_journey_fare_pool):
    journeys_map = {j.pt_journeys.internal_id: j.pt_journeys for j in pt_elements}
    for response, fare_response in pt_journey_fare_pool.wait_and_generate():
        if not fare_response:
            continue
        response.tickets.extend(fare_response.tickets)
        for f in fare_response.pt_journey_fares:
            journey = journeys_map.get(f.journey_id)
            if journey is None:
                logging.getLogger(__name__).warning(
                    "something wrong has occurred when completing journey fare: journey {} can't be found".format(
                        f.journey_id
                    )
                )
                continue
            journey.fare.CopyFrom(f.fare)
