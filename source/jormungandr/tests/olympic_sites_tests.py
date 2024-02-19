# -*- coding: utf-8 -*-

# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division

from .tests_mechanism import AbstractTestFixture
from .journey_common_tests import *
from six.moves.urllib.parse import urlencode
import requests_mock
from jormungandr.utils import get_lon_lat
from copy import deepcopy
from jormungandr.olympic_site_params_manager import OlympicSiteParamsManager
from jormungandr.instance import parse_and_get_olympics_forbidden_uris


template_journey_query = "journeys?from={place_from}&to={place_to}&datetime=20120614T080000"

s_lon, s_lat = get_lon_lat(s_coord)
r_lon, r_lat = get_lon_lat(r_coord)

from_poi_uri = "poi:from"
to_poi_uri = "poi:to"

DEFAULT_OLYMPIC_SITE_PARAMS = {
    "poi:from": {
        "events": [
            {
                "event": "20120614",
                "from_datetime": "20120614T070000",
                "to_datetime": "20120614T080000",
                "departure_scenario": "scenario a",
                "arrival_scenario": "scenario b",
            },
            {
                "event": "20120615",
                "from_datetime": "20120615T070000",
                "to_datetime": "20120615T080000",
                "departure_scenario": "scenario aa",
                "arrival_scenario": "scenario bb",
            },
        ],
        "strict": False,
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
            "scenario aa": {
                "stop_points": {
                    "stop_point:463685": {"attractivity": 11, "virtual_fallback": 101},
                    "stop_point:463686": {"attractivity": 31, "virtual_fallback": 1150},
                },
                "additional_parameters": {"max_walking_duration_to_pt": 13001},
            },
            "scenario bb": {
                "stop_points": {
                    "stop_point:463685": {"attractivity": 11, "virtual_fallback": 151},
                    "stop_point:463686": {"attractivity": 31, "virtual_fallback": 1130},
                },
                "additional_parameters": {"max_walking_duration_to_pt": 12001},
            },
        },
    },
    "poi:to": {
        "events": [
            {
                "event": "20120614",
                "from_datetime": "20120614T070000",
                "to_datetime": "20120614T080000",
                "departure_scenario": "scenario a",
                "arrival_scenario": "scenario b",
            }
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
olympic_site_params_show_natural_opg_journeys = deepcopy(DEFAULT_OLYMPIC_SITE_PARAMS)
scenarios = olympic_site_params_show_natural_opg_journeys[to_poi_uri]["scenarios"]
scenarios["scenario b"]["additional_parameters"] = {"show_natural_opg_journeys": True}

from_addr_uri = "{};{}".format(s_lon, s_lat)
to_addr_uri = "{};{}".format(r_lon, r_lat)

FROM_POI = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [s_lon, s_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": from_poi_uri,
                    "type": "poi",
                    "label": "Jardin (City)",
                    "name": "Jardin",
                    "city": "Paris",
                    "poi_types": [{"id": "poi_type:jardin", "name": "Jardin"}],
                    "properties": [{"key": "olympic", "value": "olympic:site"}],
                }
            },
        }
    ]
}


TO_POI = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [r_lon, r_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": to_poi_uri,
                    "type": "poi",
                    "label": "Jardin (City)",
                    "name": "Jardin",
                    "city": "Paris",
                    "poi_types": [{"id": "poi_type:jardin", "name": "Jardin"}],
                    "properties": [{"key": "olympic", "value": "olympic:site"}],
                },
            },
        }
    ]
}

TO_POI_NOT_OLYMPIC = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [r_lon, r_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": to_poi_uri,
                    "type": "poi",
                    "label": "Jardin (City)",
                    "name": "Jardin",
                    "city": "Paris",
                    "poi_types": [{"id": "poi_type:jardin", "name": "Jardin"}],
                    "properties": [{"key": "abcd", "value": "olympic:site"}],
                },
            },
        }
    ]
}

FROM_ADDRESS = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [s_lon, s_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": from_addr_uri,
                    "type": "house",
                    "label": "10 Rue bob (City)",
                    "name": "10 Rue bob",
                    "housenumber": "10",
                    "street": "Rue bob",
                    "postcode": "36500",
                    "city": "City",
                }
            },
        }
    ]
}

TO_ADDRESS = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [r_lon, r_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": to_addr_uri,
                    "type": "house",
                    "label": "10 Rue Victor (City)",
                    "name": "10 Rue Victor",
                    "housenumber": "10",
                    "street": "Rue Victor",
                    "postcode": "36500",
                    "city": "City",
                }
            },
        }
    ]
}

