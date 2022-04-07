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
from jormungandr import app
import pybreaker
import logging
from jormungandr.interfaces.v1.serializer.free_floating import FreeFloatingsSerializer
from jormungandr.external_services.external_service import AbstractExternalService, ExternalServiceError


class FreeFloatingProvider(AbstractExternalService):
    """
    Class managing calls to forseti webservice, providing free_floating
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

    def get_response(self, arguments):
        """
        Get free-floating information from Forseti webservice
        """
        raw_response = self._call_webservice(arguments)

        return self.response_marshaller(raw_response, arguments)

    @classmethod
    def response_marshaller(cls, response, args):
        try:
            cls._check_response(response)
        except ExternalServiceError as e:
            logging.getLogger(__name__).exception('Forseti service error: {}'.format(e))
            return None

        try:
            json_response = response.json()
            json_response = cls.build_pagination(json_response, args)
        except ValueError:
            logging.getLogger(__name__).error(
                "impossible to get json for response %s with body: %s", response.status_code, response.text
            )
            return None
        return FreeFloatingsSerializer(json_response).data

    @classmethod
    def build_pagination(cls, resp, args):
        # Update elements in the pagination if present in the response forseti else create
        if resp.get('pagination') is None:
            resp['pagination'] = {}
        pagination = resp['pagination']
        resp['pagination']['items_on_page'] = pagination.get(
            'items_on_page', len(resp.get('free_floatings', []))
        )
        resp['pagination']['items_per_page'] = pagination.get('items_per_page', args.get('count'))
        resp['pagination']['start_page'] = pagination.get('start_page', args.get('start_page'))
        resp['pagination']['total_result'] = pagination.get('total_result', len(resp.get('free_floatings', [])))
        return resp
