# encoding: utf-8
# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

import logging
import pybreaker
import requests as requests
import jmespath

from jormungandr import cache, app
from jormungandr.parking_space_availability import AbstrcatParkingPlacesProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces
from jormungandr.ptref import FeedPublisher

DEFAULT_STAR_FEED_PUBLISHER = None


class StarProvider(AbstrcatParkingPlacesProvider):

    WS_URL_TEMPLATE = 'http://data.explore.star.fr/api/records/1.0/search/?dataset={}&refine.idparc={}'

    def __init__(self, operators, timeout, dataset, feed_publisher=DEFAULT_STAR_FEED_PUBLISHER):
        self.operators = [o.lower() for o in operators]
        self.timeout = timeout
        self.dataset = dataset
        self.breaker = pybreaker.CircuitBreaker(fail_max=5, reset_timeout=10)
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None

    def support_poi(self, poi):
        properties = poi.get('properties', {})
        return properties.get('operator', '').lower() in self.operators

    def get_informations(self, poi):
        ref = poi.get('properties', {}).get('ref')
        if not ref:
            return

        data = self._call_webservice(ref)
        if not data:
            return

        available_places = jmespath.search('records[0].fields.nombreplacesdisponibles', data) or 0
        occupied_places = jmespath.search('records[0].fields.nombreplacesoccupees', data) or 0
        available_disabled = jmespath.search('records[0].fields.nombreplacesdisponiblespmr', data) or 0
        occupied_disabled = jmespath.search('records[0].fields.nombreplacesoccupeespmr', data) or 0

        return ParkingPlaces(available_places, occupied_places, available_disabled, occupied_disabled)

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_STAR', 30))
    def _call_webservice(self, parking_id):
        try:
            data = self.breaker.call(requests.get, self.WS_URL_TEMPLATE.format(self.dataset, parking_id),
                                     timeout=self.timeout)
            return data.json()
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('STAR service dead (error: {})'.format(e))
        except requests.Timeout as t:
            logging.getLogger(__name__).error('STAR service timeout (error: {})'.format(t))
        except:
            logging.getLogger(__name__).exception('STAR service error')

        return None

    def status(self):
        return {'operators': self.operators}

    def feed_publisher(self):
        return self._feed_publisher
