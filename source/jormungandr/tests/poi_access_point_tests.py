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

template_journey_query_wap = "journeys?from={place_from}&to={place_to}&datetime=20120614T080000"
template_journey_query = template_journey_query_wap + "&_poi_access_points=true"

s_lon, s_lat = get_lon_lat(s_coord)
r_lon, r_lat = get_lon_lat(r_coord)

from_poi_uri = "poi:from"
to_poi_uri = "poi:to"

from_addr_uri = "addr:{}".format(s_coord)
to_addr_uri = "addr:{}".format(r_coord)

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
                    "children": [
                        {
                            "type": "poi",
                            "id": "poi:from:porte1",
                            "label": "Jardin: Porte 1 (City)",
                            "name": "Jardin: Porte 1",
                            "coord": {"lon": s_lon + 0.0000000005, "lat": s_lat + 0.0000000005},
                            "poi_type": {"id": "poi_type:access_point", "name": "Point d'accès"},
                        },
                        {
                            "type": "poi",
                            "id": "poi:from:porte2",
                            "label": "Jardin: Porte 2 (City)",
                            "name": "Jardin: Porte 2",
                            "coord": {"lon": s_lon + 0.0000000008, "lat": s_lat + 0.0000000008},
                            "poi_type": {"id": "poi_type:access_point", "name": "Point d'accès"},
                        },
                    ],
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
                    "children": [
                        {
                            "type": "poi",
                            "id": "poi:to:porte3",
                            "label": "Jardin: Porte 3 (City)",
                            "name": "Jardin: Porte 3",
                            "coord": {"lon": r_lon + 0.0000000005, "lat": r_lat + 0.0000000005},
                            "poi_type": {"id": "poi_type:access_point", "name": "Point d'accès"},
                        },
                        {
                            "type": "poi",
                            "id": "poi:to:porte4",
                            "label": "Jardin: Porte 4 (City)",
                            "name": "Jardin: Porte 4",
                            "coord": {"lon": r_lon + 0.0000000008, "lat": r_lat + 0.0000000008},
                            "poi_type": {"id": "poi_type:access_point", "name": "Point d'accès"},
                        },
                    ],
                }
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

