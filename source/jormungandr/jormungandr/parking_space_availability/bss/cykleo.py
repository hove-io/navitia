
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
from jormungandr.parking_space_availability import AbstractParkingPlacesProvider
from jormungandr import cache, app
import pybreaker
import logging
import json
import requests as requests
from jormungandr.ptref import FeedPublisher
from jormungandr.parking_space_availability.bss.stands import Stands


DEFAULT_CYKLEO_FEED_PUBLISHER = {
    'id': 'cykleo',
    'name': 'cykleo',
    'license': 'Private',
    'url': 'www.cykleo.fr'
}


class CykleoError(RuntimeError):
    pass


class CykleoProvider(AbstractParkingPlacesProvider):
    def __init__(self, url, network, service_id, username, password, operators={'cykleo'}, timeout=10,
                 feed_publisher=DEFAULT_CYKLEO_FEED_PUBLISHER, **kwargs):
        self.url = url
        self.network = network.lower()
        self.service_id = service_id
        self.username = username
        self.password = password
        self.operators = [o.lower() for o in operators]
        self.timeout = timeout
        fail_max = kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_CYKLEO_FAIL'])
        reset_timeout = kwargs.get('circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_CYKLEO_TIMEOUT_S'])
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_CYKLEO_JETON', 30))
    def get_access_token(self):
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json"
        }
        data = {"username": self.username, "password": self.password, "serviceId": self.service_id}
        try:
            response = self.breaker.call(requests.post, '{}/pu/auth'.format(self.url),
                                         timeout=self.timeout, headers=headers, data=json.dumps(data), verify=False)
            if not response or response.status_code != 200:
                raise CykleoError('cykleo, Invalid response')
            content = response.json()
            access_token = content.get("access_token")
            if not access_token:
                raise CykleoError('cykleo, access_token not exist in response')
            return access_token
        except Exception as e:
            raise CykleoError('{}'.format(str(e)))

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_CYKLEO', 30))
    def _call_webservice(self):
        try:
            access_token = self.get_access_token()
            headers = {'Authorization': '{}'.format(access_token)}
            data = self.breaker.call(requests.get, '{}/pu/stations/availability'.format(self.url),
                                     timeout=self.timeout, headers=headers, verify=False)
            stands = {}
            for s in data.json():
                stands[str(s['station']['assetStation']['commercialNumber'])] = s
            return stands
        except CykleoError as c:
            logging.getLogger(__name__).error('cykleo error: {}'.format(c))
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('cykleo service dead (error: {})'.format(e))
        except requests.Timeout as t:
            logging.getLogger(__name__).error('cykleo service timeout (error: {})'.format(t))
        except Exception as e:
            logging.getLogger(__name__).exception('cykleo error : {}'.format(str(e)))
        return None

    def support_poi(self, poi):
        properties = poi.get('properties', {})
        return properties.get('operator', '').lower() in self.operators and \
               properties.get('network', '').lower() == self.network

    def status(self):
        return {'network': self.network, 'operators': self.operators}

    def feed_publisher(self):
        return self._feed_publisher

    def __repr__(self):
        #TODO: make this shit python 3 compatible
        return ('cykleo-{}'.format(self.network)).encode('utf-8')

    def get_informations(self, poi):
        ref = poi.get('properties', {}).get('ref')
        data = self._call_webservice()
        station = data.get(ref)
        if not station:
            return None
        if 'availableDockCount' in station \
                and ('availableClassicBikeCount' in station or 'availableElectricBikeCount' in station):
            return Stands(station.get('availableDockCount'),
                          station.get('availableClassicBikeCount', 0) + station.get('availableElectricBikeCount', 0))