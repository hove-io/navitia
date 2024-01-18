# encoding: utf-8
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
from copy import deepcopy
from jormungandr.parking_space_availability.car.forseti import ForsetiProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces
from mock import MagicMock

poi = {
    'properties': {'amenity': 'parking'},
    'poi_type': {'name': 'Parking', 'id': 'poi_type:public_parking'},
    'coord': {'lat': '42.3682265', 'lon': '-83.07793565'},
}


def parking_space_availability_forseti_support_poi_test():
    """
    ForsetiProvider car parking provider support
    Since we search car parking in forseti with coordinate, it is always True
    """
    provider = ForsetiProvider('http://forseti')
    poi_copy = deepcopy(poi)
    assert provider.support_poi(poi_copy)


def parking_space_availability_forseti_get_informations_test():
    webservice_response = {
        "parkings": [
            {
                "parkcode": "48200",
                "name": "786 Parking",
                "coord": {"lat": 42.368227, "lon": -83.0779357},
                "availability": True,
                "currency": "USD",
                "start_time": "2019-04-01T00:00:00Z",
                "end_time": "2019-04-01T23:59:59Z",
                "amount": 6000,
            }
        ],
        "pagination": {"start_page": 0, "items_on_page": 25, "items_per_page": 25, "total_result": 1},
    }

    parking_places = ParkingPlaces(
        availability=True, 
        currency='USD', 
        start_time='2019-04-01T00:00:00Z',
        end_time='2019-04-01T23:59:59Z', 
        amount=6000,
    )
    provider = ForsetiProvider('http://forseti')
    provider._call_webservice = MagicMock(return_value=webservice_response)
    parking = provider.get_informations(poi)
    assert parking == parking_places
    print(parking)
