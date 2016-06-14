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
from flask.ext.restful import marshal
import mock
from jormungandr.autocomplete import elastic_search
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
                            "id": "addr:49.847586;3.282103",
                            "label": "20 Rue Jean Jaures, 02100 Saint-Quentin",
                            "name": "Rue Jean Jaures",
                            "postcode": "02100",
                            "street": "Rue Jean Jaures, 02100 Saint-Quentin",
                            "type": "house",
                            "administrative_regions": [
                                {
                                    "id": "admin:fr:02000",
                                    "insee": "02000",
                                    "level": 8,
                                    "name": "Saint-Quentin",
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
                                    "name": "Haute Picardie",
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

@marshal_with(elastic_search.geocodejson)
@elastic_search.delete_attribute_autocomplete()
def get_response(bragi_response):
    return bragi_response


def bragi_house_jaures_response_check(feature_response):
    assert feature_response.get('embedded_type') == "address"
    assert feature_response.get('id') == "49.847586;3.282103"
    assert feature_response.get('name') == "Rue Jean Jaures"
    address = feature_response.get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
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
                            "label": "Rue Jean Jaures, 02100 Saint-Quentin",
                            "name": "Rue Jean Jaures",
                            "postcode": "02100",
                            "street": "Rue Jean Jaures, 02100 Saint-Quentin",
                            "type": "street",
                            "administrative_regions": [
                                {
                                    "id": "admin:fr:02000",
                                    "insee": "02000",
                                    "level": 8,
                                    "name": "Saint-Quentin",
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
                                    "name": "Haute Picardie",
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
    assert feature_response.get('name') == "Rue Jean Jaures"
    address = feature_response.get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
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
                                    "name": "Sommeron",
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
                                    "name": "Haute Picardie",
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
    assert len(feature_response.get('administrative_region')) == 2
    assert feature_response.get('administrative_region')[0].get('level') == 8
    assert feature_response.get('administrative_region')[0].get('name') == "Sommeron"

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
                            "label": "42 Rue Jean Lefebvre, 01100 Oyonnax",
                            "name": "Rue Jean Lefebvre",
                            "postcode": "02100",
                            "street": "Rue Jean Lefebvre, 01100 Oyonnax",
                            "type": "house",
                            "administrative_regions": [
                                {
                                    "id": "admin:fr:02000",
                                    "insee": "02000",
                                    "level": 8,
                                    "name": "Oyonnax",
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
                                    "name": "Haute Picardie",
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
    assert feature_response.get('name') == "Rue Jean Lefebvre"
    address = feature_response.get('address', {})
    assert address.get('name') == "Rue Jean Lefebvre"
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

def bragi_call_test():
    """
    test the whole autocomplete with a geocodejson service
    """
    bragi = elastic_search.GeocodeJson(host='http://bob.com/autocomplete')

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
        'http://bob.com/autocomplete?q=rue bobette':
        (bragi_response, 200)
    })

    # we mock the http call to return the hard coded mock_response
    with mock.patch('requests.get', mock_requests.get):
        response = bragi.get({'q': 'rue bobette'}, instance=None)
        places = response.get('places')
        assert len(places) == 4
        bragi_house_jaures_response_check(places[0])
        bragi_house_lefebvre_response_check(places[1])
        bragi_street_response_check(places[2])
        bragi_admin_response_check(places[3])

# TODO at least a test on a invalid call to bragi + an invalid bragi response + a py breaker test ?
