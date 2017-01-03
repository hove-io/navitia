# coding=utf-8

# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
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
import mock
from jormungandr.autocomplete import geocodejson
from jormungandr.interfaces.v1 import Places
from jormungandr.tests.utils_test import MockRequests
from flask.ext.restful import marshal_with


def bragi_house_jaures_feature():
    house_feature = {
        "geometry": {
            "coordinates": [
                3.282103,
                49.847586
            ],
            "type": "Point"
        },
        "properties": {
            "geocoding": {
                "city": "Saint-Quentin",
                "housenumber": "20",
                "id": "49.847586;3.282103",
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
                        "label": "Saint-Quentin",
                        "zip_code": "02000",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie",
                        "zip_code": "80200",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    }
                ],
            }
        },
        "type": "Feature"
    }
    return house_feature

@marshal_with(Places.geocodejson)
@geocodejson.delete_attribute_autocomplete()
def get_response(bragi_response):
    return bragi_response


def bragi_house_jaures_response_check(feature_response):
    assert feature_response.get('embedded_type') == "address"
    assert feature_response.get('id') == "49.847586;3.282103"
    assert feature_response.get('name') == "20 Rue Jean Jaures (Saint-Quentin)"
    address = feature_response.get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
    assert address.get('label') == "20 Rue Jean Jaures (Saint-Quentin)"
    assert address.get('house_number') == "20"
    assert address.get('coord', {}).get('lat') == 49.847586
    assert address.get('coord', {}).get('lon') == 3.282103
    assert len(address.get('administrative_regions')) == 2
    region_list = {region['level']: region['name'] for region in address.get('administrative_regions')}
    assert region_list.get(7) == "Haute Picardie"
    assert region_list.get(8) == "Saint-Quentin"

def bragi_house_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_house_jaures_feature()
            ]
        }
    }
    navitia_response = get_response(bragi_response).get('places', {})
    bragi_house_jaures_response_check(navitia_response[0])


def bragi_street_feature():
    street_feature = {
        "geometry": {
            "coordinates": [
                3.282103,
                49.847586
            ],
            "type": "Point"
        },
        "properties": {
            "geocoding": {
                "city": "Saint-Quentin",
                "id": "49.847586;3.282103",
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
                        "label": "Saint-Quentin",
                        "zip_code": "02000",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie",
                        "zip_code": "80200",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    }
                ],
            }
        },
        "type": "Feature"
    }
    return street_feature

def bragi_street_response_check(feature_response):

    assert feature_response.get('embedded_type') == "address"
    assert feature_response.get('id') == "49.847586;3.282103"
    assert feature_response.get('name') == "Rue Jean Jaures (Saint-Quentin)"
    address = feature_response.get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
    assert address.get('label') == "Rue Jean Jaures (Saint-Quentin)"
    assert address.get('house_number') == "0"
    assert address.get('coord', {}).get('lat') == 49.847586
    assert address.get('coord', {}).get('lon') == 3.282103
    assert len(address.get('administrative_regions')) == 2
    response = next(region for region in address.get('administrative_regions'))
    assert response.get('level') == 8
    assert response.get('name') == "Saint-Quentin"

def bragi_street_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_street_feature()
            ]
        }
    }

    navitia_response = get_response(bragi_response).get('places', {})
    bragi_street_response_check(navitia_response[0])

def bragi_street_reading_without_autocomplete_attribute_test():
    bragi_response = {
        "features": [
            bragi_street_feature()
        ]
    }

    navitia_response = get_response(bragi_response).get('places', {})
    bragi_street_response_check(navitia_response[0])


def bragi_admin_feature():
    admin_feature = {
        "geometry": "",
        "properties": {
            "geocoding": {
                "city": "",
                "housenumber": "",
                "id": "admin:fr:2725",
                "label": "Sommeron",
                "name": "Sommeron",
                "postcode": "02260",
                "street": "",
                "type": "city",
                "administrative_regions": [
                    {
                        "id": "admin:fr:02000",
                        "insee": "02000",
                        "level": 8,
                        "label": "Sommeron",
                        "zip_code": "02000",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie",
                        "zip_code": "80200",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    }
                ],
            }
        },
        "type": "Feature"
    }
    return admin_feature

def bragi_admin_response_check(feature_response):
    assert feature_response.get('embedded_type') == "administrative_region"
    assert feature_response.get('id') == "admin:fr:2725"
    assert feature_response.get('name') == "Sommeron"
    assert len(feature_response.get('administrative_regions')) == 2
    assert feature_response.get('administrative_regions')[0].get('level') == 8
    assert feature_response.get('administrative_regions')[0].get('name') == "Sommeron"

