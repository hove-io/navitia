# encoding: utf-8
# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from copy import deepcopy
from jormungandr.parking_space_availability.bss.cykleo import CykleoProvider
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from mock import MagicMock

network = u'DiviaVélodi'
poi = {
    'properties': {'network': network, 'operator': 'CYKLEO', 'ref': '2'},
    'poi_type': {'name': 'station vls', 'id': 'poi_type:amenity:bicycle_rental'},
}
poi_with_0 = {
    'properties': {'network': network, 'operator': 'Cykleo', 'ref': '02'},
    'poi_type': {'name': 'station vls', 'id': 'poi_type:amenity:bicycle_rental'},
}


def parking_space_availability_cykleo_support_poi_test():
    """
    CykleoProvider bss provider support
    """
    provider = CykleoProvider('http://bob', network, 'big', 'big', {'CYKLEO'})
    poi_copy = deepcopy(poi)
    assert provider.support_poi(poi_copy)
    poi_copy['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi_copy)
    poi_copy['properties']['operator'] = 'JCDecaux'
    poi_copy['properties']['network'] = 'Bad_network'
    assert not provider.support_poi(poi_copy)
    poi_copy['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi_copy)

    # same test with "02" in id, not "2"
    poi_with_0_copy = deepcopy(poi_with_0)
    assert provider.support_poi(poi_with_0_copy)
    poi_with_0_copy['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi_with_0_copy)
    poi_with_0_copy['properties']['operator'] = 'JCDecaux'
    poi_with_0_copy['properties']['network'] = 'Bad_network'
    assert not provider.support_poi(poi_with_0_copy)
    poi_with_0_copy['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi_with_0_copy)

    invalid_poi = {}
    assert not provider.support_poi(invalid_poi)
    invalid_poi = {'properties': {}}
    assert not provider.support_poi(invalid_poi)


def parking_space_availability_cykleo_get_informations_test():
    webservice_response = {
        '2': {
            "station": {
                "assetStation": {
                    "coordinate": {},
                    "organization": 1,
                    "commercialNumber": 1,
                    "id": 1,
                    "commercialName": "station 1",
                },
                "status": "IN_SERVICE",
                "id": 1,
            },
            "availableClassicBikeCount": 3,
            "id": 1,
            "availableElectricBikeCount": 1,
            "availableDockCount": 2,
        }
    }
    provider = CykleoProvider('http://bob', network, 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(2, 4, StandsStatus.open)
    assert provider.get_informations(poi_with_0) == Stands(2, 4, StandsStatus.open)
    provider._call_webservice = MagicMock(return_value=None)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.unavailable)
    assert provider.get_informations(poi_with_0) == Stands(0, 0, StandsStatus.unavailable)

    invalid_poi = {}
    assert provider.get_informations(invalid_poi) == Stands(0, 0, StandsStatus.unavailable)


def parking_space_availability_cykleo_get_informations_with_status_Closed_test():
    webservice_response = {
        '2': {
            "station": {
                "assetStation": {
                    "coordinate": {},
                    "organization": 1,
                    "commercialNumber": 1,
                    "id": 1,
                    "commercialName": "station 1",
                },
                "status": "OUT_OF_SERVICE",
                "id": 1,
            },
            "availableClassicBikeCount": 3,
            "id": 1,
            "availableElectricBikeCount": 1,
            "availableDockCount": 2,
        }
    }
    provider = CykleoProvider('http://bob', network, 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.closed)
    assert provider.get_informations(poi_with_0) == Stands(0, 0, StandsStatus.closed)


def parking_space_availability_cykleo_get_informations_with_status_unavailable_test():
    webservice_response = {
        '2': {
            "station": {
                "assetStation": {
                    "coordinate": {},
                    "organization": 1,
                    "commercialNumber": 1,
                    "id": 1,
                    "commercialName": "station 1",
                },
                "status": "DISCONNECTED",
                "id": 1,
            },
            "availableClassicBikeCount": 3,
            "id": 1,
            "availableElectricBikeCount": 1,
            "availableDockCount": 2,
        }
    }
    provider = CykleoProvider('http://bob', network, 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.unavailable)
    assert provider.get_informations(poi_with_0) == Stands(0, 0, StandsStatus.unavailable)


def available_classic_bike_count_only_test():
    webservice_response = {
        '2': {
            "station": {
                "assetStation": {
                    "coordinate": {},
                    "organization": 1,
                    "commercialNumber": 1,
                    "id": 1,
                    "commercialName": "station 1",
                },
                "status": "IN_SERVICE",
                "id": 1,
            },
            "id": 1,
            "availableClassicBikeCount": 1,
            "availableDockCount": 2,
        }
    }
    provider = CykleoProvider('http://bob', network, 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(2, 1, StandsStatus.open)
    assert provider.get_informations(poi_with_0) == Stands(2, 1, StandsStatus.open)


def available_electric_bike_count_only_test():
    webservice_response = {
        '2': {
            "station": {
                "assetStation": {
                    "coordinate": {},
                    "organization": 1,
                    "commercialNumber": 1,
                    "id": 1,
                    "commercialName": "station 1",
                },
                "status": "IN_SERVICE",
                "id": 1,
            },
            "id": 1,
            "availableElectricBikeCount": 1,
            "availableDockCount": 2,
        }
    }
    provider = CykleoProvider('http://bob', network, 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(2, 1, StandsStatus.open)
    assert provider.get_informations(poi_with_0) == Stands(2, 1, StandsStatus.open)


def witout_available_dock_count_test():
    webservice_response = {
        '2': {
            "station": {
                "assetStation": {
                    "coordinate": {},
                    "organization": 1,
                    "commercialNumber": 1,
                    "id": 1,
                    "commercialName": "station 1",
                },
                "status": "IN_SERVICE",
                "id": 1,
            },
            "id": 1,
            "availableElectricBikeCount": 1,
        }
    }
    provider = CykleoProvider('http://bob', network, 'big', 'big', {'CYKLEO'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    res_stands = provider.get_informations(poi)
    assert res_stands == Stands(0, 0, StandsStatus.unavailable)
    res_stands = provider.get_informations(poi_with_0)
    assert res_stands == Stands(0, 0, StandsStatus.unavailable)
