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
import datetime
from jormungandr.scenarios import default
from jormungandr.scenarios.tests.protobuf_builder import ResponseBuilder
from jormungandr.utils import str_to_time_stamp


class Instance(object):
    def __init__(self):
        self.factor_too_long_journey = 4
        self.min_duration_too_long_journey = 60
        self.max_duration_criteria = 'time'
        self.max_duration_fallback_mode = 'walking'


def find_max_duration_clockwise_test():
    resp_builder = ResponseBuilder()\
        .journey(sections=[
            {'mode': 'Walking', 'duration': 240},
            {'type': 'PT', 'duration': 300},
            {'mode': 'Walking', 'duration': 120},
        ], type='best', arrival='T1501')\
        .journey(uri='rapid', sections=[
            {'mode': 'Walking', 'duration': 120},
            {'type': 'PT', 'duration': 300},
            {'mode': 'Walking', 'duration': 120},
        ], type='rapid', arrival='T1501')\
        .journey(uri='non pt', sections=[
            {'mode': 'Walking', 'duration': 130},
        ], type='non_pt_walk', arrival='T1231')

    response = resp_builder.response

    scenario = default.Scenario()
    assert scenario._find_max_duration(response.journeys, Instance(), True) == 580
    scenario._delete_too_long_journey(response, Instance(), True)
    assert len(response.journeys) == 2
    assert response.journeys[0] == resp_builder.get_journey('rapid')
    assert response.journeys[1] == resp_builder.get_journey('non pt')


def find_max_duration__counterclockwise_test():
    resp_builder = ResponseBuilder()\
        .journey(sections=[
            {'mode': 'Walking', 'duration': 240},
            {'type': 'PT', 'duration': 300},
            {'mode': 'Walking', 'duration': 120},
        ], type='best', arrival='T1231')\
        .journey(uri='rapid', sections=[
            {'mode': 'Walking', 'duration': 120},
            {'type': 'PT', 'duration': 300},
            {'mode': 'Walking', 'duration': 120},
        ], type='rapid', arrival='T1501')\
        .journey(uri='non pt', sections=[
            {'mode': 'Walking', 'duration': 130},
        ], type='non_pt_walk', arrival='T1534')

    response = resp_builder.response

    scenario = default.Scenario()
    assert scenario._find_max_duration(response.journeys, Instance(), False) == 580
    scenario._delete_too_long_journey(response, Instance(), False)
    assert len(response.journeys) == 2
    assert response.journeys[0] == resp_builder.get_journey('rapid')
    assert response.journeys[1] == resp_builder.get_journey('non pt')


def find_max_duration_clockwise_no_walk_test():
    """
    we don't have a journey with walking so max_duration is None, and we keep all journeys
    """
    resp_builder = ResponseBuilder()\
        .journey(uri='best', sections=[
            {'mode': 'Bike', 'duration': 240},
            {'type': 'PT', 'duration': 300},
            {'mode': 'Walking', 'duration': 120},
        ], type='best', arrival='T1534')\
        .journey(uri='rapid', sections=[
            {'mode': 'Walking', 'duration': 120},
            {'type': 'PT', 'duration': 300},
            {'mode': 'Bike', 'duration': 120},
        ], type='rapid', arrival='T1534')
    response = resp_builder.response

    scenario = default.Scenario()
    assert scenario._find_max_duration(response.journeys, Instance(), True) == None
    scenario._delete_too_long_journey(response, Instance(), True)
    assert len(response.journeys) == 2
    assert response.journeys[0] == resp_builder.get_journey('best')
    assert response.journeys[1] == resp_builder.get_journey('rapid')


def next_journey_test():
    """ In the default scenario, the next journey is one minute after the first 'rapid' if we get one"""
    builder = ResponseBuilder(default_date=datetime.date(2016, 10, 10))\
        .journey(type='rapid', departure='T1200', arrival='T1500')\
        .journey(type='fastest', departure='T1100', arrival='T1700')\
        .journey(type='non_pt_walk', departure='T1000', arrival='T1534')\
        .journey(type='car', departure='T1300', arrival='T1534')

    scenario = default.Scenario()
    assert scenario.next_journey_datetime(builder.get_journeys()) == str_to_time_stamp('20161010T120100')


def next_journey_test_no_rapid_test():
    """ In the default scenario, if we don't get a rapid,
    the next journey is one minute after the earliest journey
    """
    builder = ResponseBuilder(default_date=datetime.date(2016, 10, 10))\
        .journey(type='fastest', departure='T1100', arrival='T1700')\
        .journey(type='non_pt_walk', departure='T1000', arrival='T1534')\
        .journey(type='car', departure='T2000', arrival='T1534')

    scenario = default.Scenario()
    assert scenario.next_journey_datetime(builder.get_journeys()) == str_to_time_stamp('20161010T100100')


def previous_journey_test():
    """ In the default scenario, the previous journey is one minute before the first 'rapid' if we get one"""
    builder = ResponseBuilder(default_date=datetime.date(2016, 10, 10))\
        .journey(type='rapid', departure='T1200', arrival='T1500')\
        .journey(type='fastest', departure='T1100', arrival='T1700')\
        .journey(type='non_pt_walk', departure='T1000', arrival='T1534')\
        .journey(type='car', departure='T1300', arrival='T1534')

    scenario = default.Scenario()
    assert scenario.previous_journey_datetime(builder.get_journeys()) == str_to_time_stamp('20161010T145900')


def previous_journey_no_rapid_test():
    """ In the default scenario, if we don't get a rapid,
    the previous journey is one minute before the tardiest journey
    """
    builder = ResponseBuilder(default_date=datetime.date(2016, 10, 10))\
        .journey(type='fastest', departure='T1100', arrival='T1700')\
        .journey(type='non_pt_walk', departure='T1000', arrival='T1534')\
        .journey(type='car', departure='T2000', arrival='T1534')

    scenario = default.Scenario()
    assert scenario.previous_journey_datetime(builder.get_journeys()) == str_to_time_stamp('20161010T165900')
