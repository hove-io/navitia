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
from navitiacommon import response_pb2

# https://www.cerema.fr/fr/actualites/emissions-routieres-polluants-atmospheriques-courbes
AIR_POLLUTANTS_ESTIMATION_NOX_COEFF = 0.44
AIR_POLLUTANTS_ESTIMATION_PM_COEFF = 0.056
AIR_POLLUTANTS_UNIT = 'g'


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


def get_pollutants_value(distance):
    if distance <= 0:
        return None
    return response_pb2.PollutantValues(
        nox=AIR_POLLUTANTS_ESTIMATION_NOX_COEFF * distance / 1000.0,
        pm10=AIR_POLLUTANTS_ESTIMATION_PM_COEFF * distance / 1000.0,
    )


def get_boarding_position_type(bp_str):
    try:
        return response_pb2.BoardingPosition.Value(bp_str)
    except ValueError:
        return None


def fill_best_boarding_position(pb_section, boarding_positions):
    best_positions = (get_boarding_position_type(bp) for bp in boarding_positions)
    pb_section.best_boarding_positions.extend((p for p in best_positions))
