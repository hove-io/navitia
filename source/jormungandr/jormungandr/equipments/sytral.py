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

from jormungandr import cache, app, new_relic
from navitiacommon import type_pb2
from dateutil import parser
from jormungandr.utils import date_to_timestamp, PY3

import pybreaker
import logging
import jmespath
import requests as requests
import six


class SytralProvider(object):
    """
    Class managing calls to SytralRT webservice, providing real-time equipment details
    """

    def __init__(self, provider_id, url, timeout=2, code_types=["TCL_ESCALIER", "TCL_ASCENSEUR"], **kwargs):
        self.logger = logging.getLogger(__name__)
        self.url = url
        self.provider_id = provider_id
        self.timeout = timeout
        self.code_types = code_types
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_SYTRAL_FAIL']),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_SYTRAL_TIMEOUT_S']
            ),
        )

    def get_informations_for_journeys(self, stop_points_list):
        """
        Get equipment information from Sytral webservice and update response accordingly
        """
        data = self._call_webservice()

        if data:
            return self._process_for_journeys(data, stop_points_list)

    def get_informations_for_equipment_reports(self, stop_area_equipments_list):
        """
        Get equipment information from Sytral webservice and update response accordingly
        """
        data = self._call_webservice()

        if data:
            return self._process_for_equipment_reports(data, stop_area_equipments_list)

    def __repr__(self):
        """
        used as the cache key. we use the provider_id to share the cache between servers in production
        """
        if PY3:
            return self.provider_id
        try:
            return self.provider_id.encode('utf-8', 'backslashreplace')
        except:
            return self.provider_id

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_SYTRAL'), 30))
    def _call_webservice(self):
        """
        Call equipments webservice with URL defined in settings
        :return: data received from the webservice
        """
        logging.getLogger(__name__).debug('sytralRT RT service , call url : {}'.format(self.url))
        result = None
        try:
            response = self.breaker.call(requests.get, url=self.url, timeout=self.timeout)
            result = response.json()
            self.record_call("OK")
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Service SytralRT is dead (error: {})'.format(e))
            self.record_call('failure', reason='circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('SytralRT service timeout (error: {})'.format(t))
            self.record_call('failure', reason='timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('SytralRT service error: {}'.format(e))
            self.record_call('failure', reason=str(e))
        return result

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'parking_system_id': "SytralRT", 'dataset': "sytral", 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('parking_status', params)

    def _fill_equipment_details(self, equipment_form_web_service, equipment_details):
        equipment_details.id = equipment_form_web_service['id']
        equipment_details.name = equipment_form_web_service['name']
        equipment_details.embedded_type = type_pb2.EquipmentDetails.EquipmentType.Value(
            equipment_form_web_service['embedded_type']
        )
        equipment_details.current_availability.status = type_pb2.CurrentAvailability.EquipmentStatus.Value(
            equipment_form_web_service['current_availaibity']['status']
        )
        current_availaibity = equipment_form_web_service['current_availaibity']
        for period in current_availaibity['periods']:
            p = equipment_details.current_availability.periods.add()
            p.begin = date_to_timestamp(parser.parse(period['begin']))
            p.end = date_to_timestamp(parser.parse(period['end']))
        equipment_details.current_availability.updated_at = current_availaibity['updated_at']
        equipment_details.current_availability.cause.label = current_availaibity['cause']['label']
        equipment_details.current_availability.effect.label = current_availaibity['effect']['label']

    def _fill_default_equipment_details(self, id, embedded_type, equipment_details):
        equipment_details.id = id
        equipment_details.embedded_type = type_pb2.EquipmentDetails.EquipmentType.Value(embedded_type)
        equipment_details.current_availability.status = type_pb2.CurrentAvailability.EquipmentStatus.Value(
            "unknown"
        )

    def _embedded_type(self, embedded_type_sytral):
        if embedded_type_sytral == "TCL_ASCENSEUR":
            return "elevator"
        elif embedded_type_sytral == "TCL_ESCALIER":
            return "escalator"
        else:
            self.logger.exception('impossible to use {} sytral type'.format(embedded_type_sytral))
            return ""

    def _process_for_journeys(self, data, stop_points_list):
        """
        For each stop point within journeys response, the structure 'equipment_details' is updated if the corresponding code is present
        :param data: equipments data received from the webservice
        :param stop_points_list: list of stop_points from the protobuf response
        """
        for st in stop_points_list:
            for code in st.codes:
                if code.type in self.code_types:
                    equipments_list = jmespath.search("equipments_details[?id=='{}']".format(code.value), data)

                    if equipments_list:
                        equipment = equipments_list[0]
                        # Fill PB
                        equipment_details = st.equipment_details.add()
                        self._fill_equipment_details(
                            equipment_form_web_service=equipment, equipment_details=equipment_details
                        )

    def _process_for_equipment_reports(self, data, stop_area_equipments_list):
        """
        For each stop_area_equipments within equipment_reports response, the structure 'equipment_details' is updated if the corresponding code is present
        :param data: equipments data received from the webservice
        :param stop_area_equipments_list: list of stop_area_equipments from the protobuf response
        """

        for sae in stop_area_equipments_list:
            """
            Codes might be duplicated across different stop points.
            Because we report equipments on a stop area basis, we  don't want them duplicated
            """

            unique_codes = {six.text_type(code): code for st in sae.stop_area.stop_points for code in st.codes}
            for code in unique_codes.values():
                if code.type in self.code_types:
                    equipments_list = jmespath.search("equipments_details[?id=='{}']".format(code.value), data)

                    if equipments_list:
                        equipment = equipments_list[0]
                        # Fill PB
                        equipment_details = sae.equipment_details.add()
                        self._fill_equipment_details(
                            equipment_form_web_service=equipment, equipment_details=equipment_details
                        )
                    else:
                        equipment_details = sae.equipment_details.add()
                        self._fill_default_equipment_details(
                            id=code.value,
                            embedded_type=self._embedded_type(code.type),
                            equipment_details=equipment_details,
                        )
