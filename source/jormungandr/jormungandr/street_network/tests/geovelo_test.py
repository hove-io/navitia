# coding=utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from jormungandr.street_network.geovelo import Geovelo
from navitiacommon import type_pb2, response_pb2
import pybreaker
from mock import MagicMock
from .streetnetwork_test_utils import make_pt_object, make_pt_object_with_sp_mode
from jormungandr.utils import str_to_time_stamp, PeriodExtremity
import requests_mock
import ujson
import pytest
import jormungandr.exceptions


MOCKED_REQUEST = {'walking_speed': 1, 'bike_speed': 3.33}
MOCKED_SERVICE_URL = 'https://bob.com'

service_backup = {
    "args": {"service_url": MOCKED_SERVICE_URL, "asgard_socket": "tcp://socket.andyamo.com:666"},
    "class": "jormungandr.street_network.asgard.Asgard",
}


def direct_path_response_valid():
    """
    A mock of a valid response from geovelo.
    Reply to POST of {"starts":[[48.803064,2.443385, "refStart1"]],
                      "ends":[[48.802049,2.426482, "refEnd1"]]}
    Modify with caution as it will affect every tests using these start and end uris.
    """
    return [
        {
            "distances": {
                "discouragedRoads": 65.0,
                "normalRoads": 3569.0,
                "recommendedRoads": 7759.0,
                "total": 11393.0,
            },
            "duration": 3155,
            "estimatedDatetimeOfArrival": "2017-02-24T16:52:08.711",
            "estimatedDatetimeOfDeparture": "2017-02-24T15:59:33.711",
            "id": "bG9jPTQ4Ljg4Nzk0LDIuMzE0MzM4JmxvYz00OC44Mjk5MjcsMi4zNzY3NDcjQkVHSU5ORVIjRmFsc2UjQkVHSU5ORVIjMTMjRmFsc2UjRmFsc2UjMjAxNy0wMi0yNCAxNTo1OTozMy43MTEwNjgjVFJBRElUSU9OQUwjMCMwI1JFQ09NTUVOREVEI0ZhbHNl",
            "sections": [
                {
                    "details": {
                        "averageSpeed": 13,
                        "bikeType": "TRADITIONAL",
                        "direction": "Boulevard de Clichy",
                        "distances": {
                            "cycleway": 7563.0,
                            "discouragedRoads": 65.0,
                            "footway": 363.0,
                            "greenway": 0.0,
                            "lane": 78.0,
                            "livingstreet": 500.0,
                            "normalRoads": 3569.0,
                            "opposite": 141.0,
                            "pedestrian": 206.0,
                            "recommendedRoads": 7759.0,
                            "residential": 0.0,
                            "sharebusway": 118.0,
                            "steps": 0.0,
                            "total": 11393.0,
                            "zone30": 793.0,
                        },
                        "elevations": [
                            ["distanceFromStart", "elevation", "geometryIndex"],
                            [0, 45.5, 0],
                            [128, 44, 1],
                            [274, 50, 7],
                        ],
                        "instructions": [
                            [
                                "direction",
                                "roadName",
                                "roadLength",
                                "facility",
                                "cyclability",
                                "geometryIndex",
                                "orientation",
                                "cityNames",
                            ],
                            ["HEAD_ON", "Rue Cardinet", 58, "SHAREBUSWAY", 3, 0, "SW", ""],
                            ["GO_STRAIGHT", "Rue Jouffroy d'Abbans", 40, "RESIDENTIAL", 4, 3, "W", ""],
                            ["REACHED_YOUR_DESTINATION", "", 0, "NONE", 3, 307, "N", ""],
                        ],
                        "profile": "BEGINNER",
                        "verticalGain": 51,
                    },
                    "duration": 3155,
                    "estimatedDatetimeOfArrival": "2017-02-24T16:52:08.711",
                    "estimatedDatetimeOfDeparture": "2017-02-24T15:59:33.711",
                    "geometry": "_yzf|AszglClL`ShClEzCrHj@nNnBfD~AoHfDeD`nBmeC|AqBvAuAhWyWjVqUbJsJdd@uf@uPwh@sBgG{JoYmEkMeEwLxv@iu@}Hwj@k@}DcCqQ{Ims@m@yEbBcDbCyEhEiIhSm`@rPy\\bI}OfBwD|H}CaNehA}MwiAzA_LiBsPa@sDoBaSeAoKiCcHqBQmG{Rqk@meAa@oHRwSDuDv^yfChAmAnAkHrFqZnA{Gb@wDxc@m{Btd@a|Bz@cErHum@xLobBfBqMd@aHAkGmA{LaAkCaUwoBo@sFiR{aB_@wJuFoe@u@sDm@_CyTonB`@yDy@gIdA{EyBsTNoG_OqzBe@uDuAaLjD~DzAmAzNoLxCcBtDqEj_A}y@`_@i\\vFcF~c@w^~GuFbJgJ`JaHnCmDri@kd@nQcOxCiC~\\wYfCuBlC_CpB_BtCeCxDoEdx@}q@pG_Fpi@yd@lDuBhO_NjD}CxKyJ~IcIlIsHp^{\\dRwN~DuDjBqBxEkEzh@md@xHwG~AqAhGgFzMeLrM_Lp{@_t@lEeDfKsJvQ_PxDuCnCoCzO_NlIcHhSyQzFcF~CoCdsA{hAfDoCtSqStDyCjAmEn@uJ`Ywk@xEeJxHePwWabAaB{GkKqc@wL}n@sAuG`@qFxB}A~vBc|AfUkPjE}C|b@uZ`w@oj@~j@ca@tl@mb@zMgIzIoEvNiIlHaJjIcFfNwFhH_ChMuBpMoAlK_@rLH~Lp@|OjCfFjAtD|FfCz@lBp@nHe@xkCl~@vaAf]b{@pZzMhFnCb@nFpBtGkWlBuBbBcC|NgTzWu_@fB_CtCcC|@_Cvb@qgAtw@usBdRnMtJnGb\\vTy@hEvCtAfFdC`Bv@nEtBtExBhW`Q|HtE|ChBrAjAdL_Nb^qc@zUkYxCqDlCmDxRkTrAwAlEyDdH{GhZmm@lCgFfDkIvA|@lv@xe@pCfBpDlBpC_I|DuKr[gy@xAwDhKgX`FyMzBkFdCsG|JoWdk@c{AdLsZzEcObFgMbMa\\fR}f@nBzF`c@d`BjAdF`Mra@hGfSzw@f}BxF~LfCzDzA@lCzPnZdu@p^x{@rFzNzCfIpLl[fAvCvxA}`BPaDhFkGnHkJvF_IrKoIfCiDrz@nkBvKzFdEoDfGkEbO{MbFsErDzAz^_\\t_@e]lE_Gpc@ql@r_@wg@fk@gv@fa@{h@rNcRvLvWrWrk@fB`ElD~H`Pv]pEdKhAbCbN|[fBnElBtE~AlF`Mlu@rZoXjE{Dt^e\\|AnDzAhDzJwK]eAg@CTpBrGeBzBkBk@eBHa@J_AeAY}@}ClCiCw@wJ[cAeB?gScp@YtCxBjH~DvM",
                    "transportMode": "BIKE",
                    "waypoints": [
                        {"latitude": 48.88794, "longitude": 2.314338, "title": None},
                        {"latitude": 48.829927, "longitude": 2.376747, "title": None},
                    ],
                    "waypointsIndices": [0, 308],
                }
            ],
            "title": "RECOMMENDED",
            "waypoints": [
                {"latitude": 48.88794, "longitude": 2.314338, "title": None},
                {"latitude": 48.829927, "longitude": 2.376747, "title": None},
            ],
        }
    ]


