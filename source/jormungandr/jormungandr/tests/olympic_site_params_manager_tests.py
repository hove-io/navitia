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

from __future__ import absolute_import, unicode_literals


import navitiacommon.type_pb2 as type_pb2
from jormungandr.olympic_site_params_manager import OlympicSiteParamsManager
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
from jormungandr.instance import Instance
from copy import deepcopy

default_olympic_site_params = {
    "poi:BCD": {
        "events": [
            {
                "event": "20230714",
                "from_datetime": "20230714T100000",
                "to_datetime": "20230714T120000",
                "departure_scenario": "scenario a",
                "arrival_scenario": "scenario a",
            },
            {
                "event": "20230716",
                "from_datetime": "20230716T100000",
                "to_datetime": "20230716T120000",
                "departure_scenario": "scenario a",
                "arrival_scenario": "scenario a",
            },
            {
                "event": "20230715",
                "from_datetime": "20230715T100000",
                "to_datetime": "20230715T120000",
                "departure_scenario": "scenario b",
                "arrival_scenario": "scenario b",
            },
        ],
        "strict": True,
        "scenarios": {
            "scenario a": {
                "stop_points": {
                    "stop_point:463685": {"attractivity": 1, "virtual_fallback": 10},
                    "stop_point:463686": {"attractivity": 3, "virtual_fallback": 150},
                },
                "additional_parameters": {"max_walking_duration_to_pt": 13000},
            },
            "scenario b": {
                "stop_points": {
                    "stop_point:463685": {"attractivity": 1, "virtual_fallback": 15},
                    "stop_point:463686": {"attractivity": 3, "virtual_fallback": 130},
                },
                "additional_parameters": {"max_walking_duration_to_pt": 12000},
            },
        },
    },
    "poi:EFG": {
        "events": [
            {
                "event": "20230713",
                "from_datetime": "20230713T120000",
                "to_datetime": "20230713T140000",
                "departure_scenario": "scenario b",
                "arrival_scenario": "scenario b",
            },
            {
                "event": "20230714",
                "from_datetime": "20230714T100000",
                "to_datetime": "20230714T120000",
                "departure_scenario": "scenario a",
                "arrival_scenario": "scenario a",
            },
            {
                "event": "20230715",
                "from_datetime": "20230715T100000",
                "to_datetime": "20230715T120000",
                "departure_scenario": "scenario b",
                "arrival_scenario": "scenario b",
            },
        ],
        "scenarios": {
            "scenario a": {
                "stop_points": {
                    "stop_point:463685": {"attractivity": 11, "virtual_fallback": 100},
                    "stop_point:463686": {"attractivity": 30, "virtual_fallback": 15},
                }
            },
            "scenario b": {
                "stop_points": {
                    "stop_point:463685": {"attractivity": 12, "virtual_fallback": 99},
                    "stop_point:463686": {"attractivity": 29, "virtual_fallback": 50},
                }
            },
        },
    },
}


DEFAULT_OLYMPICS_FORBIDDEN_URIS = {
    "pt_object_olympics_forbidden_uris": ["nt:abc"],
    "poi_property_key": "olympic",
    "poi_property_value": "1234",
    "min_pt_duration": 15,
}


class FakeInstance(Instance):
    def __init__(self, name="fake_instance", olympics_forbidden_uris=None):
        super(FakeInstance, self).__init__(
            context=None,
            name=name,
            zmq_socket=None,
            street_network_configurations=[],
            ridesharing_configurations={},
            instance_equipment_providers=[],
            realtime_proxies_configuration=[],
            pt_planners_configurations={},
            zmq_socket_type=None,
            autocomplete_type='kraken',
            streetnetwork_backend_manager=None,
            external_service_provider_configurations=[],
            olympics_forbidden_uris=olympics_forbidden_uris,
            pt_journey_fare_configurations={},
        )


class FakeS3Object:
    def __init__(self, key="file.json"):
        self.key = key


