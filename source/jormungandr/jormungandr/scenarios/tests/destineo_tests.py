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
from copy import deepcopy

from nose.tools import eq_

from jormungandr.scenarios import destineo
from jormungandr.scenarios.tests.protobuf_builder import ResponseBuilder
import navitiacommon.response_pb2 as response_pb2
from jormungandr.utils import str_to_time_stamp


def get_next_vj_name():
    get_next_vj_name.section_counter += 1
    return 'vj_{}'.format(get_next_vj_name.section_counter)
get_next_vj_name.section_counter = 0  # to generate different section names


def is_pure_tc_simple_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert destineo.is_pure_tc(journey)

def is_pure_tc_bss_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
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
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.is_pure_tc(journey)


def is_pure_tc_crowfly_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert destineo.is_pure_tc(journey)


def has_bike_and_tc_simple_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.has_bike_and_tc(journey)

def has_bike_and_tc_bss_test():
    resp_builder = ResponseBuilder().journey(sections=[
        {'mode': 'Walking'}, {'type': 'BSS_RENT'}, {'mode': 'Bike'}, {'type': 'BSS_PUT_BACK'},
        {'type': 'PT'},
        {'mode': 'Walking'},
    ])
    assert not destineo.has_bike_and_tc(resp_builder.response.journeys[0])

def has_bike_and_tc_crowfly_test():
    resp_builder = ResponseBuilder().journey(sections=[
        {'mode': 'Bike', 'type': 'CROW_FLY'},
        {'type': 'PT'},
        {'mode': 'Walking'},
    ])
    assert destineo.has_bike_and_tc(resp_builder.response.journeys[0])

def has_bss_and_tc_simple_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.has_bss_and_tc(journey)

def has_bss_and_tc_bss_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
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
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert destineo.has_bss_and_tc(journey)


def has_bss_and_tc_crowfly_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.has_bss_and_tc(journey)

def has_bss_and_tc_crowfly_bss_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Bss
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.has_bss_and_tc(journey)

def has_car_and_tc_simple_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.has_car_and_tc(journey)

def has_car_and_tc_bss_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
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
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.has_car_and_tc(journey)


def has_car_and_tc_crowfly_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Car
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert destineo.has_car_and_tc(journey)

def has_car_and_tc_crowfly_bss_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Bss
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    assert not destineo.has_car_and_tc(journey)

def sort_destineo_test():
    response = response_pb2.Response()

    journey_tc3 = response.journeys.add()
    journey_tc3.type = "rapid"
    journey_tc3.departure_date_time = str_to_time_stamp('20141103T123000')
    section = journey_tc3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_tc3.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_tc3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_tc1 = response.journeys.add()
    journey_tc1.type = "rapid"
    journey_tc1.departure_date_time = str_to_time_stamp('20141103T110000')
    section = journey_tc1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_tc1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_tc1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_bike = response.journeys.add()
    journey_bike.departure_date_time = str_to_time_stamp('20141103T110000')
    journey_bike.type = "non_pt_bike"

    journey_bss = response.journeys.add()
    journey_bss.departure_date_time = str_to_time_stamp('20141103T110000')
    journey_bss.type = "non_pt_bss"
    section = journey_bss.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_bss.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey_bss.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey_bss.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey_bss.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_walk = response.journeys.add()
    journey_walk.departure_date_time = str_to_time_stamp('20141103T110000')
    journey_walk.type = "non_pt_walk"

    journey_tc2 = response.journeys.add()
    journey_tc2.type = "rapid"
    journey_tc2.departure_date_time = str_to_time_stamp('20141103T120000')
    section = journey_tc2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_tc2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_tc2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_bss_and_tc = response.journeys.add()
    journey_bss_and_tc.departure_date_time = str_to_time_stamp('20141103T120000')
    journey_bss_and_tc.type = "rapid"
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_bike_and_tc = response.journeys.add()
    journey_bike_and_tc.departure_date_time = str_to_time_stamp('20141103T120000')
    journey_bike_and_tc.type = "rapid"
    section = journey_bike_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey_bike_and_tc.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_bike_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_car_and_tc = response.journeys.add()
    journey_car_and_tc.departure_date_time = str_to_time_stamp('20141103T120000')
    journey_car_and_tc.type = "rapid"
    section = journey_car_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section = journey_car_and_tc.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_car_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    scenario = destineo.Scenario()
    scenario._custom_sort_journeys(response, clockwise=True, timezone='Africa/Abidjan')
    eq_(response.journeys[0], journey_tc1)
    eq_(response.journeys[1], journey_tc2)
    eq_(response.journeys[2], journey_tc3)
    eq_(response.journeys[3], journey_bss)
    eq_(response.journeys[4], journey_bss_and_tc)
    eq_(response.journeys[5], journey_bike_and_tc)
    eq_(response.journeys[6], journey_car_and_tc)
    eq_(response.journeys[7], journey_walk)
    eq_(response.journeys[8], journey_bike)


