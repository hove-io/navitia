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

import pytest
from mock import MagicMock

from jormungandr.parking_space_availability.car.star import StarProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces

poi = {
    'properties': {
        'operator': 'Keolis Rennes',
        'ref': '42'
    },
    'poi_type': {
        'name': 'parking relais',
        'id': 'poi_type:amenity:parking'
    }
}

def car_parking_space_availability_start_support_poi_test():
    """
    STAR car provider support
    """
    provider = StarProvider({'Keolis Rennes'}, 'toto', 42)
    assert provider.support_poi(poi)

def car_parking_space_get_information_test():
    parking_places = ParkingPlaces(available=4,
                                   occupied=3,
                                   available_PRM=2,
                                   occupied_PRM=1)
    provider = StarProvider('Keolis Rennes', 'toto', 42)
    star_response = """
    {
        "records":[
            {
                "fields": {
                    "nombreplacesdisponibles": 4,
                    "nombreplacesoccupees": 3,
                    "nombreplacesdisponiblespmr": 2,
                    "nombreplacesoccupeespmr": 1
                }
            }
        ]
    }
    """
    import json
    provider._call_webservice = MagicMock(return_value=json.loads(star_response))
    assert provider.get_informations(poi) == parking_places

    invalid_poi = {}
    assert provider.get_informations(invalid_poi) is None

    provider._call_webservice = MagicMock(return_value=None)
    assert provider.get_informations(poi) is None

    star_response = """
    {
        "records":[
            {
                "fields": {
                }
            }
        ]
    }
    """
    empty_parking = ParkingPlaces(available=0,
                                  occupied=0,
                                  available_PRM=0,
                                  occupied_PRM=0)
    provider._call_webservice = MagicMock(return_value=json.loads(star_response))
    assert provider.get_informations(poi) == empty_parking

    star_response = """
    {
        "records":[
        ]
    }
    """
    provider._call_webservice = MagicMock(return_value=json.loads(star_response))
    assert provider.get_informations(poi) == empty_parking