def test_get_departure_olympic_site_params():

    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    default_scenario = osp.get_dict_scenario(
        "poi:BCD", "departure_scenario", datetime=osp.get_timestamp('20230714T110000')
    )
    attractivity_virtual_fallback = default_scenario["stop_point:463685"]
    assert attractivity_virtual_fallback.attractivity == 1
    assert attractivity_virtual_fallback.virtual_duration == 10

    attractivity_virtual_fallback = default_scenario["stop_point:463686"]
    assert attractivity_virtual_fallback.attractivity == 3
    assert attractivity_virtual_fallback.virtual_duration == 150

    default_scenario = osp.get_dict_scenario(
        "poi:EFG", "departure_scenario", datetime=osp.get_timestamp('20230714T110000')
    )
    attractivity_virtual_fallback = default_scenario["stop_point:463685"]
    assert attractivity_virtual_fallback.attractivity == 11
    assert attractivity_virtual_fallback.virtual_duration == 100

    attractivity_virtual_fallback = default_scenario["stop_point:463686"]
    assert attractivity_virtual_fallback.attractivity == 30
    assert attractivity_virtual_fallback.virtual_duration == 15


def test_get_arrival_olympic_site_params():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    default_scenario = osp.get_dict_scenario(
        "poi:BCD", "arrival_scenario", datetime=osp.get_timestamp('20230716T110000')
    )
    attractivity_virtual_fallback = default_scenario["stop_point:463685"]
    assert attractivity_virtual_fallback.attractivity == 1
    assert attractivity_virtual_fallback.virtual_duration == 10

    attractivity_virtual_fallback = default_scenario["stop_point:463686"]
    assert attractivity_virtual_fallback.attractivity == 3
    assert attractivity_virtual_fallback.virtual_duration == 150

    default_scenario = osp.get_dict_scenario(
        "poi:EFG", "arrival_scenario", datetime=osp.get_timestamp('20230713T130000')
    )
    attractivity_virtual_fallback = default_scenario["stop_point:463685"]
    assert attractivity_virtual_fallback.attractivity == 12
    assert attractivity_virtual_fallback.virtual_duration == 99

    attractivity_virtual_fallback = default_scenario["stop_point:463686"]
    assert attractivity_virtual_fallback.attractivity == 29
    assert attractivity_virtual_fallback.virtual_duration == 50


def test_build_with_request_params_and_without_criteria_without_keep_olympics_journeys():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    api_request = dict()
    api_request["_olympics_sites_attractivities[]"] = [('stop_point:24113', 3), ('stop_point:24131', 30)]
    api_request["_olympics_sites_virtual_fallback[]"] = [('stop_point:24113', 800), ('stop_point:24131', 820)]

    assert not api_request.get("criteria")
    assert "_keep_olympics_journeys" not in api_request
    assert not api_request.get("max_walking_duration_to_pt")
    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]
    pt_origin_detail = make_pt_object(type_pb2.ADDRESS, 1, 2, "SA:BCD")

    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["criteria"] == "arrival_stop_attractivity"
    assert api_request["_keep_olympics_journeys"]
    assert not api_request.get("max_walking_duration_to_pt")
    olympic_site_params = api_request["olympic_site_params"]
    assert "arrival_scenario" in olympic_site_params
    assert "departure_scenario" not in olympic_site_params
    attractivity_virtual_fallback = olympic_site_params["arrival_scenario"]["stop_point:24113"]
    assert attractivity_virtual_fallback.attractivity == 3
    assert attractivity_virtual_fallback.virtual_duration == 800

    attractivity_virtual_fallback = olympic_site_params["arrival_scenario"]["stop_point:24131"]
    assert attractivity_virtual_fallback.attractivity == 30
    assert attractivity_virtual_fallback.virtual_duration == 820


