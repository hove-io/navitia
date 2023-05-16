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

template_journey_query = "journeys?from={place_from}&to={place_to}&datetime=20120614T080000"

s_lon, s_lat = get_lon_lat(s_coord)
r_lon, r_lat = get_lon_lat(r_coord)

from_poi_uri = "poi:from"
to_poi_uri = "poi:to"

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
                    "properties": [
                        {"key": "olympic", "value": "olympic:site"}
                    ]
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
                    "properties": [
                        {"key": "olympic", "value": "olympic:site"}
                    ]
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
                    "properties": [
                        {"key": "abcd", "value": "olympic:site"}
                    ]
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

MOCKED_INSTANCE_CONF = {"scenario": "distributed", 'instance_config': {'default_autocomplete': 'bragi', "olympic_criteria":{"pt_object_olympic_uris": ["physical_mode:0x0"], "poi_property_key": "olympic","poi_property_value": "olympic:site"}}}
BARGI_URL = 'https://host_of_bragi'
BASIC_PARAMS = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}

@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class TestOlympicSites(AbstractTestFixture):

    def test_address_to_address_journeys(self):
        # forbidden_uris used :  physical_mode:0x0
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test', 'lon': s_lon,'lat':s_lat}
        from_place = "{}/reverse?{}".format(BARGI_URL, urlencode(params, doseq=True))
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test', 'lon': r_lon, 'lat': r_lat}
        to_place = "{}/reverse?{}".format(BARGI_URL, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(template_journey_query.format(place_from=from_addr_uri, place_to=to_addr_uri))
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 1
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 1
            physical_mode_id =  next((link["id"] for link in first_journey["sections"][0]["links"] if link["type"] == 'physical_mode'), None)
            assert not physical_mode_id

    def test_address_to_olympic_poi_journeys(self):
        # forbidden_uris not used :  physical_mode:0x0
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test', 'lon': s_lon,'lat':s_lat}
        from_place = "{}/reverse?{}".format(BARGI_URL, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(BARGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            response = self.query_region(template_journey_query.format(place_from=from_addr_uri, place_to=to_poi_uri))
            assert len(response['journeys']) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][1]["type"] == "public_transport"
            physical_mode_id =  next(link["id"] for link in first_journey["sections"][1]["links"] if link["type"] == 'physical_mode')
            assert physical_mode_id == 'physical_mode:0x0'

    def test_address_to_not_olympic_poi_journeys(self):
        # forbidden_uris used :  physical_mode:0x0
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test', 'lon': s_lon,'lat':s_lat}
        from_place = "{}/reverse?{}".format(BARGI_URL, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(BARGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI_NOT_OLYMPIC)
            response = self.query_region(template_journey_query.format(place_from=from_addr_uri, place_to=to_poi_uri))
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 1
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 1
            physical_mode_id =  next((link["id"] for link in first_journey["sections"][0]["links"] if link["type"] == 'physical_mode'), None)
            assert not physical_mode_id


    def test_olympic_poi_to_address_journeys(self):
        # forbidden_uris not used :  physical_mode:0x0
        from_place = "{}/features/{}?{}".format(BARGI_URL, from_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test', 'lon': r_lon, 'lat': r_lat}
        to_place = "{}/reverse?{}".format(BARGI_URL, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_POI)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(template_journey_query.format(place_from=from_poi_uri, place_to=to_addr_uri))
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][1]["type"] == "public_transport"
            physical_mode_id =  next(link["id"] for link in first_journey["sections"][1]["links"] if link["type"] == 'physical_mode')
            assert physical_mode_id == 'physical_mode:0x0'

    def test_olympic_poi_to_olympic_poi_journeys(self):
        # forbidden_uris not used :  physical_mode:0x0
        from_place = "{}/features/{}?{}".format(BARGI_URL, from_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        to_place = "{}/features/{}?{}".format(BARGI_URL, to_poi_uri, urlencode(BASIC_PARAMS, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            response = self.query_region(template_journey_query.format(place_from=from_poi_uri, place_to=to_poi_uri))
            journeys = [journey for journey in response['journeys']]
            assert len(journeys) == 2
            first_journey = response['journeys'][0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][1]["type"] == "public_transport"
            physical_mode_id =  next(link["id"] for link in first_journey["sections"][1]["links"] if link["type"] == 'physical_mode')
            assert physical_mode_id == 'physical_mode:0x0'
