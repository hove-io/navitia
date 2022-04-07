# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from navitiacommon import response_pb2


def is_car_direct_path(journey):
    car_seen = False
    for section in journey.sections:
        if section.type not in [response_pb2.STREET_NETWORK, response_pb2.PARK, response_pb2.LEAVE_PARKING]:
            return False
        if section.type != response_pb2.STREET_NETWORK:
            continue
        if section.street_network.mode not in [response_pb2.Walking, response_pb2.Car, response_pb2.CarNoPark]:
            return False
        if (
            section.street_network.mode == response_pb2.Car
            or section.street_network.mode == response_pb2.CarNoPark
        ):
            car_seen = True
    return car_seen
