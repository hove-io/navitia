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
from six.moves.urllib.parse import urlencode
from jormungandr.tests.utils_test import MockRequests

MOCKED_INSTANCE_CONF = {
    'scenario': 'new_default',
    'instance_config': {
        'external_services_providers': [
            {
                "id": "forseti_obstacles",
                "navitia_service": "obstacles",
                "args": {
                    "service_url": "https://wtf/obstacles",
                    "timeout": 10,
                    "circuit_breaker_max_fail": 4,
                    "circuit_breaker_reset_timeout": 60,
                },
                "class": "jormungandr.external_services.obstacle.ObstacleProvider",
            }
        ]
    },
}

OBSTACLES_RESPONSE = {
    "obstacles": [
        {
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "random",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
            "photo": "https://imgur.com",
            "distance": 200,
        },
        {
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "random",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
            "photo": "https://imgur.com",
            "distance": 200,
        },
        {
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "random",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
            "photo": "https://imgur.com",
            "distance": 200,
        },
    ],
    "pagination": {"items_on_page": 3, "items_per_page": 5, "start_page": 1, "total_result": 8},
}


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF})
class TestObstacle(AbstractTestFixture):
    """
    Test the structure of the obstacles_nearby api
    """

    def test_freefloatings_nearby(self):
        """
        simple obstacles_nearby call
        """
        " https://wtf/obstacles?type%5B%5D=None&distance=500&count=5&coord=2.37715%3B48.846781&start_page=1"
        url = "https://wtf/obstacles"
        mock_requests = MockRequests(
            {
                '{}?{}'.format(
                    url,
                    urlencode(
                        {
                            "distance": 500,
                            "count": 5,
                            "coord": "2.37715;48.846781",
                            "start_page": 1,
                        }
                    ),
                ): (
                    OBSTACLES_RESPONSE,
                    200,
                )
            }
        )

        with mock.patch('requests.get', mock_requests.get):
            response, status = self.query_no_assert(
                'v1/coverage/main_routing_test/coord/2.37715%3B48.846781/obstacles_nearby?count=5&start_page=1'
            )

            warnings = get_not_null(response, 'warnings')
            assert len(warnings) == 1
            assert warnings[0]['id'] == 'beta_endpoint'

            obstacles = get_not_null(response, 'obstacles')
            assert len(obstacles) == 3

            assert obstacles[0]['id'] == 'dmVsaWI6U1RBVElPTjoxMjAwOQ=='
            assert obstacles[0]['type'] == 'random'
            assert obstacles[0]['photo'] == 'https://imgur.com'
            assert obstacles[0]['distance'] == 200
            assert obstacles[0]['coord']['lat'] == '48.84617146118711'
            assert obstacles[0]['coord']['lon'] == '2.379306815564632'