def sort_destineo_date_test():
    """
    the bss journey must be first since it his on first day
    """
    response = response_pb2.Response()

    journey_tc3 = response.journeys.add()
    journey_tc3.type = "rapid"
    journey_tc3.departure_date_time = str_to_time_stamp('20141103T123000')
    section = journey_tc3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_tc3.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_tc3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_tc1 = response.journeys.add()
    journey_tc1.type = "rapid"
    journey_tc1.departure_date_time = str_to_time_stamp('20141103T110000')
    section = journey_tc1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_tc1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_tc1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking


    journey_bss_and_tc = response.journeys.add()
    journey_bss_and_tc.departure_date_time = str_to_time_stamp('20141102T120000')
    journey_bss_and_tc.type = "rapid"
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking


    scenario = destineo.Scenario()
    scenario._custom_sort_journeys(response, clockwise=True, timezone='Africa/Abidjan')
    eq_(response.journeys[0], journey_bss_and_tc)
    eq_(response.journeys[1], journey_tc1)
    eq_(response.journeys[2], journey_tc3)


def sort_destineo_date_timezone_test():
    response = response_pb2.Response()

    journey_tc3 = response.journeys.add()
    journey_tc3.type = "rapid"
    journey_tc3.departure_date_time = str_to_time_stamp('20141104T113000')
    section = journey_tc3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_tc3.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_tc3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    journey_tc1 = response.journeys.add()
    journey_tc1.type = "rapid"
    journey_tc1.departure_date_time = str_to_time_stamp('20141104T110000')
    section = journey_tc1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_tc1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_tc1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    #at paris it's 20141104T003000 so it's the same day
    journey_bss_and_tc = response.journeys.add()
    journey_bss_and_tc.departure_date_time = str_to_time_stamp('20141103T233000')
    journey_bss_and_tc.type = "rapid"
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.BSS_RENT
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey_bss_and_tc.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking


    scenario = destineo.Scenario()
    scenario._custom_sort_journeys(response, clockwise=True, timezone='Europe/Paris')
    eq_(response.journeys[0], journey_tc1)
    eq_(response.journeys[1], journey_tc3)
    eq_(response.journeys[2], journey_bss_and_tc)


class Instance(object):
    def __init__(self):
        self.min_bike = 30
        self.min_bss = 25
        self.min_car = 20
        self.min_tc_with_bike = 40
        self.min_tc_with_bss = 35
        self.min_tc_with_car = 50

def remove_not_long_enough_no_removal_test():
    response = response_pb2.Response()

    journey = response.journeys.add()
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

    journey = response.journeys.add()
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
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

    scenario = destineo.Scenario()
    scenario._remove_not_long_enough_fallback(response, Instance())
    eq_(len(response.journeys), 2)
    scenario._remove_not_long_enough_tc_with_fallback(response, Instance())
    eq_(len(response.journeys), 2)

def remove_not_long_enough_bss_test():
    response = response_pb2.Response()

    journey1 = response.journeys.add()
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    journey2 = response.journeys.add()
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey2.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 14
    section = journey2.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    journey3 = response.journeys.add()
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey3.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 34
    section = journey3.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 16
    section = journey3.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    journey4 = response.journeys.add()
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey4.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 36
    section = journey4.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 16
    section = journey4.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    scenario = destineo.Scenario()
    scenario._remove_not_long_enough_fallback(response, Instance())
    eq_(len(response.journeys), 3)
    eq_(response.journeys[0], journey1)
    eq_(response.journeys[1], journey3)
    eq_(response.journeys[2], journey4)
    scenario._remove_not_long_enough_tc_with_fallback(response, Instance())
    eq_(len(response.journeys), 2)
    eq_(response.journeys[0], journey1)
    eq_(response.journeys[1], journey4)


