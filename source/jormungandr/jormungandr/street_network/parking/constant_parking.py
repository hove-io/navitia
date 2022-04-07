#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from jormungandr.street_network.parking.abstract_parking_module import AbstractParkingModule


class ConstantParking(AbstractParkingModule):
    def __init__(self, park_duration, leave_parking_duration):
        self.park_duration = park_duration
        self.leave_parking_duration = leave_parking_duration

    def get_parking_duration(self, coord):
        return self.park_duration

    def get_leave_parking_duration(self, coord):
        return self.leave_parking_duration

    def get_parking_duration_in_batch(self, coords):
        return [self.park_duration] * len(coords)

    def get_leave_duration_in_batch(self, coords):
        return [self.leave_parking_duration] * len(coords)
