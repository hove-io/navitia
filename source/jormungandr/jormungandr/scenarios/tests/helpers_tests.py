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
from navitiacommon import type_pb2, response_pb2
from jormungandr.scenarios.helpers import (
    is_car_direct_path,
    fill_best_boarding_position,
)

BEST_BOARDING_POSITIONS = [response_pb2.FRONT, response_pb2.MIDDLE]


def get_walking_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    return journey


def get_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    return journey


def get_walking_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 30
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.duration = 60
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 40

    return journey


def get_walking_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 40
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 55
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 20
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10

    return journey


def get_walking_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 40
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10

    return journey


def get_car_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 40
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10

    return journey


def get_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5

    return journey


def get_bike_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 55
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 7

    return journey


def get_bike_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 5

    return journey


def get_bike_pt_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    section.pt_display_informations.has_equipments.has_equipments.append(
        type_pb2.hasEquipments.has_bike_accepted
    )
    section.origin.stop_point.has_equipments.has_equipments.append(type_pb2.hasEquipments.has_bike_accepted)
    section.destination.stop_point.has_equipments.has_equipments.append(type_pb2.hasEquipments.has_bike_accepted)
    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Bike
    section.duration = 5

    return journey


def get_bike_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 50
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5

    return journey


def get_bike_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 45
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 30
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10

    return journey


def get_bss_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 45
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25

    return journey


def get_bss_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 75
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 200

    return journey


def get_bss_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 45
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25

    return journey


def get_bss_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 35
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10

    return journey


def get_ridesharing_with_crowfly_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Ridesharing
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 35
    return journey


def get_ridesharing_with_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Ridesharing
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 35
    return journey


def is_car_direct_path_test():
    assert not is_car_direct_path(get_walking_journey())
    assert not is_car_direct_path(get_walking_car_journey())
    assert not is_car_direct_path(get_car_walking_journey())
    assert is_car_direct_path(get_car_journey())
    assert not is_car_direct_path(get_bike_car_journey())
    assert not is_car_direct_path(get_bss_car_journey())


def get_pt_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    return journey


def fill_best_boarding_position_test():
    journey = get_pt_journey()
    fill_best_boarding_position(journey.sections[0], BEST_BOARDING_POSITIONS)
    assert journey.sections[0].best_boarding_positions
    assert response_pb2.BoardingPosition.FRONT in journey.sections[0].best_boarding_positions
    assert response_pb2.BoardingPosition.MIDDLE in journey.sections[0].best_boarding_positions
    assert response_pb2.BoardingPosition.BACK not in journey.sections[0].best_boarding_positions
