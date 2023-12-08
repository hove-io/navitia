# -*- coding: utf-8 -*-

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

from .tests_mechanism import AbstractTestFixture
from .journey_common_tests import *
from copy import deepcopy


OLYMPIC_SITES = {
    "poi:BCY": {
        "name": "Site Olympique JO2024: Arena Bercy (Paris)",
        "events": [
            {
                "event": "100 m haies",
                "from_datetime": "20230714T100000",
                "to_datetime": "20230714T120000",
                "departure_scenario": "scenario a",
              "arrival_scenario": "scenario a"
            },
            {
                "event": "400 m haies",
                "from_datetime": "20230714T120000",
                "to_datetime": "20230714T160000",
                "departure_scenario": "scenario b",
                "to_datetime": "20230714T160000",
                "arrival_scenario": "scenario b"
            }
        ],
        "strict": False,
        "scenarios": {
            "scenario a": {
                "stop_points": {
                    "stop_point:IDFM:463685": {
                        "name": "Bercy - Arena (Paris)",
                        "attractivity": 1,
                        "virtual_fallback": 10
                    },
                    "stop_point:IDFM:463686": {
                        "name": "Pont de Tolbiac (Paris)",
                        "attractivity": 3,
                        "virtual_fallback": 150
                    }
                }
            },
            "scenario b": {
                "stop_points": {
                    "stop_point:IDFM:463685": {
                        "name": "Bercy - Arena (Paris)",
                        "attractivity": 1,
                        "virtual_fallback": 10
                    },
                    "stop_point:IDFM:463686": {
                        "name": "Pont de Tolbiac (Paris)",
                        "attractivity": 3,
                        "virtual_fallback": 150
                    }
                }
            },
            "scenario c": {
                "stop_points": {
                    "stop_point:IDFM:463685": {
                        "name": "Bercy - Arena (Paris)",
                        "attractivity": 1,
                        "virtual_fallback": 10
                    },
                    "stop_point:IDFM:463686": {
                        "name": "Pont de Tolbiac (Paris)",
                        "attractivity": 3,
                        "virtual_fallback": 150
                    }
                },
                "addtionnal_parameters": {"max_walking_duration_to_pt": 13000}
            }
        }
    }
}


@dataset({"main_routing_test": {}, "departure_board_test": {}})
class TestOpgStatus(AbstractTestFixture):
    def test_with_olympic_sites(self):
        instance = i_manager.instances["main_routing_test"]

        instance.olympic_site_params_manager.olympic_site_params = deepcopy(OLYMPIC_SITES)
        instance.olympic_site_params_manager.str_datetime_time_stamp(
            instance.olympic_site_params_manager.olympic_site_params
        )

        response = self.query("/v1/coverage/main_routing_test/opg_status")
        assert isinstance(response, dict)
        assert len(response) == 1
        assert "poi:BCY" in response

    def test_coord_valid(self):
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager.olympic_site_params = deepcopy(OLYMPIC_SITES)
        instance.olympic_site_params_manager.str_datetime_time_stamp(
            instance.olympic_site_params_manager.olympic_site_params
        )
        response, status_code = self.query_no_assert("/v1/coverage/0.000001;0.000898311281954/opg_status")
        assert isinstance(response, dict)
        assert len(response) == 1
        assert "poi:BCY" in response

    def test_without_olympic_sites(self):
        response, status_code = self.query_no_assert("/v1/coverage/departure_board_test/opg_status")
        assert isinstance(response, dict)
        assert status_code == 200
        assert len(response) == 0

    def test_instance_invalid(self):
        response, status_code = self.query_no_assert("/v1/coverage/aa/opg_status")
        assert isinstance(response, dict)
        assert status_code == 404
        assert "error" in response
        assert response["error"]["message"] == "The region aa doesn't exists"

    def test_coord_invalid(self):
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager.olympic_site_params = deepcopy(OLYMPIC_SITES)
        instance.olympic_site_params_manager.str_datetime_time_stamp(
            instance.olympic_site_params_manager.olympic_site_params
        )
        response, status_code = self.query_no_assert("/v1/coverage/10.0;10/opg_status")
        assert isinstance(response, dict)
        assert status_code == 404
        assert "error" in response
        assert response["error"]["message"] == "No region available for the coordinates:10.0, 10.0"
