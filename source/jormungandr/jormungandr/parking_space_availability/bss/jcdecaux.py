# encoding: utf-8

# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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

from jormungandr import cache, app
from jormungandr.parking_space_availability.bss.common_bss_provider import CommonBssProvider, BssProxyError
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from jormungandr.ptref import FeedPublisher
import datetime

DEFAULT_JCDECAUX_FEED_PUBLISHER = {
    'id': 'jcdecaux',
    'name': 'jcdecaux',
    'license': 'Licence Ouverte / Open License',
    'url': 'https://developer.jcdecaux.com/#/opendata/license',
}


class JcdecauxProvider(CommonBssProvider):

    WS_URL_TEMPLATE = 'https://api.jcdecaux.com/vls/v1/stations/?contract={}&apiKey={}'

    def __init__(
        self,
        network,
        contract,
        api_key,
        operators={'jcdecaux'},
        timeout=10,
        feed_publisher=DEFAULT_JCDECAUX_FEED_PUBLISHER,
        **kwargs
    ):
        self.network = network.lower()
        self.contract = contract
        self.api_key = api_key
        self.operators = [o.lower() for o in operators]
        self.timeout = timeout
        fail_max = kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_JCDECAUX_FAIL'])
        reset_timeout = kwargs.get(
            'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_JCDECAUX_TIMEOUT_S']
        )
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None
        self._data = {}
        self._last_update = datetime.datetime(1970, 1, 1)
        self._update_interval = 30

    def support_poi(self, poi):
        properties = poi.get('properties', {})
        return (
            properties.get('operator', '').lower() in self.operators
            and properties.get('network', '').lower() == self.network
        )

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_JCDECAUX'), 30))
    def _call_webservice(self):
        try:
            data = self.breaker.call(
                requests.get, self.WS_URL_TEMPLATE.format(self.contract, self.api_key), timeout=self.timeout
            )
            stands = {}
            for s in data.json():
                stands[str(s['number'])] = s
            return stands
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('JCDecaux service dead (error: {})'.format(e))
            raise BssProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('JCDecaux service timeout (error: {})'.format(t))
            raise BssProxyError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('JCDecaux error : {}'.format(str(e)))
            raise BssProxyError(str(e))

    def _get_informations(self, poi):
        # Possible status values of the station: OPEN and CLOSED
        ref = poi.get('properties', {}).get('ref')
        service_key = self.WS_URL_TEMPLATE.format(self.contract, self.api_key) + self.network
        data = self._data.get(service_key)
        if data is None:
            self._data[service_key] = self._call_webservice()
            self._last_update = datetime.datetime.utcnow()

        if self._last_update + datetime.timedelta(seconds=self._update_interval) < datetime.datetime.utcnow():
            service_url = self.WS_URL_TEMPLATE.format(self.contract, self.api_key)
            self._data[service_url] = self._call_webservice()
            self._last_update = datetime.datetime.utcnow()

        data = self._data.get(service_key)
        if data and 'status' in data.get(ref, {}):
            if data[ref]['status'] == 'OPEN':
                return Stands(
                    data[ref].get('available_bike_stands', 0),
                    data[ref].get('available_bikes', 0),
                    StandsStatus.open,
                )
            elif data[ref]['status'] == 'CLOSED':
                return Stands(0, 0, StandsStatus.closed)
        return Stands(0, 0, StandsStatus.unavailable)

    def status(self):
        return {'network': self.network, 'operators': self.operators, 'contract': self.contract}

    def feed_publisher(self):
        return self._feed_publisher

    def __repr__(self):
        # TODO: make this shit python 3 compatible
        return ('jcdecaux-{}-{}'.format(self.network, self.contract)).encode('utf-8', 'backslashreplace')
