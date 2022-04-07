# coding=utf-8
# Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from __future__ import absolute_import
import pytest

from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.street_network.valhalla import Valhalla
from jormungandr.exceptions import InvalidArguments, TechnicalError, ApiNotFound
from navitiacommon import type_pb2, response_pb2
import pybreaker
from mock import MagicMock
from .streetnetwork_test_utils import make_pt_object
from jormungandr.utils import str_to_time_stamp, PeriodExtremity
import requests_mock
import json

MOCKED_REQUEST = {'walking_speed': 1, 'bike_speed': 2}


def response_valid():
    return {
        "trip": {
            "summary": {"time": 6, "length": 0.052},
            "units": "kilometers",
            "legs": [
                {
                    "shape": "qyss{Aco|sCF?kBkHeJw[",
                    "summary": {"time": 6, "length": 0.052},
                    "maneuvers": [
                        {"end_shape_index": 3, "time": 6, "length": 0.052, "begin_shape_index": 0},
                        {"begin_shape_index": 3, "time": 0, "end_shape_index": 3, "length": 0},
                    ],
                }
            ],
            "status_message": "Found route between points",
            "status": 0,
        }
    }


def get_costing_options_func_with_empty_costing_options_test():
    instance = MagicMock()
    valhalla = Valhalla(instance=instance, service_url='http://bob.com')
    valhalla.costing_options = None
    assert valhalla._get_costing_options('bib', MOCKED_REQUEST) == None


def get_costing_options_func_with_unkown_costing_options_test():
    instance = MagicMock()
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    assert valhalla._get_costing_options('bib', MOCKED_REQUEST) == {'bib': 'bom'}


def get_costing_options_func_test():
    instance = MagicMock()
    instance.walking_speed = 1
    instance.bike_speed = 2
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    pedestrian = valhalla._get_costing_options('pedestrian', MOCKED_REQUEST)
    assert len(pedestrian) == 2
    assert 'pedestrian' in pedestrian
    assert 'walking_speed' in pedestrian['pedestrian']
    assert pedestrian['pedestrian']['walking_speed'] == 3.6 * 1
    assert 'bib' in pedestrian

    bicycle = valhalla._get_costing_options('bicycle', MOCKED_REQUEST)
    assert len(bicycle) == 2
    assert 'bicycle' in bicycle
    assert 'cycling_speed' in bicycle['bicycle']
    assert bicycle['bicycle']['cycling_speed'] == 3.6 * 2
    assert 'bib' in bicycle


def format_coord_func_invalid_pt_object_test():
    with pytest.raises(InvalidArguments) as excinfo:
        Valhalla._format_coord(MagicMock())
    assert '400 Bad Request' in str(excinfo.value)


def format_coord_func_invalid_api_test():
    with pytest.raises(ApiNotFound) as excinfo:
        Valhalla._format_coord(make_pt_object(type_pb2.ADDRESS, 1.12, 13.15), 'aaa')
    assert '404 Not Found' in str(excinfo.value)
    assert 'ApiNotFound' in str(excinfo.typename)


def format_coord_func_valid_coord_sources_to_targets_test():
    pt_object = make_pt_object(type_pb2.ADDRESS, 1.12, 13.15)

    coord = Valhalla._format_coord(pt_object, 'sources_to_targets')
    coord_res = {'lat': pt_object.address.coord.lat, 'lon': pt_object.address.coord.lon}
    assert len(coord) == 2
    for key, value in coord_res.items():
        assert coord[key] == value


def format_coord_func_valid_coord_test():
    pt_object = make_pt_object(type_pb2.ADDRESS, 1.12, 13.15)

    coord = Valhalla._format_coord(pt_object)
    coord_res = {'lat': pt_object.address.coord.lat, 'type': 'break', 'lon': pt_object.address.coord.lon}
    assert len(coord) == 3
    for key, value in coord_res.items():
        assert coord[key] == value


