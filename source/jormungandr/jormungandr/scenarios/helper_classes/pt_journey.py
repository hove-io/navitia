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
from jormungandr import utils, new_relic
from jormungandr.utils import date_to_timestamp
from jormungandr.street_network.street_network import StreetNetworkPathType
from navitiacommon import response_pb2, type_pb2
from collections import namedtuple
import six
import copy
import logging
from functools import cmp_to_key
from .helper_utils import timed_logger

PtPoolElement = namedtuple('PtPoolElement', ['dep_mode', 'arr_mode', 'pt_journey'])


class PtJourney:
    """
    Given a set of stop_points and their access time from origin and destination respectively, now we can compute the
    public transport journey
    """

    def __init__(
        self,
        future_manager,
        instance,
        orig_fallback_durtaions_pool,
        dest_fallback_durations_pool,
        dep_mode,
        arr_mode,
        periode_extremity,
        journey_params,
        bike_in_pt,
        request,
        isochrone_center,
        request_type,
        request_id,
    ):
        self._future_manager = future_manager
        self._instance = instance
        self._orig_fallback_durtaions_pool = orig_fallback_durtaions_pool
        self._dest_fallback_durations_pool = dest_fallback_durations_pool
        self._dep_mode = dep_mode
        self._arr_mode = arr_mode
        self._periode_extremity = periode_extremity
        self._journey_params = copy.deepcopy(journey_params)
        self._bike_in_pt = bike_in_pt
        self._request = request
        self._value = None
        self._isochrone_center = isochrone_center
        self._request_type = request_type
        self._logger = logging.getLogger(__name__)
        self._request_id = request_id
        self._pt_planner = self._instance.get_pt_planner(request['_pt_planner'])
        self._async_request()

    @new_relic.distributedEvent("journeys", "journeys")
    def _journeys(self, orig_fallback_durations, dest_fallback_durations):
        with timed_logger(self._logger, 'pt_journeys_calling_kraken', self._request_id):
            return self._pt_planner.journeys(
                orig_fallback_durations,
                dest_fallback_durations,
                self._periode_extremity.datetime,
                self._periode_extremity.represents_start,
                self._journey_params,
                self._bike_in_pt,
                self._request_id,
            )

    def _do_journeys_request(self):

        self._logger.debug(
            "requesting public transport journey with dep_mode: %s and arr_mode: %s",
            self._dep_mode,
            self._arr_mode,
        )

        orig_fallback_durations = self._orig_fallback_durtaions_pool.get_best_fallback_durations(self._dep_mode)
        dest_fallback_durations = self._dest_fallback_durations_pool.get_best_fallback_durations(self._arr_mode)

        if (
            not orig_fallback_durations
            or not dest_fallback_durations
            or not self._request.get('max_duration', 0)
        ):
            return None

        resp = self._journeys(self._pt_planner, orig_fallback_durations, dest_fallback_durations)

        for j in resp.journeys:
            j.internal_id = str(utils.generate_id())

        if resp.HasField(str("error")):
            self._logger.debug(
                "pt journey has error dep_mode: %s and arr_mode: %s", self._dep_mode, self._arr_mode
            )
            # Here needs to modify error message of no_solution
            if not orig_fallback_durations:
                resp.error.id = response_pb2.Error.no_origin
                resp.error.message = "Public transport is not reachable from origin"
            elif not dest_fallback_durations:
                resp.error.id = response_pb2.Error.no_destination
                resp.error.message = "Public transport is not reachable from destination"

        self._logger.debug(
            "finish public transport journey with dep_mode: %s and arr_mode: %s", self._dep_mode, self._arr_mode
        )
        return resp

    @new_relic.distributedEvent("graphical_isochrone", "graphical_isochrone")
    def _graphical_isochrone(self, orig_fallback_durations, dest_fallback_durations):
        return self._pt_planner.graphical_isochrones(
            orig_fallback_durations,
            dest_fallback_durations,
            self._periode_extremity.datetime,
            self._periode_extremity.represents_start,
            self._journey_params,
            self._bike_in_pt,
        )

    def _do_isochrone_common_request(self):
        logger = logging.getLogger(__name__)
        fallback_durations_pool = (
            self._orig_fallback_durtaions_pool
            if self._orig_fallback_durtaions_pool is not None
            else self._dest_fallback_durations_pool
        )
        mode = self._dep_mode if self._orig_fallback_durtaions_pool is not None else self._arr_mode
        self._logger.debug("waiting for fallback durations with %s", mode)
        fallback_duration_status = fallback_durations_pool.wait_and_get(mode)

        self._logger.debug("requesting public transport journey with dep_mode: %s", mode)

        fallback_durations = {k: v.duration for k, v in fallback_duration_status.items()}

        if not fallback_durations or not self._request.get('max_duration', 0):
            return None

        if self._orig_fallback_durtaions_pool is not None:
            orig_and_dest_fallback_durations = {
                "orig_fallback_durations": fallback_durations,
                "dest_fallback_durations": {},
            }
        else:
            orig_and_dest_fallback_durations = {
                "orig_fallback_durations": {},
                "dest_fallback_durations": fallback_durations,
            }

        if self._request_type == type_pb2.ISOCHRONE:
            resp = self._journeys(self._pt_planner, **orig_and_dest_fallback_durations)
        else:
            resp = self._graphical_isochrone(self._pt_planner, **orig_and_dest_fallback_durations)

        for j in resp.journeys:
            j.internal_id = str(utils.generate_id())

        if resp.HasField(six.text_type("error")):
            logger.debug("pt journey has error mode: %s", mode)
            # Here needs to modify error message of no_solution
            if not fallback_durations:
                if self._orig_fallback_durtaions_pool is not None:
                    resp.error.id = response_pb2.Error.no_origin
                    resp.error.message = "Public transport is not reachable from origin"
                else:
                    resp.error.id = response_pb2.Error.no_destination
                    resp.error.message = "Public transport is not reachable from destination"

        logger.debug("finish public transport journey with mode: %s", mode)
        return resp

    def _async_request(self):
        if self._request_type in [type_pb2.ISOCHRONE, type_pb2.graphical_isochrone]:
            self._value = self._future_manager.create_future(self._do_isochrone_common_request)
        else:
            self._value = self._future_manager.create_future(self._do_journeys_request)

    def wait_and_get(self):
        with timed_logger(self._logger, 'waiting_for_pt_journeys', self._request_id):
            return self._value.wait_and_get()


