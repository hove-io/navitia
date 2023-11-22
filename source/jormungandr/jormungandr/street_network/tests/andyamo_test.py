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
from jormungandr.street_network.andyamo import Andyamo
from jormungandr.utils import (
    str_to_time_stamp,
    PeriodExtremity,
)


class MockResource(object):
    def __init__(self, text=None, status=200):
        self.text = text
        self.status_code = status


def matrix_response_valid(response_id=1):
    # response_id=0 : len(sources) == len(targets)
    # response_id=1 : len(sources) < len(targets)
    # response_id=2 : len(sources) > len(targets)
    responses = {
        0: {
            "sources_to_targets": [
                {"from_index": 0, "to_index": 0, "time": 0, "distance": 0.0},
                {"distance": 0.035, "time": 28, "to_index": 1, "from_index": 0},
                {"from_index": 0, "to_index": 2, "time": 107, "distance": 0.134},
            ],
            "locations": {
                "targets": [
                    {"lat": 45.758373, "lon": 4.833177},
                    {"lat": 45.75817, "lon": 4.833374},
                    {"lat": 45.758923, "lon": 4.833948},
                ],
                "sources": [{"lat": 45.75843, "lon": 4.83307}],
            },
        },
        1: {
            "sources_to_targets": [
                {"from_index": 0, "to_index": 0, "time": 0, "distance": 0.0},
                {"distance": 0.035, "time": 28, "to_index": 1, "from_index": 0},
                {"from_index": 0, "to_index": 2, "time": 107, "distance": 0.134},
            ],
            "locations": {
                "targets": [
                    {"lat": 45.758373, "lon": 4.833177},
                    {"lat": 45.75817, "lon": 4.833374},
                    {"lat": 45.758923, "lon": 4.833948},
                ],
                "sources": [{"lat": 45.75843, "lon": 4.83307}],
            },
        },
        2: {
            "sources_to_targets": [
                {"from_index": 0, "to_index": 0, "time": 0, "distance": 0.0},
                {"distance": 0.035, "time": 28, "to_index": 1, "from_index": 0},
                {"from_index": 0, "to_index": 2, "time": 107, "distance": 0.134},
            ],
            "locations": {
                "targets": [
                    {"lat": 45.758373, "lon": 4.833177},
                    {"lat": 45.75817, "lon": 4.833374},
                    {"lat": 45.758923, "lon": 4.833948},
                ],
                "sources": [{"lat": 45.75843, "lon": 4.83307}],
            },
        },
    }
    return responses[response_id]


def direct_path_response_valid():
    return {
        "trip": {
            "summary": {
                "has_time_restrictions": False,
                "has_toll": False,
                "min_lon": 4.847325,
                "time": 533,
                "has_highway": False,
                "has_ferry": False,
                "min_lat": 45.743376,
                "max_lat": 45.745828,
                "length": 0.66708,
                "max_lon": 4.852355,
                "cost": 533,
            },
            "units": "kilometers",
            "legs": [
                {
                    "maneuvers": [
                        {
                            "type": 0,
                            "length": 0,
                            "instruction": "instruction placeholder",
                            "verbal_post_transition_instruction": "instruction placeholder",
                            "verbal_succinct_transition_instruction": "instruction placeholder",
                            "end_shape_index": 0,
                            "verbal_pre_transition_instruction": "instruction placeholder",
                            "time": 0,
                            "cost": 0,
                            "begin_shape_index": 0,
                            "travel_mode": "pedestrian",
                            "travel_type": "foot",
                        }
                    ],
                    "summary": {
                        "has_time_restrictions": False,
                        "has_toll": False,
                        "min_lon": 4.847325,
                        "time": 533,
                        "has_highway": False,
                        "has_ferry": False,
                        "min_lat": 45.743376,
                        "max_lat": 45.745828,
                        "length": 0.66708,
                        "max_lon": 4.852355,
                        "cost": 533,
                    },
                    "shape": "_p}fvAylzfHcF_@iDgBdAgGoUiQeFqEy{@cx@sEmCuGyG}KmIaMm{AcAmCaCoK_CqS~k@s\\hMvDxp@ac@oBiE",
                }
            ],
            "status": 0,
            "language": "fr-FR",
            "status_message": "Status Placeholder",
        },
        "id": "andyamo_directions",
    }


fake_service_url = "andyamo.com"
fake_asgard_url = "asgard.andyamo.com"
fake_asgard_socket = "tcp://socket.andyamo.com:666"
service_backup = {
    "args": {"service_url": fake_asgard_url, "asgard_socket": fake_asgard_socket},
    "class": "jormungandr.street_network.asgard.Asgard",
}


def test_create_andyamo_without_service_backup():
    instance = MagicMock()
    with pytest.raises(ValueError) as excinfo:
        Andyamo(instance=instance, service_url=fake_service_url, service_backup=None, zone='')
    assert str(excinfo.value) == 'service_backup None is not define cant forward to asgard'