def format_url_func_with_walking_mode_test():
    instance = MagicMock()
    instance.walking_speed = 1
    instance.bike_speed = 2
    origin = make_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
    destination = make_pt_object(type_pb2.ADDRESS, 2.0, 2.0)
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    data = valhalla._make_request_arguments("walking", [origin], [destination], MOCKED_REQUEST)
    assert json.loads(data) == json.loads(
        '''{
                        "costing_options": {
                          "pedestrian": {
                              "walking_speed": 3.6
                            },
                            "bib": "bom"
                          },
                          "locations": [
                            {
                              "lat": 1,
                              "type": "break",
                              "lon": 1
                            },
                            {
                              "lat": 2,
                              "type": "break",
                              "lon": 2
                            }
                          ],
                          "costing": "pedestrian",
                          "directions_options": {
                            "units": "kilometers"
                          }
                        }'''
    )


def format_url_func_with_bike_mode_test():
    instance = MagicMock()
    instance.walking_speed = 1
    instance.bike_speed = 2

    origin = make_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
    destination = make_pt_object(type_pb2.ADDRESS, 2.0, 2.0)

    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    valhalla.costing_options = None
    data = valhalla._make_request_arguments("bike", [origin], [destination], MOCKED_REQUEST)
    assert json.loads(data) == json.loads(
        '''
                                            {
                                              "costing_options": {
                                                "bicycle": {
                                                  "cycling_speed": 7.2
                                                }
                                              },
                                              "locations": [
                                                {
                                                  "lat": 1,
                                                  "type": "break",
                                                  "lon": 1
                                                },
                                                {
                                                  "lat": 2,
                                                  "type": "break",
                                                  "lon": 2
                                                }
                                              ],
                                              "costing": "bicycle",
                                              "directions_options": {
                                                "units": "kilometers"
                                              }
                                            }'''
    )


def format_url_func_with_car_mode_test():
    instance = MagicMock()
    instance.walking_speed = 1
    instance.bike_speed = 2

    origin = make_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
    destination = make_pt_object(type_pb2.ADDRESS, 2.0, 2.0)

    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    valhalla.costing_options = None
    data = valhalla._make_request_arguments("car", [origin], [destination], MOCKED_REQUEST)
    assert json.loads(data) == json.loads(
        ''' {
                                      "locations": [
                                        {
                                          "lat": 1,
                                          "type": "break",
                                          "lon": 1
                                        },
                                        {
                                          "lat": 2,
                                          "type": "break",
                                          "lon": 2
                                        }
                                      ],
                                      "costing": "auto",
                                      "directions_options": {
                                        "units": "kilometers"
                                      }
                                    }'''
    )


def format_url_func_with_different_ptobject_test():
    instance = MagicMock()
    instance.walking_speed = 1
    instance.bike_speed = 2
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    valhalla.costing_options = None
    for ptObject_type in (
        type_pb2.STOP_POINT,
        type_pb2.STOP_AREA,
        type_pb2.ADDRESS,
        type_pb2.ADMINISTRATIVE_REGION,
        type_pb2.POI,
    ):
        origin = make_pt_object(ptObject_type, 1.0, 1.0)
        destination = make_pt_object(ptObject_type, 2.0, 2.0)
        data = valhalla._make_request_arguments("car", [origin], [destination], MOCKED_REQUEST)
        assert json.loads(data) == json.loads(
            ''' {
                                         "locations": [
                                           {
                                             "lat": 1,
                                             "type": "break",
                                             "lon": 1
                                            },
                                            {
                                              "lat": 2,
                                              "type": "break",
                                              "lon": 2
                                            }
                                            ],
                                            "costing": "auto",
                                            "directions_options": {
                                            "units": "kilometers"
                                            }
                                            }'''
        )


def format_url_func_invalid_mode_test():
    instance = MagicMock()
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    with pytest.raises(InvalidArguments) as excinfo:
        origin = make_pt_object(type_pb2.ADDRESS, 1.0, 1.0)
        destination = make_pt_object(type_pb2.ADDRESS, 2.0, 2.0)
        valhalla._make_request_arguments("bob", [origin], [destination], MOCKED_REQUEST)
    assert '400 Bad Request' in str(excinfo.value)
    assert 'InvalidArguments' == str(excinfo.typename)


