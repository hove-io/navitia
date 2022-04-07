# Copyright (c) 2001-2019, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from .tests_mechanism import AbstractTestFixture, dataset, mock_equipment_providers
from .equipment_mock import *
from tests.check_utils import get_not_null, is_valid_equipment_report

default_date_filter = '_current_datetime=20120801T000000&'
default_line_filter = 'filter=line.uri(A)'
default_line_uri_filter = 'lines/A/'
default_stop_areas_uri_filter = 'stop_areas/stopA/'
default_forbidden_uris_filter = 'forbidden_uris[]=stop_point:stopB'


@dataset({"main_routing_test": {}})
class TestEquipment(AbstractTestFixture):
    """
    Test the structure of the equipment_reports api
    """

    def _check_equipment_report(self, equipment_reports, expected_result):
        """
        Check most important fields into the equipment_reports
        :equipment_reports: The equipment_reports fields of the response
        :expected_result: The expected equipment_reports response
        """
        eq_r_idx = 0
        for line, stop_areas in sorted(expected_result.items()):
            assert equipment_reports[eq_r_idx]['line']['id'] == line
            st_area_eq_idx = 0
            for stop_area, equipment_details in sorted(stop_areas.items()):
                sa_equip = equipment_reports[eq_r_idx]['stop_area_equipments'][st_area_eq_idx]
                assert sa_equip['stop_area']['id'] == stop_area
                if 'equipment_details' in sa_equip:
                    eq_d_idx = 0
                    for eq_id, eq_type, eq_status in sorted(equipment_details):
                        equipments_details = sorted(
                            equipment_reports[eq_r_idx]['stop_area_equipments'][st_area_eq_idx][
                                'equipment_details'
                            ],
                            key=lambda e: e['id'],
                        )
                        equip_dtls = equipments_details[eq_d_idx]
                        assert equip_dtls['id'] == eq_id
                        assert equip_dtls['embedded_type'] == eq_type
                        assert equip_dtls['current_availability']['status'] == eq_status
                        eq_d_idx += 1
                st_area_eq_idx += 1
            eq_r_idx += 1

    def test_equipment_reports(self):
        """
        simple equipment_reports call
        """

        # only elevator within received data
        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=standard_mock_elevator_data,
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query_region('equipment_reports?' + default_date_filter)

        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'

        # Expected response
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
            "D": {"stopA": stopA_equipment_details},
            "K": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "M": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "PM": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
        }

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 7
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

        # only escalator within received data
        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=standard_mock_escalator_data,
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query_region('equipment_reports?' + default_date_filter)

        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'

        # Expected response
        stopA_equipment_details = [
            ("5", "elevator", "unknown"),
            ("1", "escalator", "available"),
            ("2", "escalator", "unavailable"),
            ("3", "escalator", "unknown"),
            ("4", "escalator", "unknown"),
        ]
        stopb_equipment_details = [
            ("6", "elevator", "unknown"),
            ("7", "elevator", "unknown"),
            ("8", "elevator", "unknown"),
            ("9", "elevator", "unknown"),
        ]
        expected_result = {
            "A": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "B": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "C": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "D": {"stopA": stopA_equipment_details},
            "K": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "M": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "PM": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
        }

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 7
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

        # Mixed escalator/elevator within received data
        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=standard_mock_mixed_data,
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query_region('equipment_reports?' + default_date_filter)

        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'

        # Expected response
        stopA_equipment_details = [
            ("5", "elevator", "unknown"),
            ("1", "escalator", "available"),
            ("2", "escalator", "unknown"),
            ("3", "escalator", "available"),
            ("4", "escalator", "unknown"),
        ]
        stopb_equipment_details = [
            ("6", "elevator", "available"),
            ("7", "elevator", "unknown"),
            ("8", "elevator", "unknown"),
            ("9", "elevator", "unknown"),
        ]
        expected_result = {
            "A": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "B": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "C": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "D": {"stopA": stopA_equipment_details},
            "K": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "M": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "PM": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
        }

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 7
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

    def test_equipment_reports_without_config(self):
        """
        call equipment_reports without provider config
        """

        # Provider config is empty
        self.equipment_provider_manager("main_routing_test")._equipment_providers = {}
        response, status = self.query_no_assert(
            'v1/coverage/main_routing_test/equipment_reports?' + default_date_filter
        )

        assert status == 404
        assert "No code type exists into equipment provider" in response['message']

    def test_equipment_reports_with_wrong_id(self):
        """
        wrong id test
        """

        # Bad id inside received data
        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=wrong_mock_with_bad_id_data,
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query_region('equipment_reports?' + default_date_filter)

        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'

        # Expected response
        stopA_equipment_details = [
            ("5", "elevator", "unknown"),
            ("1", "escalator", "unknown"),
            ("2", "escalator", "unknown"),
            ("3", "escalator", "unknown"),
            ("4", "escalator", "unknown"),
        ]
        stopb_equipment_details = [
            ("6", "elevator", "unknown"),
            ("7", "elevator", "unknown"),
            ("8", "elevator", "unknown"),
            ("9", "elevator", "unknown"),
        ]
        expected_result = {
            "A": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "B": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "C": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "D": {"stopA": stopA_equipment_details},
            "K": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "M": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
            "PM": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details},
        }

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 7
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

    def test_equipment_reports_with_filter(self):
        """
        added an extra filter
        """

        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=standard_mock_elevator_data,
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query_region('equipment_reports?' + default_date_filter + default_line_filter)

        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'

        # Expected response
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
        # filtered with the A line
        expected_result = {"A": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details}}

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 1
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

    def test_equipment_reports_with_forbidden_uris(self):
        """
        with forbidden_uris parameters
        """

        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=standard_mock_elevator_data,
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query_region('equipment_reports?' + default_date_filter + default_forbidden_uris_filter)

        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'

        # Expected response
        stopA_equipment_details = [
            ("5", "elevator", "unknown"),
            ("1", "escalator", "unknown"),
            ("2", "escalator", "unknown"),
            ("3", "escalator", "unknown"),
            ("4", "escalator", "unknown"),
        ]
        # stop B is forbidden and there is only the D line which doesn't have stop point B.
        expected_result = {"D": {"stopA": stopA_equipment_details}}

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 1
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

    def test_equipment_reports_with_uri(self):
        """
        with uri filter
        """

        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data=standard_mock_elevator_data,
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query_region(default_line_uri_filter + 'equipment_reports?' + default_date_filter)

        # Expected response
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
        # filtered with the A line that serve stopA and stopB
        expected_result = {"A": {"stopA": stopA_equipment_details, "stopB": stopb_equipment_details}}

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 1
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)

        response = self.query_region(default_stop_areas_uri_filter + 'equipment_reports?' + default_date_filter)

        # Filtered with the stopA stop_area.
        # The A, B, C, D, M Line serve stopA
        expected_result = {
            "A": {"stopA": stopA_equipment_details},
            "B": {"stopA": stopA_equipment_details},
            "C": {"stopA": stopA_equipment_details},
            "D": {"stopA": stopA_equipment_details},
            "K": {"stopA": stopA_equipment_details},
            "M": {"stopA": stopA_equipment_details},
            "PM": {"stopA": stopA_equipment_details},
        }

        equipment_reports = get_not_null(response, 'equipment_reports')
        assert len(equipment_reports) == 7
        for equipment_report in equipment_reports:
            is_valid_equipment_report(equipment_report)
        self._check_equipment_report(equipment_reports, expected_result)
