# encoding: utf-8
# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr.parking_space_availability.bss.cykleo import CykleoProvider
from jormungandr.parking_space_availability.bss.stands import Stands
from mock import MagicMock

poi = {
    'properties': {
        'network': u'DiviaVélodi',
        'operator': 'CYKLEO',
        'ref': '2'
    },
    'poi_type': {
        'name': 'station vls',
        'id': 'poi_type:amenity:bicycle_rental'
    }
}


def parking_space_availability_cykleo_support_poi_test():
    """
    CykleoProvider bss provider support
    """
    provider = CykleoProvider('http://bob', u'DiviaVélodi', 'big', 'big', {'CYKLEO'})
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'JCDecaux'
    poi['properties']['network'] = 'Bad_network'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)

    invalid_poi = {}
    assert not provider.support_poi(invalid_poi)
    invalid_poi = {'properties': {}}
    assert not provider.support_poi(invalid_poi)


def parking_space_availability_cykleo_get_informations_test():
    webservice_response = {'2': {
        "station": {
            "assetStation": {
                "coordinate": {},
                "organization": 1,
                "commercialNumber": 1,
                "id": 1,
                "commercialName": "station 1"
            },
            "status": "IN_SERVICE",
            "id": 1
        },
        "availableClassicBikeCount": 3,
        "id": 1,
        "availableElectricBikeCount": 1,
        "availableDockCount": 2
    }}
    provider = CykleoProvider('http://bob', u'DiviaVélodi', 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(2, 4)
    provider._call_webservice = MagicMock(return_value=None)
    assert provider.get_informations(poi) is None
    invalid_poi = {}
    assert provider.get_informations(invalid_poi) is None


def available_classic_bike_count_only_test():
    webservice_response = {'2': {
        "station": {
            "assetStation": {
                "coordinate": {},
                "organization": 1,
                "commercialNumber": 1,
                "id": 1,
                "commercialName": "station 1"
            },
            "status": "IN_SERVICE",
            "id": 1
        },
        "id": 1,
        "availableClassicBikeCount": 1,
        "availableDockCount": 2
    }}
    provider = CykleoProvider('http://bob', u'DiviaVélodi', 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(2, 1)


def available_electric_bike_count_only_test():
    webservice_response = {'2': {
        "station": {
            "assetStation": {
                "coordinate": {},
                "organization": 1,
                "commercialNumber": 1,
                "id": 1,
                "commercialName": "station 1"
            },
            "status": "IN_SERVICE",
            "id": 1
        },
        "id": 1,
        "availableElectricBikeCount": 1,
        "availableDockCount": 2
    }}
    provider = CykleoProvider('http://bob', u'DiviaVélodi', 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(2, 1)


def witout_available_dock_count_test():
    webservice_response = {'2': {
        "station": {
            "assetStation": {
                "coordinate": {},
                "organization": 1,
                "commercialNumber": 1,
                "id": 1,
                "commercialName": "station 1"
            },
            "status": "IN_SERVICE",
            "id": 1
        },
        "id": 1,
        "availableElectricBikeCount": 1,
    }}
    provider = CykleoProvider('http://bob', u'DiviaVélodi', 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert not provider.get_informations(poi)
