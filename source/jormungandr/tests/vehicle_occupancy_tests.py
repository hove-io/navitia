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
from .tests_mechanism import AbstractTestFixture, dataset
from six.moves.urllib.parse import urlencode
import mock
from jormungandr.tests.utils_test import MockRequests

MOCKED_INSTANCE_CONF = {
    'scenario': 'new_default',
    'instance_config': {
        'external_services_providers': [
            {
                "id": "forseti_vehicle_occupancies",
                "navitia_service": "vehicle_occupancies",
                "args": {
                    "service_url": "https://wtf/vehicle_occupancies",
                    "timeout": 10,
                    "circuit_breaker_max_fail": 4,
                    "circuit_breaker_reset_timeout": 60,
                },
                "class": "jormungandr.external_services.vehicle_occupancy.VehicleOccupancyProvider",
            }
        ]
    },
}

RESP_1 = {
    "vehicle_occupancies": [
        {
            "vehiclejourney_id": "vehicle_journey:0:123714928-1",
            "stop_id": "stop_point:0:SP:80:4165",
            "date_time": "2021-03-03T08:56:00+01:00",
            "occupancy": "STANDING_ROOM_ONLY",
        }
    ]
}
RESP_2 = {
    "vehicle_occupancies": [
        {
            "vehiclejourney_id": "vehicle_journey:M:14",
            "stop_id": "S11",
            "date_time": "2021-03-03T08:56:00+01:00",
            "occupancy": "EMPTY",
        }
    ]
}

RESP_3 = {"vehicle_occupancies": []}
RESP_4 = None

RESP_5 = {}

url = "https://wtf/vehicle_occupancies"
mock_requests = MockRequests(
    {
        '{}?{}'.format(
            url,
            urlencode({"vehicle_journey_code[]": "vehicle_journey:C:vj1", "stop_point_code[]": "C:S0"}),
        ): (
            RESP_1,
            200,
        ),
        '{}?{}'.format(
            url,
            urlencode({"vehicle_journey_code[]": "vehicle_journey:M:14", "stop_point_code[]": "S11"}),
        ): (
            RESP_2,
            200,
        ),
    }
)


@dataset({'basic_schedule_test': MOCKED_INSTANCE_CONF})
class TestOccupancy(AbstractTestFixture):
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
        assert 'occupancy' not in stop_schedules[0]

        # With data_freshness=realtime, api vehicle_occupancies is called and occupancy value is updated
        query = self.query_template_scs.format(
            sp='C:S0', dt='20160102T1100', data_freshness='&data_freshness=realtime'
        )
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region(query)
            stop_schedules = response['stop_schedules'][0]['date_times']
            assert len(stop_schedules) == 1
            assert stop_schedules[0]['occupancy'] == "STANDING_ROOM_ONLY"

    def test_occupancy_empty_vehicle(self):
        query = self.query_template_scs.format(
            sp='S11', dt='20160101T080000', data_freshness='&data_freshness=realtime'
        )
        with mock.patch('requests.get', mock_requests.get):
            response = self.query_region(query)
            stop_schedules = response['stop_schedules'][0]['date_times']
            assert len(stop_schedules) == 1
            assert stop_schedules[0]['occupancy'] == "EMPTY"

    def test_occupancy_empty_list_occupancies(self):
        query = self.query_template_scs.format(
            sp='stopP2', dt='20160103T100000', data_freshness='&data_freshness=realtime'
        )
        response = self.query_region('vehicle_journeys?headsign=vjP:1')
        vehicle_journeys = response["vehicle_journeys"]
        assert len(vehicle_journeys) == 2
        realtime_vj = next(
            (vj for vj in vehicle_journeys if vj["id"].startswith("vehicle_journey:vjP:1:RealTime:")), None
        )
        assert realtime_vj

        m_requests = MockRequests(
            {
                '{}?{}'.format(
                    url,
                    urlencode({"vehicle_journey_code[]": realtime_vj["id"], "stop_point_code[]": "stopP2"}),
                ): (
                    RESP_3,
                    200,
                )
            }
        )

        with mock.patch('requests.get', m_requests.get):
            response = self.query_region(query)
            date_times = response['stop_schedules'][0]['date_times']
            assert len(date_times) == 1
            assert "occupancy" not in date_times[0]

    def test_occupancy_empty_list_occupancies_on_error(self):
        # Test on response = None
        response = self.query_region('vehicle_journeys?headsign=vjQ:1')
        vehicle_journeys = response["vehicle_journeys"]
        assert len(vehicle_journeys) == 2
        realtime_vj = next(
            (vj for vj in vehicle_journeys if vj["id"].startswith("vehicle_journey:vjQ:1:RealTime:")), None
        )
        assert realtime_vj

        m_requests = MockRequests(
            {
                '{}?{}'.format(
                    url,
                    urlencode(
                        {
                            "vehicle_journey_code[]": realtime_vj["id"],
                            "stop_point_code[]": "stopQ2",
                        }
                    ),
                ): (
                    RESP_4,
                    200,
                ),
                '{}?{}'.format(
                    url,
                    urlencode(
                        {"vehicle_journey_code[]": "vehicle_journey:vj:freq", "stop_point_code[]": "stopf1"}
                    ),
                ): (
                    RESP_5,
                    503,
                ),
            }
        )
        query = self.query_template_scs.format(
            sp='stopQ2', dt='20160103T100000', data_freshness='&data_freshness=realtime'
        )
        with mock.patch('requests.get', m_requests.get):
            response = self.query_region(query)
            date_times = response['stop_schedules'][0]['date_times']
            assert len(date_times) == 1
            assert "occupancy" not in date_times[0]

        # Test on response.status_code = 503
        query = self.query_template_scs.format(
            sp='stopf1', dt='20160103T100000', data_freshness='&data_freshness=realtime'
        )
        with mock.patch('requests.get', m_requests.get):
            response = self.query_region(query)
            date_times = response['stop_schedules'][0]['date_times']
            assert len(date_times) == 3
            assert "occupancy" not in date_times[0]
