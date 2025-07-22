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
from jormungandr.external_services.external_service import AbstractExternalService, ExternalServiceError
from jormungandr.utils import PY3


class VehicleOccupancyProvider(AbstractExternalService):
    """
    Class managing calls to forseti webservice, providing vehicle_occupancies
    """

    def __init__(self, id, service_url, timeout=2, **kwargs):
        self.logger = logging.getLogger(__name__)
        self.service_url = service_url
        self.id = id
        self.timeout = timeout
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_FORSETI_FAIL']),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_FORSETI_TIMEOUT_S']
            ),
        )

    def __repr__(self):
        """
        used as the cache key. we use the id to share the cache between servers in production
        """
        if PY3:
            return self.id
        try:
            return self.id.encode('utf-8', 'backslashreplace')
        except:
            return self.id

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_FORSETI'), 10))
    def get_response(self, arguments):
        """
        Get vehicle_occupancy information from Forseti webservice
        """
        raw_response = self._call_webservice(arguments)

        # We don't need any further action if raw_response is None
        if raw_response is None:
            return None
        resp = self.response_marshaller(raw_response)
        if resp is None:
            return None
        vehicle_occupancies = resp.get('vehicle_occupancies', [])
        if not vehicle_occupancies:
            return None
        first_occupancy = vehicle_occupancies[0]
        return first_occupancy["occupancy"] if "occupancy" in first_occupancy else None

    @classmethod
    def response_marshaller(cls, response):
        try:
            cls._check_response(response)
        except ExternalServiceError as e:
            logging.getLogger(__name__).exception('Forseti service error: {}'.format(e))
            return None

        try:
            json_response = response.json()
        except ValueError:
            logging.getLogger(__name__).error(
                "impossible to get json for response %s with body: %s", response.status_code, response.text
            )
            return None
        return json_response