def test_build_with_request_params_and_without_criteria_with_keep_olympics_journeys():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = default_olympic_site_params
    api_request = dict()
    api_request["_olympics_sites_attractivities[]"] = [('stop_point:24113', 3), ('stop_point:24131', 30)]
    api_request["_olympics_sites_virtual_fallback[]"] = [('stop_point:24113', 800), ('stop_point:24131', 820)]
    api_request["_keep_olympics_journeys"] = False

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]
    pt_origin_detail = make_pt_object(type_pb2.ADDRESS, 1, 2, "SA:BCD")
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["criteria"] == "arrival_stop_attractivity"
    assert not api_request["_keep_olympics_journeys"]
    assert not api_request.get("max_walking_duration_to_pt")
    olympic_site_params = api_request["olympic_site_params"]
    assert "arrival_scenario" in olympic_site_params
    assert "departure_scenario" not in olympic_site_params
    attractivity_virtual_fallback = olympic_site_params["arrival_scenario"]["stop_point:24113"]
    assert attractivity_virtual_fallback.attractivity == 3
    assert attractivity_virtual_fallback.virtual_duration == 800

    attractivity_virtual_fallback = olympic_site_params["arrival_scenario"]["stop_point:24131"]
    assert attractivity_virtual_fallback.attractivity == 30
    assert attractivity_virtual_fallback.virtual_duration == 820


def test_build_with_request_params_and_departure_criteria():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = default_olympic_site_params
    api_request = dict()
    api_request["_olympics_sites_attractivities[]"] = [('stop_point:24113', 3), ('stop_point:24131', 30)]
    api_request["_olympics_sites_virtual_fallback[]"] = [('stop_point:24113', 800), ('stop_point:24131', 820)]
    api_request["criteria"] = "departure_stop_attractivity"

    assert api_request["criteria"] == "departure_stop_attractivity"
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    pt_origin_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_origin_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]
    pt_destination_detail = make_pt_object(type_pb2.ADDRESS, 1, 2, "SA:BCD")
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["_keep_olympics_journeys"]
    assert api_request["criteria"] == "departure_stop_attractivity"
    assert not api_request.get("max_walking_duration_to_pt")
    olympic_site_params = api_request["olympic_site_params"]
    assert "arrival_scenario" not in olympic_site_params
    assert "departure_scenario" in olympic_site_params
    attractivity_virtual_fallback = olympic_site_params["departure_scenario"]["stop_point:24113"]
    assert attractivity_virtual_fallback.attractivity == 3
    assert attractivity_virtual_fallback.virtual_duration == 800

    attractivity_virtual_fallback = olympic_site_params["departure_scenario"]["stop_point:24131"]
    assert attractivity_virtual_fallback.attractivity == 30
    assert attractivity_virtual_fallback.virtual_duration == 820


def test_build_with_request_params_and_arrival_criteria():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = default_olympic_site_params
    api_request = dict()
    api_request["_olympics_sites_attractivities[]"] = [('stop_point:24113', 3), ('stop_point:24131', 30)]
    api_request["_olympics_sites_virtual_fallback[]"] = [('stop_point:24113', 800), ('stop_point:24131', 820)]
    api_request["criteria"] = "arrival_stop_attractivity"

    assert api_request["criteria"] == "arrival_stop_attractivity"
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]
    pt_origin_detail = make_pt_object(type_pb2.ADDRESS, 1, 2, "SA:BCD")
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["_keep_olympics_journeys"]
    assert api_request["criteria"] == "arrival_stop_attractivity"
    assert not api_request.get("max_walking_duration_to_pt")
    olympic_site_params = api_request["olympic_site_params"]
    assert "arrival_scenario" in olympic_site_params
    assert "departure_scenario" not in olympic_site_params
    attractivity_virtual_fallback = olympic_site_params["arrival_scenario"]["stop_point:24113"]
    assert attractivity_virtual_fallback.attractivity == 3
    assert attractivity_virtual_fallback.virtual_duration == 800

    attractivity_virtual_fallback = olympic_site_params["arrival_scenario"]["stop_point:24131"]
    assert attractivity_virtual_fallback.attractivity == 30
    assert attractivity_virtual_fallback.virtual_duration == 820


