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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from .tests_mechanism import AbstractTestFixture, dataset
import pytest
from jormungandr.tests.utils_test import MockResponse

MOCKED_INSTANCE_CONF = {
    'scenario': 'new_default',
    'instance_config': {
        'external_services_providers': [
            {
                "id": "forseti_vehicle_occupancies",
                "navitia_service": "vehicle_occupancies",
                "args": {
                    "service_url": "http://wtf/vehicle_occupancies",
                    "timeout": 10,
                    "circuit_breaker_max_fail": 4,
                    "circuit_breaker_reset_timeout": 60,
                },
                "class": "jormungandr.external_services.vehicle_occupancy.VehicleOccupancyProvider",
            }
        ]
    },
}

VEHICLE_OCCUPANCIES_RESPONSE = {
    "vehicle_occupancies": [
        {
            "vehiclejourney_id": "vehicle_journey:0:123714928-1",
            "stop_id": "stop_point:0:SP:80:4165",
            "date_time": "2021-03-03T08:56:00+01:00",
            "occupancy": 55,
        }
    ]
}


def mock_free_floating(_, params):
    return MockResponse(VEHICLE_OCCUPANCIES_RESPONSE, 200)


@pytest.fixture(scope="function", autouse=True)
def mock_http_free_floating(monkeypatch):
    monkeypatch.setattr(
        'jormungandr.external_services.vehicle_occupancy.VehicleOccupancyProvider._call_webservice',
        mock_free_floating,
    )


@dataset({'basic_schedule_test': MOCKED_INSTANCE_CONF})
class TestFreeFloating(AbstractTestFixture):
    """
    Test occupancy in each date_time feed by external api /vehicle_occupancies
    """

    query_template_scs = 'stop_points/{sp}/stop_schedules?from_datetime={dt}&show_codes=true{data_freshness}'

    def test_occupancy_values_in_stop_schedules(self):
        # With data_freshness=base_schedule, api vehicle_occupancies is not called
        query = self.query_template_scs.format(
            sp='C:S0', dt='20160102T1100', data_freshness='&data_freshness=base_schedule'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        assert len(stop_schedules) == 1
        assert stop_schedules[0]['occupancy'] == 0

        # With data_freshness=realtime, api vehicle_occupancies is called and occupancy value is updated
        query = self.query_template_scs.format(
            sp='C:S0', dt='20160102T1100', data_freshness='&data_freshness=realtime'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        assert len(stop_schedules) == 1
        assert stop_schedules[0]['occupancy'] == 55