def direct_path_response_zero():
    return [
        {
            "distances": {"discouragedRoads": 0.0, "normalRoads": 0.0, "recommendedRoads": 0.0, "total": 0.0},
            "duration": 0,
            "estimatedDatetimeOfArrival": "2017-03-27T19:00:32.434",
            "estimatedDatetimeOfDeparture": "2017-03-27T19:00:32.434",
            "id": "bG9jPTQ4LjAsMi4wJmxvYz00OC4wLDIuMCNCRUdJTk5FUiNGYWxzZSNCRUdJTk5FUiMxMyNGYWxzZSNGYWxzZSMyMDE3LTAzLTI3IDE5OjAwOjMyLjQzNDMyNyNUUkFESVRJT05BTCMwIzAjUkVDT01NRU5ERUQjRmFsc2U=",
            "sections": [
                {
                    "details": {
                        "averageSpeed": 13,
                        "bikeType": "TRADITIONAL",
                        "direction": None,
                        "distances": {
                            "cycleway": 0.0,
                            "discouragedRoads": 0.0,
                            "footway": 0.0,
                            "greenway": 0.0,
                            "lane": 0.0,
                            "livingstreet": 0.0,
                            "normalRoads": 0.0,
                            "opposite": 0.0,
                            "pedestrian": 0.0,
                            "recommendedRoads": 0.0,
                            "residential": 0.0,
                            "sharebusway": 0.0,
                            "steps": 0.0,
                            "total": 0.0,
                            "zone30": 0.0,
                        },
                        "elevations": None,
                        "instructions": [
                            [
                                "direction",
                                "roadName",
                                "roadLength",
                                "facility",
                                "cyclability",
                                "geometryIndex",
                                "orientation",
                                "cityNames",
                            ],
                            ["HEAD_ON", "voie pietonne", 0, "FOOTWAY", 4, 0, "N", ""],
                            ["REACHED_YOUR_DESTINATION", "", 0, "NONE", 3, 1, "N", ""],
                        ],
                        "profile": "BEGINNER",
                        "verticalGain": 0,
                    },
                    "duration": 0,
                    "estimatedDatetimeOfArrival": "2017-03-27T19:00:32.434",
                    "estimatedDatetimeOfDeparture": "2017-03-27T19:00:32.434",
                    "geometry": "wytpzAg}ayB??",
                    "transportMode": "BIKE",
                    "waypoints": [
                        {"latitude": 48.0, "longitude": 2.0, "title": None},
                        {"latitude": 48.0, "longitude": 2.0, "title": None},
                    ],
                    "waypointsIndices": [0, 2],
                }
            ],
            "title": "RECOMMENDED",
            "waypoints": [
                {"latitude": 48.0, "longitude": 2.0, "title": None},
                {"latitude": 48.0, "longitude": 2.0, "title": None},
            ],
        }
    ]