def test_build_without_request_params():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = default_olympic_site_params
    api_request = dict()

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    osp.build(None, None, api_request)
    assert "_keep_olympics_journeys" not in api_request
    assert not api_request.get("max_walking_duration_to_pt")
    olympic_site_params = api_request["olympic_site_params"]
    assert "arrival_scenario" not in olympic_site_params
    assert "departure_scenario" not in olympic_site_params


def test_build_origin_poi_jo():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    api_request = dict()
    pt_origin_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_origin_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]
    pt_destination_detail = make_pt_object(type_pb2.ADDRESS, 1, 2, "SA:BCD")

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    api_request["datetime"] = osp.get_timestamp('20230714T110000')
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["_keep_olympics_journeys"]
    assert api_request["criteria"] == "departure_stop_attractivity"
    assert api_request.get("max_walking_duration_to_pt") == 13000
    olympic_site_params = api_request["olympic_site_params"]
    assert olympic_site_params["strict"]
    assert "arrival_scenario" not in olympic_site_params
    assert "departure_scenario" in olympic_site_params

    assert olympic_site_params["departure_scenario"]["stop_point:463685"].attractivity == 1
    assert olympic_site_params["departure_scenario"]["stop_point:463685"].virtual_duration == 10

    assert olympic_site_params["departure_scenario"]['stop_point:463686'].attractivity == 3
    assert olympic_site_params["departure_scenario"]['stop_point:463686'].virtual_duration == 150


def test_build_arrival_poi_jo():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    api_request = dict()
    pt_origin_detail = make_pt_object(type_pb2.ADDRESS, 1, 2, "SA:BCD")

    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    api_request["datetime"] = osp.get_timestamp('20230715T110000')
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["_keep_olympics_journeys"]
    assert api_request.get("max_walking_duration_to_pt") == 12000
    assert api_request["criteria"] == "arrival_stop_attractivity"
    olympic_site_params = api_request["olympic_site_params"]
    assert olympic_site_params["strict"]
    assert "arrival_scenario" in olympic_site_params
    assert "departure_scenario" not in olympic_site_params

    assert olympic_site_params["arrival_scenario"]["stop_point:463685"].attractivity == 1
    assert olympic_site_params["arrival_scenario"]["stop_point:463685"].virtual_duration == 15

    assert olympic_site_params["arrival_scenario"]['stop_point:463686'].attractivity == 3
    assert olympic_site_params["arrival_scenario"]['stop_point:463686'].virtual_duration == 130


def test_build_departure_and_arrival_poi_jo():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    api_request = dict()
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)

    pt_origin_detail = make_pt_object(type_pb2.POI, 1, 2, "poi:EFG")
    property = pt_origin_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    api_request["datetime"] = osp.get_timestamp('20230715T110000')
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["_keep_olympics_journeys"]
    assert api_request["criteria"] == "arrival_stop_attractivity"
    assert api_request.get("max_walking_duration_to_pt") == 12000
    olympic_site_params = api_request["olympic_site_params"]
    assert olympic_site_params["strict"]
    assert "arrival_scenario" in olympic_site_params
    assert "departure_scenario" not in olympic_site_params

    assert olympic_site_params["arrival_scenario"]["stop_point:463685"].attractivity == 1
    assert olympic_site_params["arrival_scenario"]["stop_point:463685"].virtual_duration == 15

    assert olympic_site_params["arrival_scenario"]['stop_point:463686'].attractivity == 3
    assert olympic_site_params["arrival_scenario"]['stop_point:463686'].virtual_duration == 130


def test_get_dict_scenario_empty_scenario():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    res = osp.get_dict_scenario(None, {}, datetime=osp.get_timestamp('20230715T110000'))
    assert not res


