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
from jormungandr.street_network.street_network import StreetNetworkPathType
import logging
from .helper_utils import timed_logger, prepend_first_coord, append_last_coord
from navitiacommon import type_pb2, response_pb2
from jormungandr.exceptions import GeoveloTechnicalError
from .helper_exceptions import StreetNetworkException


class StreetNetworkPath:
    """
    A StreetNetworkPath is a journey from orig_obj to dest_obj purely in street network(without any pt)
    """

    def __init__(
        self,
        future_manager,
        instance,
        streetnetwork_service,
        orig_obj,
        dest_obj,
        mode,
        fallback_extremity,
        request,
        streetnetwork_path_type,
        request_id,
    ):
        """
        :param future_manager: a module that manages the future pool properly
        :param instance: instance of the coverage, all outside services callings pass through it(street network,
                         auto completion)
        :param streetnetwork_service: service that will be used to compute the path
        :param orig_obj: proto obj
        :param dest_obj: proto obj
        :param mode: street network mode, should be one of ['walking', 'bike', 'bss', 'car']
        :param fallback_extremity: departure after datetime or arrival after datetime
        :param request: original user request
        :param streetnetwork_path_type: street network path's type
        """
        self._future_manager = future_manager
        self._instance = instance
        self._streetnetwork_service = streetnetwork_service
        self._orig_obj = orig_obj
        self._dest_obj = dest_obj
        self._mode = mode
        self._fallback_extremity = fallback_extremity
        self._request = request
        self._path_type = streetnetwork_path_type
        self._value = None
        self._logger = logging.getLogger(__name__)
        self._request_id = request_id
        self._async_request()

    @new_relic.distributedEvent("direct_path", "street_network")
    def _direct_path_with_fp(self):
        with timed_logger(self._logger, 'direct_path_calling_external_service', self._request_id):
            try:
                return self._streetnetwork_service.direct_path_with_fp(
                    self._instance,
                    self._mode,
                    self._orig_obj,
                    self._dest_obj,
                    self._fallback_extremity,
                    self._request,
                    self._path_type,
                    self._request_id,
                )
            except GeoveloTechnicalError as e:
                logging.getLogger(__name__).exception('')
                raise StreetNetworkException(response_pb2.Error.service_unavailable, e.data["message"])
            except Exception:
                logging.getLogger(__name__).exception('')
                return None

    def _do_request(self):
        self._logger.debug(
            "requesting %s direct path from %s to %s by %s",
            self._path_type,
            self._orig_obj.uri,
            self._dest_obj.uri,
            self._mode,
        )

        dp = self._direct_path_with_fp(self._streetnetwork_service)
        prepend_first_coord(dp, self._orig_obj)
        append_last_coord(dp, self._dest_obj)

        if getattr(dp, "journeys", None):
            dp.journeys[0].internal_id = str(utils.generate_id())

        self._logger.debug(
            "finish %s direct path from %s to %s by %s",
            self._path_type,
            self._orig_obj.uri,
            self._dest_obj.uri,
            self._mode,
        )
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
    According to its usage, a StreetNetworkPath can be direct, beginning_fallback and ending_fallback
    """

    def __init__(self, future_manager, instance):
        self._future_manager = future_manager
        self._instance = instance
        self._value = {}
        self._direct_paths_future_by_mode = {}

    def add_async_request(
        self,
        requested_orig_obj,
        requested_dest_obj,
        mode,
        period_extremity,
        request,
        streetnetwork_path_type,
        request_id,
    ):
        streetnetwork_service = self._instance.get_street_network(mode, request)
        key = (
            streetnetwork_service.make_path_key(
                mode, requested_orig_obj.uri, requested_dest_obj.uri, streetnetwork_path_type, period_extremity
            )
            if streetnetwork_service
            else None
        )
        path = self._value.get(key)
        if not path:
            path = self._value[key] = StreetNetworkPath(
                self._future_manager,
                self._instance,
                streetnetwork_service,
                requested_orig_obj,
                requested_dest_obj,
                mode,
                period_extremity,
                request,
                streetnetwork_path_type,
                request_id,
            )
        if streetnetwork_path_type is StreetNetworkPathType.DIRECT:
            self._direct_paths_future_by_mode[mode] = path

    def get_all_direct_paths(self):
        """
        :return: a dict of mode vs direct_path future
        """
        return self._direct_paths_future_by_mode

    def has_valid_direct_paths(self):
        for k in self._value:
            if k.streetnetwork_path_type is not StreetNetworkPathType.DIRECT:
                continue
            dp = self._value[k].wait_and_get()
            if getattr(dp, "journeys", None):
                return True
        return False

    def wait_and_get(
        self, requested_orig_obj, requested_dest_obj, mode, period_extremity, streetnetwork_path_type, request
    ):
        streetnetwork_service = self._instance.get_street_network(mode, request)
        key = (
            streetnetwork_service.make_path_key(
                mode, requested_orig_obj.uri, requested_dest_obj.uri, streetnetwork_path_type, period_extremity
            )
            if streetnetwork_service
            else None
        )
        dp_future = self._value.get(key)
        return dp_future.wait_and_get() if dp_future else None

    def add_feed_publishers(self, request, requested_direct_path_modes, responses):
        def _feed_publisher_not_present(feed_publishers, id):
            for fp in feed_publishers:
                if id == fp.id:
                    return False
            return True

        for mode in requested_direct_path_modes:
            streetnetwork_service = self._instance.get_street_network(mode, request)
            fp = streetnetwork_service.feed_publisher()
            if fp != None:
                for resp in responses:
                    if _feed_publisher_not_present(resp.feed_publishers, fp.id):
                        feed = resp.feed_publishers.add()
                        feed.id = fp.id
                        feed.name = fp.name
                        feed.license = fp.license
                        feed.url = fp.url