def isochrone_response_valid():
    """
    reply to POST of {"starts":[[48.85568,2.326355, "refStart1"]],
                      "ends":[[48.852291,2.359829, "refEnd1"],
                              [48.854607,2.388582, "refEnd2"]]}
    """
    return [
        ["start_reference", "end_reference", "duration"],
        ["refStart1", "refEnd1", 1051],
        ["refStart1", "refEnd2", 1656],
    ]


def pt_object_summary_test():
    summary = Geovelo._pt_object_summary_isochrone(
        make_pt_object(type_pb2.ADDRESS, lon=1.12, lat=13.15, uri='toto')
    )
    assert summary == [13.15, 1.12, None]


def make_data_test():
    origins = [make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')]
    destinations = [
        make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1'),
        make_pt_object(type_pb2.ADDRESS, lon=4, lat=48.4, uri='refEnd2'),
    ]
    data = Geovelo._make_request_arguments_isochrone(origins, destinations)
    assert ujson.loads(ujson.dumps(data)) == ujson.loads(
        '''{
            "starts": [[48.2, 2.0, null]], "ends": [[48.3, 3.0, null], [48.4, 4.0, null]],
            "transportMode": "BIKE",
            "bikeDetails": {"profile": "MEDIAN", "averageSpeed": 12, "bikeType": "TRADITIONAL"}}'''
    )


def call_geovelo_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url=MOCKED_SERVICE_URL)
    geovelo.breaker = MagicMock()
    geovelo.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    with pytest.raises(jormungandr.exceptions.GeoveloTechnicalError):
        geovelo._call_geovelo(geovelo.service_url)


def call_geovelo_func_with_unknown_exception_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url=MOCKED_SERVICE_URL)
    geovelo.breaker = MagicMock()
    geovelo.breaker.call = MagicMock(side_effect=ValueError())
    with pytest.raises(jormungandr.exceptions.GeoveloTechnicalError):
        geovelo._call_geovelo(geovelo.service_url)


def get_matrix_test():
    resp_json = isochrone_response_valid()
    matrix = Geovelo._get_matrix(resp_json)
    assert matrix.rows[0].routing_response[0].duration == 1051
    assert matrix.rows[0].routing_response[0].routing_status == response_pb2.reached
    assert matrix.rows[0].routing_response[1].duration == 1656
    assert matrix.rows[0].routing_response[1].routing_status == response_pb2.reached


