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
from __future__ import absolute_import
import logging
from . import helper_future
from .helper_utils import (
    complete_pt_journey,
    compute_fallback,
    clean_pt_journey_error_or_raise,
    _build_crowflies,
)
from jormungandr.street_network.street_network import StreetNetworkPathType


def wait_and_get_pt_journeys(future_pt_journey, has_valid_direct_paths):
    pt_journeys = future_pt_journey.wait_and_get()
    clean_pt_journey_error_or_raise(pt_journeys, has_valid_direct_paths)
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
    logger = logging.getLogger(__name__)
    res = []
    for (dep_mode, arr_mode, future_pt_journey) in pt_journey_pool:
        logger.debug("waiting for pt journey starts with %s and ends with %s", dep_mode, arr_mode)
        pt_journeys = wait_and_get_pt_journeys(future_pt_journey, has_valid_direct_paths)

        origin_crowfly = {
            "requested_obj": requested_orig_obj,
            "mode": dep_mode,
            "places_free_access": orig_places_free_access.wait_and_get(),
            "fallback_durations": orig_fallback_durations_pool.wait_and_get(dep_mode),
            "fallback_type": StreetNetworkPathType.BEGINNING_FALLBACK,
        }

        dest_crowfly = {
            "requested_obj": requested_dest_obj,
            "mode": arr_mode,
            "places_free_access": dest_places_free_acces.wait_and_get(),
            "fallback_durations": dest_fallback_durations_pool.wait_and_get(arr_mode),
            "fallback_type": StreetNetworkPathType.ENDING_FALLBACK,
        }

        _build_crowflies(pt_journeys, origin_crowfly, dest_crowfly)

        res.append((dep_mode, arr_mode, pt_journeys))

    return res


def wait_and_complete_pt_journey(
    future_manager,
    requested_orig_obj,
    requested_dest_obj,
    streetnetwork_path_pool,
    orig_places_free_access,
    dest_places_free_access,
    orig_fallback_durations_pool,
    dest_fallback_durations_pool,
    request,
    pt_journeys,
):
    """
    In this function, we compute all fallback path once the pt journey is finished, then we build the
    whole pt journey by adding the fallback path to the beginning and the ending section of pt journey
    """
    # launch fallback direct path asynchronously
    compute_fallback(
        from_obj=requested_orig_obj,
        to_obj=requested_dest_obj,
        streetnetwork_path_pool=streetnetwork_path_pool,
        orig_places_free_access=orig_places_free_access,
        dest_places_free_access=dest_places_free_access,
        request=request,
        pt_journeys=pt_journeys,
    )

    futures = []
    for elem in pt_journeys:

        f = future_manager.create_future(
            complete_pt_journey,
            requested_orig_obj=requested_orig_obj,
            requested_dest_obj=requested_dest_obj,
            dep_mode=elem[0],
            arr_mode=elem[1],
            pt_journeys_=elem[2],
            streetnetwork_path_pool=streetnetwork_path_pool,
            orig_places_free_access=orig_places_free_access,
            dest_places_free_access=dest_places_free_access,
            orig_fallback_durations_pool=orig_fallback_durations_pool,
            dest_fallback_durations_pool=dest_fallback_durations_pool,
            request=request,
        )
        futures.append(f)
    # return a generator, so we block the main thread later when they are evaluated
    return (f.wait_and_get() for f in futures)
