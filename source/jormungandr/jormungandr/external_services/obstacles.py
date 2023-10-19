# Copyright (c) 2001-2021, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr import cache, app, new_relic
import pybreaker
import logging
import requests as requests
from six.moves.urllib.parse import urlencode
from jormungandr.interfaces.v1.serializer.obstacles import ObstaclesSerializer


class ObstacleProvider(object):
    """
    Class managing calls to forseti webservice, providing external_services
    """

    def __init__(self, service_url, timeout=2, **kwargs):
        self.logger = logging.getLogger(__name__)
        self.service_url = service_url
        self.timeout = timeout
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_FORSETI_FAIL']),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_FORSETI_TIMEOUT_S']
            ),
        )

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_FORSETI'), 30))
    def _call_webservice(self, arguments):
        """
        Call external_services webservice with URL defined in settings
        :return: data received from the webservice
        """
        logging.getLogger(__name__).debug(
            'forseti external_services service , call url : {}'.format(self.service_url)
        )
        result = None
        try:
            url = "{}?{}".format(self.service_url, urlencode(arguments, doseq=True))
            result = self.breaker.call(requests.get, url=url, timeout=self.timeout)
            self.record_call(url=url, status="OK")
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Service Forseti is dead (error: {})'.format(e))
            self.record_call(url=url, status='failure', reason='circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Forseti service timeout (error: {})'.format(t))
            self.record_call(url=url, status='failure', reason='timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('Forseti service error: {}'.format(e))
            self.record_call(url=url, status='failure', reason=str(e))
        return result

    def record_call(self, url, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'obstacle_service_id': "Forseti", 'status': status, 'obstacle_service_url': url}
        params.update(kwargs)
        new_relic.record_custom_event('obstacle_status', params)

    def get_obstacles(self, arguments):
        """
        Get obstacle information from Forseti webservice
        """
        raw_response = self._call_webservice(arguments)

        return self.response_marshaler(raw_response)

    @classmethod
    def _check_response(cls, response):
        if response is None:
            raise ObstacleError('impossible to access free-floating service')
        if response.status_code == 503:
            raise ObstacleUnavailable('forseti responded with 503')
        if response.status_code != 200:
            error_msg = 'free-floating request failed with HTTP code {}'.format(response.status_code)
            if response.text:
                error_msg += ' ({})'.format(response.text)
            raise ObstacleError(error_msg)

    @classmethod
    def response_marshaler(cls, response):
        cls._check_response(response)
        try:
            json_response = response.json()
        except ValueError:
            logging.getLogger(__name__).error(
                "impossible to get json for response %s with body: %s", response.status_code, response.text
            )
            raise
        return ObstaclesSerializer(json_response).data


class ObstacleError(RuntimeError):
    pass


class ObstacleUnavailable(RuntimeError):
    pass