def direct_path_geovelo_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url=MOCKED_SERVICE_URL)
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')
    destination = make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), False)
    with requests_mock.Mocker() as req:
        req.post(
            '{}/api/v2/computedroutes?instructions=true&elevations=true&geometry=true'
            '&single_result=true&bike_stations=false&objects_as_ids=true&'.format(MOCKED_SERVICE_URL),
            json=resp_json,
        )
        geovelo_resp = geovelo.direct_path_with_fp(
            instance, 'bike', origin, destination, fallback_extremity, MOCKED_REQUEST, None, None
        )
        assert geovelo_resp.status_code == 200
        assert geovelo_resp.response_type == response_pb2.ITINERARY_FOUND
        assert len(geovelo_resp.journeys) == 1
        assert geovelo_resp.journeys[0].duration == 3155  # 52min35s
        assert geovelo_resp.journeys[0].requested_date_time == 0  # parameter datetime absent in MOCKED_REQUEST
        assert len(geovelo_resp.journeys[0].sections) == 1
        assert geovelo_resp.journeys[0].arrival_date_time == str_to_time_stamp('20161010T152000')
        assert geovelo_resp.journeys[0].departure_date_time == str_to_time_stamp('20161010T142725')
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].duration == 3155
        assert geovelo_resp.journeys[0].sections[0].length == 11393
        assert geovelo_resp.journeys[0].sections[0].street_network.coordinates[2].lon == 2.314258
        assert geovelo_resp.journeys[0].sections[0].street_network.coordinates[2].lat == 48.887428
        assert geovelo_resp.journeys[0].sections[0].origin == origin
        assert geovelo_resp.journeys[0].sections[0].destination == destination
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].name == "Rue Jouffroy d'Abbans"
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].direction == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].length == 40
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].duration == 11
        assert geovelo_resp.journeys[0].sections[0].street_network.elevations[0].distance_from_start == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.elevations[0].elevation == 45.5
        assert geovelo_resp.journeys[0].sections[0].street_network.elevations[1].distance_from_start == 128
        assert geovelo_resp.journeys[0].sections[0].street_network.elevations[1].elevation == 44
        assert geovelo_resp.journeys[0].sections[0].street_network.elevations[2].distance_from_start == 274
        assert geovelo_resp.journeys[0].sections[0].street_network.elevations[2].elevation == 50
        assert geovelo_resp.journeys[0].sections[0].cycle_lane_length == 98
        assert len(geovelo_resp.journeys[0].sections[0].street_network.street_information) == 3
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[0].cycle_path_type == 2
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[0].length == 58.0
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[1].cycle_path_type == 2
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[1].length == 40.0
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[2].cycle_path_type == 2
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[2].length == 0.0


def direct_path_geovelo_zero_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url=MOCKED_SERVICE_URL)
    resp_json = direct_path_response_zero()

    origin = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48, uri='refStart1')
    destination = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48, uri='refEnd1')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), False)
    # Add parameter datetime in MOCKED_REQUEST to verify
    MOCKED_REQUEST['datetime'] = str_to_time_stamp('20161010T150000')
    with requests_mock.Mocker() as req:
        req.post(
            '{}/api/v2/computedroutes?instructions=true&elevations=true&geometry=true'
            '&single_result=true&bike_stations=false&objects_as_ids=true&'.format(MOCKED_SERVICE_URL),
            json=resp_json,
        )
        geovelo_resp = geovelo.direct_path_with_fp(
            instance, 'bike', origin, destination, fallback_extremity, MOCKED_REQUEST, None, None
        )
        assert geovelo_resp.status_code == 200
        assert geovelo_resp.response_type == response_pb2.ITINERARY_FOUND
        assert len(geovelo_resp.journeys) == 1
        assert geovelo_resp.journeys[0].duration == 0
        assert geovelo_resp.journeys[0].requested_date_time == str_to_time_stamp('20161010T150000')
        assert len(geovelo_resp.journeys[0].sections) == 1
        assert geovelo_resp.journeys[0].arrival_date_time == str_to_time_stamp('20161010T152000')
        assert geovelo_resp.journeys[0].departure_date_time == str_to_time_stamp('20161010T152000')
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].duration == 0
        assert geovelo_resp.journeys[0].sections[0].length == 0
        assert geovelo_resp.journeys[0].sections[0].origin == origin
        assert geovelo_resp.journeys[0].sections[0].destination == destination
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].name == "voie pietonne"
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].direction == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].length == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].duration == 0
        assert geovelo_resp.journeys[0].sections[0].cycle_lane_length == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[0].length == 0.0
        assert geovelo_resp.journeys[0].sections[0].street_network.street_information[1].length == 0.0


