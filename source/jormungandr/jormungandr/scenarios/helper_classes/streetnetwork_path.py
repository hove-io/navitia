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
from jormungandr import utils
from jormungandr.street_network.street_network import StreetNetworkPathType
import logging

class StreetNetworkPath:
    """
    A StreetNetworkPath is a journey from orig_obj to dest_obj purely in street network(without any pt)
    """
    def __init__(self, future_manager, instance, orig_obj, dest_obj, mode, fallback_extremity, request, streetnetwork_path_type):
        """
        :param future_manager: a module that manages the future pool properly
        :param instance: instance of the coverage, all outside services callings pass through it(street network,
                         auto completion)
        :param orig_obj: proto obj
        :param dest_obj: proto obj
        :param mode: street network mode, should be one of ['walking', 'bike', 'bss', 'car']
        :param fallback_extremity: departure after datetime or arrival after datetime
        :param request: original user request
        :param streetnetwork_path_type: street network path's type
        """
        self._future_manager = future_manager
        self._instance = instance
        self._orig_obj = orig_obj
        self._dest_obj = dest_obj
        self._mode = mode
        self._fallback_extremity = fallback_extremity
        self._request = request
        self._path_type = streetnetwork_path_type
        self._value = None

        self._async_request()

    def _do_request(self):
        logger = logging.getLogger(__name__)
        logger.debug("requesting %s direct path from %s to %s by %s", self._path_type,
                     self._orig_obj.uri, self._dest_obj.uri, self._mode)

        dp = self._instance.direct_path(self._mode, self._orig_obj, self._dest_obj,
                                        self._fallback_extremity, self._request,
                                        self._path_type)
        if getattr(dp, "journeys", None):
            dp.journeys[0].internal_id = str(utils.generate_id())

        logger.debug("finish %s direct path from %s to %s by %s", self._path_type,
                     self._orig_obj.uri, self._dest_obj.uri, self._mode)
        return dp

    def _async_request(self):
        self._value = self._future_manager.create_future(self._do_request)

    def wait_and_get(self):
        if self._value:
            return self._value.wait_and_get()
        return None


class StreetNetworkPathPool:
    """
    A direct path pool is a set of pure street network journeys which are computed by the given street network service.
    According to its usage, a StreetNetworkPath can be direct, begnning_fallback and ending_fallback
    """
    def __init__(self, future_manager, instance):
        self._future_manager = future_manager
        self._instance = instance
        self._value = {}

    def add_async_request(self,
                          requested_orig_obj,
                          requested_dest_obj,
                          mode,
                          period_extremity,
                          request,
                          streetnetwork_path_type):
        streetnetwork_service = self._instance.street_network_services.get(mode)
        key = streetnetwork_service.make_path_key(mode,
                                                  requested_orig_obj.uri,
                                                  requested_dest_obj.uri,
                                                  streetnetwork_path_type,
                                                  period_extremity) if streetnetwork_service else None
        if key in self._value:
            return
        self._value[key] = StreetNetworkPath(self._future_manager, streetnetwork_service, requested_orig_obj, requested_dest_obj, mode,
                                             period_extremity, request, streetnetwork_path_type)

    def get_all_direct_paths(self):
        """
        Get all streetnetwork path of DIRECT type
        :return: a dictionary of mode vs dp_future
        """
        streetnetwork_path_type = StreetNetworkPathType.DIRECT
        return {k.mode: v for k, v in self._value.items() if k.streetnetwork_path_type is streetnetwork_path_type}

    def has_valid_direct_paths(self):
        for k in self._value:
            if k.streetnetwork_path_type is not StreetNetworkPathType.DIRECT:
                continue
            dp = self._value[k].wait_and_get()
            if getattr(dp, "journeys", None):
                return True
        return False

    def wait_and_get(self, requested_orig_obj, requested_dest_obj, mode, period_extremity, streetnetwork_path_type):
        streetnetwork_service = self._instance.street_network_services.get(mode)
        key = streetnetwork_service.make_path_key(mode,
                                                  requested_orig_obj.uri,
                                                  requested_dest_obj.uri,
                                                  streetnetwork_path_type,
                                                  period_extremity) if streetnetwork_service else None
        dp = self._value.get(key)
        return dp.wait_and_get() if dp else None
