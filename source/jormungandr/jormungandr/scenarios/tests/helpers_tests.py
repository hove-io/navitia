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

from navitiacommon import response_pb2
from nose.tools import eq_
from jormungandr.scenarios.helpers import *

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


def bike_duration_test():
    eq_(bike_duration(get_walking_walking_journey()), 0)

    eq_(bike_duration(get_walking_bike_journey()), 40)

    eq_(bike_duration(get_walking_bss_journey()), 0)

    eq_(bike_duration(get_walking_car_journey()), 0)

    eq_(bike_duration(get_bss_walking_journey()), 0)

    eq_(bike_duration(get_bss_bike_journey()), 200)

    eq_(bike_duration(get_bss_bss_journey()), 0)

    eq_(bike_duration(get_bss_car_journey()), 0)

    eq_(bike_duration(get_bike_walking_journey()), 55)

    eq_(bike_duration(get_bike_bike_journey()), 30)

    eq_(bike_duration(get_bike_bss_journey()), 25)

    eq_(bike_duration(get_bike_car_journey()), 25)

def car_duration_walking_test():
    eq_(car_duration(get_walking_walking_journey()), 0)

    eq_(car_duration(get_walking_bike_journey()), 0)

    eq_(car_duration(get_walking_bss_journey()), 0)

    eq_(car_duration(get_walking_car_journey()), 70)

    eq_(car_duration(get_bss_walking_journey()), 0)

    eq_(car_duration(get_bss_bike_journey()), 0)

    eq_(car_duration(get_bss_bss_journey()), 0)

    eq_(car_duration(get_bss_car_journey()), 70)

    eq_(car_duration(get_bike_walking_journey()), 0)

    eq_(car_duration(get_bike_bike_journey()), 0)

    eq_(car_duration(get_bike_bss_journey()), 0)

    eq_(car_duration(get_bike_car_journey()), 30)


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

def walking_duration_test():
    eq_(walking_duration(get_walking_walking_journey()), 120)

    eq_(walking_duration(get_walking_bike_journey()), 30)

    eq_(walking_duration(get_walking_bss_journey()), 75)

    eq_(walking_duration(get_walking_car_journey()), 15)

    eq_(walking_duration(get_bss_walking_journey()), 40)

    eq_(walking_duration(get_bss_bike_journey()), 40)

    eq_(walking_duration(get_bss_bss_journey()), 70)

    eq_(walking_duration(get_bss_car_journey()), 75)

    eq_(walking_duration(get_bike_walking_journey()), 7)

    eq_(walking_duration(get_bike_bike_journey()), 0)

    eq_(walking_duration(get_bike_bss_journey()), 10)

    eq_(walking_duration(get_bike_car_journey()), 5)

def bss_duration_test():
    eq_(bss_duration(get_walking_walking_journey()), 0)

    eq_(bss_duration(get_walking_bike_journey()), 0)

    eq_(bss_duration(get_walking_bss_journey()), 85)

    eq_(bss_duration(get_walking_car_journey()), 0)

    eq_(bss_duration(get_bss_walking_journey()), 45)

    eq_(bss_duration(get_bss_bike_journey()), 85)

    eq_(bss_duration(get_bss_bss_journey()), 100)

    eq_(bss_duration(get_bss_car_journey()), 25)

    eq_(bss_duration(get_bike_walking_journey()), 0)

    eq_(bss_duration(get_bike_bike_journey()), 0)

    eq_(bss_duration(get_bike_bss_journey()), 70)

    eq_(bss_duration(get_bike_car_journey()), 0)

def fallback_mode_sort_test():
    l = ['car', 'walking']
    l.sort(fallback_mode_comparator)
    eq_(l[0], 'walking')
    eq_(l[1], 'car')

    l = ['bss', 'bike', 'car', 'walking']
    l.sort(fallback_mode_comparator)
    eq_(l[0], 'walking')
    eq_(l[1], 'bss')
    eq_(l[2], 'bike')
    eq_(l[3], 'car')
