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


def bragi_house_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                {
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
                },
            ]
        }
    }

    navitia_response = marshal(bragi_response, elastic_search.geocodejson).get('places', {})

    assert navitia_response[0].get('embedded_type') == "address"
    assert navitia_response[0].get('id') == "addr:49.847586;3.282103"
    assert navitia_response[0].get('name') == "Rue Jean Jaures"
    address = navitia_response[0].get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
    assert address.get('house_number') == "20"
    assert address.get('coord', {}).get('lat') == 49.847586
    assert address.get('coord', {}).get('lon') == 3.282103
    assert len(address.get('administrative_regions')) == 5
    region_list = {region['level']: region['name'] for region in address.get('administrative_regions')}
    assert region_list.get(2) == "France"
    assert region_list.get(4) == "Nord-Pas-de-Calais-Picardie"
    assert region_list.get(6) == "Aisne"
    assert region_list.get(7) == "Saint-Quentin"
    assert region_list.get(8) == "Saint-Quentin"


def bragi_street_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                {
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
                            "id": "addr:49.847586;3.282103",
                            "label": "Rue Jean Jaures, 02100 Saint-Quentin",
                            "name": "Rue Jean Jaures",
                            "postcode": "02100",
                            "street": "Rue Jean Jaures, 02100 Saint-Quentin",
                            "type": "street",
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
                },
            ]
        }
    }

    navitia_response = marshal(bragi_response, elastic_search.geocodejson).get('places', {})

    assert navitia_response[0].get('embedded_type') == "address"
    assert navitia_response[0].get('id') == "addr:49.847586;3.282103"
    assert navitia_response[0].get('name') == "Rue Jean Jaures"
    address = navitia_response[0].get('address', {})
    assert address.get('name') == "Rue Jean Jaures"
    assert address.get('house_number') == "0"
    assert address.get('coord', {}).get('lat') == 49.847586
    assert address.get('coord', {}).get('lon') == 3.282103
    assert len(address.get('administrative_regions')) == 5
    response = next(region for region in address.get('administrative_regions'))
    print(response)
    assert response.get('level') == 8
    assert response.get('name') == "Saint-Quentin"


def bragi_admin_reading_test():
    bragi_response = {
        "Autocomplete": {
            "features": [
                {
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
                            "admin": {
                                "level8": "Sommeron"
                            },
                        }
                    },
                    "type": "Feature"
                },
            ]
        }
    }
    navitia_response = marshal(bragi_response, elastic_search.geocodejson).get('places', {})

    assert navitia_response[0].get('embedded_type') == "administrative_region"
    assert navitia_response[0].get('id') == "admin:fr:2725"
    assert navitia_response[0].get('name') == "Sommeron"
    assert len(navitia_response[0].get('administrative_region')) == 1
    assert navitia_response[0].get('administrative_region')[0].get('level') == 8
    assert navitia_response[0].get('administrative_region')[0].get('name') == "Sommeron"


def mock_good_geocodejson_response():
    # TODO concatenate the 3 others geocodejson flow to make a godd response
    return ''


def elastic_search_call_test():
    """
    test the whole autocomplete with a geocodejson service
    """
    bragi = elastic_search.GeocodeJson(host='http://bob.com/autocomplete')

    mock_requests = MockRequests({
        'http://bob.com/autocomplete?q=rue bobette':
        (mock_good_geocodejson_response(), 200)
    })

    # we mock the http call to return the hard coded mock_response
    with mock.patch('requests.get', mock_requests.get):
        response = bragi.get({'q': 'rue bobette'}, instance=None)
        # TODO assert on response
        assert False

# TODO at least a test on a invalid call to bragi + an invalid bragi response + a py breaker test ?
