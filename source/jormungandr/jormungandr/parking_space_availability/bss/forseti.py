# coding: utf-8

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
from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr import cache, app
import pybreaker
import logging
import requests as requests
from jormungandr.ptref import FeedPublisher
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from jormungandr.parking_space_availability.bss.common_bss_provider import CommonBssProvider, BssProxyError
import six

DEFAULT_FORSETI_FEED_PUBLISHER = {
    'id': 'forseti',
    'name': 'forseti',
    'license': 'Private',
    'url': 'www.navitia.io',
}


class ForsetiProvider(CommonBssProvider):
    """
    class managing calls to Forseti external service providing real-time BSS stands availability

    """

    def __init__(
        self,
        service_url,
        distance=50,
        organizations=[],
        feed_publisher=DEFAULT_FORSETI_FEED_PUBLISHER,
        timeout=2,
        **kwargs
    ):
        self.logger = logging.getLogger(__name__)
        self.service_url = service_url
        self.distance = distance
        self.timeout = timeout
        self.network = "Forseti"
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_FORSETI_FAIL']),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_FORSETI_TIMEOUT_S']
            ),
        )

        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None
        if not isinstance(organizations, list):
            self.organizations = organizations.strip('[').strip(']').split(',')
        else:
            self.organizations = organizations

    def service_caller(self, method, url):
        try:
            response = self.breaker.call(method, url, timeout=self.timeout, verify=False)
            if not response or response.status_code != 200:
                logging.getLogger(__name__).error(
                    'Forseti, Invalid response, status_code: {}'.format(response.status_code)
                )
                raise BssProxyError('non 200 response')
            return response
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('forseti service dead (error: {})'.format(e))
            raise BssProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('forseti service timeout (error: {})'.format(t))
            raise BssProxyError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('forseti error : {}'.format(str(e)))
            raise BssProxyError(str(e))

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_FORSETI'), 30))
    def _call_webservice(self, arguments):
        url = "{}?{}".format(self.service_url, arguments)
        data = self.service_caller(method=requests.get, url=url)
        return data.json()

    def support_poi(self, poi):
        return True

    def status(self):
        # return {'network': self.network, 'operators': self.operators}
        return {
            'id': six.text_type(self.network),
            'url': self.service_url,
            'class': self.__class__.__name__,
        }

    def feed_publisher(self):
        return self._feed_publisher

    def _get_informations(self, poi):
        longitude = poi.get('coord', {}).get('lon', None)
        latitude = poi.get('coord', {}).get('lat', None)
        if latitude is None or longitude is None:
            return Stands(0, 0, StandsStatus.unavailable)

        params_organizations = ''
        for param in self.organizations:
            params_organizations += '&organization[]={}'.format(param)

        # /stations?coord=lon%3Blat&distance=self.distance&organization[]=org1&organization[]=org2 ...
        arguments = 'coord={}%3B{}&distance={}{}'.format(
            longitude, latitude, self.distance, params_organizations
        )
        data = self._call_webservice(arguments)

        if not data:
            return Stands(0, 0, StandsStatus.unavailable)
        obj_stations = data.get('stations', [])

        if not obj_stations:
            return Stands(0, 0, StandsStatus.unavailable)
        vehicle_count = 0
        for v in obj_stations[0].get('vehicles'):
            vehicle_count = vehicle_count + v.get('count', 0)

        stand = Stands(obj_stations[0].get('docks', {}).get('available', 0), vehicle_count, StandsStatus.open)
        return stand