def isochrone_geovelo_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url=MOCKED_SERVICE_URL, service_backup=None)
    resp_json = isochrone_response_valid()

    origins = [make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')]
    destinations = [
        make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1'),
        make_pt_object(type_pb2.ADDRESS, lon=4, lat=48.4, uri='refEnd2'),
    ]

    with requests_mock.Mocker() as req:
        req.post('{}/api/v2/routes_m2m'.format(MOCKED_SERVICE_URL), json=resp_json, status_code=200)
        geovelo_response = geovelo.get_street_network_routing_matrix(
            instance, origins, destinations, 'bike', 13371337, MOCKED_REQUEST, None
        )
        assert geovelo_response.rows[0].routing_response[0].duration == 1051
        assert geovelo_response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert geovelo_response.rows[0].routing_response[1].duration == 1656
        assert geovelo_response.rows[0].routing_response[1].routing_status == response_pb2.reached


def distances_durations_test():
    """
    Check that the response from geovelo is correctly formatted with 'distances' and 'durations' sections
    """
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url=MOCKED_SERVICE_URL)
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')
    destination = make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)

    proto_resp = geovelo._get_response(
        json_response=resp_json,
        pt_object_origin=origin,
        pt_object_destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20161010T151000')},
    )
    assert proto_resp.journeys[0].durations.total == 3155
    assert proto_resp.journeys[0].durations.bike == 3155
    assert proto_resp.journeys[0].distances.bike == 11393.0
    assert proto_resp.journeys[0].requested_date_time == str_to_time_stamp('20161010T151000')


def make_request_arguments_bike_details_test():
    """
    Check that the bikeDetails is well formatted for the request with right averageSpeed value
    """
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url=MOCKED_SERVICE_URL)
    data = geovelo._make_request_arguments_bike_details(bike_speed_mps=3.33)
    assert ujson.loads(ujson.dumps(data)) == ujson.loads(
        '''{"profile": "MEDIAN", "averageSpeed": 12,
    "bikeType": "TRADITIONAL"}'''
    )

    data = geovelo._make_request_arguments_bike_details(bike_speed_mps=4.1)
    assert ujson.loads(ujson.dumps(data)) == ujson.loads(
        '''{"profile": "MEDIAN", "averageSpeed": 15,
    "bikeType": "TRADITIONAL"}'''
    )


def status_test():
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
    )
    status = geovelo.status()
    assert len(status) == 5
    assert status['id'] == u'tata-é$~#@"*!\'`§èû'
    assert status['class'] == "Geovelo"
    assert status['modes'] == ["walking", "bike", "car"]
    assert status['timeout'] == 56
    assert len(status['circuit_breaker']) == 3
    assert status['circuit_breaker']['current_state'] == 'closed'
    assert status['circuit_breaker']['fail_counter'] == 0
    assert status['circuit_breaker']['reset_timeout'] == 60


def sort_by_mode_test():
    # Sort order Train, RapidTransit, Metro, Tramway, Car, Bus
    points = [
        make_pt_object_with_sp_mode(lon=3, lat=48.3, uri='ref_Bus', mode_uris=['physical_mode:Bus']),
        make_pt_object_with_sp_mode(lon=4, lat=48.4, uri='ref_Metro', mode_uris=['physical_mode:Metro']),
        make_pt_object_with_sp_mode(lon=5, lat=48.5, uri='ref_Tram', mode_uris=['physical_mode:Tramway']),
        make_pt_object_with_sp_mode(lon=6, lat=48.6, uri='ref_Car', mode_uris=['physical_mode:Car']),
        make_pt_object_with_sp_mode(lon=6, lat=48.6, uri='ref_NoMode'),
        make_pt_object_with_sp_mode(lon=7, lat=48.7, uri='ref_Train', mode_uris=['physical_mode:Train']),
        make_pt_object_with_sp_mode(lon=8, lat=48.8, uri='ref_Rapid', mode_uris=['physical_mode:RapidTransit']),
    ]
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
    )
    points = geovelo.sort_places_by_physical_mode_and_distance(points)
    assert points[0].uri == 'ref_Train'
    assert points[1].uri == 'ref_Rapid'
    assert points[2].uri == 'ref_Metro'
    assert points[3].uri == 'ref_Tram'
    assert points[4].uri == 'ref_Car'
    assert points[5].uri == 'ref_Bus'
    assert points[6].uri == 'ref_NoMode'
    # Default mode_weight
    assert len(list(geovelo.mode_weight.keys())) == 7
    for mode in [
        'physical_mode:Train',
        'physical_mode:LocalTrain',
        'physical_mode:RapidTransit',
        'physical_mode:Metro',
        'physical_mode:Tramway',
        'physical_mode:Car',
        'physical_mode:Bus',
    ]:
        assert mode in geovelo.mode_weight


def mode_weight_test():
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={
            'physical_mode:Train': 1,
            'physical_mode:RapidTransit': 2,
        },
    )
    assert len(list(geovelo.mode_weight.keys())) == 2
    for mode in ['physical_mode:Train', 'physical_mode:RapidTransit']:
        assert mode in geovelo.mode_weight


def mode_weight_none_test():
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id="abcd",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight=None,
    )
    # Default mode_weight
    assert len(list(geovelo.mode_weight.keys())) == 7
    for mode in [
        'physical_mode:Train',
        'physical_mode:LocalTrain',
        'physical_mode:RapidTransit',
        'physical_mode:Metro',
        'physical_mode:Tramway',
        'physical_mode:Car',
        'physical_mode:Bus',
    ]:
        assert mode in geovelo.mode_weight


