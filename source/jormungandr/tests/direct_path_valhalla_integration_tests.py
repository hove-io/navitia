# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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

from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr.street_network.valhalla import Valhalla
import requests as requests
from mock import MagicMock
import json

MOCKED_VALHALLA_CONF = [{
    "modes": ['walking', 'car', 'bss', 'bike'],
    "class": "tests.direct_path_valhalla_integration_tests.MockValhalla",
    "args": {
        "service_url": "http://bob.com",
        "costing_options": {
            "bicycle": {
                "bicycle_type": "Hybrid",
                "cycling_speed": 18
            },
            "pedestrian": {
                "walking_speed": 50.1
            }
        }
    }
}]
s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset

journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
    .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T080000")


def route_response(mode):
    map_mod = {"pedestrian": 20,
               "auto": 5,
               "bicycle": 10}
    return {
        "trip": {
            "summary": {
                "time": map_mod.get(mode, 6),
                "length": 0.052
            },
            "units": "kilometers",
            "legs": [
                {
                    "shape": "qyss{Aco|sCF?kBkHeJw[",
                    "summary": {
                        "time": map_mod.get(mode, 6),
                        "length": 0.052
                    },
                    "maneuvers": [
                        {
                            "end_shape_index": 3,
                            "time": map_mod.get(mode, 6),
                            "length": 0.052,
                            "begin_shape_index": 0
                        },
                        {
                            "begin_shape_index": 3,
                            "time": 0,
                            "end_shape_index": 3,
                            "length": 0
                        }
                    ]
                }
            ],
            "status_message": "Found route between points",
            "status": 0
        }
    }


def one_to_many_response():
    return {
        "locations": [
            [
                {
                    "lon": 2.428405,
                    "lat": 48.625626
                },
                {
                    "lon": 2.428379,
                    "lat": 48.625679
                }
            ]
        ],
        "units": "kilometers",
        "one_to_many": [
            [
                {
                    "distance": 0,
                    "time": 0,
                    "to_index": 0,
                    "from_index": 0
                },
                {
                    "distance": 0.227,
                    "time": 177,
                    "to_index": 1,
                    "from_index": 0
                }
            ]
        ]
    }


def response_valid(api, mode):
    if api == 'route':
        return route_response(mode)
    elif api == 'one_to_many':
        return one_to_many_response()


def check_journeys(resp):
    assert not resp.get('journeys') or sum([1 for j in resp['journeys'] if j['type'] == "best"]) == 1


def get_api(url):
    return url.split('/')[-1]


class MockValhalla(Valhalla):

    def __init__(self, instance, service_url, timeout=10, api_key=None, **kwargs):
        Valhalla.__init__(self, instance, service_url, timeout, api_key, **kwargs)

    def _format_url(self, mode, pt_object_origin, pt_object_destination, api='route'):
        return '{}/{}'.format(self.service_url, api)

    def _call_valhalla(self, url, method=requests.post, data=None):
        mode = json.loads(data).get('costing') if data else None
        api = get_api(url)
        response = requests.Response()
        response.status_code = 200
        response.json = MagicMock(return_value=response_valid(api, mode))
        response.url = self.service_url
        return response


@dataset({'main_routing_test': {'scenario': 'distributed',
                                'instance_config': {'street_network': MOCKED_VALHALLA_CONF}}})
class TestValhallaDirectPath(AbstractTestFixture):

    def test_journey_with_bike_direct_path(self):
        query = journey_basic_query + \
                "&first_section_mode[]=bss" + \
                "&first_section_mode[]=walking" + \
                "&first_section_mode[]=bike" + \
                "&first_section_mode[]=car" + \
                "&debug=true"
        response = self.query_region(query)
        check_journeys(response)
        assert len(response['journeys']) == 3

        # car from valhalla
        assert('car' in response['journeys'][0]['tags'])
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['duration'] == 5

        # bike from valhalla
        assert('bike' in response['journeys'][1]['tags'])
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][1]['duration'] == 10

        # walking from valhalla
        assert('walking' in response['journeys'][2]['tags'])
        assert len(response['journeys'][2]['sections']) == 1
        assert response['journeys'][2]['duration'] == 20


@dataset({
    'main_routing_test': {
        'scenario': 'distributed',
        'instance_config': {
            'street_network': [{
                "modes": ['walking', 'car', 'bss', 'bike'],
                "class": "tests.direct_path_valhalla_integration_tests.MockValhalla",
                "args": {
                    "service_url": "http://bob.com",
                    "costing_options": {
                        "bicycle": {
                            "bicycle_type": "Hybrid",
                            "cycling_speed": 18
                        },
                        "pedestrian": {
                            "walking_speed": 50.1
                        }
                    },
                    "mode_park_cost": {
                        "car": 5 * 60,
                        "bike": 30,
                    },
                }
            }]
        }
    }
})
class TestValhallaParkingCost(AbstractTestFixture):
    def test_with_a_car_park_cost(self):
        query = journey_basic_query + \
                "&first_section_mode[]=bss" + \
                "&first_section_mode[]=walking" + \
                "&first_section_mode[]=bike" + \
                "&first_section_mode[]=car" + \
                "&debug=true"
        response = self.query_region(query)
        check_journeys(response)
        assert len(response['journeys']) == 3

        assert('walking' in response['journeys'][0]['tags'])
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['duration'] == 20
        # no park section

        # bike from valhalla
        assert('bike' in response['journeys'][1]['tags'])
        assert len(response['journeys'][1]['sections']) == 2
        assert response['journeys'][1]['duration'] == 10 + 30
        assert response['journeys'][1]['sections'][1]['type'] == 'park'

        # car from valhalla
        assert('car' in response['journeys'][2]['tags'])
        assert len(response['journeys'][2]['sections']) == 2
        assert response['journeys'][2]['duration'] == 5 + 5 * 60
        assert response['journeys'][2]['sections'][1]['type'] == 'park'
