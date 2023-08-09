# coding=utf-8
#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import
import pytest
from mock import MagicMock
import pybreaker
from navitiacommon import response_pb2
import navitiacommon.type_pb2 as type_pb2
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
import jormungandr.exceptions
from jormungandr.street_network.handimap import Handimap
from jormungandr.utils import (
    str_to_time_stamp,
    PeriodExtremity,
)


class MockResource(object):
    def __init__(self, text=None, status=200):
        self.text = text
        self.status_code = status


fake_service_url = "bob.com"


def matrix_response_valid(response_id=1):
    # response_id=0 : len(sources) == len(targets)
    # response_id=1 : len(sources) < len(targets)
    # response_id=2 : len(sources) > len(targets)
    responses = {
        0: {
            "sources": [[{"lon": -1.680150, "lat": 48.108770}]],
            "targets": [[{"lon": -1.679860, "lat": 48.109340}]],
            "sources_to_targets": [[{"distance": 0.089, "time": 68, "to_index": 0, "from_index": 0}]],
            "units": "kilometers",
        },
        1: {
            "sources": [[{"lon": -1.680150, "lat": 48.108770}]],
            "targets": [[{"lon": -1.679860, "lat": 48.109340}, {"lon": -1.678750, "lat": 48.109390}]],
            "sources_to_targets": [
                [
                    {"distance": 0.089, "time": 68, "to_index": 0, "from_index": 0},
                    {"distance": 0.200, "time": 145, "to_index": 1, "from_index": 0},
                ]
            ],
            "units": "kilometers",
        },
        2: {
            "sources": [[{"lon": -1.679860, "lat": 48.109340}, {"lon": -1.678750, "lat": 48.109390}]],
            "targets": [[{"lon": -1.680150, "lat": 48.108770}]],
            "sources_to_targets": [
                [{"distance": 0.089, "time": 68, "to_index": 0, "from_index": 0}],
                [{"distance": 0.200, "time": 145, "to_index": 0, "from_index": 1}],
            ],
            "units": "kilometers",
        },
    }
    return responses[response_id]


def direct_path_response_valid():
    return {
        "trip": {
            "locations": [
                {"type": "break", "lat": 48.100246, "lon": -1.676117, "original_index": 0},
                {"type": "break", "lat": 48.097592, "lon": -1.674005, "original_index": 1},
            ],
            "legs": [
                {
                    "maneuvers": [
                        {
                            "instruction": "Marchez vers l'est sur Rue Ange Blaize.",
                            "street_names": ["Rue Ange Blaize"],
                            "time": 32.9,
                            "length": 0.103,
                        },
                        {
                            "instruction": "Serrez à gauche dans Rue Ginguené.",
                            "street_names": ["Rue Ginguené"],
                            "time": 1.5,
                            "length": 0.005,
                        },
                        {
                            "instruction": "Tournez à gauche pour rester sur Rue Ginguené.",
                            "street_names": ["Rue Ginguené"],
                            "time": 20.999,
                            "length": 0.069,
                        },
                        {
                            "instruction": "Tournez à droite pour rester sur Rue Ginguené.",
                            "street_names": ["Rue Ginguené"],
                            "time": 2.4,
                            "length": 0.008,
                        },
                        {
                            "instruction": "Tournez à gauche pour rester sur Rue Ginguené.",
                            "street_names": ["Rue Ginguené"],
                            "time": 3.299,
                            "length": 0.011,
                        },
                        {
                            "instruction": "Serrez à droite dans Rue Corentin Carré.",
                            "street_names": ["Rue Corentin Carré"],
                            "time": 64.8,
                            "length": 0.215,
                        },
                        {"instruction": "Vous êtes arrivé à votre destination.", "time": 0.0, "length": 0.0},
                    ],
                    "summary": {"time": 125.9, "length": 0.412},
                    "shape": "myxvzA~rheBASAa@AYASEi@Wc@K[CWAa@BUF[FONSTO|Cm@hSwD`ASNKNUPQZMtCk@dB_@h@K^I??fFeAZGLKLSHe@AI?CCk@Gy@Ey@OgB[kFIoA}Cwe@pCi@?I?E?]B_@?CBE@KNa@T[@C@CJGDEZMhCc@FEt@OnB]bEy@`@EbAUzB_@`@MlAUtHuAJE`GeAFG?DBA|GsAbDo@j@KABFApFeARIv@KJGvCg@vDu@tHyALCD@RIrOwCvE_AF@TGPELC",
                }
            ],
            "summary": {"time": 125.9, "length": 0.412},
            "status_message": "Found route between points",
            "status": 0,
            "units": "kilometers",
            "language": "fr-FR",
        }
    }