def bragi_admin_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_admin_feature()
            ]
        }
    }

    navitia_response = get_response(bragi_response).get('places', {})
    bragi_admin_response_check(navitia_response[0])


def bragi_house_lefebvre_feature():
    house_feature = {
        "geometry": {
            "coordinates": [
                3.282103,
                49.847586
            ],
            "type": "Point"
        },
        "properties": {
            "geocoding": {
                "city": "Saint-Quentin",
                "housenumber": "42",
                "id": "49.847586;3.282103",
                "label": "42 Rue Jean Lefebvre (Oyonnax)",
                "name": "Rue Jean Lefebvre",
                "postcode": "02100",
                "street": "Rue Jean Lefebvre",
                "type": "house",
                "administrative_regions": [
                    {
                        "id": "admin:fr:02000",
                        "insee": "02000",
                        "level": 8,
                        "label": "Oyonnax",
                        "zip_code": "02000",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    },
                    {
                        "id": "admin:fr:248000549",
                        "insee": "248000549",
                        "level": 7,
                        "label": "Haute Picardie",
                        "zip_code": "80200",
                        "weight": 1,
                        "coord": {
                            "lat": 48.8396154,
                            "lon": 2.3957517
                        }
                    }
                ],
            }
        },
        "type": "Feature"
    }
    return house_feature

def bragi_house_lefebvre_response_check(feature_response):
    assert feature_response.get('embedded_type') == "address"
    assert feature_response.get('id') == "49.847586;3.282103"
    assert feature_response.get('name') == "42 Rue Jean Lefebvre (Oyonnax)"
    address = feature_response.get('address', {})
    assert address.get('id') == "49.847586;3.282103"
    assert address.get('name') == "Rue Jean Lefebvre"
    assert address.get('label') == "42 Rue Jean Lefebvre (Oyonnax)"
    assert address.get('house_number') == "42"
    assert address.get('coord', {}).get('lat') == 49.847586
    assert address.get('coord', {}).get('lon') == 3.282103
    assert len(address.get('administrative_regions')) == 2
    region_list = {region['level']: region['name'] for region in address.get('administrative_regions')}
    assert region_list.get(7) == "Haute Picardie"
    assert region_list.get(8) == "Oyonnax"

def bragi_good_geocodejson_response_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_house_jaures_feature(),
                bragi_house_lefebvre_feature(),
                bragi_street_feature(),
                bragi_admin_feature()
            ]
        }
    }

    navitia_response = get_response(bragi_response).get('places', {})
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
            bragi_admin_feature()
        ]
    }

    navitia_response = get_response(bragi_response).get('places', {})
    assert len(navitia_response) == 4
    bragi_house_jaures_response_check(navitia_response[0])
    bragi_house_lefebvre_response_check(navitia_response[1])
    bragi_street_response_check(navitia_response[2])
    bragi_admin_response_check(navitia_response[3])


def bragi_geocodejson_spec_feature():
    return {
        "geometry": {
            "coordinates": [
                3.282103,
                49.847586
            ],
            "type": "Point"
        },
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
                    "level8": "Saint-Quentin"
                },
            }
        },
        "type": "Feature"
    }


def bragi_geocodejson_compatibility_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_geocodejson_spec_feature(),
            ]
        }
    }
    navitia_response = get_response(bragi_response).get('places', {})[0]
    assert navitia_response.get('embedded_type') == "address"
    assert navitia_response.get('id') == '49.847586;3.282103'
    assert navitia_response.get('name') == '20 Rue Jean Jaures (Saint-Quentin)'
    address = navitia_response.get('address', {})
    assert len(address.get('administrative_regions')) == 5
    region_list = {region['level']: region['name'] for region in address.get('administrative_regions')}
    assert region_list.get(2) == 'France'
    assert region_list.get(4) == 'Nord-Pas-de-Calais-Picardie'
    assert region_list.get(7) == 'Saint-Quentin'


def bragi_poi_feature():
    return {
        "geometry": {
            "coordinates": [8.9028068, 42.599235500000009],
            "type": "Point"
        },
        "properties": {
            "geocoding": {
                "id": "poi:osm:3224270910",
                "type": "poi",
                "label": "Mairie de Pigna (Pigna)",
                "name": "Mairie de Pigna",
                "housenumber": None,
                "street": None,
                "postcode": None,
                "city": "Pigna",
                "administrative_regions": [{
                    "id": "admin:fr:2B231",
                    "insee": "2B231",
                    "level": 8,
                    "label": "Pigna",
                    "zip_codes": ["20220", "20221", "20222"],
                    "weight": 0,
                    "coord": {
                        "lat": 42.5996043,
                        "lon": 8.9027334
                    }
                }]
            }
        },
        "type": "Feature"
    }


