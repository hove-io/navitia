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
from tests.check_utils import get_not_null
import mock
from jormungandr.tests.utils_test import MockRequests

MOCKED_INSTANCE_CONF = {
    'scenario': 'new_default',
    'instance_config': {
        'external_services_providers': [
            {
                "id": "forseti_free_floatings",
                "navitia_service": "free_floatings",
                "args": {
                    "service_url": "http://wtf/free_floatings",
                    "timeout": 10,
                    "circuit_breaker_max_fail": 4,
                    "circuit_breaker_reset_timeout": 60,
                },
                "class": "jormungandr.external_services.free_floating.FreeFloatingProvider",
            }
        ]
    },
}

FREE_FLOATINGS_RESPONSE = {
    "free_floatings": [
        {
            "public_id": "12009",
            "provider_name": "Velib",
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "STATION",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
        },
        {
            "public_id": "FB-435-HD",
            "provider_name": "Cityscoot",
            "id": "Y2l0eXNjb290Ok1PVE9SU0NPT1RFUjpGQi00MzUtSEQ=",
            "type": "MOTORSCOOTER",
            "coord": {"lat": 48.848715, "lon": 2.37618},
            "propulsion": "ELECTRIC",
            "battery": 91,
            "distance": 100.55,
            "attributes": ["ELECTRIC"],
            "deeplink": "https://cityscoot.onelink.me/8GT9?pid=FLUCTUO&c=CS-API-FLUCTUO&af_dp=cityscoot%3A%2F%2Fcities%2F4%2Fscooters%2F18055&af_web_dp=https%3A%2F%2Fwww.cityscoot.eu%2F&af_force_deeplink=true",
        },
        {
            "public_id": "FH-116-WN",
            "provider_name": "Cityscoot",
            "id": "Y2l0eXNjb290Ok1PVE9SU0NPT1RFUjpGSC0xMTYtV04=",
            "type": "MOTORSCOOTER",
            "coord": {"lat": 48.84902833333333, "lon": 2.376105},
            "propulsion": "ELECTRIC",
            "battery": 91,
            "attributes": ["ELECTRIC"],
            "deeplink": "https://cityscoot.onelink.me/8GT9?pid=FLUCTUO&c=CS-API-FLUCTUO&af_dp=cityscoot%3A%2F%2Fcities%2F4%2Fscooters%2F20840&af_web_dp=https%3A%2F%2Fwww.cityscoot.eu%2F&af_force_deeplink=true",
        },
    ],
    "pagination": {"items_on_page": 3, "items_per_page": 5, "start_page": 1, "total_result": 8},
}

#
# def mock_free_floating(_, params):
#     return MockResponse(FREE_FLOATINGS_RESPONSE, 200)
#
#
# @pytest.fixture(scope="function", autouse=True)
# def mock_http_free_floating(monkeypatch):
#     monkeypatch.setattr(
#         'jormungandr.external_services.free_floating.FreeFloatingProvider._call_webservice', mock_free_floating
#     )


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF})
class TestFreeFloating(AbstractTestFixture):
    """
    Test the structure of the freefloatings_nearby api
    """

    def test_freefloatings_nearby(self):
        """
        simple freefloatings_nearby call
        """

        mock_requests = MockRequests(
            {
                'http://wtf/free_floatings?type%5B%5D=None&distance=500&count=5&coord=2.37715%3B48.846781&start_page=1':
                    (
                        FREE_FLOATINGS_RESPONSE,
                        200,
                    )
            }
        )

        with mock.patch('requests.get', mock_requests.get):
            response, status = self.query_no_assert(
                'v1/coverage/main_routing_test/coord/2.37715%3B48.846781/freefloatings_nearby?count=5&start_page=1'
            )

            warnings = get_not_null(response, 'warnings')
            assert len(warnings) == 1
            assert warnings[0]['id'] == 'beta_endpoint'

            free_floatings = get_not_null(response, 'free_floatings')
            assert len(free_floatings) == 3
            # First free_floating doesn't contain all attributes
            assert free_floatings[0]['public_id'] == '12009'
            assert free_floatings[0]['provider_name'] == 'Velib'
            assert free_floatings[0]['type'] == 'STATION'
            assert free_floatings[0]['id'] == 'dmVsaWI6U1RBVElPTjoxMjAwOQ=='
            assert free_floatings[0]['coord']['lat'] == '48.84617146118711'
            assert free_floatings[0]['coord']['lon'] == '2.379306815564632'

            # Second free_floating contains more attributes
            assert free_floatings[1]['public_id'] == 'FB-435-HD'
            assert free_floatings[1]['distance'] == 100.55
            assert free_floatings[1]['provider_name'] == 'Cityscoot'
            assert free_floatings[1]['type'] == 'MOTORSCOOTER'
            assert free_floatings[1]['id'] == 'Y2l0eXNjb290Ok1PVE9SU0NPT1RFUjpGQi00MzUtSEQ='
            assert free_floatings[1]['coord']['lat'] == '48.848715'
            assert free_floatings[1]['coord']['lon'] == '2.37618'
            assert (
                free_floatings[1]['deeplink']
                == 'https://cityscoot.onelink.me/8GT9?pid=FLUCTUO&c=CS-API-FLUCTUO&af_dp=cityscoot%3A%2F%2Fcities%2F4%2Fscooters%2F18055&af_web_dp=https%3A%2F%2Fwww.cityscoot.eu%2F&af_force_deeplink=true'
            )
            assert free_floatings[1]['battery'] == 91
            assert free_floatings[1]['propulsion'] == 'ELECTRIC'
            assert free_floatings[1]['attributes'] == ['ELECTRIC']
            assert 'pagination' in response
            assert response['pagination']['start_page'] == 1
            assert response['pagination']['items_on_page'] == 3
            assert response['pagination']['items_per_page'] == 5
            assert response['pagination']['total_result'] == 8