def test_build_olympic_site_params_empty_scenario():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    res = osp.build_olympic_site_params(None, {})
    assert not res


def test_get_dict_additional_parameters_invalid_poi():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    res = osp.get_dict_additional_parameters("poi:id", "default", datetime=osp.get_timestamp('20230715T110000'))
    assert not res


def test_get_dict_additional_parameters_invalid_scenario():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    res = osp.get_dict_additional_parameters("poi:BCD", "abc", datetime=osp.get_timestamp('20230715T110000'))
    assert not res


def test_get_strict_parameter_invalid_poi():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    res = osp.get_strict_parameter("poi:id")
    assert not res


def test_get_json_content_invalid_s3_object():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)
    fake_s3_object = FakeS3Object()
    res = osp.get_json_content(fake_s3_object)
    assert not res


def test_fill_olympic_site_params_from_s3_without_bucket_name():
    from jormungandr import app

    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    app.config["OLYMPIC_SITE_PARAMS_BUCKET"] = {"test": "test"}
    osp.fill_olympic_site_params_from_s3()
    assert not osp.olympic_site_params


def test_fill_olympic_site_params_from_s3_invalid_access():
    from jormungandr import app

    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    app.config["OLYMPIC_SITE_PARAMS_BUCKET"] = {
        "name": "test",
        "args": {"connect_timeout": 0.01, "read_timeout": 0.01, "retries": {'max_attempts': 0}},
    }
    osp.fill_olympic_site_params_from_s3()
    assert not osp.olympic_site_params


def test_build_departure_and_arrival_poi_jo_empty_olympic_site_params():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)

    pt_origin_detail = make_pt_object(type_pb2.POI, 1, 2, "poi:EFG")
    property = pt_origin_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    api_request = {}

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    api_request["datetime"] = osp.get_timestamp('20230715T110000')
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert not api_request["olympic_site_params"]


def test_build_departure_and_arrival_poi_jo_add_forbidden_uris():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)

    pt_origin_detail = make_pt_object(type_pb2.POI, 1, 2, "poi:EFG")
    property = pt_origin_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    api_request = {"forbidden_uris[]": ["uri1", "uri2"]}

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    api_request["datetime"] = osp.get_timestamp('20230715T110000')
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["olympic_site_params"]
    assert len(api_request["forbidden_uris[]"]) == 3


def test_get_dict_scenario_invalid_scanario_name():
    instance = FakeInstance(olympics_forbidden_uris=DEFAULT_OLYMPICS_FORBIDDEN_URIS)
    osp = OlympicSiteParamsManager(instance)
    osp.olympic_site_params = deepcopy(default_olympic_site_params)
    osp.str_datetime_time_stamp(osp.olympic_site_params)

    pt_origin_detail = make_pt_object(type_pb2.POI, 1, 2, "poi:EFG")
    property = pt_origin_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    pt_destination_detail = make_pt_object(type_pb2.POI, 2, 3, "poi:BCD")
    property = pt_destination_detail.poi.properties.add()
    property.type = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_key"]
    property.value = DEFAULT_OLYMPICS_FORBIDDEN_URIS["poi_property_value"]

    api_request = dict()

    assert not api_request.get("criteria")
    assert not api_request.get("max_walking_duration_to_pt")
    assert "_keep_olympics_journeys" not in api_request
    api_request["datetime"] = osp.get_timestamp('20230715T110000')
    osp.build(pt_origin_detail, pt_destination_detail, api_request)
    assert api_request["olympic_site_params"]
    assert len(api_request["forbidden_uris[]"]) == 1
    scenario_name = osp.get_valid_scenario_name(
        osp.olympic_site_params["poi:BCD"]["events"], "departure_scenario", osp.get_timestamp('20230720T110000')
    )
    assert not scenario_name
    scenario = osp.get_dict_scenario("poi:BCD", "departure_scenario", osp.get_timestamp('20230720T110000'))
    assert not scenario
