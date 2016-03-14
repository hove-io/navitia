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

from nose.tools import eq_

from jormungandr.scenarios import default
import navitiacommon.response_pb2 as response_pb2
from jormungandr.utils import str_to_time_stamp


class Instance(object):
    def __init__(self):
        self.factor_too_long_journey = 4
        self.min_duration_too_long_journey = 60
        self.max_duration_criteria = 'time'
        self.max_duration_fallback_mode = 'walking'

def find_max_duration_clockwise_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()

    journey1.type = "best"
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 240
    section = journey1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey1.duration = sum([section.duration for section in journey1.sections])
    journey1.arrival_date_time = 1418222071
    journey1.departure_date_time = journey1.arrival_date_time - journey1.duration

    journey2 = response.journeys.add()
    journey2.type = "rapid"
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey2.duration = sum([section.duration for section in journey2.sections])
    journey2.arrival_date_time = 1418220071
    journey2.departure_date_time = journey2.arrival_date_time - journey2.duration

    journey3 = response.journeys.add()
    journey3.type = "non_pt_walk"
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 130
    journey3.duration = sum([section.duration for section in journey3.sections])
    journey3.arrival_date_time = 1418211071
    journey3.departure_date_time = journey3.arrival_date_time - journey3.duration

    scenario = default.Scenario()
    eq_(scenario._find_max_duration(response.journeys, Instance(), True), 580)
    scenario._delete_too_long_journey(response, Instance(), True)
    eq_(len(response.journeys), 2)
    eq_(response.journeys[0], journey2)
    eq_(response.journeys[1], journey3)

def find_max_duration__counterclockwise_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()

    journey1.type = "best"
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 240
    section = journey1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey1.duration = sum([section.duration for section in journey1.sections])
    journey1.arrival_date_time = 1418211071
    journey1.departure_date_time = journey1.arrival_date_time - journey1.duration

    journey2 = response.journeys.add()
    journey2.type = "rapid"
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey2.duration = sum([section.duration for section in journey2.sections])
    journey2.arrival_date_time = 1418220071
    journey2.departure_date_time = journey2.arrival_date_time - journey2.duration

    journey3 = response.journeys.add()
    journey3.type = "non_pt_walk"
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 130
    journey3.duration = sum([section.duration for section in journey3.sections])
    journey3.arrival_date_time = 1418222071
    journey3.departure_date_time = journey3.arrival_date_time - journey3.duration

    scenario = default.Scenario()
    eq_(scenario._find_max_duration(response.journeys, Instance(), False), 580)
    scenario._delete_too_long_journey(response, Instance(), False)
    eq_(len(response.journeys), 2)
    eq_(response.journeys[0], journey2)
    eq_(response.journeys[1], journey3)


def find_max_duration_clockwise_test():
    """
    we don't have a journey with walking so max_duration is None, and we keep all journeys
    """
    response = response_pb2.Response()
    journey1 = response.journeys.add()

    journey1.type = "best"
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 240
    section = journey1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey1.duration = sum([section.duration for section in journey1.sections])
    journey1.arrival_date_time = 1418222071
    journey1.departure_date_time = journey1.arrival_date_time - journey1.duration

    journey2 = response.journeys.add()
    journey2.type = "rapid"
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 120
    journey2.duration = sum([section.duration for section in journey2.sections])
    journey2.arrival_date_time = 1418220071
    journey2.departure_date_time = journey2.arrival_date_time - journey2.duration

    scenario = default.Scenario()
    eq_(scenario._find_max_duration(response.journeys, Instance(), True), None)
    scenario._delete_too_long_journey(response, Instance(), True)
    eq_(len(response.journeys), 2)
    eq_(response.journeys[0], journey1)
    eq_(response.journeys[1], journey2)
