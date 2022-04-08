# coding=utf-8

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

from __future__ import absolute_import, unicode_literals
import mock
from jormungandr.autocomplete.geocodejson import GeocodeJson
from jormungandr.interfaces.v1.serializer.geocode_json import GeocodePlacesSerializer
from jormungandr.interfaces.v1.decorators import get_serializer
import requests_mock
from jormungandr import app


def bragi_house_jaures_feature():
    house_feature = {
        "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
        "properties": {
            "geocoding": {
                "city": "Saint-Quentin",
                "housenumber": "20",
                "id": "3.282103;49.847586",
                "label": "20 Rue Jean Jaures (Saint-Quentin)",
                "name": "Rue Jean Jaures",
                "postcode": "02100",
                "street": "Rue Jean Jaures",
                "type": "house",
                "administrative_regions": [
                    {
                        "id": "admin:fr:02000",
                        "insee": "02000",
                        "level": 8,
                        "label": "Saint-Quentin (02000)",
                        "name": "Saint-Quentin",
                        "zip_code": "02000",
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie (80200)",
                        "name": "Haute Picardie",
                        "zip_code": "80200",
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                ],
            }
        },
        "type": "Feature",
    }
    return house_feature


@get_serializer(serpy=GeocodePlacesSerializer)
def get_response(bragi_response):
    return bragi_response


def context_response_check(response):
    assert "context" in response
    context = response["context"]
    assert "timezone" in context
    assert "current_datetime" in context


def bragi_house_jaures_response_check(feature_response):
    assert feature_response.get('embedded_type') == "address"
    assert feature_response.get('id') == "3.282103;49.847586"
    assert feature_response.get('name') == "20 Rue Jean Jaures (Saint-Quentin)"
    address = feature_response.get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
    assert address.get('label') == "20 Rue Jean Jaures (Saint-Quentin)"
    assert address.get('house_number') == 20
    assert address.get('coord', {}).get('lat') == "49.847586"
    assert address.get('coord', {}).get('lon') == "3.282103"
    assert len(address.get('administrative_regions')) == 2
    region_list = {region['level']: region['name'] for region in address.get('administrative_regions')}
    assert region_list.get(7) == "Haute Picardie"
    assert region_list.get(8) == "Saint-Quentin"


def bragi_house_reading_test():
    bragi_response = {"features": [bragi_house_jaures_feature()]}
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        assert len(response['warnings']) == 1
        assert response['warnings'][0]['id'] == 'beta_endpoint'
        navitia_response = response.get('places', {})
        bragi_house_jaures_response_check(navitia_response[0])


def bragi_street_feature():
    street_feature = {
        "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
        "properties": {
            "geocoding": {
                "city": "Saint-Quentin",
                "id": "38059376",
                "label": "Rue Jean Jaures (Saint-Quentin)",
                "name": "Rue Jean Jaures",
                "postcode": "02100",
                "street": "Rue Jean Jaures",
                "type": "street",
                "administrative_regions": [
                    {
                        "id": "admin:fr:02000",
                        "insee": "02000",
                        "level": 8,
                        "label": "Saint-Quentin (02000)",
                        "name": "Saint-Quentin",
                        "zip_code": "02000",
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie (80200)",
                        "name": "Haute Picardie",
                        "zip_code": "80200",
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                ],
            }
        },
        "type": "Feature",
    }
    return street_feature


def bragi_street_response_check(feature_response):

    assert feature_response.get('embedded_type') == "address"
    assert feature_response.get('id') == "3.282103;49.847586"
    assert feature_response.get('name') == "Rue Jean Jaures (Saint-Quentin)"
    address = feature_response.get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
    assert address.get('label') == "Rue Jean Jaures (Saint-Quentin)"
    assert address.get('house_number') == 0
    assert address.get('coord', {}).get('lat') == "49.847586"
    assert address.get('coord', {}).get('lon') == "3.282103"
    assert len(address.get('administrative_regions')) == 2
    response = next(region for region in address.get('administrative_regions'))
    assert response.get('level') == 8
    assert response.get('name') == "Saint-Quentin"


def bragi_street_reading_test():
    bragi_response = {"features": [bragi_street_feature()]}
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})
        bragi_street_response_check(navitia_response[0])


def bragi_street_reading_without_autocomplete_attribute_test():
    bragi_response = {"features": [bragi_street_feature()]}
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})
        bragi_street_response_check(navitia_response[0])


