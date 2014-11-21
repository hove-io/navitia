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

from navitiacommon import response_pb2
from nose.tools import eq_
from jormungandr.scenarios.helpers import *

def get_walking_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    return journey

def get_walking_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike

    return journey

def get_walking_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    return journey

def get_walking_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section = journey.sections.add()

    return journey

def get_bike_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    return journey

def get_bike_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike

    return journey

def get_bike_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    return journey

def get_bike_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section = journey.sections.add()

    return journey

def get_bss_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    return journey

def get_bss_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike

    return journey

def get_bss_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    return journey

def get_bss_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section = journey.sections.add()

    return journey


#==============================================================================
# has_bss_first_and_walking_last test
#==============================================================================
def has_bss_first_and_walking_last__walking_walking_test():
    assert not has_bss_first_and_walking_last(get_walking_walking_journey())

def has_bss_first_and_walking_last__walking_bike_test():
    assert not has_bss_first_and_walking_last(get_walking_bike_journey())

def has_bss_first_and_walking_last__walking_bss_test():
    assert not has_bss_first_and_walking_last(get_walking_bss_journey())

def has_bss_first_and_walking_last__walking_car_test():
    assert not has_bss_first_and_walking_last(get_walking_car_journey())

def has_bss_first_and_walking_last__bss_walking_test():
    assert has_bss_first_and_walking_last(get_bss_walking_journey())

def has_bss_first_and_walking_last__bss_bike_test():
    assert not has_bss_first_and_walking_last(get_bss_bike_journey())

def has_bss_first_and_walking_last__bss_bss_test():
    assert not has_bss_first_and_walking_last(get_bss_bss_journey())

def has_bss_first_and_walking_last__bss_car_test():
    assert not has_bss_first_and_walking_last(get_bss_car_journey())

def has_bss_first_and_walking_last__bike_walking_test():
    assert not has_bss_first_and_walking_last(get_bike_walking_journey())

def has_bss_first_and_walking_last__bike_bike_test():
    assert not has_bss_first_and_walking_last(get_bike_bike_journey())

def has_bss_first_and_walking_last__bike_bss_test():
    assert not has_bss_first_and_walking_last(get_bike_bss_journey())

def has_bss_first_and_walking_last__bike_car_test():
    assert not has_bss_first_and_walking_last(get_bike_car_journey())


#==============================================================================
# has_walking_first_and_bss_last test
#==============================================================================
def has_walking_first_and_bss_last__walking_walking_test():
    assert not has_walking_first_and_bss_last(get_walking_walking_journey())

def has_walking_first_and_bss_last__walking_bike_test():
    assert not has_walking_first_and_bss_last(get_walking_bike_journey())

def has_walking_first_and_bss_last__walking_bss_test():
    assert has_walking_first_and_bss_last(get_walking_bss_journey())

def has_walking_first_and_bss_last__walking_car_test():
    assert not has_walking_first_and_bss_last(get_walking_car_journey())

def has_walking_first_and_bss_last__bss_walking_test():
    assert not has_walking_first_and_bss_last(get_bss_walking_journey())

def has_walking_first_and_bss_last__bss_bike_test():
    assert not has_walking_first_and_bss_last(get_bss_bike_journey())

def has_walking_first_and_bss_last__bss_bss_test():
    assert not has_walking_first_and_bss_last(get_bss_bss_journey())

def has_walking_first_and_bss_last__bss_car_test():
    assert not has_walking_first_and_bss_last(get_bss_car_journey())

def has_walking_first_and_bss_last__bike_walking_test():
    assert not has_walking_first_and_bss_last(get_bike_walking_journey())

def has_walking_first_and_bss_last__bike_bike_test():
    assert not has_walking_first_and_bss_last(get_bike_bike_journey())

def has_walking_first_and_bss_last__bike_bss_test():
    assert not has_walking_first_and_bss_last(get_bike_bss_journey())

def has_walking_first_and_bss_last__bike_car_test():
    assert not has_walking_first_and_bss_last(get_bike_car_journey())

#==============================================================================
# has_bss_first_and_bss_last test
#==============================================================================
def has_bss_first_and_bss_last__walking_walking_test():
    assert not has_bss_first_and_bss_last(get_walking_walking_journey())

def has_bss_first_and_bss_last__walking_bike_test():
    assert not has_bss_first_and_bss_last(get_walking_bike_journey())

def has_bss_first_and_bss_last__walking_bss_test():
    assert not has_bss_first_and_bss_last(get_walking_bss_journey())

def has_bss_first_and_bss_last__walking_car_test():
    assert not has_bss_first_and_bss_last(get_walking_car_journey())

def has_bss_first_and_bss_last__bss_walking_test():
    assert not has_bss_first_and_bss_last(get_bss_walking_journey())

def has_bss_first_and_bss_last__bss_bike_test():
    assert not has_bss_first_and_bss_last(get_bss_bike_journey())