FROM_ADDRESS_WITH_WITHIN = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [s_lon, s_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": from_addr_uri,
                    "type": "house",
                    "label": "10 Rue bob (City)",
                    "name": "10 Rue bob",
                    "housenumber": "10",
                    "street": "Rue bob",
                    "postcode": "36500",
                    "city": "City",
                }
            },
        }
    ],
    "zones": [
        {
            "type": "zone",
            "geometry": {"coordinates": [s_lon, s_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": from_poi_uri,
                    "type": "poi",
                    "label": "Jardin (City)",
                    "name": "Jardin",
                    "city": "Paris",
                    "poi_types": [{"id": "poi_type:jardin", "name": "Jardin"}],
                    "properties": [{"key": "olympic", "value": "olympic:site"}],
                }
            },
        }
    ],
}


FROM_ADDRESS_WITH_INVALID_WITHIN = {
    "features": [
        {
            "type": "Feature",
            "geometry": {"coordinates": [s_lon, s_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": from_addr_uri,
                    "type": "house",
                    "label": "10 Rue bob (City)",
                    "name": "10 Rue bob",
                    "housenumber": "10",
                    "street": "Rue bob",
                    "postcode": "36500",
                    "city": "City",
                }
            },
        }
    ],
    "zones": [
        {
            "type": "zone",
            "geometry": {"coordinates": [s_lon, s_lat], "type": "Point"},
            "properties": {
                "geocoding": {
                    "id": from_poi_uri,
                    "type": "stop_area",
                    "label": "Jardin (City)",
                    "name": "Jardin",
                    "city": "Paris",
                    "poi_types": [{"id": "poi_type:jardin", "name": "Jardin"}],
                    "properties": [{"key": "olympic", "value": "olympic:site"}],
                }
            },
        }
    ],
}

DEFAULT_FORBIDDEN_URIS = {
    "pt_object_olympics_forbidden_uris": ["physical_mode:0x0"],
    "poi_property_key": "olympic",
    "poi_property_value": "olympic:site",
    "min_pt_duration": 600,
}

MOCKED_INSTANCE_CONF = {
    "scenario": "distributed",
    'instance_config': {
        'default_autocomplete': 'bragi',
        "use_multi_reverse": True,
        "olympics_forbidden_uris": DEFAULT_FORBIDDEN_URIS,
    },
}
BRAGI_URL = 'https://host_of_bragi'
BASIC_PARAMS = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}

DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET = {"name": "aa", "folder": "olympic_site_params"}