def mode_weight_empty_test():
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id="abcd",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={},
    )
    # Default mode_weight
    assert len(list(geovelo.mode_weight.keys())) == 7
    for mode in [
        'physical_mode:Train',
        'physical_mode:LocalTrain',
        'physical_mode:RapidTransit',
        'physical_mode:Metro',
        'physical_mode:Tramway',
        'physical_mode:Car',
        'physical_mode:Bus',
    ]:
        assert mode in geovelo.mode_weight


def filter_places_isochrone_test():
    points = [
        make_pt_object_with_sp_mode(lon=3, lat=48.3, uri='ref_Bus', mode_uris=['physical_mode:Bus']),
        make_pt_object_with_sp_mode(lon=4, lat=48.4, uri='ref_Metro', mode_uris=['physical_mode:Metro']),
        make_pt_object_with_sp_mode(lon=5, lat=48.5, uri='ref_Tram', mode_uris=['physical_mode:Tramway']),
        make_pt_object_with_sp_mode(lon=6, lat=48.6, uri='ref_Car', mode_uris=['physical_mode:Car']),
        make_pt_object_with_sp_mode(lon=6, lat=48.6, uri='ref_NoMode'),
        make_pt_object_with_sp_mode(lon=7, lat=48.7, uri='ref_Train', mode_uris=['physical_mode:Train']),
        make_pt_object_with_sp_mode(lon=8, lat=48.8, uri='ref_Rapid', mode_uris=['physical_mode:RapidTransit']),
    ]
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    points = geovelo.filter_places_isochrone(points)
    assert len(points) == 2
    assert points[0].uri == 'ref_Rapid'
    assert points[1].uri == 'ref_Train'

    # Default mode_weight
    assert len(list(geovelo.mode_weight.keys())) == 3
    for mode in ['physical_mode:Train', 'physical_mode:RapidTransit', 'physical_mode:LocalTrain']:
        assert mode in geovelo.mode_weight


def filter_places_isochrone_modes_not_in_mode_weight_test():
    points = [
        make_pt_object_with_sp_mode(lon=3, lat=48.3, uri='ref_Bus', mode_uris=['physical_mode:Bus']),
        make_pt_object_with_sp_mode(lon=4, lat=48.4, uri='ref_Metro', mode_uris=['physical_mode:Metro']),
        make_pt_object_with_sp_mode(lon=5, lat=48.5, uri='ref_Tram', mode_uris=['physical_mode:Tramway']),
        make_pt_object_with_sp_mode(lon=6, lat=48.6, uri='ref_Car', mode_uris=['physical_mode:Car']),
    ]
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    points = geovelo.filter_places_isochrone(points)
    assert len(points) == 0

    # Default mode_weight
    assert len(list(geovelo.mode_weight.keys())) == 3
    for mode in ['physical_mode:Train', 'physical_mode:RapidTransit', 'physical_mode:LocalTrain']:
        assert mode in geovelo.mode_weight


def filter_places_isochrone_modes_multiple_modes_test():
    points = [
        make_pt_object_with_sp_mode(
            lon=3, lat=48.3, uri='ref_bus_train', mode_uris=['physical_mode:Bus', 'physical_mode:Train']
        ),
        make_pt_object_with_sp_mode(lon=4, lat=48.4, uri='ref_Metro', mode_uris=['physical_mode:Metro']),
        make_pt_object_with_sp_mode(lon=5, lat=48.5, uri='ref_Tram', mode_uris=['physical_mode:Tramway']),
        make_pt_object_with_sp_mode(
            lon=6, lat=48.6, uri='ref_car_rapid', mode_uris=['physical_mode:Car', 'physical_mode:RapidTransit']
        ),
    ]
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    points = geovelo.filter_places_isochrone(points)
    assert len(points) == 2
    assert points[0].uri == 'ref_car_rapid'
    assert points[1].uri == 'ref_bus_train'

    # Default mode_weight
    assert len(list(geovelo.mode_weight.keys())) == 3
    for mode in ['physical_mode:Train', 'physical_mode:RapidTransit', 'physical_mode:LocalTrain']:
        assert mode in geovelo.mode_weight


