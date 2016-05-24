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


def skojig_house_reading_test():
    skojig_response = {
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
                        "name": "20 Rue Jean Jaures, 02100 Saint-Quentin",
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


    places_response = """
{
    "places":
        [
            {
                "embedded_type":"address",
                "quality":90,
                "id":"3.282103;49.847586",
                "name":"20 Rue Jean Jaures (Saint-Quentin)",
                "address":{
                    "name":"Rue Jean Jaures",
                    "house_number":20,
                    "coord":{
                        "lat":"49.847586",
                        "lon":"3.282103"
                    },
                    "label":"20 Rue Jean Jaures (Saint-Quentin)",
                    "administrative_regions":[
                        {
                        "insee":"02691",
                        "name":"Saint-Quentin",
                        "level":8,
                        "coord":{
                            "lat":"48.97183",
                            "lon":"2.202007"
                        },
                        "label":"Saint-Quentin (02100)",
                        "id":"402367",
                        "zip_code":"02100"
                        }
                    ],
                    "id":"2.201313942397864;48.9780114067219"
                }
            }
        ]
}
"""

    navitia_response = marshal(skojig_response, elastic_search.geocodejson).get('places', {})

    assert navitia_response[0].get('embedded_type') == "address"
    assert navitia_response[0].get('id') == "addr:49.847586;3.282103"
    assert navitia_response[0].get('name') == "20 Rue Jean Jaures, 02100 Saint-Quentin"
    address = navitia_response[0].get('address', {})
    print(address)

def skojig_street_reading_test():
    skojig_response = """
{
    "Autocomplete": {
        "features": [
            {
                "geometry": null,
                "properties": {
                    "geocoding": {
                        "city": "Mennessis",
                        "housenumber": null,
                        "id": "street:024740025K",
                        "label": "Rue Jean Jaures, 02700 Mennessis",
                        "name": "Rue Jean Jaures",
                        "postcode": "02700",
                        "street": null,
                        "type": "street",
                    }
                },
                "type": "Feature"
            },
        ]
    }
}
     """
    navitia_response = marshal(skojig_response, elastic_search.geocodejson)

    assert False # TODO check the response :)


def skojig_admin_reading_test():
    skojig_response = """
{
    "Autocomplete": {
        "features": [
            {
                "geometry": null,
                "properties": {
                    "geocoding": {
                        "city": null,
                        "housenumber": null,
                        "id": "admin:fr:2725",
                        "label": "Sommeron",
                        "name": "Sommeron",
                        "postcode": "02260",
                        "street": null,
                        "type": "city"
                    }
                },
                "type": "Feature"
            },
        ]
    }
}
     """
    navitia_response = marshal(skojig_response, elastic_search.geocodejson)

    assert False # TODO check the response :)


def mock_good_geocodejson_response():
    # TODO concatenate the 3 others geocodejson flow to make a godd response
    return ''


def elastic_search_call_test():
    """
    test the whole autocomplete with a geocodejson service
    """
    skojig = elastic_search.GeocodeJson(host='http://bob.com/autocomplete')

    mock_requests = MockRequests({
        'http://bob.com/autocomplete?q=rue bobette':
        (mock_good_geocodejson_response(), 200)
    })

    # we mock the http call to return the hard coded mock_response
    with mock.patch('requests.get', mock_requests.get):
         response = skojig.get({'q': 'rue bobette'}, instance=None)
         # TODO assert on response
         assert False

# TODO at least a test on a invalid call to skojig + an invalid skojig response + a py breaker test ?