class _PtJourneySorter(object):
    """
    This sorter is used as a trick to optimize the computation

    With this sorter, the PtJourneyPool is sorted in a way from (likely) quickest computation to slowest computation.
    For instance, we assume that a journey starts with "walking" and ends with "walking" may be computed faster than
    a journey starts with "bike" and ends with "bike" which is faster than a journey starts with "car" and ends with
    "car". This assumption is based on the fact that the computation of fallback durations for "walking" is much faster
    than for "bike" than for "car".

    The use case can be found in the function "compute_fallback(...)". In that function, we launch the computation of
    fallbacks once the pt_journey is returned, with this sorter, the time of computation is slightly optimized, since
    we don't need to wait for the slowest pt_journey(ex. "car-car") to launch the fallback of a quick pt_journey
    ("walking-walking").
    """

    from jormungandr.fallback_modes import FallbackModes

    mode_weight = {
        FallbackModes.walking.name: 1,
        FallbackModes.bike.name: 100,
        FallbackModes.bss.name: 500,
        FallbackModes.car.name: 1000,
        FallbackModes.car_no_park.name: 1000,
        FallbackModes.ridesharing.name: 1000,
        FallbackModes.taxi.name: 1000,
    }

    def __call__(self, a, b):
        a_weight = self.mode_weight.get(a.dep_mode) + self.mode_weight.get(a.dep_mode)
        b_weight = self.mode_weight.get(b.dep_mode) + self.mode_weight.get(b.dep_mode)
        return -1 if a_weight < b_weight else 1


