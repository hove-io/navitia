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
import helper_future
from jormungandr.street_network.street_network import StreetNetworkPathType
from helper_utils import get_max_fallback_duration
from jormungandr import utils
import logging


class ProximitiesByCrowfly:
    """
    A ProximitiesByCrowfly is a set of stop_points that are accessible by crowfly within a time of 'max_duration'.
    """
    def __init__(self, future_manager, instance, requested_place_obj, mode, max_duration, max_nb_crowfly=5000):
        self._future_manager = future_manager
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._mode = mode
        self._max_duration = max_duration
        self._max_nb_crowfly = max_nb_crowfly
        self._speed_switcher = {
            "walking": instance.walking_speed,
            "bike": instance.bike_speed,
            "car": instance.car_speed,
            "bss": instance.bss_speed,
        }
        self._value = None
        self._async_request()

    def _do_request(self):
        logger = logging.getLogger(__name__)
        logger.debug("requesting proximities by crowfly from %s in %s", self._requested_place_obj.uri, self._mode)

        # When max_duration_to_pt is 0, there is no need to compute the fallback to pt, except if place is a stop_point
        # or a stop_area
        if self._max_duration == 0:
            logger.debug("max duration equals to 0, no need to compute proximities by crowfly")

            # When max_duration_to_pt is 0, we can get on the public transport ONLY if the place is a stop_point
            if self._instance.georef.get_stop_points_from_uri(self._requested_place_obj.uri):
                return [self._requested_place_obj]

        coord = utils.get_pt_object_coord(self._requested_place_obj)
        if coord.lat and coord.lon:
            crow_fly = self._instance.georef.get_crow_fly(utils.get_uri_pt_object(self._requested_place_obj),
                                                          self._mode,  self._max_duration, self._max_nb_crowfly,
                                                          **self._speed_switcher)

            logger.debug("finish proximities by crowfly from %s in %s", self._requested_place_obj.uri, self._mode)
            return crow_fly

        logger.debug("the coord of requested places is not valid: %s", coord)
        return []

    def _async_request(self):
        self._value = self._future_manager.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get()


class ProximitiesByCrowflyPool:
    def __init__(self, future_manager, instance, requested_place_obj, modes, request, direct_paths_by_mode, max_nb_crowfly=5000):
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
        :param max_nb_crowfly:
        """
        self._future_manager = future_manager
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        # we don't want to compute the same mode several times
        self._modes = set(modes)
        self._request = request
        self._direct_paths_by_mode = direct_paths_by_mode
        self._max_nb_crowfly = max_nb_crowfly

        self._future = None
        self._value = {}

        self._async_request()

    def _async_request(self):

        for mode in self._modes:
            max_fallback_duration = get_max_fallback_duration(self._request, mode, self._direct_paths_by_mode.get(mode))
            p = ProximitiesByCrowfly(future_manager=self._future_manager,
                                     instance=self. _instance,
                                     requested_place_obj=self._requested_place_obj,
                                     mode=mode,
                                     max_duration=max_fallback_duration,
                                     max_nb_crowfly=self._max_nb_crowfly)

            self._value[mode] = p

    def wait_and_get(self, mode):
        c = self._value.get(mode)
        return c.wait_and_get() if c else None
