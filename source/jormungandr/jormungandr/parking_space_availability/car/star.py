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

from jormungandr import cache, app, utils, new_relic
from jormungandr.parking_space_availability import AbstractParkingPlacesProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces
from jormungandr.ptref import FeedPublisher

DEFAULT_STAR_FEED_PUBLISHER = None


class StarProvider(AbstractParkingPlacesProvider):

    def __init__(self, url, operators, dataset, timeout=1, feed_publisher=DEFAULT_STAR_FEED_PUBLISHER, **kwargs):

        self.ws_service_template = url + '/?dataset={}&refine.idparc={}'

        self.operators = [o.lower() for o in operators]
        self.timeout = timeout
        self.dataset = dataset

        fail_max = kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_STAR_FAIL'])
        reset_timeout = kwargs.get('circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_STAR_TIMEOUT_S'])

        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None

        self.log = logging.LoggerAdapter(logging.getLogger(__name__), extra={'dataset': self.dataset})

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

        available = jmespath.search('records[0].fields.nombreplacesdisponibles', data)
        occupied = jmespath.search('records[0].fields.nombreplacesoccupees', data)
        # Person with reduced mobility
        available_PRM = jmespath.search('records[0].fields.nombreplacesdisponiblespmr', data)
        occupied_PRM = jmespath.search('records[0].fields.nombreplacesoccupeespmr', data)

        return ParkingPlaces(available, occupied, available_PRM, occupied_PRM)

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_STAR', 30))
    def _call_webservice(self, parking_id):
        try:
            data = self.breaker.call(requests.get, self.ws_service_template.format(self.dataset, parking_id),
                                     timeout=self.timeout)
            # record in newrelic
            self.record_call("OK")
            return data.json()
        except pybreaker.CircuitBreakerError as e:
            msg = 'STAR service dead (error: {})'.format(e)
            self.log.error(msg)
            # record in newrelic
            utils.record_external_failure(msg, 'parking', 'STAR')
        except requests.Timeout as t:
            msg = 'STAR service timeout (error: {})'.format(t)
            self.log.error(msg)
            # record in newrelic
            utils.record_external_failure(msg, 'parking', 'STAR')
        except:
            msg = 'STAR service error'
            self.log.exception(msg)
            # record in newrelic
            utils.record_external_failure(msg, 'parking', 'STAR')

        return None

    def status(self):
        return {'operators': self.operators}

    def feed_publisher(self):
        return self._feed_publisher

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'parking_service': 'STAR', 'dataset': self.dataset, 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('parking_service', params)