def bragi_poi_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_poi_feature(),
            ]
        }
    }
    navitia_response = get_response(bragi_response).get('places', {})[0]
    assert navitia_response.get('embedded_type') == "poi"
    assert navitia_response.get('id') == 'poi:osm:3224270910'
    assert navitia_response.get('name') == 'Mairie de Pigna (Pigna)'
    assert navitia_response.get('quality') == '0'
    poi = navitia_response.get('poi', {})
    assert poi.get('label') == 'Mairie de Pigna (Pigna)'
    assert poi.get('name') == 'Mairie de Pigna'
    assert poi.get('id') == 'poi:osm:3224270910'
    assert poi.get('coord').get('lat') == 42.599235500000009
    assert poi.get('coord').get('lon') == 8.9028068
    assert len(poi.get('administrative_regions')) == 1
    administrative_region = poi.get('administrative_regions')[0]
    assert administrative_region.get('insee') == '2B231'
    assert administrative_region.get('name') == 'Pigna'
    assert administrative_region.get('label') == 'Pigna'
    assert administrative_region.get('level') == 8
    assert administrative_region.get('id') == 'admin:fr:2B231'
    assert administrative_region.get('coord').get('lat') == 42.5996043
    assert administrative_region.get('coord').get('lon') == 8.9027334
    assert administrative_region.get('zip_code') == "20220-20222"


def bragi_stop_area_feature():
    return {
        "geometry": {
            "coordinates": [
                2.389462,
                48.87958
            ],
            "type": "Point"
        },
        "properties": {
            "geocoding": {
                "administrative_regions": [],
                "city": None,
                "id": "stop_area:OIF:SA:59332",
                "label": "BOTZARIS (Paris)",
                "name": "BOTZARIS",
                "postcode": None,
                "type": "public_transport:stop_area",
                "timezone": "Europe/Paris"
            }
        },
        "type": "Feature"
    }


def bragi_stop_area_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_stop_area_feature(),
            ]
        }
    }
    navitia_response = get_response(bragi_response).get('places', {})[0]
    assert navitia_response.get('embedded_type') == "stop_area"
    assert navitia_response.get('id') == 'stop_area:OIF:SA:59332'
    assert navitia_response.get('name') == 'BOTZARIS (Paris)'
    assert navitia_response.get('quality') == '0'
    sa = navitia_response.get('stop_area', {})
    assert sa.get('label') == 'BOTZARIS (Paris)'
    assert sa.get('name') == 'BOTZARIS'
    assert sa.get('id') == 'stop_area:OIF:SA:59332'
    assert sa.get('timezone') == 'Europe/Paris'
    assert sa.get('coord').get('lat') == 48.87958
    assert sa.get('coord').get('lon') == 2.389462
    assert len(sa.get('administrative_regions')) == 0



def geojson():
    return {"type": "Feature",
              "geometry": {"type": "Point", "coordinates": [102.0, 0.5]},
              "properties": {"prop0": "value0"}
            }

def bragi_call_test():
    """
    test the whole autocomplete with a geocodejson service
    """
    bragi = geocodejson.GeocodeJson(host='http://bob.com/autocomplete')

    bragi_response = {
        "Autocomplete": {
            "features": [
                bragi_house_jaures_feature(),
                bragi_house_lefebvre_feature(),
                bragi_street_feature(),
                bragi_admin_feature()
            ]
        }
    }
    mock_requests = MockRequests({
        'http://bob.com/autocomplete?q=rue bobette&limit=10':
        (bragi_response, 200)
    })

    # we mock the http call to return the hard coded mock_response
    with mock.patch('requests.get', mock_requests.get):
        raw_response = bragi.get({'q': 'rue bobette', 'count': 10}, instance=None, shape=None)
        places = get_response(raw_response).get('places')
        assert len(places) == 4
        bragi_house_jaures_response_check(places[0])
        bragi_house_lefebvre_response_check(places[1])
        bragi_street_response_check(places[2])
        bragi_admin_response_check(places[3])

    with mock.patch('requests.post', mock_requests.get):
        raw_response = bragi.get({'q': 'rue bobette', 'count': 10}, instance=None, shape=geojson())
        places = get_response(raw_response).get('places')
        assert len(places) == 4
        bragi_house_jaures_response_check(places[0])
        bragi_house_lefebvre_response_check(places[1])
        bragi_street_response_check(places[2])
        bragi_admin_response_check(places[3])

# TODO at least a test on a invalid call to bragi + an invalid bragi response + a py breaker test ?
