# Copyright (c) 2001-2017, Hove and/or its affiliates. All rights reserved.
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
from jormungandr import new_relic
import logging
from .helper_utils import timed_logger


class PlaceByUri:
    def __init__(self, future_manager, instance, uri, request_id):
        self._future_manager = future_manager
        self._instance = instance
        self._uri = uri
        self._value = None
        self._logger = logging.getLogger(__name__)
        self._request_id = request_id
        self._async_request()

    @new_relic.distributedEvent("place_by_uri", "places")
    def _place(self):
        with timed_logger(self._logger, 'place_by_uri_calling_external_service', self._request_id):
            return self._instance.georef.place(self._uri, request_id=self._request_id)

    def _do_request(self):
        return self._place(self._instance.georef)

    def _async_request(self):
        self._value = self._future_manager.create_future(self._do_request)

    def wait_and_get(self):
        with timed_logger(self._logger, 'waiting_for_place_by_uri', self._request_id):
            return self._value.wait_and_get()
