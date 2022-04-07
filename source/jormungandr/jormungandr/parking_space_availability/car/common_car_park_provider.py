# encoding: utf-8
# Copyright (c) 2001-2018, Hove and/or its affiliates. All rights reserved.
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

import logging
import pybreaker
import requests as requests

from jormungandr import cache, app, new_relic
from jormungandr.parking_space_availability import AbstractParkingPlacesProvider
from jormungandr.ptref import FeedPublisher

from abc import abstractmethod


class CommonCarParkProvider(AbstractParkingPlacesProvider):
    def __init__(self, url, operators, dataset, timeout, feed_publisher, **kwargs):
        super(CommonCarParkProvider, self).__init__(**kwargs)
        self.ws_service_template = url + '?dataset={}'
        self.operators = [o.lower() for o in operators]
        self.timeout = timeout
        self.dataset = dataset
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None
        self.fail_max = kwargs.get(
            'circuit_breaker_max_fail', app.config.get(str('CIRCUIT_BREAKER_MAX_CAR_PARK_FAIL'), 5)
        )
        self.reset_timeout = kwargs.get(
            'circuit_breaker_reset_timeout', app.config.get(str('CIRCUIT_BREAKER_CAR_PARK_TIMEOUT_S'), 60)
        )
        self.breaker = pybreaker.CircuitBreaker(fail_max=self.fail_max, reset_timeout=self.reset_timeout)
        self.log = logging.LoggerAdapter(logging.getLogger(__name__), extra={'dataset': self.dataset})

        self.api_key = kwargs.get('api_key')

    @abstractmethod
    def process_data(self, data, poi):
        pass

    def get_informations(self, poi):
        if not poi.get('properties', {}).get('ref'):
            return None

        data = self._call_webservice(self.ws_service_template.format(self.dataset))

        if data:
            return self.process_data(data, poi)

    def _has_supported_poi_operator(self, poi):
        properties = poi.get('properties', {})
        operator = properties.get('operator', '').lower()
        return operator in self.operators

    def support_poi(self, poi):
        return self._has_supported_poi_operator(poi)

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_CAR_PARK'), 30))
    def _call_webservice(self, request_url):
        try:
            if self.api_key:
                headers = {'Authorization': 'apiKey {}'.format(self.api_key)}
            else:
                headers = None
            data = self.breaker.call(requests.get, url=request_url, headers=headers, timeout=self.timeout)
            json_data = data.json()
            self.record_call("OK")
            return json_data

        except pybreaker.CircuitBreakerError as e:
            self.log.error('{} service dead (error: {})'.format(self.provider_name, e))
            self.record_call('failure', reason='circuit breaker open')
        except requests.Timeout as t:
            self.log.error('{} service timeout (error: {})'.format(self.provider_name, t))
            self.record_call('failure', reason='timeout')
        except Exception as e:
            self.log.exception('{} service error: {}'.format(self.provider_name, e))
            self.record_call('failure', reason=str(e))
        return None

    def status(self):
        return {'operators': self.operators}

    def feed_publisher(self):
        return self._feed_publisher

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'parking_system_id': self.provider_name, 'dataset': self.dataset, 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('parking_status', params)
