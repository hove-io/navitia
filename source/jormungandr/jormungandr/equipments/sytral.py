# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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

from jormungandr import cache, app
from navitiacommon import type_pb2
from dateutil import parser
from jormungandr.utils import date_to_timestamp

import pybreaker
import logging
import jmespath

SYTRAL_TYPE_PREFIX = "TCL_"


class SytralProvider(object):
    """
    Class managing calls to SytralRT webservice, providing real-time equipment details
    """

    def __init__(self, url, timeout=2, **kwargs):
        self.url = url + '/equipments'
        self.timeout = timeout
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_SYTRAL_FAIL']),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_SYTRAL_TIMEOUT_S']
            ),
        )

    def get_informations(self, stop_points_list):
        """
        Get equipment information from Sytral webservice and update response accordingly
        """
        data = self._call_webservice()

        if data:
            return self._process_data(data, stop_points_list)

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_SYTRAL'), 30))
    def _call_webservice(self):
        """
        Call equipments webservice with URL defined in settings
        :return: data received from the webservice

        NOTE: CALL TO ACTUAL WEBSERVICE WILL BE DONE WHEN IT WILL BE AVAILABLE (NAVP-1231)
        """
        logging.getLogger(__name__).debug('sytralRT RT service , call url : {}'.format(self.url))
        pass

    def _process_data(self, data, stop_points_list):
        """
        For each stop point within journeys response, the structure 'equipment_details' is updated if the corresponding code is present
        :param data: equipments data received from the webservice
        :param stop_points_list: list of stop_points from the protobuf response
        """
        for st in stop_points_list:
            for code in st.codes:
                if SYTRAL_TYPE_PREFIX in code.type:
                    equipments_list = jmespath.search(
                        'equipments_details[?to_number(id)==`{}`]'.format(code.value), data
                    )

                    if equipments_list:
                        equipment = equipments_list[0]
                        # Fill PB
                        details = st.equipment_details.add()
                        details.id = equipment['id']
                        details.name = equipment['name']
                        details.embedded_type = type_pb2.EquipmentDetails.EquipmentType.Value(
                            '{}'.format(equipment['embedded_type'])
                        )
                        details.current_availability.status = type_pb2.CurrentAvailability.EquipmentStatus.Value(
                            '{}'.format(equipment['current_availaibity']['status'])
                        )
                        for period in equipment['current_availaibity']['periods']:
                            p = details.current_availability.periods.add()
                            p.begin = date_to_timestamp(parser.parse(period['begin']))
                            p.end = date_to_timestamp(parser.parse(period['end']))
                        details.current_availability.updated_at = equipment['current_availaibity']['updated_at']
                        details.current_availability.cause.label = equipment['current_availaibity']['cause']['label']
                        details.current_availability.effect.label = equipment['current_availaibity']['effect']['label']
