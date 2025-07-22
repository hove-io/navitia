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
    'instance_config': {
        'external_services_providers': [
            {
                "id": "forseti_vehicle_positions",
                "navitia_service": "vehicle_positions",
                "args": {
                    "service_url": "https://wtf/vehicle_positions",
                    "timeout": 10,
                    "circuit_breaker_max_fail": 4,
                    "circuit_breaker_reset_timeout": 60,
                },
                "class": "jormungandr.external_services.vehicle_position.VehiclePosition",
            }
        ]
    }
}

RESP_VJ_11 = {"vehicle_positions": [{"latitude": 46.0, "longitude": -71.0, "bearing": 214, "speed": 11}]}

RESP_VJ_12 = {
    "vehicle_positions": [
        {
            "latitude": 45.0,
            "longitude": -72.0,
            "bearing": 215,
            "speed": 12,
            "occupancy": "EMPTY",
            "feed_created_at": "2022-01-28T11:51:52Z",
        }
    ]
}

RESP_VJ_13 = {
    "vehicle_positions": [
        {
            "latitude": 43.0,
            "longitude": -69.0,
            "bearing": 218,
            "speed": 18,
            "occupancy": "MANY_SEATS_AVAILABLE",
            "feed_created_at": "2022-01-28T02:38:27Z",
        }
    ]
}


@dataset({'basic_schedule_test': MOCKED_INSTANCE_CONF})
class TestVehiclePosition(AbstractTestFixture):
    def test_vehicle_position(self):
        url = "https://wtf/vehicle_positions"
        mock_requests = MockRequests(
            {
                '{}?{}'.format(url, urlencode({"vehicle_journey_code[]": "vj:l:11"})): (
                    RESP_VJ_11,
                    200,
                ),
                '{}?{}'.format(url, urlencode({"vehicle_journey_code[]": "vj:l:12"})): (
                    RESP_VJ_12,
                    200,
                ),
                '{}?{}'.format(url, urlencode({"vehicle_journey_code[]": "vj:l:13"})): (
                    RESP_VJ_13,
                    200,
                ),
            }
        )

        with mock.patch('requests.get', mock_requests.get):
            query = 'lines/L/vehicle_positions?_current_datetime=20160101T101500'
            response = self.query_region(query)
            vehicle_positions = response['vehicle_positions']
            assert len(vehicle_positions) == 1
            line = vehicle_positions[0]["line"]
            assert line["id"] == "L"
            vehicle_journey_positions = vehicle_positions[0]["vehicle_journey_positions"]
            assert len(vehicle_journey_positions) == 3
            assert vehicle_journey_positions[0]["vehicle_journey"]["id"] == "vehicle_journey:L:11"
            assert vehicle_journey_positions[0]["bearing"] == 214
            assert vehicle_journey_positions[0]["speed"] == 11
            assert vehicle_journey_positions[0]["coord"]["lat"] == "46"
            assert vehicle_journey_positions[0]["coord"]["lon"] == "-71"
            assert "occupancy" not in vehicle_journey_positions[0]
            assert "feed_created_at" not in vehicle_journey_positions[0]

            assert vehicle_journey_positions[1]["vehicle_journey"]["id"] == "vehicle_journey:L:12"
            assert vehicle_journey_positions[1]["bearing"] == 215
            assert vehicle_journey_positions[1]["speed"] == 12
            assert vehicle_journey_positions[1]["coord"]["lat"] == "45"
            assert vehicle_journey_positions[1]["coord"]["lon"] == "-72"
            assert vehicle_journey_positions[1]["occupancy"] == "EMPTY"
            assert vehicle_journey_positions[1]["feed_created_at"] == "20220128T115152"

            assert vehicle_journey_positions[2]["vehicle_journey"]["id"] == "vehicle_journey:L:13"
            assert vehicle_journey_positions[2]["bearing"] == 218
            assert vehicle_journey_positions[2]["speed"] == 18
            assert vehicle_journey_positions[2]["coord"]["lat"] == "43"
            assert vehicle_journey_positions[2]["coord"]["lon"] == "-69"
            assert vehicle_journey_positions[2]["occupancy"] == "MANY_SEATS_AVAILABLE"
            assert vehicle_journey_positions[2]["feed_created_at"] == "20220128T023827"
