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
import logging
from datetime import timedelta

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *
import jormungandr.scenarios.destineo
from jormungandr.instance import Instance


def filter_prev_next_journeys(journeys):
    section_is_pt = lambda section: section['type'] in ("public_transport", "on_demand_transport")
    filter_journey = lambda journey: any(section_is_pt(section) for section in journey.get('sections', []))
    filter_journey_pure_tc = lambda journey: 'is_pure_tc' in journey['tags']

    list_journeys = filter(filter_journey_pure_tc, journeys)
    if not list_journeys:
        #if there is no pure tc journeys, we consider all journeys with TC
        list_journeys = filter(filter_journey, journeys)
    return list_journeys


@dataset({"main_routing_test": {"scenario": "destineo"}})
class TestJourneysDestineo(AbstractTestFixture):
    """
    Test the structure of the journeys response
    All the tests are defined in "TestJourneys" class, we only change the scenario
    if needed we can override the tests in this class
    """

    @staticmethod
    def check_next_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """destineo prev/next link mechanism is different"""
        j_to_compare = max(filter_prev_next_journeys(response.get('journeys', [])),
                           key=lambda j: get_valid_datetime(j['departure_date_time']))

        j_departure = get_valid_datetime(j_to_compare['departure_date_time'])
        assert j_departure + timedelta(minutes=1) == dt

    @staticmethod
    def check_previous_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """destineo prev/next link mechanism is different"""
        j_to_compare = min(filter_prev_next_journeys(response.get('journeys', [])),
                           key=lambda j: get_valid_datetime(j['arrival_date_time']))

        j_departure = get_valid_datetime(j_to_compare['arrival_date_time'])
        assert j_departure - timedelta(minutes=1) == dt

    def test_journeys(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        response = self.query_region(journey_basic_query)
        #not to use the jormungandr database

        self.is_valid_journey_response(response, journey_basic_query)
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['type'] == 'best'
        assert response['journeys'][1]['type'] == 'non_pt_walk'

    def test_journeys_destineo_with_bss(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = journey_basic_query + "&first_section_mode=bss&first_section_mode=bike&first_section_mode=car" \
                "&last_section_mode=bss&last_section_mode=car"
        response = self.query_region(query)

        self.is_valid_journey_response(response, query)
        logging.debug([j['type'] for j in response['journeys']])
        assert len(response['journeys']) == 4
        assert response['journeys'][0]['type'] == 'best'
        assert response['journeys'][1]['type'] == 'non_pt_bss'
        assert response['journeys'][2]['type'] == 'non_pt_walk'
        assert response['journeys'][3]['type'] == 'non_pt_bike'