def has_bss_first_and_bss_last__bss_bss_test():
    assert has_bss_first_and_bss_last(get_bss_bss_journey())

def has_bss_first_and_bss_last__bss_car_test():
    assert not has_bss_first_and_bss_last(get_bss_car_journey())

def has_bss_first_and_bss_last__bike_walking_test():
    assert not has_bss_first_and_bss_last(get_bike_walking_journey())

def has_bss_first_and_bss_last__bike_bike_test():
    assert not has_bss_first_and_bss_last(get_bike_bike_journey())

def has_bss_first_and_bss_last__bike_bss_test():
    assert not has_bss_first_and_bss_last(get_bike_bss_journey())

def has_bss_first_and_bss_last__bike_car_test():
    assert not has_bss_first_and_bss_last(get_bike_car_journey())

#==============================================================================
# has_bike_first_and_walking_last test
#==============================================================================
def has_bike_first_and_walking_last__walking_walking_test():
    assert not has_bike_first_and_walking_last(get_walking_walking_journey())

def has_bike_first_and_walking_last__walking_bike_test():
    assert not has_bike_first_and_walking_last(get_walking_bike_journey())

def has_bike_first_and_walking_last__walking_bss_test():
    assert not has_bike_first_and_walking_last(get_walking_bss_journey())

def has_bike_first_and_walking_last__walking_car_test():
    assert not has_bike_first_and_walking_last(get_walking_car_journey())

def has_bike_first_and_walking_last__bss_walking_test():
    assert not has_bike_first_and_walking_last(get_bss_walking_journey())

def has_bike_first_and_walking_last__bss_bike_test():
    assert not has_bike_first_and_walking_last(get_bss_bike_journey())

def has_bike_first_and_walking_last__bss_bss_test():
    assert not has_bike_first_and_walking_last(get_bss_bss_journey())

def has_bike_first_and_walking_last__bss_car_test():
    assert not has_bike_first_and_walking_last(get_bss_car_journey())

def has_bike_first_and_walking_last__bike_walking_test():
    assert has_bike_first_and_walking_last(get_bike_walking_journey())

def has_bike_first_and_walking_last__bike_bike_test():
    assert not has_bike_first_and_walking_last(get_bike_bike_journey())

def has_bike_first_and_walking_last__bike_bss_test():
    assert not has_bike_first_and_walking_last(get_bike_bss_journey())

def has_bike_first_and_walking_last__bike_car_test():
    assert not has_bike_first_and_walking_last(get_bike_car_journey())

#==============================================================================
# has_bike_first_and_bss_last test
#==============================================================================
def has_bike_first_and_bss_last__walking_walking_test():
    assert not has_bike_first_and_bss_last(get_walking_walking_journey())

def has_bike_first_and_bss_last__walking_bike_test():
    assert not has_bike_first_and_bss_last(get_walking_bike_journey())

def has_bike_first_and_bss_last__walking_bss_test():
    assert not has_bike_first_and_bss_last(get_walking_bss_journey())

def has_bike_first_and_bss_last__walking_car_test():
    assert not has_bike_first_and_bss_last(get_walking_car_journey())

def has_bike_first_and_bss_last__bss_walking_test():
    assert not has_bike_first_and_bss_last(get_bss_walking_journey())

def has_bike_first_and_bss_last__bss_bike_test():
    assert not has_bike_first_and_bss_last(get_bss_bike_journey())

def has_bike_first_and_bss_last__bss_bss_test():
    assert not has_bike_first_and_bss_last(get_bss_bss_journey())

def has_bike_first_and_bss_last__bss_car_test():
    assert not has_bike_first_and_bss_last(get_bss_car_journey())

def has_bike_first_and_bss_last__bike_walking_test():
    assert not has_bike_first_and_bss_last(get_bike_walking_journey())

def has_bike_first_and_bss_last__bike_bike_test():
    assert not has_bike_first_and_bss_last(get_bike_bike_journey())

def has_bike_first_and_bss_last__bike_bss_test():
    assert has_bike_first_and_bss_last(get_bike_bss_journey())

def has_bike_first_and_bss_last__bike_car_test():
    assert not has_bike_first_and_bss_last(get_bike_car_journey())


def bike_duration_walking_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    eq_(bike_duration(journey), 0)

def bike_duration_bike_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 70

    eq_(bike_duration(journey), 130)


def bike_duration_bss_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 30
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 5

    eq_(bike_duration(journey), 5)

def car_duration_walking_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    eq_(car_duration(journey), 0)

def car_duration_car_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 70

    eq_(car_duration(journey), 60)


def car_duration_bss_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 30
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 180
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10

    eq_(car_duration(journey), 180)

def pt_duration_walking_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    eq_(pt_duration(journey), 80)

def pt_duration_bike_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.TRANSFER
    section.duration = 20
    section = journey.sections.add()
    section.type = response_pb2.WAITING
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 40
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 70

    eq_(pt_duration(journey), 120)


def pt_duration_bss_test():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 30
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 5

    eq_(pt_duration(journey), 80)
