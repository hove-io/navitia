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
from jormungandr.scenarios import experimental
import requests as requests
from mock import MagicMock

MOCKED_VALHALLA_CONF = {
    "class": "tests.direct_path_valhalla_integration_tests.MockValhalla",
    "args": {
        "service_url": "http://bob.com",
        "language": "fr-FR",
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
}
s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset

journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
    .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T080000")


def response_valid(mode):
    map_mod = {"walking": 20,
               "car": 5,
               "bike": 10}
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


def check_journeys(resp):
    assert not resp.get('journeys') or sum([1 for j in resp['journeys'] if j['type'] == "best"]) == 1


class MockValhalla(Valhalla):

    def __init__(self, instance, url,
                 timeout=10, language='fr-FR',
                 api_key=None, **kwargs):
        Valhalla.__init__(self, instance, url, timeout, language, api_key, **kwargs)

    def _format_url(self, mode, pt_object_origin, pt_object_destination):
        return mode

    def _call_valhalla(self, mode):
        response = requests.Response()
        response.status_code = 200
        response.json = MagicMock(return_value=response_valid(mode))
        response.url = self.service_url
        return response



@dataset({"main_routing_test": {"street_network": MOCKED_VALHALLA_CONF}})
class TestValhallaDirectPath(AbstractTestFixture):

    def setup(self):
        from jormungandr import i_manager
        dest_instance = i_manager.instances['main_routing_test']
        self.old_scenario = dest_instance._scenario
        dest_instance._scenario = experimental.Scenario()

    def teardown(self):
        from jormungandr import i_manager
        i_manager.instances['main_routing_test']._scenario = self.old_scenario

    def test_journey_with_bike_direct_path(self):
        query = journey_basic_query + \
                "&first_section_mode[]=bss" + \
                "&first_section_mode[]=walking" + \
                "&first_section_mode[]=bike" + \
                "&first_section_mode[]=car" + \
        "&debug=true&_override_scenario=experimental"
        response = self.query_region(query)
        check_journeys(response)
        assert len(response['journeys']) == 6

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

        # walking from kraken
        assert('walking' in response['journeys'][3]['tags'])
        assert len(response['journeys'][3]['sections']) == 3
        assert response['journeys'][3]['duration'] == 83

        # walking from kraken
        assert('walking' in response['journeys'][4]['tags'])
        assert len(response['journeys'][4]['sections']) == 3
        assert response['journeys'][4]['duration'] == 86

        # walking from kraken
        assert('walking' in response['journeys'][5]['tags'])
        assert len(response['journeys'][5]['sections']) == 3
        assert response['journeys'][5]['duration'] == 99
