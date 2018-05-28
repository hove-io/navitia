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
from .tests_mechanism import AbstractTestFixture, config
from .check_utils import *
from .journey_common_tests import *


'''
This unit runs all the common tests in journey_common_tests.py.
'''

@config()
class TestJourneysDefault(JourneyCommon, AbstractTestFixture):

    def test_max_duration_equals_to_0(self):
        query = journey_basic_query + \
            "&first_section_mode[]=bss" + \
            "&first_section_mode[]=walking" + \
            "&first_section_mode[]=bike" + \
            "&first_section_mode[]=car" + \
            "&debug=true"
        response = self.query_region(query)
        check_best(response)
        assert len(response['journeys']) == 4

        query += "&max_duration=0"
        response = self.query_region(query)
        # the pt journey is eliminated
        assert len(response['journeys']) == 3

        # first is bike
        assert('non_pt_bike' == response['journeys'][0]['type'])
        assert len(response['journeys'][0]['sections']) == 1

        # second is empty
        assert('' == response['journeys'][1]['type'])
        assert len(response['journeys'][1]['sections']) == 3

        # last is bss
        assert('non_pt_bss' == response['journeys'][2]['type'])
        assert len(response['journeys'][-1]['sections']) == 5

    def test_journey_stop_area_to_stop_point(self):
        """
        When the departure is stop_area:A and the destination is stop_point:B belonging to stop_area:B
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa='stopA', to_sa='stop_point:stopB', datetime="20120614T080000")
        response = self.query_region(query)
        check_best(response)
        jrnys = response['journeys']

        j = next((j for j in jrnys if j['type'] == 'non_pt_walk'), None)
        assert j
        assert j['sections'][0]['from']['id'] == 'stopA'
        assert j['sections'][0]['to']['id'] == 'stop_point:stopB'
        assert j['type'] == 'non_pt_walk'

    def test_min_nb_journeys_with_night_bus_filter(self):
        """
        This new feature is not supported in default
        """
        pass

    def test_shared_section(self):
        """
        This new feature is not supported in default
        """
        pass

@config()
class TestJourneysNoRegionDefault(JourneysNoRegion, AbstractTestFixture):
    pass


@config()
class TestOnBasicRoutingDefault(OnBasicRouting, AbstractTestFixture):

    def test_novalidjourney_on_first_call_debug(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        We had a debug argument, hence 2 journeys are returned, only one is typed
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['arrival_date_time'] == "20120614T150000"
        assert response['journeys'][0]['type'] == ""
        assert response['journeys'][1]['arrival_date_time'] == "20120614T160000"
        assert response['journeys'][1]['type'] == "best"

@config()
class TestOneDeadRegionDefault(OneDeadRegion, AbstractTestFixture):
    pass


@config()
class TestWithoutPtDefault(WithoutPt, AbstractTestFixture):
    pass


@config()
class TestJourneysWithPtrefDefault(JourneysWithPtref, AbstractTestFixture):
    pass
