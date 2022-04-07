# encoding: utf-8
# Copyright (c) 2001-2021, Hove and/or its affiliates. All rights reserved.
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
from jormungandr.external_services.vehicle_occupancy import VehicleOccupancyProvider

mock_data = {
    "vehicle_occupancies": [
        {
            "vehiclejourney_id": "vehicle_journey:0:123714928-1",
            "stop_id": "stop_point:0:SP:80:4165",
            "date_time": "2021-03-03T08:56:00+01:00",
            "occupancy": "STANDING_ROOM_ONLY",
        },
        {
            "vehiclejourney_id": "vehicle_journey:0:123714950-1",
            "stop_id": "stop_point:0:SP:80:4165",
            "date_time": "2021-03-03T17:23:00+01:00",
            "occupancy": "FEW_SEATS_AVAILABLE",
        },
        {
            "vehiclejourney_id": "vehicle_journey:0:123714970-1",
            "stop_id": "stop_point:0:SP:80:4165",
            "date_time": "2021-03-03T20:31:00+01:00",
            "occupancy": "CRUSHED_STANDING_ROOM_ONLY",
        },
        {
            "vehiclejourney_id": "vehicle_journey:0:123714944-1",
            "stop_id": "stop_point:0:SP:80:4165",
            "date_time": "2021-03-03T16:26:00+01:00",
            "occupancy": "CRUSHED_STANDING_ROOM_ONLY",
        },
    ]
}


def vehicle_occupancy_get_information_test():
    """
    Test that 'vehicle_occupancies'
    """
    service = VehicleOccupancyProvider(service_url="vehicle_occupancy.url", timeout=3)
    brut_result = service._call_webservice = MagicMock(return_value=mock_data)
    vehicle_occupancies = brut_result.return_value.get('vehicle_occupancies', [])
    assert len(vehicle_occupancies) == 4
    assert vehicle_occupancies[0]['vehiclejourney_id'] == "vehicle_journey:0:123714928-1"
    assert vehicle_occupancies[0]['stop_id'] == "stop_point:0:SP:80:4165"
    assert vehicle_occupancies[0]['date_time'] == "2021-03-03T08:56:00+01:00"
    assert vehicle_occupancies[0]['occupancy'] == "STANDING_ROOM_ONLY"
