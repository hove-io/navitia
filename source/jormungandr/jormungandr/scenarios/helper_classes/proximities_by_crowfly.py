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

import jormungandr.street_network.utils
from .helper_utils import get_max_fallback_duration, timed_logger
from jormungandr import utils, new_relic, fallback_modes as fm
import logging
from navitiacommon import type_pb2


class ProximitiesByCrowfly:
    """
    A ProximitiesByCrowfly is a set of stop_points that are accessible by crowfly within a time of 'max_duration'.
    """

    def __init__(
        self,
        future_manager,
        instance,
        requested_place_obj,
        mode,
        max_duration,
        max_nb_crowfly,
        object_type,
        filter,
        stop_points_nearby_duration,
        request,
        request_id,
        depth,
    ):
        self._future_manager = future_manager
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._mode = mode
        self._max_duration = max_duration
        self._max_nb_crowfly = max_nb_crowfly
        self._object_type = object_type
        self._filter = filter
        self._stop_points_nearby_duration = stop_points_nearby_duration
        self._speed_switcher = jormungandr.street_network.utils.make_speed_switcher(request)
        self._value = None
        self._logger = logging.getLogger(__name__)
        self._request_id = request_id
        self._depth = depth
        self._forbidden_uris = utils.get_poi_params(request['forbidden_uris[]'])
        self._allowed_id = utils.get_poi_params(request['allowed_id[]'])
        self._pt_planner = self._instance.get_pt_planner(request['_pt_planner'])
        self._async_request()

    @new_relic.distributedEvent("get_crowfly", "street_network")
    def _get_crow_fly(self):
        with timed_logger(self._logger, 'get_crow_fly_calling_external_service', self._request_id):
            pt_planner = self._pt_planner
            if self._mode == 'car':
                # loki is unable to handle car park P+R so for kraken to be the pt_planner
                pt_planner = self._instance.get_pt_planner('kraken')

            return pt_planner.get_crow_fly(
                utils.get_uri_pt_object(self._requested_place_obj),
                self._mode,
                self._max_duration,
                self._max_nb_crowfly,
                self._object_type,
                self._filter,
                self._stop_points_nearby_duration,
                self._request_id,
                self._depth,
                self._forbidden_uris,
                self._allowed_id,
                **self._speed_switcher
            )

    def _do_request(self):
        logger = logging.getLogger(__name__)

        # When max_duration_to_pt is 0, there is no need to compute the fallback to pt, except if place is a stop_point
        # or a stop_area
        if self._max_duration == 0:
            logger.debug("max duration equals to 0, no need to compute proximities by crowfly")

            # When max_duration_to_pt is 0, we can get on the public transport ONLY if the place is a stop_point
            if self._instance.georef.get_stop_points_from_uri(self._requested_place_obj.uri, self._request_id):
                return [self._requested_place_obj]

        coord = utils.get_pt_object_coord(self._requested_place_obj)
        if coord.lat and coord.lon:
            crow_fly = self._get_crow_fly(self._instance.georef)

            if self._mode == fm.FallbackModes.car.name:
                # pick up only sytral_parkings with park_ride = yes
                crow_fly = jormungandr.street_network.utils.pick_up_park_ride_car_park(crow_fly)

            logger.debug(
                "finish proximities by crowfly from %s in %s", self._requested_place_obj.uri, self._mode
            )
            return crow_fly

        logger.debug("the coord of requested places is not valid: %s", coord)
        return []

    def _async_request(self):
        self._value = self._future_manager.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get()


class ProximitiesByCrowflyPool:
    def __init__(
        self,
        future_manager,
        instance,
        requested_place_obj,
        modes,
        request,
        direct_paths_by_mode,
        max_nb_crowfly_by_mode,
        request_id,
        o_d_crowfly_distance,
    ):
        """
        A ProximitiesByCrowflyPool is a set of ProximitiesByCrowfly grouped by mode

        :param instance: instance of the coverage, all outside services callings pass through it(street network,
                         auto completion)
        :param requested_place_obj: a protobuf object, returned by PlaceUri
        :param modes: a set of modes
        :param request: original request
        :param direct_paths_by_mode: a map of "mode" vs "direct path futures", used to optimize the max duration when
                                     searching for ProximitiesByCrowfly
                                     Ex. If we find a direct path from A to B by car whose duration is 15min, then there
                                     is no need to compute ProximitiesByCrowfly from A with a max_duration more than
                                     15min (max_duration is 30min by default).
        :param max_nb_crowfly_by_mode: a map of "mode" vs "nb of proximities by crowfly", used to reduce the load of
                                       street network services if necessary
        :param o_d_crowfly_distance: if no direct_path is found with the given mode, we use min(o_d_crowfly_distance, max_fallback_duration_to_pt)
        """
        self._future_manager = future_manager
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        # we don't want to compute the same mode several times
        self._modes = set(modes)
        self._request = request
        self._direct_paths_by_mode = direct_paths_by_mode
        self._max_nb_crowfly_by_mode = max_nb_crowfly_by_mode

        self._future = None
        self._value = {}
        self._request_id = request_id
        self._o_d_crowfly_distance = o_d_crowfly_distance
        self._async_request()

    def _async_request(self):

        for mode in self._modes:
            object_type = type_pb2.STOP_POINT
            filter = None
            depth = 2
            if mode == fm.FallbackModes.car.name:
                object_type = type_pb2.POI
                filter = "poi_type.uri=\"poi_type:amenity:parking\""

            dp_future = self._direct_paths_by_mode.get(mode)
            max_fallback_duration = get_max_fallback_duration(self._request, mode, dp_future)
            speed = jormungandr.street_network.utils.make_speed_switcher(self._request).get(mode)

            no_dp = (
                dp_future is None or dp_future.wait_and_get() is None or not dp_future.wait_and_get().journeys
            )

            if mode == fm.FallbackModes.car.name and no_dp and self._o_d_crowfly_distance is not None:
                max_fallback_duration = min(max_fallback_duration, self._o_d_crowfly_distance / float(speed))

            if mode == fm.FallbackModes.bss.name:
                # we subtract the time spent when renting and returning the bike
                max_fallback_duration -= (
                    self._request['bss_rent_duration'] + self._request['bss_return_duration']
                )

            p = ProximitiesByCrowfly(
                future_manager=self._future_manager,
                instance=self._instance,
                requested_place_obj=self._requested_place_obj,
                mode=mode,
                max_duration=max_fallback_duration,
                max_nb_crowfly=self._max_nb_crowfly_by_mode.get(mode, 5000),
                object_type=object_type,
                filter=filter,
                stop_points_nearby_duration=self._request['_stop_points_nearby_duration'],
                request=self._request,
                request_id=self._request_id,
                depth=depth,
            )

            self._value[mode] = p

    def wait_and_get(self, mode):
        c = self._value.get(mode)
        return c.wait_and_get() if c else None