def filter_places_isochrone_modes_distance_test():
    points = [
        make_pt_object_with_sp_mode(
            lon=3,
            lat=48.3,
            uri='ref_bus_LocalTrain_d_14',
            mode_uris=['physical_mode:Bus', 'physical_mode:LocalTrain'],
            distance=14,
        ),
        make_pt_object_with_sp_mode(
            lon=3,
            lat=48.3,
            uri='ref_bus_LocalTrain_d_12',
            mode_uris=['physical_mode:Bus', 'physical_mode:LocalTrain'],
            distance=12,
        ),
        make_pt_object_with_sp_mode(
            lon=3,
            lat=48.3,
            uri='ref_bus_LocalTrain_d_15',
            mode_uris=['physical_mode:Bus', 'physical_mode:LocalTrain'],
            distance=15,
        ),
        make_pt_object_with_sp_mode(
            lon=4, lat=48.4, uri='ref_Train_d_5', mode_uris=['physical_mode:Train'], distance=5
        ),
        make_pt_object_with_sp_mode(
            lon=4, lat=48.4, uri='ref_Train_d_17', mode_uris=['physical_mode:Train'], distance=17
        ),
        make_pt_object_with_sp_mode(
            lon=4, lat=48.4, uri='ref_Train_d_7', mode_uris=['physical_mode:Train'], distance=7
        ),
        make_pt_object_with_sp_mode(
            lon=6,
            lat=48.6,
            uri='ref_car_rapid_d_15',
            mode_uris=['physical_mode:Car', 'physical_mode:RapidTransit'],
            distance=15,
        ),
        make_pt_object_with_sp_mode(
            lon=6,
            lat=48.6,
            uri='ref_car_rapid_d_3',
            mode_uris=['physical_mode:Car', 'physical_mode:RapidTransit'],
            distance=3,
        ),
        make_pt_object_with_sp_mode(
            lon=6,
            lat=48.6,
            uri='ref_car_rapid_d_16',
            mode_uris=['physical_mode:Car', 'physical_mode:RapidTransit'],
            distance=16,
        ),
    ]
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 2, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    points = geovelo.filter_places_isochrone(points)
    result = [
        "ref_bus_LocalTrain_d_12",
        "ref_bus_LocalTrain_d_14",
        "ref_bus_LocalTrain_d_15",
        "ref_car_rapid_d_3",
        "ref_Train_d_5",
        "ref_Train_d_7",
        "ref_car_rapid_d_15",
        "ref_car_rapid_d_16",
        "ref_Train_d_17",
    ]
    assert len(points) == len(result)
    for index, uri in enumerate(result):
        assert points[index].uri == uri
    # Default mode_weight
    assert len(list(geovelo.mode_weight.keys())) == 3
    for mode in ['physical_mode:Train', 'physical_mode:RapidTransit', 'physical_mode:LocalTrain']:
        assert mode in geovelo.mode_weight


def is_reached_by_physical_mode_test():
    place = make_pt_object_with_sp_mode(lon=3, lat=48.3, uri='ref_Bus', mode_uris=['physical_mode:Bus'])
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    res = geovelo.is_reached_by_physical_mode(place)
    assert not res


def is_reached_by_physical_mode_empt__place_test():
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    res = geovelo.is_reached_by_physical_mode(None)
    assert not res


def get_physical_modes_uris_test():
    point = make_pt_object_with_sp_mode(
        lon=3, lat=48.3, uri='ref_bus_train', mode_uris=['physical_mode:Bus', 'physical_mode:Train']
    )
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    uris = list(geovelo.get_physical_modes_uris(point))
    assert len(uris) == 2


def get_physical_modes_uris_empty_list_test():
    point = make_pt_object_with_sp_mode(lon=3, lat=48.3, uri='ref_bus_train', mode_uris=[])
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    uris = list(geovelo.get_physical_modes_uris(point))
    assert len(uris) == 0


def get_physical_modes_uris_not_stop_point_object_type_test():
    point = make_pt_object(type_pb2.ADDRESS, lon=1.12, lat=13.15, uri='toto')
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    uris = list(geovelo.get_physical_modes_uris(point))
    assert len(uris) == 0


def direct_path_invalid_mode_test():
    point = make_pt_object(type_pb2.ADDRESS, lon=1.12, lat=13.15, uri='toto')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), False)
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        service_backup=None,
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    with pytest.raises(jormungandr.exceptions.InvalidArguments):
        geovelo._direct_path(
            instance=None,
            mode="walking",
            pt_object_origin=point,
            pt_object_destination=point,
            fallback_extremity=fallback_extremity,
            request={},
            direct_path_type=None,
            request_id="",
        )

def zone():
    return [
    [
      -1.683365,
      48.116216
    ],
    [
      -1.686063,
      48.112116
    ],
    [
      -1.679806,
      48.110085
    ],
    [
      -1.67429761,
      48.112499
    ],
    [
      -1.675216,
      48.116483
    ],
    [
      -1.683365,
      48.116216
    ]
  ]

