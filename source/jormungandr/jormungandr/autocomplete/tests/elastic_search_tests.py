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
    skojig_response = """
{
    "Autocomplete": {
        "features": [
            {
                "geometry": null,
                "properties": {
                    "geocoding": {
                        "city": "Saint-Quentin",
                        "housenumber": "20",
                        "id": "addr:49.847586;3.282103",
                        "label": "20 Rue Jean Jaurès, 02100 Saint-Quentin",
                        "name": "20 Rue Jean Jaurès, 02100 Saint-Quentin",
                        "postcode": "02100",
                        "street": "Rue Jean Jaurès, 02100 Saint-Quentin",
                        "type": "house"
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
                        "label": "Rue Jean Jaurès, 02700 Mennessis",
                        "name": "Rue Jean Jaurès",
                        "postcode": "02700",
                        "street": null,
                        "type": "street"
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