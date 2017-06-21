#  Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
import requests_mock
from mock import MagicMock

from jormungandr.street_network.here import Here
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
from jormungandr.utils import PeriodExtremity, str_to_time_stamp
from navitiacommon import type_pb2, response_pb2


@pytest.fixture()
def valid_here_matrix():
    return {
        "response": {
            "matrixEntry": [
                {
                    "destinationIndex": 0,
                    "startIndex": 0,
                    "summary": {
                        "travelTime": 440
                    }
                },
                {
                    "destinationIndex": 1,
                    "startIndex": 0,
                    "status": "failed"
                },
                {
                    "destinationIndex": 2,
                    "startIndex": 0,
                    "summary": {
                        "travelTime": 701
                    }
                },
            ],
        }
    }


@pytest.fixture()
def valid_here_routing_response():
    # it's not a real response, it's a truncated to reduce its size
    return {
            "response": {
                "route": [
                    {
                        "leg": [
                            {
                                "length": 4012,
                                "maneuver": [
                                    {
                                        "_type": "PrivateTransportManeuverType",
                                        "id": "M1",
                                        "instruction": "Head <span class=\"heading\">southeast</span> on "
                                                       "<span class=\"street\">Bob street</span>. <span "
                                                       "class=\"distance-description\">Go for <span class=\"length\">46 m</span>.</span>",
                                        "length": 46,
                                        "position": {
                                            "latitude": 52.4999825,
                                            "longitude": 13.3999652
                                        },
                                        "travelTime": 27
                                    },
                                    {
                                        "_type": "PrivateTransportManeuverType",
                                        "id": "M10",
                                        "instruction": "Arrive at <span class=\"street\">Schleusenufer</span>.",
                                        "length": 0,
                                        "position": {
                                            "latitude": 52.4987912,
                                            "longitude": 13.4510744
                                        },
                                        "travelTime": 0
                                    }
                                ],
                                "travelTime": 588
                            }
                        ],
                        "mode": {
                            "feature": [],
                            "trafficMode": "disabled",
                            "transportModes": [
                                "car"
                            ],
                            "type": "fastest"
                        },
                        "shape": [
                            "52.4999825,13.3999652",
                            "52.4999607,13.3999944",
                            "52.4972355,13.4488964",
                            "52.4987912,13.4510744"
                        ],
                        "summary": {
                            "_type": "RouteSummaryType",
                            "baseTime": 588,
                            "distance": 4012,
                            "flags": [
                                "noThroughRoad",
                                "builtUpArea",
                                "park",
                                "privateRoad"
                            ],
                            "text": "The trip takes <span class=\"length\">4.0 km</span> and <span class=\"time\">10 mins</span>.",
                            "trafficTime": 1036,
                            "travelTime": 588
                        }
                    }
                ]
            }
        }


def test_matrix(valid_here_matrix):
    instance = MagicMock()
    instance.walking_speed = 1.12
    here = Here(instance=instance, service_base_url='bob.com', app_id='toto', app_code='tata')
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    with requests_mock.Mocker() as req:
        req.get(requests_mock.ANY,
                json=valid_here_matrix,
                status_code=200)
        response = here.get_street_network_routing_matrix(
            [origin],
            [destination, destination, destination],
            mode='walking',
            max_duration=42,
            request={'datetime': str_to_time_stamp('20170621T174600')})
        assert response.rows[0].routing_response[0].duration == 440
        assert response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert response.rows[0].routing_response[1].duration == -1
        assert response.rows[0].routing_response[1].routing_status == response_pb2.unreached
        assert response.rows[0].routing_response[2].duration == 701
        assert response.rows[0].routing_response[2].routing_status == response_pb2.reached


def here_basic_routing_test(valid_here_routing_response):
    origin = make_pt_object(type_pb2.POI, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.STOP_AREA, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    response = Here._read_response(response=valid_here_routing_response,
                                   mode='walking',
                                   origin=origin,
                                   destination=destination,
                                   fallback_extremity=fallback_extremity,
                                   request={'datetime': str_to_time_stamp('20170621T174600')})
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert response.journeys[0].duration == 588
    assert len(response.journeys[0].sections) == 1
    section = response.journeys[0].sections[0]
    assert section.type == response_pb2.STREET_NETWORK
    assert section.length == 4012
    assert section.duration == 588
    assert section.destination == destination
    assert section.origin == origin
    assert section.begin_date_time == str_to_time_stamp('20161010T152000')
    assert section.end_date_time == section.begin_date_time + section.duration