class PtJourneyPool:
    def __init__(
        self,
        future_manager,
        instance,
        requested_orig_obj,
        requested_dest_obj,
        streetnetwork_path_pool,
        krakens_call,
        orig_fallback_durations_pool,
        dest_fallback_durations_pool,
        request,
        request_type,
        request_id,
        isochrone_center=None,
    ):
        self._future_manager = future_manager
        self._instance = instance
        self._requested_orig_obj = requested_orig_obj
        self._requested_dest_obj = requested_dest_obj
        self._streetnetwork_path_pool = streetnetwork_path_pool
        self._krakens_call = krakens_call
        self._orig_fallback_durations_pool = orig_fallback_durations_pool
        self._dest_fallback_durations_pool = dest_fallback_durations_pool
        self._isochrone_center = isochrone_center
        self._request_type = request_type
        self._journey_params = self._create_parameters(request, self._isochrone_center, self._request_type)
        self._request = request
        self._value = []
        self._request_id = request_id
        self._async_request()

    @staticmethod
    def _create_parameters(request, isochrone_center, request_type):
        from jormungandr.pt_planners.pt_planner import (
            JourneyParameters,
            GraphicalIsochronesParameters,
            StreetNetworkParameters,
        )

        if request_type == type_pb2.graphical_isochrone:
            # Yes, graphical_isochrones needs that...
            sn_params = StreetNetworkParameters(
                origin_mode=request["origin_mode"][0],
                destination_mode=request["destination_mode"][0],
                walking_speed=request["walking_speed"],
                bike_speed=request["bike_speed"],
                car_speed=request["car_speed"],
                bss_speed=request["bss_speed"],
                car_no_park_speed=request["car_no_park_speed"],
            )
            journey_parameters = JourneyParameters(
                max_duration=request.get('max_duration', None),
                forbidden_uris=request['forbidden_uris[]'],
                allowed_id=request['allowed_id[]'],
                isochrone_center=isochrone_center,
                sn_params=sn_params,
            )
            return GraphicalIsochronesParameters(
                journeys_parameters=journey_parameters,
                min_duration=request.get("min_duration"),
                boundary_duration=request.get("boundary_duration[]"),
            )
        else:
            return JourneyParameters(
                max_duration=request['max_duration'],
                max_transfers=request['max_transfers'],
                wheelchair=request['wheelchair'] or False,
                realtime_level=request['data_freshness'],
                max_extra_second_pass=request['max_extra_second_pass'],
                arrival_transfer_penalty=request['_arrival_transfer_penalty'],
                walking_transfer_penalty=request['_walking_transfer_penalty'],
                forbidden_uris=request['forbidden_uris[]'],
                allowed_id=request['allowed_id[]'],
                night_bus_filter_max_factor=request['_night_bus_filter_max_factor'],
                night_bus_filter_base_factor=request['_night_bus_filter_base_factor'],
                min_nb_journeys=request['min_nb_journeys'],
                timeframe=request['timeframe_duration'],
                depth=request['depth'],
                isochrone_center=isochrone_center,
                current_datetime=date_to_timestamp(request['_current_datetime']),
                criteria=request.get('_criteria', None),
            )

    def _async_request(self):
        direct_path_type = StreetNetworkPathType.DIRECT
        periode_extremity = utils.PeriodExtremity(self._request['datetime'], self._request['clockwise'])
        for dep_mode, arr_mode, dp_type in self._krakens_call:
            if dp_type == "only":
                continue

            # We don't need direct_paths in isochrone
            if not self._isochrone_center:
                dp = self._streetnetwork_path_pool.wait_and_get(
                    self._requested_orig_obj,
                    self._requested_dest_obj,
                    dep_mode,
                    periode_extremity,
                    direct_path_type,
                    request=self._request,
                )
                if dp and dp.journeys:
                    self._journey_params.direct_path_duration = dp.journeys[0].durations.total
                else:
                    self._journey_params.direct_path_duration = None

            bike_in_pt = dep_mode == 'bike' and arr_mode == 'bike'
            pt_journey = PtJourney(
                future_manager=self._future_manager,
                instance=self._instance,
                orig_fallback_durtaions_pool=self._orig_fallback_durations_pool,
                dest_fallback_durations_pool=self._dest_fallback_durations_pool,
                dep_mode=dep_mode,
                arr_mode=arr_mode,
                periode_extremity=periode_extremity,
                journey_params=self._journey_params,
                bike_in_pt=bike_in_pt,
                request=self._request,
                isochrone_center=self._isochrone_center,
                request_type=self._request_type,
                request_id=self._request_id,
            )

            self._value.append(PtPoolElement(dep_mode, arr_mode, pt_journey))

        self._value.sort(key=cmp_to_key(_PtJourneySorter()))

    def __iter__(self):
        return self._value.__iter__()