class FakeOlympicSiteParamsManager(OlympicSiteParamsManager):
    def __init__(self, instance, config, default_olympic_site_paramas=DEFAULT_OLYMPIC_SITE_PARAMS):
        super(FakeOlympicSiteParamsManager, self).__init__(instance, config)
        self.test = default_olympic_site_paramas

    def fetch_and_get_data(self, instance_name, bucket_name, folder, **kwargs):
        result = deepcopy(self.test)
        self.str_datetime_time_stamp(result)
        return result


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class TestOlympicSites(AbstractTestFixture):
    def test_address_with_within_to_address_journeys(self):
        # forbidden_uris used :  physical_mode:0x0
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': s_lon,
            'lat': s_lat,
        }
        from_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': r_lon,
            'lat': r_lat,
        }
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        to_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS_WITH_WITHIN)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(
                template_journey_query.format(place_from=from_addr_uri, place_to=to_addr_uri)
            )
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 1
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 1
            assert first_journey["sections"][0]["type"] == "street_network"

    def test_address_with_invalid_within_to_address_journeys(self):
        # forbidden_uris not used :  physical_mode:0x0
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': s_lon,
            'lat': s_lat,
        }
        from_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': r_lon,
            'lat': r_lat,
        }
        to_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS_WITH_INVALID_WITHIN)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(
                template_journey_query.format(place_from=from_addr_uri, place_to=to_addr_uri)
            )
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            physical_mode_id = next(
                (
                    link["id"]
                    for link in first_journey["sections"][1]["links"]
                    if link["type"] == 'physical_mode'
                ),
                None,
            )
            assert physical_mode_id

    def test_address_to_address_journeys(self):
        # forbidden_uris not used :  physical_mode:0x0
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': s_lon,
            'lat': s_lat,
        }
        from_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': r_lon,
            'lat': r_lat,
        }
        to_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(
                template_journey_query.format(place_from=from_addr_uri, place_to=to_addr_uri)
            )
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            physical_mode_id = next(
                (
                    link["id"]
                    for link in first_journey["sections"][1]["links"]
                    if link["type"] == 'physical_mode'
                ),
                None,
            )
            assert physical_mode_id

    def test_address_to_olympic_poi_journeys(self):
        # forbidden_uris used :  physical_mode:0x0
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': s_lon,
            'lat': s_lat,
        }
        from_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(BRAGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            response = self.query_region(
                template_journey_query.format(place_from=from_addr_uri, place_to=to_poi_uri)
            )
            assert len(response['journeys']) == 1
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 1
            assert first_journey["sections"][0]["type"] == "street_network"

    def test_address_to_not_olympic_poi_journeys(self):
        # forbidden_uris not used :  physical_mode:0x0
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': s_lon,
            'lat': s_lat,
        }
        from_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(BRAGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI_NOT_OLYMPIC)
            response = self.query_region(
                template_journey_query.format(place_from=from_addr_uri, place_to=to_poi_uri)
            )
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            physical_mode_id = next(
                (
                    link["id"]
                    for link in first_journey["sections"][1]["links"]
                    if link["type"] == 'physical_mode'
                ),
                None,
            )
            assert physical_mode_id

    def test_olympic_poi_to_address_journeys(self):
        # forbidden_uris used :  physical_mode:0x0
        from_place = "{}/features/{}?{}".format(BRAGI_URL, from_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        params = {
            'reverse_timeout': 200,
            'within_timeout': 200,
            'pt_dataset[]': 'main_routing_test',
            'lon': r_lon,
            'lat': r_lat,
        }
        to_place = "{}/multi-reverse?{}".format(BRAGI_URL, urlencode(params, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_POI)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(
                template_journey_query.format(place_from=from_poi_uri, place_to=to_addr_uri)
            )
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 1
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 1
            assert first_journey["sections"][0]["type"] == "street_network"

    def test_olympic_poi_to_olympic_poi_journeys(self):
        # forbidden_uris used :  physical_mode:0x0
        from_place = "{}/features/{}?{}".format(BRAGI_URL, from_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        to_place = "{}/features/{}?{}".format(BRAGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            response = self.query_region(
                template_journey_query.format(place_from=from_poi_uri, place_to=to_poi_uri)
            )
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 1
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 1
            assert first_journey["sections"][0]["type"] == "street_network"

    def test_olympic_poi_to_olympic_poi_journeys_show_natural_opg_journeys_in_query(self):
        # Invalid forbidden_uris :  physical_mode:axb
        from_place = "{}/features/{}?{}".format(BRAGI_URL, from_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        to_place = "{}/features/{}?{}".format(BRAGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        forbidden_uris = deepcopy(DEFAULT_FORBIDDEN_URIS)
        forbidden_uris.update({"pt_object_olympics_forbidden_uris": ["physical_mode:axb"]})
        instance.olympics_forbidden_uris = parse_and_get_olympics_forbidden_uris(forbidden_uris)
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            query = template_journey_query.format(place_from=from_poi_uri, place_to=to_poi_uri)
            response = self.query_region("{}&{}".format(query, "_show_natural_opg_journeys=true"))
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            assert "to_delete" not in first_journey["tags"]

            last_journey = response['journeys'][1]
            assert len(last_journey["sections"]) == 1
            assert "to_delete" not in first_journey["tags"]

    def test_olympic_poi_to_olympic_poi_journeys_show_natural_opg_journeys_in_config(self):
        # Invalid forbidden_uris :  physical_mode:axb
        from_place = "{}/features/{}?{}".format(BRAGI_URL, from_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        to_place = "{}/features/{}?{}".format(BRAGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        forbidden_uris = deepcopy(DEFAULT_FORBIDDEN_URIS)
        forbidden_uris.update({"pt_object_olympics_forbidden_uris": ["physical_mode:axb"]})
        instance.olympics_forbidden_uris = parse_and_get_olympics_forbidden_uris(forbidden_uris)
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance,
            DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET,
            default_olympic_site_paramas=olympic_site_params_show_natural_opg_journeys,
        )

        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            query = template_journey_query.format(place_from=from_poi_uri, place_to=to_poi_uri)
            response = self.query_region(query)
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            assert "to_delete" not in first_journey["tags"]

            last_journey = response['journeys'][1]
            assert len(last_journey["sections"]) == 1
            assert "to_delete" not in first_journey["tags"]

    def test_olympic_poi_to_olympic_poi_journeys_without_show_natural_opg_journeyss(self):
        # Only direct path, mode=walking
        from_place = "{}/features/{}?{}".format(BRAGI_URL, from_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        to_place = "{}/features/{}?{}".format(BRAGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        instance = i_manager.instances["main_routing_test"]
        forbidden_uris = deepcopy(DEFAULT_FORBIDDEN_URIS)
        forbidden_uris.update({"pt_object_olympics_forbidden_uris": ["physical_mode:axb"]})
        instance.olympics_forbidden_uris = parse_and_get_olympics_forbidden_uris(forbidden_uris)
        instance.olympic_site_params_manager = FakeOlympicSiteParamsManager(
            instance, DEFAULT_OLYMPIC_SITE_PARAMS_BUCKET
        )
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            query = template_journey_query.format(place_from=from_poi_uri, place_to=to_poi_uri)
            response = self.query_region(query)
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 1
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 1
            assert "to_delete" not in first_journey["tags"]
            assert first_journey["sections"][0]["mode"] == "walking"