def remove_not_long_enough_bike_test():
    response = response_pb2.Response()

    journey = response.journeys.add()
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

    journey2 = response.journeys.add()
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 29

    journey3 = response.journeys.add()
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey3.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 39
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 30

    journey4 = response.journeys.add()
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey4.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 40
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 30

    scenario = destineo.Scenario()
    scenario._remove_not_long_enough_fallback(response, Instance())
    eq_(len(response.journeys), 3)
    eq_(response.journeys[0], journey)
    eq_(response.journeys[1], journey3)
    eq_(response.journeys[2], journey4)
    scenario._remove_not_long_enough_tc_with_fallback(response, Instance())
    eq_(len(response.journeys), 2)
    eq_(response.journeys[0], journey)
    eq_(response.journeys[1], journey4)


def remove_not_long_enough_car_test():
    response = response_pb2.Response()

    journey = response.journeys.add()
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

    journey2 = response.journeys.add()
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 19

    journey3 = response.journeys.add()
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey3.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 49
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 20

    journey4 = response.journeys.add()
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey4.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 50
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 20

    scenario = destineo.Scenario()
    scenario._remove_not_long_enough_fallback(response, Instance())
    eq_(len(response.journeys), 3)
    eq_(response.journeys[0], journey)
    eq_(response.journeys[1], journey3)
    eq_(response.journeys[2], journey4)
    scenario._remove_not_long_enough_tc_with_fallback(response, Instance())
    eq_(len(response.journeys), 2)
    eq_(response.journeys[0], journey)
    eq_(response.journeys[1], journey4)


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


def generate_journey(vj_names):
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    for name in vj_names:
        section = journey.sections.add()
        section.type = response_pb2.PUBLIC_TRANSPORT
        section.uris.vehicle_journey = name

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

def get_bike_car_journey(random_vjs=True):
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    if random_vjs:
        section.uris.vehicle_journey = get_next_vj_name()
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

def get_bss_walking_journey(random_vjs=True):
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
    if random_vjs:
        section.uris.vehicle_journey = get_next_vj_name()
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking

    return journey

def get_bss_bike_journey(random_vjs=True):
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
    if random_vjs:
        section.uris.vehicle_journey = get_next_vj_name()
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike

    return journey

def get_bss_bss_journey(random_vjs=True):
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
    if random_vjs:
        section.uris.vehicle_journey = get_next_vj_name()
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

def get_bss_car_journey(random_vjs=True):
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
    if random_vjs:
        section.uris.vehicle_journey = get_next_vj_name()
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

def choose_best_alternatives_simple_test():
    journeys = [get_bss_bss_journey(), get_bike_car_journey(), get_bss_walking_journey()]
    saved_journeys = deepcopy(journeys)
    scenario = destineo.Scenario()
    scenario._choose_best_alternatives(journeys)
    eq_(len(journeys), 1)
    eq_(journeys[0], saved_journeys[2])

def choose_best_alternatives__bike_bss_test():
    journeys = [get_bike_bss_journey(), get_bike_car_journey()]
    saved_journeys = deepcopy(journeys)
    scenario = destineo.Scenario()
    scenario._choose_best_alternatives(journeys)
    eq_(len(journeys), 1)
    eq_(journeys[0], saved_journeys[0])

def choose_best_alternatives__car_test():
    journeys = [get_bike_car_journey()]
    saved_journeys = deepcopy(journeys)
    scenario = destineo.Scenario()
    scenario._choose_best_alternatives(journeys)
    eq_(len(journeys), 1)
    eq_(journeys[0], saved_journeys[0])

def choose_best_alternatives_non_pt_test():
    journeys = [get_bss_bss_journey(), get_bike_car_journey(), get_bss_walking_journey()]
    j1 = response_pb2.Journey()
    j1.type = 'non_pt_bss'
    journeys.append(j1)

    j2 = response_pb2.Journey()
    j2.type = 'non_pt_bike'
    journeys.append(j2)

    saved_journeys = deepcopy(journeys)

    scenario = destineo.Scenario()
    scenario._choose_best_alternatives(journeys)
    eq_(len(journeys), 3)
    eq_(journeys[0], saved_journeys[2])
    eq_(journeys[1], j1)
    eq_(journeys[2], j2)

def remove_extra_journeys_less_test():
    journeys = [get_bss_bss_journey(), get_bike_car_journey()]

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, 3, clockwise=True, timezone='UTC')
    eq_(len(journeys), 2)

def remove_extra_journeys_enougth_test():
    journeys = [get_bss_bss_journey(), get_bike_car_journey()]

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, 2, clockwise=True, timezone='UTC')
    eq_(len(journeys), 2)