def bragi_admin_feature():
    admin_feature = {
        "geometry": {"coordinates": [2, 48], "type": "Point"},
        "properties": {
            "geocoding": {
                "citycode": "2725",
                "level": 8,
                "city": None,
                "housenumber": "",
                "id": "admin:fr:2725",
                "label": "Sommeron (02260)",
                "name": "Sommeron",
                "postcode": "02260",
                "street": "",
                "type": "city",
                "administrative_regions": [
                    {
                        "id": "admin:fr:02000",
                        "insee": "02000",
                        "level": 8,
                        "label": "Sommeron (02000)",
                        "name": "Sommeron",
                        "zip_code": "02000",
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie (80200)",
                        "name": "Haute Picardie",
                        "zip_code": "80200",
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                ],
            }
        },
        "type": "Feature",
    }
    return admin_feature


def bragi_admin_response_check(feature_response):
    assert feature_response.get('embedded_type') == "administrative_region"
    assert feature_response.get('id') == "admin:fr:2725"
    assert feature_response.get('name') == "Sommeron"
    admin = feature_response.get('administrative_region')
    assert admin.get('level') == 8
    assert admin.get('name') == "Sommeron"
    assert admin.get('label') == "Sommeron (02260)"
    assert admin.get('zip_code') == "02260"
    assert admin.get('insee') == "2725"
    assert admin.get('coord').get('lat') == "48"
    assert admin.get('coord').get('lon') == "2"
    assert len(admin.get('administrative_regions')) == 2
    assert admin.get('administrative_regions')[0].get('level') == 8
    assert admin.get('administrative_regions')[0].get('name') == "Sommeron"


def bragi_admin_reading_test():
    bragi_response = {"features": [bragi_admin_feature()]}
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})
        bragi_admin_response_check(navitia_response[0])


def bragi_house_lefebvre_feature():
    house_feature = {
        "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
        "properties": {
            "geocoding": {
                "city": "Saint-Quentin",
                "housenumber": "42bis",
                "id": "49.847586;3.282103",
                "label": "42bis Rue Jean Lefebvre (Oyonnax)",
                "name": "Rue Jean Lefebvre",
                "postcode": "02100",
                "street": "Rue Jean Lefebvre",
                "type": "house",
                "administrative_regions": [
                    {
                        "id": "admin:fr:02000",
                        "insee": "02000",
                        "level": 8,
                        "label": "Oyonnax (02000)",
                        "name": "Oyonnax",
                        "zip_codes": ["02000"],
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie (80200)",
                        "name": "Haute Picardie",
                        "zip_codes": ["80200"],
                        "weight": 1,
                        "coord": {"lat": 48.8396154, "lon": 2.3957517},
                    },
                ],
            }
        },
        "type": "Feature",
    }
    return house_feature


def bragi_house_lefebvre_response_check(feature_response):
    assert feature_response.get('embedded_type') == "address"
    assert feature_response.get('id') == "3.282103;49.847586"
    assert feature_response.get('name') == "42bis Rue Jean Lefebvre (Oyonnax)"
    address = feature_response.get('address', {})
    assert address.get('id') == "3.282103;49.847586"
    assert address.get('name') == "Rue Jean Lefebvre"
    assert address.get('label') == "42bis Rue Jean Lefebvre (Oyonnax)"
    assert address.get('house_number') == 42
    assert address.get('coord', {}).get('lat') == "49.847586"
    assert address.get('coord', {}).get('lon') == "3.282103"
    assert len(address.get('administrative_regions')) == 2
    region_list = {region['level']: region['name'] for region in address.get('administrative_regions')}
    assert region_list.get(7) == "Haute Picardie"
    assert region_list.get(8) == "Oyonnax"


def bragi_good_geocodejson_response_test():
    bragi_response = {
        "features": [
            bragi_house_jaures_feature(),
            bragi_house_lefebvre_feature(),
            bragi_street_feature(),
            bragi_admin_feature(),
        ]
    }
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})
        assert len(navitia_response) == 4
        bragi_house_jaures_response_check(navitia_response[0])
        bragi_house_lefebvre_response_check(navitia_response[1])
        bragi_street_response_check(navitia_response[2])
        bragi_admin_response_check(navitia_response[3])