# Points outside zone
# [[-1.683709, 48.117941],[-1.685259, 48.116715],[-1.670163, 48.111618]]
# Points inside zone
# [[-1.682446, 48.112997],[-1.678429, 48.115257],[-1.679347, 48.111848]]

def service_without_zone_and_backup_test():
    # If zone is absent then we should use the service without any condition
    point = make_pt_object(type_pb2.ADDRESS, lon=1.12, lat=13.15, uri='toto')
    geovelo = Geovelo(
        instance=None,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        zone=None,
        service_backup=None,
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    result = geovelo.use_direct_path_service_backup(pt_object_origin=point, pt_object_destination=point, represents_start=True)
    assert result == False
    origins = [point]
    destinations = [
        make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1'),
        make_pt_object(type_pb2.ADDRESS, lon=4, lat=48.4, uri='refEnd2'),
    ]
    result = geovelo.use_sn_matrix_service_backup(origins=origins, destinations=destinations)
    assert result == False


def service_with_zone_but_without_backup_test():
    # If zone is present without service_backup, it raises an exception.
    instance = MagicMock()
    with pytest.raises(ValueError) as excinfo:
        geovelo = Geovelo(
            instance=instance,
            service_url=MOCKED_SERVICE_URL,
            id=u"tata-é$~#@\"*!'`§èû",
            modes=["walking", "bike", "car"],
            zone=zone(),
            service_backup=None,
            timeout=56,
            mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
        )
    assert str(excinfo.value) == 'service_backup None is not defined hence can not forward to asgard'


def service_with_zone_and_backup_test():
    # If zone and service_backup are present we may use service backup depending on conditions as explained below
    instance = MagicMock()
    geovelo = Geovelo(
        instance=instance,
        service_url=MOCKED_SERVICE_URL,
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        zone=zone(),
        service_backup=service_backup,
        timeout=56,
        mode_weight={"physical_mode:Train": 3, "physical_mode:RapidTransit": 2, "physical_mode:LocalTrain": 1},
    )
    outside_origin = make_pt_object(type_pb2.ADDRESS, lon=-1.683709, lat=48.117941, uri='toto')
    outside_destination = make_pt_object(type_pb2.ADDRESS, lon=-1.685259, lat=48.116715, uri='toto')
    inside_origin = make_pt_object(type_pb2.ADDRESS, lon=-1.682446, lat=48.112997, uri='toto')
    inside_destination = make_pt_object(type_pb2.ADDRESS, lon=-1.679347, lat=48.111848, uri='toto')
    # When origin of start fallback is outside the zone, we should use service_backup
    # The destination of start fallback is not verified
    result = geovelo.use_direct_path_service_backup(
        pt_object_origin=outside_origin,
        pt_object_destination=outside_destination,
        represents_start=True
    )
    assert result == True
    result = geovelo.use_direct_path_service_backup(
        pt_object_origin=outside_origin,
        pt_object_destination=outside_destination,
        represents_start=False
    )
    assert result == True

    # We should use this service if origin of start fallback or destination of end fallback is inside the zone
    result = geovelo.use_direct_path_service_backup(
        pt_object_origin=inside_origin,
        pt_object_destination=outside_destination,
        represents_start=True
    )
    assert result == False
    result = geovelo.use_direct_path_service_backup(
        pt_object_origin=outside_origin,
        pt_object_destination=inside_destination,
        represents_start=False
    )
    assert result == False

    # For stree_network matrix, origins, destinations for start and end fallbacks are recognized by their size
    # if lengh = 1 then it's origins of start fallback or destinations of end fallback (1 to N or N to 1)
    # Origins of start fallback is outside zone hence use service_backup without verifying destinations
    origins = [outside_origin]
    destinations = [outside_destination, inside_destination]
    result = geovelo.use_sn_matrix_service_backup(origins=origins, destinations=destinations)
    assert result == True
    # Origins of start fallback is inside the zone hence use this service
    origins = [inside_origin]
    result = geovelo.use_sn_matrix_service_backup(origins=origins, destinations=destinations)
    assert result == False
    # Destinations of end fallback is outside the zone hence use service_backup without verifying origins
    origins = [outside_origin, inside_origin]
    destinations = [outside_destination]
    result = geovelo.use_sn_matrix_service_backup(origins=origins, destinations=destinations)
    assert result == True
    # Destinations of end fallback is inside the zone hence use this service
    destinations = [inside_destination]
    result = geovelo.use_sn_matrix_service_backup(origins=origins, destinations=destinations)
    assert result == False