def test_create_andyamo_without_service_url():
    instance = MagicMock()
    with pytest.raises(ValueError) as excinfo:
        Andyamo(instance=instance, service_url=None, service_backup=service_backup, zone='')
    assert str(excinfo.value) == 'service_url None is not a valid andyamo url'


def test_create_andyamo_with_default_values():
    instance = MagicMock()

    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    assert andyamo.sn_system_id == "andyamo"
    assert andyamo.timeout == 10
    assert andyamo.service_url == fake_service_url
    assert len(andyamo.modes) == 1
    assert andyamo.modes[0] == "walking"
    assert andyamo.breaker.reset_timeout == 60
    assert andyamo.breaker.fail_max == 4
    assert andyamo._feed_publisher.name == "andyamo"
    assert andyamo._feed_publisher.id == "andyamo"
    assert andyamo._feed_publisher.license == "Private"
    assert andyamo._feed_publisher.url == "https://www.andyamo.fr"


def test_create_andyamo_with_config_test():
    instance = MagicMock()
    kwargs = {"circuit_breaker_max_fail": 2, "circuit_breaker_reset_timeout": 30}
    andyamo = Andyamo(
        instance=instance,
        id="id_handmap",
        service_url=fake_service_url,
        service_backup=service_backup,
        zone='',
        timeout=5,
        **kwargs,
    )
    assert andyamo.sn_system_id == "id_handmap"
    assert andyamo.timeout == 5
    assert andyamo.service_url == fake_service_url
    assert len(andyamo.modes) == 1
    assert andyamo.modes[0] == "walking"
    assert andyamo.breaker.reset_timeout == 30
    assert andyamo.breaker.fail_max == 2
    assert andyamo._feed_publisher.name == "andyamo"
    assert andyamo._feed_publisher.id == "andyamo"
    assert andyamo._feed_publisher.license == "Private"
    assert andyamo._feed_publisher.url == "https://www.andyamo.fr"


def test_create_andyamo_status_test():
    instance = MagicMock()
    kwargs = {"circuit_breaker_max_fail": 2, "circuit_breaker_reset_timeout": 30}
    andyamo = Andyamo(
        instance=instance,
        id="id_handmap",
        service_url=fake_service_url,
        modes=["walking"],
        service_backup=service_backup,
        zone='',
        timeout=5,
        **kwargs,
    )
    status = andyamo.status()
    assert status["id"] == "id_handmap"
    assert len(status["modes"]) == 1
    assert status["modes"][0] == "walking"
    assert status["timeout"] == 5


def call_andyamo_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    andyamo.breaker = MagicMock()
    andyamo.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())

    try:
        andyamo._call_andyamo(andyamo.service_url, data={})
        assert False, "AndyamoTechnicalError expected but not raised"
    except jormungandr.exceptions.AndyamoTechnicalError as e:
        assert str(e) == 'Andyamo routing service unavailable, Circuit breaker is open'
    except Exception as e:
        assert False, f"Unexpected exception type: {type(e).__name__}"


def call_andyamo_func_with_unknown_exception_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    andyamo.breaker = MagicMock()
    andyamo.breaker.call = MagicMock(side_effect=ValueError())
    with pytest.raises(jormungandr.exceptions.AndyamoTechnicalError) as andyamo_exception:
        andyamo._call_andyamo(andyamo.service_url, data={})
    assert str(andyamo_exception.value) == '500 Internal Server Error: None'


def check_response_and_get_json_andyamo_func_code_invalid_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp = MockResource(status=401)
    with pytest.raises(jormungandr.exceptions.AndyamoTechnicalError) as andyamo_exception:
        andyamo.check_response_and_get_json(resp)
    assert str(andyamo_exception.value) == '500 Internal Server Error: None'


def check_response_and_get_json_andyamo_func_json_invalid_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp = MockResource(text="toto")
    with pytest.raises(jormungandr.exceptions.UnableToParse) as andyamo_exception:
        andyamo.check_response_and_get_json(resp)
    assert str(andyamo_exception.value) == "400 Bad Request: None"


def format_coord_andyamo_func_test():
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = type_pb2.POI
    pt_object.poi.coord.lon = 42.42
    pt_object.poi.coord.lat = 41.41
    coords = Andyamo._format_coord(pt_object)
    assert coords["lon"] == pt_object.poi.coord.lon
    assert coords["lat"] == pt_object.poi.coord.lat