def test_create_handimap_without_service_url():
    instance = MagicMock()
    with pytest.raises(ValueError) as excinfo:
        Handimap(instance=instance, service_url='')
    assert str(excinfo.value) == 'service_url  is not a valid handimap url'


def test_create_handimap_with_default_values():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    assert handimap.sn_system_id == "handimap"
    assert handimap.timeout == 10
    assert handimap.service_url == fake_service_url
    assert len(handimap.modes) == 1
    assert handimap.modes[0] == "walking"
    assert handimap.breaker.reset_timeout == 60
    assert handimap.breaker.fail_max == 4
    assert handimap._feed_publisher.name == "handimap"
    assert handimap._feed_publisher.id == "handimap"
    assert handimap._feed_publisher.license == "Private"
    assert handimap._feed_publisher.url == "https://www.handimap.fr"


def test_create_handimap_with_config_test():
    instance = MagicMock()
    kwargs = {"circuit_breaker_max_fail": 2, "circuit_breaker_reset_timeout": 30}
    handimap = Handimap(instance=instance, id="id_handmap", service_url=fake_service_url, timeout=5, **kwargs)
    assert handimap.sn_system_id == "id_handmap"
    assert handimap.timeout == 5
    assert handimap.service_url == fake_service_url
    assert len(handimap.modes) == 1
    assert handimap.modes[0] == "walking"
    assert handimap.breaker.reset_timeout == 30
    assert handimap.breaker.fail_max == 2
    assert handimap._feed_publisher.name == "handimap"
    assert handimap._feed_publisher.id == "handimap"
    assert handimap._feed_publisher.license == "Private"
    assert handimap._feed_publisher.url == "https://www.handimap.fr"


def test_create_handimap_status_test():
    instance = MagicMock()
    kwargs = {"circuit_breaker_max_fail": 2, "circuit_breaker_reset_timeout": 30}
    handimap = Handimap(
        instance=instance, id="id_handmap", service_url=fake_service_url, modes=["walking"], timeout=5, **kwargs
    )
    status = handimap.status()
    assert status["id"] == "id_handmap"
    assert len(status["modes"]) == 1
    assert status["modes"][0] == "walking"
    assert status["timeout"] == 5


def call_handimap_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    handimap.breaker = MagicMock()
    handimap.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    with pytest.raises(jormungandr.exceptions.HandimapTechnicalError) as handimap_exception:
        handimap._call_handimap(handimap.service_url, data={})
    assert (
        handimap_exception.value.data["message"]
        == 'Handimap routing service unavailable, Circuit breaker is open'
    )


def call_handimap_func_with_unknown_exception_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    handimap.breaker = MagicMock()
    handimap.breaker.call = MagicMock(side_effect=ValueError())
    with pytest.raises(jormungandr.exceptions.HandimapTechnicalError) as handimap_exception:
        handimap._call_handimap(handimap.service_url, data={})
    assert handimap_exception.value.data["message"] == 'Handimap routing has encountered unknown error'


def direct_path_handimap_func_with_mode_invalid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    with pytest.raises(jormungandr.exceptions.InvalidArguments) as handimap_exception:
        handimap._direct_path(instance, "bike", None, None, None, None, None, "123")
    assert (
        handimap_exception.value.data["message"]
        == "Invalid arguments Handimap, mode(s) ['bike'] not implemented"
    )


def check_response_and_get_json_handimap_func_code_invalid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp = MockResource(status=401)
    with pytest.raises(jormungandr.exceptions.HandimapTechnicalError) as handimap_exception:
        handimap.check_response_and_get_json(resp)
    assert handimap_exception.value.data["message"] == 'Handimap service unavailable, impossible to query'


def check_response_and_get_json_handimap_func_json_invalid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp = MockResource(text="toto")
    with pytest.raises(jormungandr.exceptions.UnableToParse) as handimap_exception:
        handimap.check_response_and_get_json(resp)
    assert (
        handimap_exception.value.data["message"]
        == "Handimap unable to parse response, error: Unexpected character found when decoding 'true'"
    )


def get_language_handimap_func_language_invalid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    language = handimap._get_language("toto")
    assert language == "en-EN"


def get_language_handimap_func_language_valid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    language = handimap._get_language("english")
    assert language == "en-EN"


def get_language_parameter_handimap_func_language_invalid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    request = {"language": "toto"}
    language = handimap.get_language_parameter(request)
    assert language == "fr-FR"


def get_language_parameter_handimap_func_language_invalid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    request = {"language": "english"}
    language = handimap.get_language_parameter(request)
    assert language == "en-EN"


