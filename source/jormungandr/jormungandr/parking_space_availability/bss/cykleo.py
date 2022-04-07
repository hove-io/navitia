# coding: utf-8

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
from jormungandr import cache, app
import pybreaker
import logging
import json
import requests as requests
from jormungandr.ptref import FeedPublisher
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from jormungandr.parking_space_availability.bss.common_bss_provider import CommonBssProvider, BssProxyError


DEFAULT_CYKLEO_FEED_PUBLISHER = {'id': 'cykleo', 'name': 'cykleo', 'license': 'Private', 'url': 'www.cykleo.fr'}


class CykleoProvider(CommonBssProvider):
    """
    class managing calls to Cykleo external service providing real-time BSS stands availability

    The service requires authentication

    curl example to check/test that external service is working:
    # first get a token
    curl -H 'Content-Type: application/json' -H 'Accept: application/json'
         -d '{"username": "{username}", "password": "{password}", "sphere": "VLS"}'
         -X POST '{url}/bo/auth'
    # then retrieve the whole message referential using the 'access_token' value obtained
    curl -H 'Authorization: Bearer {access_token}' -d 'organization_id={organization_id}'
         -X GET '{url}/bo/stations/availability'

    All variables are provided on connector's init, except for the {access_token}
    Then the connector matches 'station/assetStation/commercialNumber' in Cykleo API with
    the 'ref' value in OSM.

    So in practice it will look like:
    curl -H 'Content-Type: application/json' -H 'Accept: application/json'
         -d '{"username": "toto@bobito.com", "password": "fzeDrof#czea", "sphere": "VLS"}'
         -X POST 'https://url.cykleo.com/bo/auth
    curl -H 'Authorization: Bearer fzkS.Ekb4S.QA' -d 'organization_id=42'
         -X GET 'https://url.cykleo.com/bo/stations/availability'
    """

    def __init__(
        self,
        url,
        network,
        username,
        password,
        operators={'cykleo'},
        verify_certificate=False,
        service_id=None,
        organization_id=None,
        timeout=2,
        feed_publisher=DEFAULT_CYKLEO_FEED_PUBLISHER,
        **kwargs
    ):
        self.url = url
        self.network = network.lower()
        self.service_id = service_id
        self.organization_id = organization_id
        self.verify_certificate = verify_certificate
        self.username = username
        self.password = password
        self.operators = [o.lower() for o in operators]
        self.timeout = timeout
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_CYKLEO_FAIL']),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_CYKLEO_TIMEOUT_S']
            ),
        )
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None

    def service_caller(self, method, url, headers, data=None, params=None):
        try:
            kwargs = {"timeout": self.timeout, "verify": self.verify_certificate, "headers": headers}
            if data:
                kwargs.update({"data": data})
            if params:
                kwargs.update({'params': params})
            response = self.breaker.call(method, url, **kwargs)
            if not response or response.status_code != 200:
                logging.getLogger(__name__).error(
                    'cykleo, Invalid response, status_code: {}'.format(response.status_code)
                )
                raise BssProxyError('non 200 response')
            return response
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('cykleo service dead (error: {})'.format(e))
            raise BssProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('cykleo service timeout (error: {})'.format(t))
            raise BssProxyError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('cykleo error : {}'.format(str(e)))
            raise BssProxyError(str(e))

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_CYKLEO_JETON'), 10 * 60))
    def get_access_token(self):
        headers = {"Content-Type": "application/json", "Accept": "application/json"}
        data = {"username": self.username, "password": self.password, "sphere": "VLS"}
        if self.service_id is not None:
            data.update({"serviceId": self.service_id})

        response = self.service_caller(
            method=requests.post, url='{}/bo/auth'.format(self.url), headers=headers, data=json.dumps(data)
        )
        if not response:
            return None
        content = response.json()
        access_token = content.get("access_token")
        if not access_token:
            logging.getLogger(__name__).error('cykleo, no access_token in response')
            return None
        return access_token

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_CYKLEO'), 30))
    def _call_webservice(self):
        access_token = self.get_access_token()
        headers = {'Authorization': 'Bearer {}'.format(access_token)}
        params = None if self.organization_id is None else {'organization_id': self.organization_id}
        data = self.service_caller(
            method=requests.get,
            url='{}/bo/stations/availability'.format(self.url),
            headers=headers,
            params=params,
        )
        stands = {}
        if not data:
            return stands
        for s in data.json():
            stands[str(s['station']['assetStation']['commercialNumber'])] = s
        return stands

    def support_poi(self, poi):
        properties = poi.get('properties', {})
        return (
            properties.get('operator', '').lower() in self.operators
            and properties.get('network', '').lower() == self.network
        )

    def status(self):
        return {'network': self.network, 'operators': self.operators}

    def feed_publisher(self):
        return self._feed_publisher

    def __repr__(self):
        return ('cykleo-{}'.format(self.network)).encode('utf-8', 'backslashreplace')

    def _get_informations(self, poi):
        ref = poi.get('properties', {}).get('ref')
        if ref is not None:
            ref = ref.lstrip('0')
        data = self._call_webservice()

        # Possible status values of the station: IN_SERVICE, IN_MAINTENANCE, OUT_OF_SERVICE, DISCONNECTED
        # and DECOMMISSIONED
        if not data:
            return Stands(0, 0, StandsStatus.unavailable)
        obj_station = data.get(ref)
        if not obj_station:
            return Stands(0, 0, StandsStatus.unavailable)

        if obj_station.get('station', {}).get('status') == 'IN_SERVICE' and 'availableDockCount' in obj_station:
            return Stands(
                obj_station.get('availableDockCount', 0),
                obj_station.get('availableClassicBikeCount', 0)
                + obj_station.get('availableElectricBikeCount', 0),
                StandsStatus.open,
            )
        elif obj_station.get('station', {}).get('status') in ('OUT_OF_SERVICE', 'DECOMMISSIONED'):
            return Stands(0, 0, StandsStatus.closed)
        else:
            return Stands(0, 0, StandsStatus.unavailable)
