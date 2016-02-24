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
import logging

from tests_mechanism import AbstractTestFixture, dataset
from routing_tests import TestJourneys
from check_utils import *
from nose.tools import eq_
import jormungandr.scenarios.destineo
from jormungandr.instance import Instance

@dataset({"main_routing_test": {}})
class TestJourneysDestineo(TestJourneys):
    """
    Test the structure of the journeys response
    All the tests are defined in "TestJourneys" class, we only change the scenario
    if needed we can override the tests in this class
    """

    def setup(self):
        logging.debug('setup for destineo')
        from jormungandr import i_manager
        dest_instance = i_manager.instances['main_routing_test']
        self.old_scenario = dest_instance._scenario
        dest_instance._scenario = jormungandr.scenarios.destineo.Scenario()

    def teardown(self):
        from jormungandr import i_manager
        i_manager.instances['main_routing_test']._scenario = self.old_scenario


    def test_journeys(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        response = self.query_region(journey_basic_query)

        is_valid_journey_response(response, self.tester, journey_basic_query)
        eq_(len(response['journeys']), 2)
        eq_(response['journeys'][0]['type'], 'best')
        eq_(response['journeys'][1]['type'], 'non_pt_walk')

    def test_journeys_destineo_with_bss(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = journey_basic_query + "&first_section_mode=bss&first_section_mode=bike&first_section_mode=car" \
                "&last_section_mode=bss&last_section_mode=car"
        response = self.query_region(query)

        is_valid_journey_response(response, self.tester, query)
        logging.debug([j['type'] for j in response['journeys']])
        eq_(len(response['journeys']), 4)
        eq_(response['journeys'][0]['type'], 'best')
        eq_(response['journeys'][1]['type'], 'non_pt_bss')
        eq_(response['journeys'][2]['type'], 'non_pt_walk')
        eq_(response['journeys'][3]['type'], 'non_pt_bike')