def make_request_arguments_walking_details_handimap_func_invalid_test():
    request = {"walking_speed": 1.5}
    res = Handimap._make_request_arguments_walking_details(request, "en-EN")
    assert res["costing"] == "walking"
    assert res["costing_options"]["walking"]["walking_speed"] == 5.0
    assert res["directions_options"]["language"] == "en-EN"


def make_request_arguments_direct_path_handimap_func_test():
    origin = type_pb2.PtObject()
    origin.embedded_type = type_pb2.POI
    origin.poi.coord.lon = 42.42
    origin.poi.coord.lat = 41.41

    destination = type_pb2.PtObject()
    destination.embedded_type = type_pb2.POI
    destination.poi.coord.lon = 32.41
    destination.poi.coord.lat = 31.42
    request = {"walking_speed": 1.5}
    arguments_direct_path = Handimap._make_request_arguments_direct_path(origin, destination, request, "en-EN")
    assert arguments_direct_path["costing"] == "walking"
    assert arguments_direct_path["costing_options"] == {'walking': {'walking_speed': 5.0}}
    assert arguments_direct_path["directions_options"]["units"] == "kilometers"
    assert arguments_direct_path["directions_options"]["language"] == "en-EN"
    assert len(arguments_direct_path["locations"]) == 2
    assert arguments_direct_path["locations"][0]["lat"] == origin.poi.coord.lat
    assert arguments_direct_path["locations"][0]["lon"] == origin.poi.coord.lon
    assert arguments_direct_path["locations"][1]["lat"] == destination.poi.coord.lat
    assert arguments_direct_path["locations"][1]["lon"] == destination.poi.coord.lon

    # Request with wheelchair or/and traveler_type in the request
    request["traveler_type"] = "fast_walker"
    arguments_direct_path = Handimap._make_request_arguments_direct_path(origin, destination, request, "en-EN")
    assert arguments_direct_path["costing"] == "walking"
    assert arguments_direct_path["costing_options"] == {'walking': {'walking_speed': 5.0}}
    request["traveler_type"] = "wheelchair"
    arguments_direct_path = Handimap._make_request_arguments_direct_path(origin, destination, request, "en-EN")
    assert arguments_direct_path["costing"] == "wheelchair"
    assert arguments_direct_path["costing_options"] == {'wheelchair': {'travel_speed': 5.0}}

    # parameter wheelchair has priority on parameter  traveler_type
    request["wheelchair"] = False
    arguments_direct_path = Handimap._make_request_arguments_direct_path(origin, destination, request, "en-EN")
    assert arguments_direct_path["costing"] == "walking"
    assert arguments_direct_path["costing_options"] == {'walking': {'walking_speed': 5.0}}
    request["wheelchair"] = True
    arguments_direct_path = Handimap._make_request_arguments_direct_path(origin, destination, request, "en-EN")
    assert arguments_direct_path["costing"] == "wheelchair"
    assert arguments_direct_path["costing_options"] == {'wheelchair': {'travel_speed': 5.0}}


def format_coord_handimap_func_test():
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = type_pb2.POI
    pt_object.poi.coord.lon = 42.42
    pt_object.poi.coord.lat = 41.41
    coords = Handimap._format_coord(pt_object)
    assert coords["lon"] == pt_object.poi.coord.lon
    assert coords["lat"] == pt_object.poi.coord.lat


def get_response_handimap_represents_start_true_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=-1.6761174, lat=48.1002462, uri='HandimapStart')
    destination = make_pt_object(type_pb2.ADDRESS, lon=-1.6740057, lat=48.097592, uri='HandimapEnd')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20220503T060000'), True)

    proto_resp = handimap._get_response(resp_json, origin, destination, fallback_extremity)

    assert len(proto_resp.journeys) == 1
    assert proto_resp.journeys[0].durations.total == 126
    assert proto_resp.journeys[0].durations.walking == 126
    assert proto_resp.journeys[0].distances.walking == 412

    assert len(proto_resp.journeys[0].sections) == 1
    assert proto_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
    assert proto_resp.journeys[0].sections[0].origin.uri == "HandimapStart"
    assert proto_resp.journeys[0].sections[0].destination.uri == "HandimapEnd"
    assert proto_resp.journeys[0].sections[0].street_network.length == 412
    assert proto_resp.journeys[0].sections[0].street_network.duration == 126
    assert proto_resp.journeys[0].sections[0].street_network.mode == response_pb2.Walking
    assert proto_resp.journeys[0].arrival_date_time == str_to_time_stamp('20220503T060206')
    assert proto_resp.journeys[0].departure_date_time == str_to_time_stamp('20220503T060000')

    assert proto_resp.journeys[0].sections[0].length == 412
    assert proto_resp.journeys[0].sections[0].duration == 126
    assert len(proto_resp.journeys[0].sections[0].street_network.path_items) == 7
    assert proto_resp.journeys[0].sections[0].street_network.path_items[0].length == 103
    assert proto_resp.journeys[0].sections[0].street_network.path_items[0].duration == 33
    assert (
        proto_resp.journeys[0].sections[0].street_network.path_items[0].instruction
        == resp_json["trip"]["legs"][0]["maneuvers"][0]["instruction"]
    )
    assert (
        proto_resp.journeys[0].sections[0].street_network.path_items[0].name
        == resp_json["trip"]["legs"][0]["maneuvers"][0]["street_names"][0]
    )


