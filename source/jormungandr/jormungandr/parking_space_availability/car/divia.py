# encoding: utf-8
# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
import jmespath

from jormungandr.parking_space_availability.car.common_car_park_provider import CommonCarParkProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces
from jormungandr.ptref import FeedPublisher
from jormungandr import app

DEFAULT_DIVIA_FEED_PUBLISHER = None


class DiviaProvider(CommonCarParkProvider):

    def __init__(self, url, operators, dataset, timeout=1, feed_publisher=DEFAULT_DIVIA_FEED_PUBLISHER, **kwargs):

        self.ws_service_template = url + '?dataset={}'
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None
        self.provider_name = 'DIVIA'
        self.fail_max = kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_DIVIA_FAIL'])
        self.reset_timeout = kwargs.get('circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_DIVIA_TIMEOUT_S'])

        super(DiviaProvider, self).__init__(operators, dataset, timeout)

        if kwargs.get('api_key'):
            self.api_key = kwargs.get('api_key')

    def _get_information(self, poi):
        data = self._call_webservice(self.ws_service_template.format(self.dataset))

        if not data:
            return

        park = jmespath.search('records[?fields.numero_parking==\'{}\']|[0]'.format(poi['properties']['ref'].zfill(2)), data)
        if park:
            available = park['fields']['nombre_places_libres']
            occupied = park['fields']['nombre_places'] - available
            return ParkingPlaces(available, occupied, None, None)
