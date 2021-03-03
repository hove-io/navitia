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
import abc
import six

from jormungandr import cache, app, new_relic
import pybreaker
import logging
import requests as requests
from six.moves.urllib.parse import urlencode
from jormungandr.interfaces.v1.serializer.free_floating import FreeFloatingsSerializer


class ExternalServiceError(RuntimeError):
    def __init__(self, reason, http_status=None, http_reason=None, http_txt=None):
        super(RuntimeError, self).__init__(reason)
        self.http_status = http_status
        self.http_reason = http_reason
        self.http_txt = http_txt

    def get_params(self):
        return {
            'reason': str(self),
            'http_status': self.http_status,
            'http_reason': self.http_reason,
            'http_response': self.http_txt,
        }


@six.add_metaclass(abc.ABCMeta)
class AbstractExternalService(object):
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
        params = {'freefloating_service_id': "Forseti", 'status': status, 'freefloating_service_url': url}
        params.update(kwargs)
        new_relic.record_custom_event('freefloating_status', params)

    @abc.abstractmethod
    def get_response(self, arguments):
        """
        Get free-floating information from Forseti webservice
        """
        pass

    @classmethod
    def _check_response(cls, response):
        if response is None:
            raise FreeFloatingError('impossible to access free-floating service')
        if response.status_code == 503:
            raise FreeFloatingUnavailable('forseti responded with 503')
        if response.status_code != 200:
            error_msg = 'free-floating request failed with HTTP code {}'.format(response.status_code)
            if response.text:
                error_msg += ' ({})'.format(response.text)
            raise FreeFloatingError(error_msg)


class FreeFloatingError(RuntimeError):
    pass


class FreeFloatingUnavailable(RuntimeError):
    pass
