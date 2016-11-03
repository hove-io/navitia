# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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
from datetime import timedelta
from .tests_mechanism import config
from jormungandr.scenarios.qualifier import min_from_criteria
from .journey_common_tests import *
from unittest import skip
from .routing_tests import OnBasicRouting

'''
This unit runs all the common tests in journey_common_tests.py along with locals tests added in this
unit for scenario experimental
'''

@config({'scenario': 'experimental'})
class TestJourneysExperimental(JourneyCommon, DirectPath, AbstractTestFixture):
    """
    Test the experiental scenario
    All the tests are defined in "TestJourneys" class, we only change the scenario


    NOTE: for the moment we cannot import all routing tests, so we only get 2, but we need to add some more
    """

    @staticmethod
    def check_next_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default next behaviour is 1s after the best or the soonest"""
        j_to_compare = min_from_criteria(generate_pt_journeys(response),
                                         new_default_pagination_journey_comparator(clockwise=clockwise))

        j_departure = get_valid_datetime(j_to_compare['departure_date_time'])
        eq_(j_departure + timedelta(seconds=1), dt)

    @staticmethod
    def check_previous_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default previous behaviour is 1s before the best or the latest """
        j_to_compare = min_from_criteria(generate_pt_journeys(response),
                                         new_default_pagination_journey_comparator(clockwise=clockwise))

        j_departure = get_valid_datetime(j_to_compare['arrival_date_time'])
        eq_(j_departure - timedelta(seconds=1), dt)

    def test_best_filtering(self):
        """
        This feature is no longer supported"""
        pass

    def test_datetime_represents_arrival(self):
        super(TestJourneysExperimental, self).test_datetime_represents_arrival()

    def test_journeys_wheelchair_profile(self):
        """
        This feature is no longer supported
        """
        pass

    def test_not_existent_filtering(self):
        """
        This feature is no longer supported
        """
        pass

    def test_other_filtering(self):
        """
        This feature is no longer supported
        """
        pass


@config({"scenario": "experimental"})
class TestExperimentalJourneysWithPtref(JourneysWithPtref, AbstractTestFixture):
    pass


@config({"scenario": "experimental"})
class TestExperimentalOnBasicRouting(OnBasicRouting, AbstractTestFixture):

    @skip("temporarily disabled")
    def test_sp_to_sp(self):
        super(OnBasicRouting, self).test_sp_to_sp()

    @skip("temporarily disabled")
    def test_isochrone(self):
        super(OnBasicRouting, self).test_isochrone()