MOCKED_INSTANCE_CONF = {"scenario": "distributed", 'instance_config': {'default_autocomplete': 'bragi'}}


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF}, global_config={'activate_bragi': True})
class TestJourneysDistributedPoiAccessPoint(AbstractTestFixture):
    def test_from_poi_to_address(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        from_place = "{}/features/{}?{}".format(url, from_poi_uri, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(url, to_addr_uri, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_POI)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(
                template_journey_query.format(place_from=from_poi_uri, place_to=to_addr_uri), display=True
            )
            check_best(response)
            journeys = response["journeys"]
            assert len(journeys) == 2
            first_journey = journeys[0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][0]["mode"] == "walking"
            assert first_journey["sections"][0]["type"] == "street_network"
            assert first_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert first_journey["sections"][0]["from"]["name"] == 'Jardin (City)'
            assert len(first_journey["sections"][0]["vias"]) == 1
            assert first_journey["sections"][0]["vias"][0]["id"] == 'poi:from:porte1'
            assert first_journey["sections"][0]["vias"][0]["name"] == 'Jardin: Porte 1'
            assert first_journey["sections"][0]["vias"][0]["access_point"]["embedded_type"] == 'poi_access_point'
            path = first_journey["sections"][0]["path"]
            assert len(path) == 3
            assert path[0]["length"] == 0
            assert path[0]["name"] == "Jardin: Porte 1"
            assert path[0]["instruction"] == "Exit Jardin (City) via Jardin: Porte 1."
            assert path[0]["via_uri"] == 'poi:from:porte1'

            assert first_journey["sections"][2]["mode"] == "walking"
            assert first_journey["sections"][2]["type"] == "street_network"
            assert first_journey["sections"][2]["to"]["id"] == "{};{}".format(r_lon, r_lat)
            assert first_journey["sections"][2]["to"]["name"] == '10 Rue Victor (City)'
            assert first_journey["sections"][2]["to"]["embedded_type"] == 'address'

            last_journey = journeys[1]
            assert len(last_journey["sections"]) == 1
            assert last_journey["sections"][0]["mode"] == "walking"
            assert last_journey["sections"][0]["type"] == "street_network"
            assert last_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert last_journey["sections"][0]["to"]["id"] == "{};{}".format(r_lon, r_lat)
            assert "vias" not in last_journey["sections"][0]

    def test_from_poi_to_address_with_pt_access_point(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        from_place = "{}/features/{}?{}".format(url, from_poi_uri, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(url, to_addr_uri, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_POI)
            m.get(to_place, json=TO_ADDRESS)
            response = self.query_region(
                template_journey_query.format(place_from=from_poi_uri, place_to=to_addr_uri)
                + "&_access_points=true",
                display=True,
            )
            check_best(response)
            journeys = response["journeys"]
            assert len(journeys) == 2
            first_journey = journeys[0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][0]["mode"] == "walking"
            assert first_journey["sections"][0]["type"] == "street_network"
            assert first_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert first_journey["sections"][0]["from"]["name"] == 'Jardin (City)'
            assert len(first_journey["sections"][0]["vias"]) == 2
            assert first_journey["sections"][0]["vias"][0]["id"] == 'poi:from:porte1'
            assert first_journey["sections"][0]["vias"][0]["name"] == 'Jardin: Porte 1'
            assert first_journey["sections"][0]["vias"][0]["access_point"]["embedded_type"] == 'poi_access_point'
            assert first_journey["sections"][0]["vias"][1]["id"] == 'access_point:B1'
            assert first_journey["sections"][0]["vias"][1]["name"] == 'access_point:B1'
            assert first_journey["sections"][0]["vias"][1]["access_point"]["embedded_type"] == 'pt_access_point'

            path = first_journey["sections"][0]["path"]
            first_path = path[0]
            last_path = path[-1]
            assert first_path["length"] == 0
            assert first_path["name"] == "Jardin: Porte 1"
            assert first_path["instruction"] == "Exit Jardin (City) via Jardin: Porte 1."
            assert first_path["via_uri"] == 'poi:from:porte1'
            assert last_path["length"] == 1
            assert last_path["name"] == 'access_point:B1'
            assert last_path["instruction"] == 'Then Enter stop_point:stopB (Condom) via access_point:B1.'
            assert last_path["via_uri"] == 'access_point:B1'

            assert first_journey["sections"][2]["mode"] == "walking"
            assert first_journey["sections"][2]["type"] == "street_network"
            assert first_journey["sections"][2]["to"]["id"] == "{};{}".format(r_lon, r_lat)
            assert first_journey["sections"][2]["to"]["name"] == '10 Rue Victor (City)'
            assert first_journey["sections"][2]["to"]["embedded_type"] == 'address'

            last_journey = journeys[1]
            assert len(last_journey["sections"]) == 1
            assert last_journey["sections"][0]["mode"] == "walking"
            assert last_journey["sections"][0]["type"] == "street_network"
            assert last_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert last_journey["sections"][0]["to"]["id"] == "{};{}".format(r_lon, r_lat)
            assert "vias" not in last_journey["sections"][0]

    def test_from_address_to_poi(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        from_place = "{}/features/{}?{}".format(url, from_addr_uri, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(url, to_poi_uri, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            response = self.query_region(
                template_journey_query.format(place_from=from_addr_uri, place_to=to_poi_uri), display=True
            )
            check_best(response)
            journeys = response["journeys"]
            assert len(journeys) == 2
            first_journey = journeys[0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][0]["mode"] == "walking"
            assert first_journey["sections"][0]["type"] == "street_network"
            assert first_journey["sections"][0]["from"]["id"] == "{};{}".format(s_lon, s_lat)
            assert first_journey["sections"][0]["from"]["embedded_type"] == 'address'
            assert first_journey["sections"][0]["from"]["name"] == '10 Rue bob (City)'

            assert first_journey["sections"][2]["mode"] == "walking"
            assert first_journey["sections"][2]["type"] == "street_network"
            assert first_journey["sections"][2]["to"]["id"] == to_poi_uri
            assert first_journey["sections"][2]["to"]["embedded_type"] == 'poi'
            assert first_journey["sections"][2]["to"]["name"] == 'Jardin (City)'
            assert len(first_journey["sections"][2]["vias"]) == 1
            assert first_journey["sections"][2]["vias"][0]["id"] == 'poi:to:porte3'
            assert first_journey["sections"][2]["vias"][0]["name"] == 'Jardin: Porte 3'
            assert first_journey["sections"][2]["vias"][0]["access_point"]["embedded_type"] == 'poi_access_point'
            path = first_journey["sections"][2]["path"]
            assert len(path) == 3
            assert path[2]["length"] == 0
            assert path[2]["name"] == "Jardin: Porte 3"
            assert path[2]["instruction"] == "Then Enter Jardin (City) via Jardin: Porte 3."
            assert path[2]["via_uri"] == 'poi:to:porte3'

            last_journey = journeys[1]
            assert len(last_journey["sections"]) == 1
            assert last_journey["sections"][0]["mode"] == "walking"
            assert last_journey["sections"][0]["type"] == "street_network"
            assert last_journey["sections"][0]["from"]["id"] == "{};{}".format(s_lon, s_lat)
            assert last_journey["sections"][0]["to"]["id"] == to_poi_uri
            assert "vias" not in last_journey["sections"][0]

    def test_from_address_to_poi_with_pt_access_point(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        from_place = "{}/features/{}?{}".format(url, from_addr_uri, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(url, to_poi_uri, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_ADDRESS)
            m.get(to_place, json=TO_POI)
            response = self.query_region(
                template_journey_query.format(place_from=from_addr_uri, place_to=to_poi_uri)
                + "&_access_points=true",
                display=True,
            )
            check_best(response)
            journeys = response["journeys"]
            assert len(journeys) == 2
            first_journey = journeys[0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][0]["mode"] == "walking"
            assert first_journey["sections"][0]["type"] == "street_network"
            assert first_journey["sections"][0]["from"]["id"] == "{};{}".format(s_lon, s_lat)
            assert first_journey["sections"][0]["from"]["embedded_type"] == 'address'
            assert first_journey["sections"][0]["from"]["name"] == '10 Rue bob (City)'
            assert len(first_journey["sections"][0]["vias"]) == 1
            assert first_journey["sections"][0]["vias"][0]["id"] == 'access_point:B1'
            assert first_journey["sections"][0]["vias"][0]["name"] == 'access_point:B1'
            assert first_journey["sections"][0]["vias"][0]["access_point"]["embedded_type"] == 'pt_access_point'

            assert first_journey["sections"][2]["mode"] == "walking"
            assert first_journey["sections"][2]["type"] == "street_network"
            assert first_journey["sections"][2]["to"]["id"] == to_poi_uri
            assert first_journey["sections"][2]["to"]["embedded_type"] == 'poi'
            assert first_journey["sections"][2]["to"]["name"] == 'Jardin (City)'
            assert len(first_journey["sections"][2]["vias"]) == 2

            vias = first_journey["sections"][2]["vias"]
            assert vias[0]["id"] == 'poi:to:porte3'
            assert vias[0]["name"] == 'Jardin: Porte 3'
            assert vias[0]["access_point"]["embedded_type"] == 'poi_access_point'
            assert vias[1]["id"] == 'access_point:A2'
            assert vias[1]["name"] == 'access_point:A2'
            assert vias[1]["access_point"]["embedded_type"] == 'pt_access_point'

            path = first_journey["sections"][2]["path"]
            first_path = path[0]
            assert first_path["length"] == 3
            assert first_path["name"] == 'access_point:A2'
            assert first_path["instruction"] == 'Exit stop_point:stopA (Condom) via access_point:A2.'
            assert first_path["via_uri"] == 'access_point:A2'

            last_path = path[-1]
            assert last_path["length"] == 0
            assert last_path["name"] == "Jardin: Porte 3"
            assert last_path["instruction"] == "Then Enter Jardin (City) via Jardin: Porte 3."
            assert last_path["via_uri"] == 'poi:to:porte3'

            last_journey = journeys[1]
            assert len(last_journey["sections"]) == 1
            assert last_journey["sections"][0]["mode"] == "walking"
            assert last_journey["sections"][0]["type"] == "street_network"
            assert last_journey["sections"][0]["from"]["id"] == "{};{}".format(s_lon, s_lat)
            assert last_journey["sections"][0]["to"]["id"] == to_poi_uri
            assert "vias" not in last_journey["sections"][0]

    def test_from_poi_to_poi(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        from_place = "{}/features/{}?{}".format(url, from_poi_uri, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(url, to_poi_uri, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_POI)
            m.get(to_place, json=TO_POI)
            response = self.query_region(
                template_journey_query.format(place_from=from_poi_uri, place_to=to_poi_uri), display=True
            )
            check_best(response)
            journeys = response["journeys"]
            assert len(journeys) == 2
            first_journey = journeys[0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][0]["mode"] == "walking"
            assert first_journey["sections"][0]["type"] == "street_network"
            assert first_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert first_journey["sections"][0]["from"]["name"] == 'Jardin (City)'
            assert len(first_journey["sections"][0]["vias"]) == 1
            assert first_journey["sections"][0]["vias"][0]["id"] == 'poi:from:porte1'
            assert first_journey["sections"][0]["vias"][0]["name"] == 'Jardin: Porte 1'
            assert first_journey["sections"][0]["vias"][0]["access_point"]["embedded_type"] == 'poi_access_point'
            path = first_journey["sections"][0]["path"]
            assert len(path) == 3
            assert path[0]["length"] == 0
            assert path[0]["name"] == "Jardin: Porte 1"
            assert path[0]["instruction"] == "Exit Jardin (City) via Jardin: Porte 1."
            assert path[0]["via_uri"] == 'poi:from:porte1'

            assert first_journey["sections"][2]["mode"] == "walking"
            assert first_journey["sections"][2]["type"] == "street_network"
            assert first_journey["sections"][2]["to"]["id"] == to_poi_uri
            assert first_journey["sections"][2]["to"]["embedded_type"] == 'poi'
            assert first_journey["sections"][2]["to"]["name"] == 'Jardin (City)'
            assert len(first_journey["sections"][2]["vias"]) == 1
            assert first_journey["sections"][2]["vias"][0]["id"] == 'poi:to:porte3'
            assert first_journey["sections"][2]["vias"][0]["name"] == 'Jardin: Porte 3'
            assert first_journey["sections"][2]["vias"][0]["access_point"]["embedded_type"] == 'poi_access_point'
            path = first_journey["sections"][2]["path"]
            assert len(path) == 3
            assert path[2]["length"] == 0
            assert path[2]["name"] == "Jardin: Porte 3"
            assert path[2]["instruction"] == "Then Enter Jardin (City) via Jardin: Porte 3."
            assert path[2]["via_uri"] == 'poi:to:porte3'

            last_journey = journeys[1]
            assert len(last_journey["sections"]) == 1
            assert last_journey["sections"][0]["mode"] == "walking"
            assert last_journey["sections"][0]["type"] == "street_network"
            assert last_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert last_journey["sections"][0]["to"]["id"] == to_poi_uri
            assert "vias" not in last_journey["sections"][0]

    def test_from_poi_to_poi_with_pt_access_point(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        from_place = "{}/features/{}?{}".format(url, from_poi_uri, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(url, to_poi_uri, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_POI)
            m.get(to_place, json=TO_POI)
            response = self.query_region(
                template_journey_query.format(place_from=from_poi_uri, place_to=to_poi_uri)
                + "&_access_points=true",
                display=True,
            )
            check_best(response)
            journeys = response["journeys"]
            assert len(journeys) == 2
            first_journey = journeys[0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][0]["mode"] == "walking"
            assert first_journey["sections"][0]["type"] == "street_network"
            assert first_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert first_journey["sections"][0]["from"]["name"] == 'Jardin (City)'

            vias = first_journey["sections"][0]["vias"]
            assert len(vias) == 2
            assert vias[0]["id"] == 'poi:from:porte1'
            assert vias[0]["name"] == 'Jardin: Porte 1'
            assert vias[0]["access_point"]["embedded_type"] == 'poi_access_point'
            assert vias[1]["id"] == 'access_point:B1'
            assert vias[1]["name"] == 'access_point:B1'
            assert vias[1]["access_point"]["embedded_type"] == 'pt_access_point'

            path = first_journey["sections"][0]["path"]
            first_path = path[0]
            last_path = path[-1]
            assert first_path["length"] == 0
            assert first_path["name"] == "Jardin: Porte 1"
            assert first_path["instruction"] == "Exit Jardin (City) via Jardin: Porte 1."
            assert first_path["via_uri"] == 'poi:from:porte1'
            assert last_path["length"] == 1
            assert last_path["name"] == 'access_point:B1'
            assert last_path["instruction"] == 'Then Enter stop_point:stopB (Condom) via access_point:B1.'
            assert last_path["via_uri"] == 'access_point:B1'

            assert first_journey["sections"][2]["mode"] == "walking"
            assert first_journey["sections"][2]["type"] == "street_network"
            assert first_journey["sections"][2]["to"]["id"] == to_poi_uri
            assert first_journey["sections"][2]["to"]["embedded_type"] == 'poi'
            assert first_journey["sections"][2]["to"]["name"] == 'Jardin (City)'

            vias = first_journey["sections"][2]["vias"]
            assert len(vias) == 2
            assert vias[0]["id"] == 'poi:to:porte3'
            assert vias[0]["name"] == 'Jardin: Porte 3'
            assert vias[0]["access_point"]["embedded_type"] == 'poi_access_point'
            assert vias[1]["id"] == 'access_point:A2'
            assert vias[1]["name"] == 'access_point:A2'
            assert vias[1]["access_point"]["embedded_type"] == 'pt_access_point'

            path = first_journey["sections"][2]["path"]
            first_path = path[0]
            last_path = path[-1]
            assert first_path["length"] == 3
            assert first_path["name"] == 'access_point:A2'
            assert first_path["instruction"] == 'Exit stop_point:stopA (Condom) via access_point:A2.'
            assert first_path["via_uri"] == 'access_point:A2'
            assert last_path["length"] == 0
            assert last_path["name"] == "Jardin: Porte 3"
            assert last_path["instruction"] == "Then Enter Jardin (City) via Jardin: Porte 3."
            assert last_path["via_uri"] == 'poi:to:porte3'

            last_journey = journeys[1]
            assert len(last_journey["sections"]) == 1
            assert last_journey["sections"][0]["mode"] == "walking"
            assert last_journey["sections"][0]["type"] == "street_network"
            assert last_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert last_journey["sections"][0]["to"]["id"] == to_poi_uri
            assert "vias" not in last_journey["sections"][0]

    def test_from_poi_to_poi_without_poi_access_points(self):
        url = 'https://host_of_bragi'
        params = {'timeout': 200, 'pt_dataset[]': 'main_routing_test'}
        from_place = "{}/features/{}?{}".format(url, from_poi_uri, urlencode(params, doseq=True))
        to_place = "{}/features/{}?{}".format(url, to_poi_uri, urlencode(params, doseq=True))
        with requests_mock.Mocker() as m:
            m.get(from_place, json=FROM_POI)
            m.get(to_place, json=TO_POI)
            response = self.query_region(
                template_journey_query_wap.format(place_from=from_poi_uri, place_to=to_poi_uri),
                display=True,
            )
            check_best(response)
            journeys = response["journeys"]
            assert len(journeys) == 2
            first_journey = journeys[0]
            assert len(first_journey["sections"]) == 3
            assert first_journey["sections"][0]["mode"] == "walking"
            assert first_journey["sections"][0]["type"] == "street_network"
            assert first_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert first_journey["sections"][0]["from"]["name"] == 'Jardin (City)'
            assert "vias" not in first_journey["sections"][0]

            last_journey = journeys[1]
            assert len(last_journey["sections"]) == 1
            assert last_journey["sections"][0]["mode"] == "walking"
            assert last_journey["sections"][0]["type"] == "street_network"
            assert last_journey["sections"][0]["from"]["id"] == from_poi_uri
            assert last_journey["sections"][0]["to"]["id"] == to_poi_uri
            assert "vias" not in last_journey["sections"][0]