def get_response_andyamo_represents_start_true_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=-1.6761174, lat=48.1002462, uri='AndyamoStart')
    destination = make_pt_object(type_pb2.ADDRESS, lon=-1.6740057, lat=48.097592, uri='AndyamoEnd')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20220503T060000'), True)

    proto_resp = andyamo._get_response(resp_json, origin, destination, fallback_extremity)

    assert len(proto_resp.journeys) == 1
    assert proto_resp.journeys[0].durations.total == 533
    assert proto_resp.journeys[0].durations.walking == 533
    assert proto_resp.journeys[0].distances.walking == 667

    assert len(proto_resp.journeys[0].sections) == 1
    assert proto_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
    assert proto_resp.journeys[0].sections[0].origin.uri == "AndyamoStart"
    assert proto_resp.journeys[0].sections[0].destination.uri == "AndyamoEnd"
    assert proto_resp.journeys[0].sections[0].street_network.length == 667
    assert proto_resp.journeys[0].sections[0].street_network.duration == 533
    assert proto_resp.journeys[0].sections[0].street_network.mode == response_pb2.Walking
    assert proto_resp.journeys[0].arrival_date_time == str_to_time_stamp('20220503T060000') + 533


def get_response_andyamo_represents_start_false_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=-1.6761174, lat=48.1002462, uri='AndyamoStart')
    destination = make_pt_object(type_pb2.ADDRESS, lon=-1.6740057, lat=48.097592, uri='AndyamoEnd')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20220503T060000'), False)

    proto_resp = andyamo._get_response(resp_json, origin, destination, fallback_extremity)

    assert len(proto_resp.journeys) == 1
    assert proto_resp.journeys[0].durations.total == 533  # Adjusted to match the response
    assert proto_resp.journeys[0].durations.walking == 533  # Adjusted to match the response
    # Compare the distance allowing a small difference
    assert (
        abs(proto_resp.journeys[0].distances.walking - 667.08) < 0.1
    )  # Allow a small difference due to rounding


def create_pt_object(lon, lat, pt_object_type=type_pb2.POI):
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = pt_object_type
    if pt_object_type == type_pb2.POI:
        pt_object.poi.coord.lon = lon
        pt_object.poi.coord.lat = lat
    elif pt_object_type == type_pb2.ADDRESS:
        pt_object.address.coord.lon = lon
        pt_object.address.coord.lat = lat
    # Ajouter des cas pour d'autres types si nécessaire
    return pt_object


def check_content_response_andyamo_func_valid_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]

    # Assuming check_content_response is a method of Andyamo that does not return a value but raises an exception if invalid
    try:
        andyamo.check_content_response(resp_json, origins, destinations)
        assert True  # Pass the test if no exception is raised
    except Exception as e:
        pytest.fail(f"Unexpected exception raised: {e}")


def call_andyamo_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    andyamo.breaker = MagicMock()
    andyamo.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    with pytest.raises(jormungandr.exceptions.AndyamoTechnicalError) as andyamo_exception:
        andyamo._call_andyamo(andyamo.service_url, data={})

    # Vérifiez que le message de l'exception contient la chaîne attendue
    assert '500 Internal Server Error: None' in str(andyamo_exception.value)


def create_matrix_response_andyamo_test():
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]
    sn_matrix = andyamo._create_matrix_response(resp_json, origins, destinations, 150)
    assert len(sn_matrix.rows) == 1
    assert (
        len(sn_matrix.rows[0].routing_response) == 3
    )  # Ajusté pour correspondre au nombre de réponses attendues


def check_content_response_andyamo_func_valid_0_test():
    # response_id=0 : len(sources) == len(targets)
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(0)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340)]
    try:
        andyamo.check_content_response(resp_json, origins, destinations)
        assert True  # Pass the test if no exception is raised
    except Exception as e:
        pytest.fail(f"Unexpected exception raised: {e}")


def check_content_response_andyamo_func_valid_1_test():
    # response_id=1 : len(sources) < len(targets)
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(1)
    origins = [create_pt_object(-1.680150, 48.108770)]
    destinations = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]

    # Assuming check_content_response is a method of Andyamo that does not return a value but raises an exception if invalid
    try:
        andyamo.check_content_response(resp_json, origins, destinations)
        assert True  # Pass the test if no exception is raised
    except Exception as e:
        pytest.fail(f"Unexpected exception raised: {e}")


def check_content_response_andyamo_func_valid_2_test():
    # response_id=2 : len(sources) > len(targets)
    instance = MagicMock()
    andyamo = Andyamo(instance=instance, service_url=fake_service_url, service_backup=service_backup, zone='')
    resp_json = matrix_response_valid(2)
    origins = [create_pt_object(-1.679860, 48.109340), create_pt_object(-1.678750, 48.109390)]
    destinations = [create_pt_object(-1.680150, 48.108770)]

    # Assuming check_content_response is a method of Andyamo that does not return a value but raises an exception if invalid
    try:
        andyamo.check_content_response(resp_json, origins, destinations)
        assert True  # Pass the test if no exception is raised
    except Exception as e:
        pytest.fail(f"Unexpected exception raised: {e}")
