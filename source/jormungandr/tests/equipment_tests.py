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
from .tests_mechanism import AbstractTestFixture, dataset, mock_equipment_providers
from .equipment_mock import *
from tests.check_utils import get_not_null, is_valid_equipment_report

default_date_filter = '_current_datetime=20120801T000000&'
TCL_escalator_filter = 'filter=stop_point.has_code_type(TCL_ESCALIER)&'
TCL_elevator_filter = 'filter=stop_point.has_code_type(TCL_ASCENCEUR)&'
TCL_escalator_elevator_filter = 'filter=stop_point.has_code_type(TCL_ASCENCEUR, TCL_ESCALIER)&'


@dataset({"main_routing_test": {}})
class TestEquipment(AbstractTestFixture):
    """
    Test the structure of the equipment_reports api
    """

    def _check_equipment_report(self, equipment_reports, expected_result):
        i = 0
        for line, stop_points in sorted(expected_result.items()):
            print("INFO---> ", line)
            assert equipment_reports[i]['line']['id'] == line
            j = 0
            for stop_point, equipment_details in sorted(stop_points.items()):
                assert equipment_reports[i]['stop_area_equipments'][j]['stop_area']['id'] == stop_point
                if 'equipment_details' in equipment_reports[i]['stop_area_equipments'][j]:
                    eq_d_idx = 0
                    for eq_id, eq_type, eq_status in equipment_details:
                        assert (
                            equipment_reports[i]['stop_area_equipments'][j]['equipment_details'][eq_d_idx]['id']
                            == eq_id
                        )
                        assert (
                            equipment_reports[i]['stop_area_equipments'][j]['equipment_details'][eq_d_idx][
                                'embedded_type'
                            ]
                            == eq_type
                        )
                        assert (
                            equipment_reports[i]['stop_area_equipments'][j]['equipment_details'][eq_d_idx][
                                'current_availability'
                            ]['status']
                            == eq_status
                        )
                        eq_d_idx = eq_d_idx + 1
                j = j + 1
            i = i + 1

    def test_equipment_reports_end_point(self):
        """
        simple equipment_reports call
        """
        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=standard_mock_elevator_data,
        )
        response = self.query_region('equipment_reports?' + default_date_filter)

        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'

        equipment_reports = get_not_null(response, 'equipment_reports')
        import json

        print("resp ", json.dumps(equipment_reports, indent=4))

        stopA_equipment_details = [
            ("5", "elevator", "unknown"),
            ("1", "escalator", "unknown"),
            ("2", "escalator", "unknown"),
            ("3", "escalator", "unknown"),
            ("4", "escalator", "unknown"),
        ]
        stopb_equipment_details = [
            ("6", "elevator", "available"),
            ("7", "elevator", "unavailable"),
            ("8", "elevator", "unknown"),
            ("9", "elevator", "unknown"),
        ]
        expected_result = {
            "A": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "B": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "C": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "D": {"stopA": stopA_equipment_details, "stopC": []},
            "M": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
        }

        assert len(equipment_reports) == 5
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

        # with mock_equipment_providers(data=standard_mock_elevator_data):
        # response = self.query_region('equipment_reports?' + default_date_filter)

        # equipment_reports = get_not_null(response, 'equipment_reports')
        # for equipment_report in equipment_reports:
        # is_valid_equipment_report(equipment_report)

        # with mock_equipment_providers(data=standard_mock_mixed_data):
        # response = self.query_region('equipment_reports?' + default_date_filter)

        # equipment_reports = get_not_null(response, 'equipment_reports')
        # for equipment_report in equipment_reports:
        # is_valid_equipment_report(equipment_report)

    # def test_equipment_reports_with_filter(self):
    # """
    # equipment_reports call with filter
    # """
    # with mock_equipment_providers(data=standard_mock_escalator_data):
    # response = self.query_region('equipment_reports?' + default_date_filter + TCL_escalator_filter)

    # equipment_reports = get_not_null(response, 'equipment_reports')
    # for equipment_report in equipment_reports:
    # is_valid_equipment_report(equipment_report)

    # with mock_equipment_providers(data=standard_mock_elevator_data):
    # response = self.query_region('equipment_reports?' + default_date_filter + TCL_elevator_filter)

    # equipment_reports = get_not_null(response, 'equipment_reports')
    # for equipment_report in equipment_reports:
    # is_valid_equipment_report(equipment_report)

    # with mock_equipment_providers(data=standard_mock_mixed_data):
    # response = self.query_region('equipment_reports?' + default_date_filter + TCL_escalator_filter)

    # equipment_reports = get_not_null(response, 'equipment_reports')
    # for equipment_report in equipment_reports:
    # is_valid_equipment_report(equipment_report)

    # def test_equipment_reports_with_wrong_id(self):

    # """
    # wrong id test
    # """
    # with mock_equipment_providers(data=wrong_mock_with_bad_id_data):
    # response = self.query_region('equipment_reports?' + default_date_filter)