def get_response_handimap_represents_start_false_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=-1.6761174, lat=48.1002462, uri='HandimapStart')
    destination = make_pt_object(type_pb2.ADDRESS, lon=-1.6740057, lat=48.097592, uri='HandimapEnd')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20220503T060000'), False)

    proto_resp = handimap._get_response(resp_json, origin, destination, fallback_extremity)

    assert len(proto_resp.journeys) == 1
    assert proto_resp.journeys[0].durations.total == 126
    assert proto_resp.journeys[0].durations.walking == 126
    assert proto_resp.journeys[0].distances.walking == 412

    assert len(proto_resp.journeys[0].sections) == 1
    assert proto_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
    assert proto_resp.journeys[0].sections[0].origin.uri == "HandimapStart"
    assert proto_resp.journeys[0].sections[0].destination.uri == "HandimapEnd"
    assert proto_resp.journeys[0].sections[0].street_network.length == 412
    assert proto_resp.journeys[0].sections[0].street_network.duration == 126
    assert proto_resp.journeys[0].sections[0].street_network.mode == response_pb2.Walking
    assert proto_resp.journeys[0].arrival_date_time == str_to_time_stamp('20220503T060000')
    assert proto_resp.journeys[0].departure_date_time == str_to_time_stamp('20220503T055754')

    assert proto_resp.journeys[0].sections[0].length == 412
    assert proto_resp.journeys[0].sections[0].duration == 126
    assert len(proto_resp.journeys[0].sections[0].street_network.path_items) == 7
    assert proto_resp.journeys[0].sections[0].street_network.path_items[0].length == 103
    assert proto_resp.journeys[0].sections[0].street_network.path_items[0].duration == 33
    assert (
        proto_resp.journeys[0].sections[0].street_network.path_items[0].instruction
        == resp_json["trip"]["legs"][0]["maneuvers"][0]["instruction"]
    )
    assert (
        proto_resp.journeys[0].sections[0].street_network.path_items[0].name
        == resp_json["trip"]["legs"][0]["maneuvers"][0]["street_names"][0]
    )


def create_pt_object(lon, lat, pt_object_type=type_pb2.POI):
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = pt_object_type
    pt_object.poi.coord.lon = lon
    pt_object.poi.coord.lat = lat
    return pt_object


def check_content_response_handimap_func_valid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]
    handimap.check_content_response(resp_json, origins, destinations)


def check_content_response_handimap_func_invalid_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340)]
    with pytest.raises(jormungandr.exceptions.UnableToParse) as handimap_exception:
        handimap.check_content_response(resp_json, origins, destinations)
    assert handimap_exception.value.data["message"] == "Handimap nb response != nb requested"


def create_matrix_response_handimap_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]
    sn_matrix = handimap._create_matrix_response(resp_json, origins, destinations, 150)
    assert len(sn_matrix.rows) == 1
    assert len(sn_matrix.rows[0].routing_response) == 2
    assert sn_matrix.rows[0].routing_response[0].duration == 68
    assert sn_matrix.rows[0].routing_response[0].routing_status == response_pb2.reached
    assert sn_matrix.rows[0].routing_response[1].duration == 145
    assert sn_matrix.rows[0].routing_response[1].routing_status == response_pb2.reached


def check_content_response_handimap_func_valid_0_test():
    # response_id=0 : len(sources) == len(targets)
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = matrix_response_valid(0)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340)]
    handimap.check_content_response(resp_json, origins, destinations)


def check_content_response_handimap_func_valid_1_test():
    # response_id=1 : len(sources) < len(targets)
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]
    handimap.check_content_response(resp_json, origins, destinations)


def check_content_response_handimap_func_valid_2_test():
    # response_id=2 : len(sources) > len(targets)
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url=fake_service_url)
    resp_json = matrix_response_valid(2)
    origins = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]
    destinations = [create_pt_object(-1.680150, 48.108770)]
    handimap.check_content_response(resp_json, origins, destinations)
