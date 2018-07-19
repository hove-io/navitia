# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
import pytest

from jormungandr.tests.utils_test import MockResponse
from tests.check_utils import get_not_null, s_coord, r_coord
from tests.tests_mechanism import dataset, NewDefaultScenarioAbstractTestFixture

MOCKED_INSTANCE_CONF = {
    'scenario': 'distributed',
    'instance_config': {
        'street_network': [
            {
                "args": {
                    "api_id": "bob_id",
                    "api_code": "bob_code",
                    "service_base_url": "route.bob.here.com/routing/7.2/",
                    "timeout": 20
                },
                "modes": [
                    "car"
                ],
                "class": "jormungandr.street_network.here.Here"
            },
            {
                "modes": [
                    "walking",
                    "bike",
                    "bss"
                ],
                "class": "jormungandr.street_network.kraken.Kraken"
            }
        ]
    }
}

# the destinations are B, C, A and useless_a
HERE_BEGGINING_MATRIX_RESPONSE = {
    "response": {
        "matrixEntry": [
            {
                "destinationIndex": 0,
                "startIndex": 0,
                "summary": {
                    "travelTime": 15
                }
            },
            {
                "destinationIndex": 0,
                "startIndex": 0,
                "summary": {
                    "travelTime": 100
                }
            },
            {
                "destinationIndex": 2,
                "startIndex": 0,
                "status": "failed"
            },
            {
                "destinationIndex": 2,
                "startIndex": 0,
                "status": "failed"
            },
        ],
    }
}

# the start and A, useless_A and C
HERE_END_MATRIX_RESPONSE = {
    "response": {
        "matrixEntry": [
            {
                "destinationIndex": 0,
                "startIndex": 0,
                "summary": {
                    "travelTime": 60
                }
            },
            {
                "destinationIndex": 0,
                "startIndex": 0,
                "summary": {
                    "travelTime": 60
                }
            },
            {
                "destinationIndex": 2,
                "startIndex": 0,
                "status": "failed"
            },
        ],
    }
}

HERE_ROUTING_RESPONSE_BEGINNING_FALLBACK_PATH = {
    "response": {
        "route": [
            {
                "leg": [
                    {
                        "length": 10,
                        "maneuver": [
                            {
                                "length": 10,
                                "travelTime": 15
                            }
                        ],
                        "travelTime": 15
                    }
                ],
                "shape": [
                    "52.4999825,13.3999652",
                    "52.4987912,13.4510744"
                ],
                "summary": {
                    "_type": "RouteSummaryType",
                    "baseTime": 13,
                    "distance": 10,
                    "trafficTime": 2,
                    "travelTime": 15
                }
            }
        ]
    }
}


HERE_ROUTING_RESPONSE_END_FALLBACK_PATH = {
    "response": {
        "route": [
            {
                "leg": [
                    {
                        "length": 100,
                        "maneuver": [
                            {
                                "length": 10,
                                "travelTime": 10
                            },
                            {
                                "length": 90,
                                "travelTime": 50
                            }
                        ],
                        "travelTime": 60,
                        'BaseTime': 60
                    }
                ],
                "shape": [
                    "52.4999825,13.3999652",
                    "52.4987912,13.4510744"
                ],
                "summary": {
                    "_type": "RouteSummaryType",
                    "baseTime": 50,
                    "distance": 100,
                    "trafficTime": 10,
                    "travelTime": 60
                }
            }
        ]
    }
}


HERE_ROUTING_RESPONSE_DIRECT_PATH = {
    "response": {
        "route": [
            {
                "leg": [
                    {
                        "length": 100,
                        "maneuver": [
                            {
                                "length": 20,
                                "travelTime": 100
                            },
                            {
                                "length": 80,
                                "travelTime": 200
                            },
                        ],
                        "travelTime": 300,
                        'BaseTime': 200
                    }
                ],
                "shape": [
                    "52.4999825,13.3999652",
                    "52.4987912,13.4510744"
                ],
                "summary": {
                    "_type": "RouteSummaryType",
                    "baseTime": 200,
                    "distance": 1000,
                    "trafficTime": 100,
                    "travelTime": 300
                }
            }
        ]
    }
}

QUERY_DATETIME_STR = "20120614T070000"


