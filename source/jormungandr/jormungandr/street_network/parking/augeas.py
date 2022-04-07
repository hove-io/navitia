#  Copyright (c) 2001-2019, Hove and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import, print_function, unicode_literals, division
import requests
import pybreaker
import logging
from datetime import timedelta
from jormungandr.street_network.parking.abstract_parking_module import AbstractParkingModule

DEFAULT_MAX_PARKING_DURATION = int(timedelta(seconds=1200).total_seconds())


class Augeas(AbstractParkingModule):
    def __init__(self, service_url, max_park_duration=DEFAULT_MAX_PARKING_DURATION):
        self.service_url = service_url
        self.max_park_duration = max_park_duration
        # TODO: put into a config when the POC is validated
        one_minute = timedelta(seconds=60).total_seconds()
        self.breaker = pybreaker.CircuitBreaker(fail_max=10, reset_timeout=one_minute)
        self.request_timeout = timedelta(seconds=0.1).total_seconds()

        self.logger = logging.getLogger(__name__)

    def _breaker_wrapper(self, fun, *args, **kwargs):
        try:
            return self.breaker.call(fun, *args, **kwargs)
        except Exception as e:
            self.logger.exception('error occurred when requesting Augeas: {}'.format(str(e)))
            raise

    def _request_in_batch(self, coords):
        data = {
            "n": 1,
            "walking_speed": 1.11,
            "max_park_duration": self.max_park_duration,
            "coords": [[c.lon, c.lat] for c in coords],
        }
        url = requests.compat.urljoin(self.service_url, '/v0/park_duration')
        r = requests.post(url=url, json=data, timeout=self.request_timeout)
        return r.json().get('durations')

    def _request(self, coord):
        params = {
            'lon': coord.lon,
            'lat': coord.lat,
            'n': 1,
            "walking_speed": 1.11,
            'max_park_duration': self.max_park_duration,
        }
        url = requests.compat.urljoin(self.service_url, '/v0/park_duration')
        r = requests.get(url=url, params=params, timeout=self.request_timeout)
        durations = r.json().get('durations')
        if not durations:
            return self.max_park_duration
        return durations[0].get('duration', self.max_park_duration)

    def get_parking_duration(self, coord):
        return self._breaker_wrapper(self._request, coord=coord)

    def get_leave_parking_duration(self, coord):
        return self._breaker_wrapper(self._request, coord=coord)

    def get_parking_duration_in_batch(self, coords):
        return self._breaker_wrapper(self._request_in_batch, coords=coords)

    def get_leave_duration_in_batch(self, coords):
        return self._breaker_wrapper(self._request_in_batch, coords=coords)