def call_valhalla_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    valhalla.breaker = MagicMock()
    valhalla.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    assert valhalla._call_valhalla(valhalla.service_url) == None


def call_valhalla_func_with_unknown_exception_test():
    instance = MagicMock()
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    valhalla.breaker = MagicMock()
    valhalla.breaker.call = MagicMock(side_effect=ValueError())
    assert valhalla._call_valhalla(valhalla.service_url) == None


def get_response_basic_test():
    resp_json = response_valid()
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    response = Valhalla._get_response(
        resp_json,
        'walking',
        origin,
        destination,
        fallback_extremity,
        StreetNetworkPathType.BEGINNING_FALLBACK,
        mode_park_cost=None,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert response.journeys[0].duration == 6
    assert len(response.journeys[0].sections) == 1
    assert response.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
    assert response.journeys[0].sections[0].length == 52
    assert response.journeys[0].sections[0].duration == 6
    assert response.journeys[0].sections[0].destination == destination
    # Note: we don't have a park section as there is no mode_park_cost


def get_response_with_park_test():
    resp_json = response_valid()
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    response = Valhalla._get_response(
        resp_json,
        'walking',
        origin,
        destination,
        fallback_extremity,
        StreetNetworkPathType.BEGINNING_FALLBACK,
        mode_park_cost=5 * 60,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert response.journeys[0].duration == 6 + 5 * 60
    assert len(response.journeys[0].sections) == 2
    assert response.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
    assert response.journeys[0].sections[0].length == 52
    assert response.journeys[0].sections[0].duration == 6
    assert response.journeys[0].sections[0].destination == destination

    assert response.journeys[0].sections[1].type == response_pb2.PARK
    assert response.journeys[0].sections[1].length == 0
    assert response.journeys[0].sections[1].duration == 5 * 60
    assert response.journeys[0].sections[1].origin == destination
    assert response.journeys[0].sections[1].destination == destination


def get_response_with_leave_parking_test():
    resp_json = response_valid()
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    response = Valhalla._get_response(
        resp_json,
        'walking',
        origin,
        destination,
        fallback_extremity,
        StreetNetworkPathType.ENDING_FALLBACK,
        mode_park_cost=5 * 60,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert response.journeys[0].duration == 6 + 5 * 60
    assert len(response.journeys[0].sections) == 2

    assert response.journeys[0].sections[0].type == response_pb2.LEAVE_PARKING
    assert response.journeys[0].sections[0].length == 0
    assert response.journeys[0].sections[0].duration == 5 * 60
    assert response.journeys[0].sections[0].origin == origin
    assert response.journeys[0].sections[0].destination == origin

    assert response.journeys[0].sections[1].type == response_pb2.STREET_NETWORK
    assert response.journeys[0].sections[1].length == 52
    assert response.journeys[0].sections[1].duration == 6
    assert response.journeys[0].sections[1].destination == destination


def get_response_with_0_duration_park_test():
    """even if the mode_park_cost is 0, if it's not None we got a park section"""
    resp_json = response_valid()
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    response = Valhalla._get_response(
        resp_json,
        'walking',
        origin,
        destination,
        fallback_extremity,
        StreetNetworkPathType.BEGINNING_FALLBACK,
        mode_park_cost=0,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert response.journeys[0].duration == 6
    assert len(response.journeys[0].sections) == 2
    assert response.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
    assert response.journeys[0].sections[0].length == 52
    assert response.journeys[0].sections[0].duration == 6
    assert response.journeys[0].sections[0].destination == destination

    assert response.journeys[0].sections[1].type == response_pb2.PARK
    assert response.journeys[0].sections[1].length == 0
    assert response.journeys[0].sections[1].duration == 0  # very quick park section
    assert response.journeys[0].sections[1].origin == destination
    assert response.journeys[0].sections[1].destination == destination


def direct_path_func_without_response_valhalla_test():
    instance = MagicMock()
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    valhalla.breaker = MagicMock()
    valhalla._call_valhalla = MagicMock(return_value=None)
    valhalla._make_request_arguments = MagicMock(return_value=None)
    with pytest.raises(TechnicalError) as excinfo:
        valhalla._direct_path(None, None, None, None, None, None, None, None)
    assert '500 Internal Server Error' in str(excinfo.value)
    assert 'TechnicalError' == str(excinfo.typename)


def direct_path_func_with_status_code_400_response_valhalla_test():
    instance = MagicMock()
    instance.walking_speed = 1.12
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    with requests_mock.Mocker() as req:
        with pytest.raises(TechnicalError) as excinfo:
            req.post('http://bob.com/route', json={'error_code': 42}, status_code=400)
            valhalla.direct_path_with_fp(
                instance, 'walking', origin, destination, fallback_extremity, MOCKED_REQUEST, None, None
            )
        assert '500 Internal Server Error' in str(excinfo.value)
        assert (
            str(excinfo.value.data['message'])
            == 'Valhalla service unavailable, impossible to query : http://bob.com/route'
        )
        assert str(excinfo.typename) == 'TechnicalError'


def direct_path_func_with_no_response_valhalla_test():
    instance = MagicMock()
    instance.walking_speed = 1.12
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    with requests_mock.Mocker() as req:
        req.post('http://bob.com/route', json={'error_code': 442}, status_code=400)
        valhalla_response = valhalla.direct_path_with_fp(
            instance, 'walking', origin, destination, fallback_extremity, MOCKED_REQUEST, None, None
        )
        assert valhalla_response.status_code == 200
        assert valhalla_response.response_type == response_pb2.NO_SOLUTION
        assert len(valhalla_response.journeys) == 0


def direct_path_func_with_valid_response_valhalla_test():
    instance = MagicMock()
    instance.walking_speed = 1.12
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    resp_json = response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    with requests_mock.Mocker() as req:
        req.post('http://bob.com/route', json=resp_json)
        valhalla_response = valhalla.direct_path_with_fp(
            instance, 'walking', origin, destination, fallback_extremity, MOCKED_REQUEST, None, None
        )
        assert valhalla_response.status_code == 200
        assert valhalla_response.response_type == response_pb2.ITINERARY_FOUND
        assert len(valhalla_response.journeys) == 1
        assert valhalla_response.journeys[0].duration == 6
        assert len(valhalla_response.journeys[0].sections) == 1
        assert valhalla_response.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert valhalla_response.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert valhalla_response.journeys[0].sections[0].length == 52
        assert valhalla_response.journeys[0].sections[0].destination == destination


def get_valhalla_mode_invalid_mode_test():
    with pytest.raises(InvalidArguments) as excinfo:
        Valhalla._get_valhalla_mode('bss')
    assert '400 Bad Request' in str(excinfo.value)
    assert 'InvalidArguments' == str(excinfo.typename)


def get_valhalla_mode_valid_mode_test():
    map_mode = {"walking": "pedestrian", "car": "auto", "bike": "bicycle"}
    for kraken_mode, valhalla_mode in map_mode.items():
        assert Valhalla._get_valhalla_mode(kraken_mode) == valhalla_mode


def sources_to_targets_valhalla_test():
    instance = MagicMock()
    instance.walking_speed = 1.12
    valhalla = Valhalla(instance=instance, service_url='http://bob.com', costing_options={'bib': 'bom'})
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    response = {'sources_to_targets': [[{'time': 42}, {'time': None}, {'time': 1337}]]}
    with requests_mock.Mocker() as req:
        req.post('http://bob.com/sources_to_targets', json=response, status_code=200)
        valhalla_response = valhalla.get_street_network_routing_matrix(
            instance, [origin], [destination, destination, destination], 'walking', 42, MOCKED_REQUEST, None
        )
        assert valhalla_response.rows[0].routing_response[0].duration == 42
        assert valhalla_response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert valhalla_response.rows[0].routing_response[1].duration == -1
        assert valhalla_response.rows[0].routing_response[1].routing_status == response_pb2.unknown
        assert valhalla_response.rows[0].routing_response[2].duration == 1337
        assert valhalla_response.rows[0].routing_response[2].routing_status == response_pb2.reached


def sources_to_targets_valhalla_with_park_cost_test():
    """
    test with a parking cost,
    we add a different cost on car and bike and no cost on walking
    """
    instance = MagicMock()
    instance.walking_speed = 1.12
    valhalla = Valhalla(
        instance=instance,
        service_url='http://bob.com',
        costing_options={'bib': 'bom'},
        mode_park_cost={
            'bike': 30,  # 30s for the bike
            'car': 5 * 60  # 5 mn for the car
            # and no park section for walking
        },
    )
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    response = {'sources_to_targets': [[{'time': 42}, {'time': None}, {'time': 1337}]]}
    with requests_mock.Mocker() as req:
        req.post('http://bob.com/sources_to_targets', json=response, status_code=200)
        # it changes nothing for walking
        valhalla_response = valhalla._get_street_network_routing_matrix(
            instance,
            [origin],
            [destination, destination, destination],
            mode='walking',
            max_duration=42,
            request=MOCKED_REQUEST,
            request_id=None,
        )
        assert valhalla_response.rows[0].routing_response[0].duration == 42
        assert valhalla_response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert valhalla_response.rows[0].routing_response[1].duration == -1
        assert valhalla_response.rows[0].routing_response[1].routing_status == response_pb2.unknown
        assert valhalla_response.rows[0].routing_response[2].duration == 1337
        assert valhalla_response.rows[0].routing_response[2].routing_status == response_pb2.reached

        # for bike every reached point have an additional 30s
        valhalla_response = valhalla._get_street_network_routing_matrix(
            instance,
            [origin],
            [destination, destination, destination],
            mode='bike',
            max_duration=42,
            request=MOCKED_REQUEST,
            request_id=None,
        )
        assert valhalla_response.rows[0].routing_response[0].duration == 42 + 30
        assert valhalla_response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert valhalla_response.rows[0].routing_response[1].duration == -1
        assert valhalla_response.rows[0].routing_response[1].routing_status == response_pb2.unknown
        assert valhalla_response.rows[0].routing_response[2].duration == 1337 + 30
        assert valhalla_response.rows[0].routing_response[2].routing_status == response_pb2.reached

        # for car every reached point have an additional 5mn
        valhalla_response = valhalla._get_street_network_routing_matrix(
            instance,
            [origin],
            [destination, destination, destination],
            mode='car',
            max_duration=42,
            request=MOCKED_REQUEST,
            request_id=None,
        )
        assert valhalla_response.rows[0].routing_response[0].duration == 42 + 5 * 60
        assert valhalla_response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert valhalla_response.rows[0].routing_response[1].duration == -1
        assert valhalla_response.rows[0].routing_response[1].routing_status == response_pb2.unknown
        assert valhalla_response.rows[0].routing_response[2].duration == 1337 + 5 * 60
        assert valhalla_response.rows[0].routing_response[2].routing_status == response_pb2.reached


def status_test():
    valhalla = Valhalla(
        instance=None,
        service_url='http://bob.com',
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=23,
    )
    status = valhalla.status()
    assert len(status) == 5
    assert status['id'] == u'tata-é$~#@"*!\'`§èû'
    assert status['class'] == "Valhalla"
    assert status['modes'] == ["walking", "bike", "car"]
    assert status['timeout'] == 23
    assert len(status['circuit_breaker']) == 3
    assert status['circuit_breaker']['current_state'] == 'closed'
    assert status['circuit_breaker']['fail_counter'] == 0
    assert status['circuit_breaker']['reset_timeout'] == 60