def mock_here(_, url, params):
    s = 'geo!8.98312e-05,8.98312e-05'
    r = 'geo!0.00071865,0.00188646'

    a = 'geo!0.000718649585564,0.00107797437835'
    b = 'geo!0.000269493594586,8.98311981955e-05'
    c = 'geo!0.00107797437835,0.000628818387368'
    useless_a = a

    if url == 'https://matrix.route.bob.here.com/routing/7.2/calculatematrix.json':
        if params['start0'] == s:
            # we check that the destinations are correct since the order is important
            assert params.get('destination0') == b
            assert params.get('destination1') == c
            assert params.get('destination2') == a
            assert params.get('destination3') == useless_a

            # we also check that we gave HERE the departure datetime (to get realtime info)
            # NOTE: since the instance has no timezone we consider the request as UTC
            assert params.get('departure') == '2012-06-14T07:00:00Z'

            return MockResponse(HERE_BEGGINING_MATRIX_RESPONSE, 200)
        if params['destination0'] == r:
            assert params.get('start0') == a
            assert params.get('start1') == useless_a
            assert params.get('start2') == c
            assert params.get('departure') == '2012-06-14T07:00:00Z'
            return MockResponse(HERE_END_MATRIX_RESPONSE, 200)
    elif url == 'https://route.bob.here.com/routing/7.2/calculateroute.json':
        if params['waypoint0'] == s and params['waypoint1'] == r:
            assert params.get('departure') == '2012-06-14T07:00:00Z'
            return MockResponse(HERE_ROUTING_RESPONSE_DIRECT_PATH, 200)
        if params['waypoint0'] == s and params['waypoint1'] == b:
            assert params.get('departure') == '2012-06-14T08:01:00Z'
            return MockResponse(HERE_ROUTING_RESPONSE_BEGINNING_FALLBACK_PATH, 200)
        if params['waypoint0'] == a and params['waypoint1'] == r:
            assert params.get('departure') == '2012-06-14T08:01:02Z'
            return MockResponse(HERE_ROUTING_RESPONSE_END_FALLBACK_PATH, 200)
    assert False, 'invalid url'


@pytest.fixture(scope="function", autouse=True)
def mock_http_here(monkeypatch):
    monkeypatch.setattr('jormungandr.street_network.here.Here._call_here', mock_here)


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF})
class TestHere(NewDefaultScenarioAbstractTestFixture):
    """
    Integration test with HERE

    To better understand the dataset see routing_api_test_data.h

    As a reminder the simplified dataset is:

    S == B -- A == R

    We want to go from S to R

    We mock Here response with the following durations:

    S -> R = 5mn
    S -> B = 15s
    A -> R = 1mn
    """

    def test_basic_car_routing(self):
        """
        test with car fallback and car direct path
        """
        q = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&first_section_mode[]=car" \
            "&last_section_mode[]=car&debug=true&min_nb_journeys=0"\
            .format(from_coord=s_coord, to_coord=r_coord, datetime=QUERY_DATETIME_STR)
        response = self.query_region(q)
        # we don't want to check the journeys links as that will make more HERE call (when testing the links)
        self.is_valid_journey_response(response, q, check_journey_links=False)

        journeys = get_not_null(response, 'journeys')

        assert len(journeys) == 2

        car_direct = journeys[0]

        assert car_direct.get('departure_date_time') == '20120614T070000'
        assert car_direct.get('arrival_date_time') == '20120614T070500'
        sections = car_direct.get('sections')
        assert len(sections) == 1
        assert sections[0].get('mode') == 'car'
        assert sections[0].get('departure_date_time') == '20120614T070000'
        assert sections[0].get('arrival_date_time') == '20120614T070500'
        assert sections[0].get('duration') == 300
        assert sections[0].get('type') == 'street_network'

        car_fallback = journeys[1]

        assert car_fallback.get('departure_date_time') == '20120614T080045'
        assert car_fallback.get('arrival_date_time') == '20120614T080202'

        sections = car_fallback.get('sections')
        assert len(sections) == 3
        assert sections[0].get('mode') == 'car'
        assert sections[0].get('departure_date_time') == '20120614T080045'
        assert sections[0].get('arrival_date_time') == '20120614T080100'
        assert sections[0].get('duration') == 15

        assert sections[1].get('departure_date_time') == '20120614T080100'
        assert sections[1].get('arrival_date_time') == '20120614T080102'
        assert sections[1].get('duration') == 2
        assert sections[1].get('type') == 'public_transport'

        assert sections[2].get('departure_date_time') == u'20120614T080102'
        assert sections[2].get('arrival_date_time') == u'20120614T080202'
        assert sections[2].get('duration') == 60
        assert sections[2].get('type') == 'street_network'
        assert sections[2].get('mode') == 'car'

        feeds = get_not_null(response, 'feed_publishers')
        assert len(feeds) == 2
        assert feeds[0].get('id') == 'here'
        assert feeds[0].get('name') == 'here'
        assert feeds[0].get('license') == 'Private'
        assert feeds[0].get('url') == 'https://developer.here.com/terms-and-conditions'

        assert feeds[1].get('id') == 'builder'
        assert feeds[1].get('name') == 'routing api data'
        assert feeds[1].get('license') == 'ODBL'
        assert feeds[1].get('url') == 'www.canaltp.fr'
