# encoding: utf-8
# Copyright (c) 2001-2019, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from mock import MagicMock

from jormungandr.parking_space_availability.car.sytral import SytralProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces

import json

poi = {
    'properties': {'operator': 'sytral', 'ref': 'park_42'},
    'poi_type': {'name': 'Parking', 'id': 'poi_type:public_parking'},
}

sytral_response = """
    {
        "records":
        [
            {
                "car_park_id": "park_42",
                "updated_time": "2019-01-29T14:47:21.826327317+01:00",
                "available": 4,
                "occupied": 3,
                "available_PRM": 1,
                "occupied_PRM": 0
            }
        ]
    }
    """


def car_park_space_get_information_test():
    parking_places = ParkingPlaces(available=4, occupied=3, available_PRM=1, occupied_PRM=0)
    provider = SytralProvider(url="sytral.url", operators={'Sytral'}, dataset='sytral_dataset', timeout=42)
    provider._call_webservice = MagicMock(return_value=json.loads(sytral_response))
    info = provider.get_informations(poi)
    assert info == parking_places
