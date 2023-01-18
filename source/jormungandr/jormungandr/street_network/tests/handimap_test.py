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
        Handimap(instance=instance, service_url='', username='aa', password="bb")
    assert str(excinfo.value) == 'service_url  is not a valid handimap url'


def test_create_handimap_with_default_values():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url='bob.com', username='aa', password="bb")
    assert handimap.sn_system_id == "handimap"
    assert handimap.timeout == 10
    assert handimap.service_url == "bob.com"
    assert handimap.modes == []
    assert handimap.breaker.reset_timeout == 60
    assert handimap.breaker.fail_max == 4
    assert handimap._feed_publisher.name == "handimap"
    assert handimap._feed_publisher.id == "handimap"
    assert handimap._feed_publisher.license == "Private"
    assert handimap._feed_publisher.url == "https://www.handimap.fr"


def test_create_handimap_with_config():
    instance = MagicMock()
    kwargs = {"circuit_breaker_max_fail": 2, "circuit_breaker_reset_timeout": 30}
    handimap = Handimap(
        instance=instance,
        id="id_handmap",
        service_url='bob.com',
        username='aa',
        password="bb",
        timeout=5,
        **kwargs
    )
    assert handimap.sn_system_id == "id_handmap"
    assert handimap.username == "aa"
    assert handimap.password == "bb"
    assert handimap.timeout == 5
    assert handimap.service_url == "bob.com"
    assert handimap.modes == []
    assert handimap.breaker.reset_timeout == 30
    assert handimap.breaker.fail_max == 2
    assert handimap._feed_publisher.name == "handimap"
    assert handimap._feed_publisher.id == "handimap"
    assert handimap._feed_publisher.license == "Private"
    assert handimap._feed_publisher.url == "https://www.handimap.fr"


def call_handimap_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url='bob.com', username='aa', password="bb")
    handimap.breaker = MagicMock()
    handimap.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    with pytest.raises(jormungandr.exceptions.HandimapTechnicalError) as handimap_exception:
        handimap._call_handimap(handimap.service_url, data={})
    assert handimap_exception.value.data["message"] == 'Handimap routing service unavailable'


def call_handimap_func_with_unknown_exception_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url='bob.com', username='aa', password="bb")
    handimap.breaker = MagicMock()
    handimap.breaker.call = MagicMock(side_effect=ValueError())
    with pytest.raises(jormungandr.exceptions.HandimapTechnicalError) as handimap_exception:
        handimap._call_handimap(handimap.service_url, data={})
    assert handimap_exception.value.data["message"] == 'Handimap routing has encountered unknown error'


def direct_path_handimap_func_with_mode_invalid():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url='bob.com', username='aa', password="bb")
    with pytest.raises(jormungandr.exceptions.InvalidArguments) as handimap_exception:
        handimap._direct_path(instance, "bike", None, None, None, None, None, "123")
    assert handimap_exception.value.data["message"] == 'Handimap, mode baike not implemented'


def check_response_handimap_func_code_invalid():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url='bob.com', username='aa', password="bb")
    resp = response_pb2.Response()
    resp.status_code = 401
    with pytest.raises(jormungandr.exceptions.HandimapTechnicalError) as handimap_exception:
        handimap._check_response(resp)
    assert handimap_exception.value.data["message"] == 'Handimap service unavailable, impossible to query'


def get_response_handimap_test():
    instance = MagicMock()
    handimap = Handimap(instance=instance, service_url='bob.com', username='aa', password="bb")
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

    assert proto_resp.journeys[0].sections[0].length == 412
    assert proto_resp.journeys[0].sections[0].duration == 126
    assert len(proto_resp.journeys[0].sections[0].street_network.path_items) == 7
    assert proto_resp.journeys[0].sections[0].street_network.path_items[0].length == 103
    assert proto_resp.journeys[0].sections[0].street_network.path_items[0].duration == 33
    assert (
        proto_resp.journeys[0].sections[0].street_network.path_items[0].instruction
        == "Marchez vers l'est sur Rue Ange Blaize."
    )
    assert proto_resp.journeys[0].sections[0].street_network.path_items[0].name == "Rue Ange Blaize"