def bragi_good_geocodejson_response_without_autocomplete_attribute_test():
    bragi_response = {
        "features": [
            bragi_house_jaures_feature(),
            bragi_house_lefebvre_feature(),
            bragi_street_feature(),
            bragi_admin_feature(),
        ]
    }
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})
        assert len(navitia_response) == 4
        bragi_house_jaures_response_check(navitia_response[0])
        bragi_house_lefebvre_response_check(navitia_response[1])
        bragi_street_response_check(navitia_response[2])
        bragi_admin_response_check(navitia_response[3])


def bragi_geocodejson_spec_feature():
    return {
        "geometry": {"coordinates": [3.282103, 49.847586], "type": "Point"},
        "properties": {
            "geocoding": {
                "city": "Saint-Quentin",
                "housenumber": "20",
                "id": "addr:49.847586;3.282103",
                "label": "20 Rue Jean Jaures (Saint-Quentin)",
                "name": "Rue Jean Jaures",
                "postcode": "02100",
                "street": "Rue Jean Jaures",
                "type": "house",
                "admin": {
                    "level2": "France",
                    "level4": "Nord-Pas-de-Calais-Picardie",
                    "level6": "Aisne",
                    "level7": "Saint-Quentin",
                    "level8": "Saint-Quentin",
                },
            }
        },
        "type": "Feature",
    }


def bragi_geocodejson_compatibility_test():
    bragi_response = {"features": [bragi_geocodejson_spec_feature()]}
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})[0]
        assert navitia_response.get('embedded_type') == "address"
        assert navitia_response.get('id') == '3.282103;49.847586'
        assert navitia_response.get('name') == '20 Rue Jean Jaures (Saint-Quentin)'
        address = navitia_response.get('address', {})
        assert len(address.get('administrative_regions')) == 5
        region_list = {region['level']: region['name'] for region in address.get('administrative_regions')}
        assert region_list.get(2) == 'France'
        assert region_list.get(4) == 'Nord-Pas-de-Calais-Picardie'
        assert region_list.get(7) == 'Saint-Quentin'


def bragi_poi_feature():
    return {
        "geometry": {"coordinates": [8.9028068, 42.599235500000009], "type": "Point"},
        "properties": {
            "geocoding": {
                "id": "poi:osm:3224270910",
                "type": "poi",
                "poi_types": [{"id": "poi_type:amenity:townhall", "name": "Mairie"}],
                "label": "Mairie de Pigna (Pigna)",
                "name": "Mairie de Pigna",
                "housenumber": None,
                "street": None,
                "postcode": None,
                "city": "Pigna",
                "administrative_regions": [
                    {
                        "id": "admin:fr:2B231",
                        "insee": "2B231",
                        "level": 8,
                        "label": "Pigna (20220)",
                        "name": "Pigna",
                        "zip_codes": ["20220", "20221", "20222"],
                        "weight": 0,
                        "coord": {"lat": 42.5996043, "lon": 8.9027334},
                    }
                ],
            }
        },
        "type": "Feature",
    }


def bragi_poi_reading_test():
    bragi_response = {"features": [bragi_poi_feature()]}
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})[0]
        assert navitia_response.get('embedded_type') == "poi"
        assert navitia_response.get('id') == 'poi:osm:3224270910'
        assert navitia_response.get('name') == 'Mairie de Pigna (Pigna)'
        assert navitia_response.get('quality') == 0
        poi = navitia_response.get('poi', {})
        assert poi.get('label') == 'Mairie de Pigna (Pigna)'
        assert poi.get('name') == 'Mairie de Pigna'
        assert poi.get('id') == 'poi:osm:3224270910'
        assert poi.get('poi_type').get('id') == 'poi_type:amenity:townhall'
        assert poi.get('poi_type').get('name') == 'Mairie'
        assert poi.get('coord').get('lat') == str(42.599235500000009)
        assert poi.get('coord').get('lon') == "8.9028068"
        assert len(poi.get('administrative_regions')) == 1
        administrative_region = poi.get('administrative_regions')[0]
        assert administrative_region.get('insee') == '2B231'
        assert administrative_region.get('name') == 'Pigna'
        assert administrative_region.get('label') == 'Pigna (20220)'
        assert administrative_region.get('level') == 8
        assert administrative_region.get('id') == 'admin:fr:2B231'
        assert administrative_region.get('coord').get('lat') == "42.5996043"
        assert administrative_region.get('coord').get('lon') == "8.9027334"
        assert administrative_region.get('zip_code') == "20220-20222"


