# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from jormungandr.street_network.geovelo import Geovelo
from navitiacommon import type_pb2, response_pb2
import pybreaker
from mock import MagicMock
from streetnetwork_test_utils import get_pt_object
from jormungandr.utils import str_to_time_stamp
import requests_mock
import json


def direct_path_response_valid():
    """
    reply to POST of {"starts":[[48.803064,2.443385, "refStart1"]],
                      "ends":[[48.802049,2.426482, "refEnd1"]]}
    """
    return [["start_reference","end_reference","duration"],["refStart1","refEnd1",1546]]


def isochrone_response_valid():
    """
    reply to POST of {"starts":[[48.85568,2.326355, "refStart1"]],
                      "ends":[[48.852291,2.359829, "refEnd1"],
                              [48.854607,2.388582, "refEnd2"]]}
    """
    return [["start_reference","end_reference","duration"],
            ["refStart1","refEnd1",1051],
            ["refStart1","refEnd2",1656]]


def pt_object_summary_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    summary = geovelo._pt_object_summary(get_pt_object(type_pb2.ADDRESS, lon=1.12, lat=13.15, uri='toto'))
    assert summary == [13.15, 1.12, 'toto']


def make_data_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    origins = [get_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')]
    destinations = [get_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1'),
                    get_pt_object(type_pb2.ADDRESS, lon=4, lat=48.4, uri='refEnd2')]
    data = geovelo._make_data(origins, destinations)
    assert json.loads(json.dumps(data)) == json.loads('''{
            "starts":[[48.2,2, "refStart1"]],
            "ends":[[48.3,3, "refEnd1"],
                    [48.4,4, "refEnd2"]]
        }''')


def call_geovelo_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    geovelo.breaker = MagicMock()
    geovelo.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    assert geovelo._call_geovelo(geovelo.service_url) == None


def call_geovelo_func_with_unknown_exception_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    geovelo.breaker = MagicMock()
    geovelo.breaker.call = MagicMock(side_effect=ValueError())
    assert geovelo._call_geovelo(geovelo.service_url) == None


def get_matrix_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                        service_url='http://bob.com')
    resp_json = direct_path_response_valid()
    matrix = geovelo._get_matrix(resp_json)
    assert matrix.rows[0].routing_response[0].duration == 1546
    assert matrix.rows[0].routing_response[0].routing_status == response_pb2.reached


def direct_path_geovelo_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    resp_json = direct_path_response_valid()

    origin = get_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')
    destination = get_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1')
    with requests_mock.Mocker() as req:
        req.post('http://bob.com', json=resp_json)
        geovelo_resp = geovelo.direct_path('bike',
                                           origin,
                                           destination,
                                           str_to_time_stamp('20161010T152000'),
                                           False,
                                           None)
        assert geovelo_resp.status_code == 200
        assert geovelo_resp.response_type == response_pb2.ITINERARY_FOUND
        assert len(geovelo_resp.journeys) == 1
        assert geovelo_resp.journeys[0].duration == 1546
        assert len(geovelo_resp.journeys[0].sections) == 1
        assert geovelo_resp.journeys[0].arrival_date_time == str_to_time_stamp('20161010T152000')
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].length == 1546*3 #totally arbitrary in connector
        assert geovelo_resp.journeys[0].sections[0].origin == origin
        assert geovelo_resp.journeys[0].sections[0].destination == destination


def isochrone_geovelo_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    resp_json = isochrone_response_valid()

    origins = [get_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')]
    destinations = [get_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1'),
                   get_pt_object(type_pb2.ADDRESS, lon=4, lat=48.4, uri='refEnd2')]

    with requests_mock.Mocker() as req:
        req.post('http://bob.com', json=resp_json, status_code=200)
        geovelo_response = geovelo.get_street_network_routing_matrix(
            origins,
            destinations,
            'bike',
            13371337,
            None)
        assert geovelo_response.rows[0].routing_response[0].duration == 1051
        assert geovelo_response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert geovelo_response.rows[0].routing_response[1].duration == 1656
        assert geovelo_response.rows[0].routing_response[1].routing_status == response_pb2.reached