def remove_extra_journeys_more_test():
    journeys = [get_bss_bss_journey(), get_bike_car_journey(), get_bss_bike_journey()]
    saved_journeys = deepcopy(journeys)

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, None, clockwise=True, timezone='UTC')
    eq_(len(journeys), 3)

    scenario._remove_extra_journeys(journeys, 2, clockwise=True, timezone='UTC')

    eq_(len(journeys), 2)
    eq_(journeys[0], saved_journeys[0])
    eq_(journeys[1], saved_journeys[1])

def remove_extra_journeys_more_with_walking_last_test():
    journeys = [get_bss_bss_journey(), get_bike_car_journey(), get_bss_bike_journey()]
    j1 = response_pb2.Journey()
    j1.type = 'non_pt_walk'
    journeys.append(j1)
    saved_journeys = deepcopy(journeys)

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, None, clockwise=True, timezone='UTC')
    eq_(len(journeys), 4)

    scenario._remove_extra_journeys(journeys, 2, clockwise=True, timezone='UTC')

    eq_(len(journeys), 3)
    eq_(journeys[0], saved_journeys[0])
    eq_(journeys[1], saved_journeys[1])
    eq_(journeys[2], j1)

def remove_extra_journeys_more_with_walking_first_test():
    j1 = response_pb2.Journey()
    j1.type = 'non_pt_walk'
    journeys = [j1, get_bss_bss_journey(), get_bike_car_journey(), get_bss_bike_journey()]
    saved_journeys = deepcopy(journeys)

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, None, clockwise=True, timezone='UTC')
    eq_(len(journeys), 4)

    scenario._remove_extra_journeys(journeys, 2, clockwise=True, timezone='UTC')

    eq_(len(journeys), 3)
    eq_(journeys[0], j1)
    eq_(journeys[1], saved_journeys[1])
    eq_(journeys[2], saved_journeys[2])


def remove_extra_journeys_similar_journey_latest_dep():
    j1 = response_pb2.Journey()
    j1.type = 'non_pt_walk'

    j_eq1 = generate_journey(['bob'])
    j_eq1.type = 'bob'  # used as an id in the test
    j_eq1.departure_date_time = 1425989999 # leave after, it's better
    j_eq1.arrival_date_time = 1425990000

    j_eq2 = generate_journey(['bob'])
    j_eq2.type = 'bobette'
    j_eq2.departure_date_time = 1425984855
    j_eq2.arrival_date_time = 1425990000

    journeys = [j1, j_eq1, j_eq2]

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, None, clockwise=True, timezone='UTC')
    eq_(len(journeys), 2)
    eq_(journeys[1].type, 'bob')


def remove_extra_journeys_not_similar_journeys():

    j_eq1 = generate_journey(['bob', 'bobette', '', 'bobitto'])
    j_eq1.type = 'bobitto'  # used as an id in the test

    j_eq2 = generate_journey(['bob', 'bobette', '', 'babitta'])
    j_eq2.type = 'babitta'

    journeys = [j_eq1, j_eq2]

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, None, clockwise=True, timezone='UTC')
    eq_(len(journeys), 2) #nothing filtered, they are not equivalent
    eq_(journeys[0].type, 'bobitto')
    eq_(journeys[1].type, 'babitta')


def remove_extra_journeys_max_and_similar():
    """
    test the filtering with a max and similar journeys

    bobitto1 is equivalent to bobitto2, but 2 has less fallback, to bobitto1 is removed

    we still has too much journey, so babitta is removed too
    """
    j1 = response_pb2.Journey()
    j1.type = 'non_pt_walk'

    j_eq1 = generate_journey(['bob', 'bobette', '', 'bobitto'])
    j_eq1.type = 'bobitto1'  # used as an id in the test
    j_eq1.sections[0].duration = 12

    j_eq2 = generate_journey(['bob', 'bobette', '', 'bobitto'])
    j_eq2.type = 'bobitto2'
    j_eq1.sections[0].duration = 24

    j_not_eq = generate_journey(['bob', 'bobette', '', 'babitta'])
    j_not_eq.type = 'babitta'

    journeys = [j_eq1, j_eq2, j_not_eq, j1]

    scenario = destineo.Scenario()
    scenario._remove_extra_journeys(journeys, 1, clockwise=True, timezone='UTC')
    eq_(len(journeys), 2)
    eq_(journeys[0].type, 'bobitto1')
    eq_(journeys[1].type, 'non_pt_walk')