def bragi_stop_area_feature():
    return {
        "geometry": {"coordinates": [2.389462, 48.87958], "type": "Point"},
        "properties": {
            "geocoding": {
                "administrative_regions": [],
                "city": None,
                "id": "stop_area:OIF:SA:59332",
                "label": "BOTZARIS (Paris)",
                "name": "BOTZARIS",
                "postcode": None,
                "type": "public_transport:stop_area",
                "timezone": "Europe/Paris",
            }
        },
        "type": "Feature",
    }


def bragi_stop_area_reading_test():
    bragi_response = {"features": [bragi_stop_area_feature()]}
    with app.test_request_context():
        response = get_response(bragi_response)
        context_response_check(response)
        navitia_response = response.get('places', {})[0]
        assert navitia_response.get('embedded_type') == "stop_area"
        assert navitia_response.get('id') == 'stop_area:OIF:SA:59332'
        assert navitia_response.get('name') == 'BOTZARIS (Paris)'
        assert navitia_response.get('quality') == 0
        sa = navitia_response.get('stop_area', {})
        assert sa.get('label') == 'BOTZARIS (Paris)'
        assert sa.get('name') == 'BOTZARIS'
        assert sa.get('id') == 'stop_area:OIF:SA:59332'
        assert sa.get('timezone') == 'Europe/Paris'
        assert sa.get('coord').get('lat') == "48.87958"
        assert sa.get('coord').get('lon') == "2.389462"
        assert 'administrative_regions' not in sa


def geojson():
    return {
        "type": "Feature",
        "geometry": {"type": "Point", "coordinates": [102.0, 0.5]},
        "properties": {"prop0": "value0"},
    }


def bragi_call_test():
    """
    test the whole autocomplete with a geocodejson service
    """
    bragi = GeocodeJson(host='http://bob.com')

    bragi_response = {
        "features": [
            bragi_house_jaures_feature(),
            bragi_house_lefebvre_feature(),
            bragi_street_feature(),
            bragi_admin_feature(),
        ]
    }

    with app.test_request_context():
        # we mock the http call to return the hard coded mock_response
        with requests_mock.Mocker() as m:
            m.get(
                'http://bob.com/autocomplete?limit=10&q=rue+bobette&timeout=2000&request_id=123',
                json=bragi_response,
            )
            raw_response = bragi.get({'q': 'rue bobette', 'count': 10, 'request_id': '123'}, instances=[])
            places = raw_response.get('places')
            assert len(places) == 4
            context_response_check(raw_response)
            bragi_house_jaures_response_check(places[0])
            bragi_house_lefebvre_response_check(places[1])
            bragi_street_response_check(places[2])
            bragi_admin_response_check(places[3])
            assert m.called

        with requests_mock.Mocker() as m:
            m.post(
                'http://bob.com/autocomplete?limit=10&q=rue+bobette&timeout=2000&request_id=123',
                json=bragi_response,
            )
            raw_response = bragi.get(
                {'q': 'rue bobette', 'count': 10, "request_id": "123", 'shape': geojson()}, instances=[]
            )
            context_response_check(raw_response)
            places = raw_response.get('places')
            assert len(places) == 4
            bragi_house_jaures_response_check(places[0])
            bragi_house_lefebvre_response_check(places[1])
            bragi_street_response_check(places[2])
            bragi_admin_response_check(places[3])
            assert m.called


def bragi_make_params_with_instance_test():
    """
    test of generate params with instance
    """
    instance = mock.MagicMock()
    instance.name = 'bib'
    instance.poi_dataset = None
    bragi = GeocodeJson(host='http://bob.com/autocomplete', timeout=1)

    request = {"q": "aa", "count": 20, "request_id": "1234"}

    params = bragi.make_params(request=request, instances=[instance])
    rsp = [
        ('q', 'aa'),
        ('limit', 20),
        ('pt_dataset[]', 'bib'),
        ('timeout', 1000),
        ("request_id", "1234"),
        ('shape_scope[]', 'addr'),
        ('shape_scope[]', 'admin'),
        ('shape_scope[]', 'poi'),
        ('shape_scope[]', 'street'),
    ]
    params.sort()
    rsp.sort()
    assert rsp == params


