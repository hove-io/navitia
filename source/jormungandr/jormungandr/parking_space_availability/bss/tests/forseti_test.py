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
from jormungandr.parking_space_availability.bss.forseti import ForsetiProvider
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from mock import MagicMock

poi = {
    'poi_type': {'name': 'station vls', 'id': 'poi_type:amenity:bicycle_rental'},
    'coord': {'lat': '48.0981147', 'lon': '-1.6552921'},
}

BSS_PROVIDER = [
    {
        "id": "forseti_stations",
        "class": "jormungandr.parking_space_availability.bss.forseti.ForsetiProvider",
        "args": {
            "service_url": "https://gbfs-station.forseti.sbx.aws.private/stations",
            "timeout": 20,
            "distance": 20,
        },
    }
]


def parking_space_availability_forseti_support_poi_test():
    """
    ForsetiProvider bss provider support
    Since we search bss station in forseti with coordinate, it is always True
    """
    provider = ForsetiProvider('http://forseti')
    poi_copy = deepcopy(poi)
    assert provider.support_poi(poi_copy)


def parking_space_availability_forseti_get_informations_test():
    webservice_response = {
        "stations": [
            {
                "id": "TAN:Station:18",
                "name": "018-VIARME",
                "coord": {"lat": 48.0981147, "lon": -1.6552921},
                "vehicles": [{"type": "bicycle", "count": 9}],
                "docks": {"available": 4, "total": 13},
                "status": "OPEN",
            }
        ],
        "pagination": {"start_page": 0, "items_on_page": 2, "items_per_page": 25, "total_result": 2},
    }

    provider = ForsetiProvider('http://forseti')
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(4, 9, StandsStatus.open)

    provider._call_webservice = MagicMock(return_value=None)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.unavailable)
    invalid_poi = {}
    assert provider.get_informations(invalid_poi) == Stands(0, 0, StandsStatus.unavailable)
