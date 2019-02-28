#  Copyright (c) 2001-2019, Kisio Digital and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Kisio Digital (http://www.kisiodigital.com/).
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

from jormungandr.street_network.parking.abstract_parking_module import AbstractParkingModule
import requests
import functools


class Augeas(AbstractParkingModule):
    def __init__(self, service_url, max_park_duration=1200):
        self.service_url = service_url
        self.max_park_duration = max_park_duration

    def _request_in_batch(self, coords):
        data = {
            "n": 1,
            "walking_speed": 1.11,
            "max_park_duration": self.max_park_duration,
            "coords": [[c.lon, c.lat] for c in coords],
        }
        url = requests.compat.urljoin(self.service_url, '/v0/park_duration')
        r = requests.post(url=url, json=data)
        return r.json().get('durations')

    def _request(self, coords):
        params = {'lon': coords.lon, 'lat': coords.lat, 'n': 1, 'max_park_duration': self.max_park_duration}
        url = requests.compat.urljoin(self.service_url, '/v0/park_duration')
        r = requests.get(url=url, params=params)
        durations = r.json().get('durations')
        if not durations:
            return self.max_park_duration
        return durations[0].get('duration', self.max_park_duration)

    def get_parking_duration(self, coord):
        return self._request(coord)

    def get_leave_parking_duration(self, coord):
        return self._request(coord)

    def get_parking_duration_in_batch(self, coords):
        return self._request_in_batch(coords)

    def get_leave_duration_in_batch(self, coords):
        return self._request_in_batch(coords)