def bragi_make_params_with_places_proximity_radius_test():
    """
    test of generate params with instance & places_proximity_radius
    """
    instance = mock.MagicMock()
    instance.name = 'bib'
    instance.poi_dataset = None
    bragi = GeocodeJson(host='http://bob.com/autocomplete', timeout=1)

    request = {"q": "aa", "count": 20, "request_id": "1234", "places_proximity_radius": 5000}

    params = bragi.make_params(request=request, instances=[instance])
    rsp = [
        ('q', 'aa'),
        ('limit', 20),
        ('pt_dataset[]', 'bib'),
        ('timeout', 1000),
        ("request_id", "1234"),
        ('shape_scope[]', 'addr'),
        ('shape_scope[]', 'admin'),
        ('shape_scope[]', 'poi'),
        ('shape_scope[]', 'street'),
        ('proximity_scale', 32.5),
        ('proximity_offset', 5.0),
        ('proximity_decay', 0.4),
    ]
    params.sort()
    rsp.sort()
    assert rsp == params


def bragi_make_params_with_multiple_instances_test():
    """
    test of generate params with instance
    """
    instance1 = mock.MagicMock()
    instance1.name = 'bib'
    instance1.poi_dataset = None
    instance2 = mock.MagicMock()
    instance2.name = 'bob'
    instance2.poi_dataset = None
    bragi = GeocodeJson(host='http://bob.com/autocomplete', timeout=1)

    request = {"q": "aa", "count": 20, "request_id": "1234"}

    params = bragi.make_params(request=request, instances=[instance1, instance2])
    rsp = [
        ('q', 'aa'),
        ('limit', 20),
        ('pt_dataset[]', 'bib'),
        ('pt_dataset[]', 'bob'),
        ('timeout', 1000),
        ("request_id", "1234"),
        ('shape_scope[]', 'addr'),
        ('shape_scope[]', 'admin'),
        ('shape_scope[]', 'poi'),
        ('shape_scope[]', 'street'),
    ]

    params.sort()
    rsp.sort()
    assert params == rsp


def bragi_make_params_without_instance_test():
    """
    test of generate params without instance
    """
    bragi = GeocodeJson(host='http://bob.com/autocomplete', timeout=0.1)

    request = {"q": "aa", "count": 20, "request_id": "1234"}

    params = bragi.make_params(request=request, instances=[])
    rsp = [
        ('q', 'aa'),
        ('limit', 20),
        ('timeout', 100),
        ("request_id", "1234"),
        ('shape_scope[]', 'addr'),
        ('shape_scope[]', 'admin'),
        ('shape_scope[]', 'poi'),
        ('shape_scope[]', 'street'),
    ]
    params.sort()
    rsp.sort()
    assert rsp == params


def bragi_make_params_with_instance_and_poi_test():
    """
    test of generate params with instance which includes a poi dataset
    """
    instance = mock.MagicMock()
    instance.name = 'bob'
    instance.poi_dataset = 'priv.bob'
    bragi = GeocodeJson(host='http://bob.com/autocomplete', timeout=1)

    request = {"q": "aa", "count": 20, "request_id": "1234"}

    params = bragi.make_params(request=request, instances=[instance])
    rsp = [
        ('q', 'aa'),
        ('limit', 20),
        ('pt_dataset[]', 'bob'),
        ('poi_dataset[]', 'priv.bob'),
        ('timeout', 1000),
        ("request_id", "1234"),
        ('shape_scope[]', 'addr'),
        ('shape_scope[]', 'admin'),
        ('shape_scope[]', 'poi'),
        ('shape_scope[]', 'street'),
    ]
    params.sort()
    rsp.sort()
    assert rsp == params


def bragi_make_params_with_shape_scope_test():
    """
    test of generate params with instance which includes a poi dataset
    """
    instance = mock.MagicMock()
    instance.name = 'bob'
    instance.poi_dataset = 'priv.bob'
    bragi = GeocodeJson(host='http://bob.com/autocomplete', timeout=1)

    request = {"q": "aa", "count": 20, "request_id": "1234", "shape_scope[]": ["poi", "street"]}

    params = bragi.make_params(request=request, instances=[instance])
    rsp = [
        ('q', 'aa'),
        ('limit', 20),
        ('pt_dataset[]', 'bob'),
        ('poi_dataset[]', 'priv.bob'),
        ('timeout', 1000),
        ("request_id", "1234"),
        ('shape_scope[]', 'poi'),
        ('shape_scope[]', 'street'),
    ]
    params.sort()
    rsp.sort()
    assert rsp == params


# TODO at least a test on a invalid call to bragi + an invalid bragi response + a py breaker test ?
