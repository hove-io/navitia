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
from .tests_mechanism import AbstractTestFixture
from datetime import timedelta
from .tests_mechanism import config
from .check_utils import *
from nose.tools import eq_
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

    @skip("temporarily disabled")
    def test_best_filtering(self):
        """Filter to get the best journey, we should have only one journey, the best one"""
        query = "{query}&type=best".format(query=journey_basic_query)
        response = self.query_region(query)
        check_journeys(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        assert response['journeys'][0]["type"] == "best"
        assert response['journeys'][0]['durations']['total'] == 99
        assert response['journeys'][0]['durations']['walking'] == 97

    @skip("temporarily disabled")
    def test_datetime_represents_arrival(self):
        """
        Checks journeys when datetime == start date of production datetime.
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "min_nb_journeys=3&_night_bus_filter_base_factor=86400&"\
                "datetime_represents=arrival"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T185500")
        response = self.query_region(query)
        check_journeys(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) >= 3

    @skip("temporarily disabled")
    def test_journeys_wheelchair_profile(self):
        """
        Test a query with a wheelchair profile.
        We want to go from S to R after 8h as usual, but between S and R, the first VJ is not accessible,
        so we have to wait for the bus at 18h to leave
        """

        response = self.query_region(journey_basic_query + "&traveler_type=wheelchair")
        assert(len(response['journeys']) == 2)
        #Note: we do not test order, because that can change depending on the scenario
        eq_(sorted(get_used_vj(response)), sorted([[], ['vjB']]))
        eq_(sorted(get_arrivals(response)), sorted(['20120614T080612', '20120614T180250']))

        # same response if we just give the wheelchair=True
        response = self.query_region(journey_basic_query + "&traveler_type=wheelchair&wheelchair=True")
        assert(len(response['journeys']) == 2)
        eq_(sorted(get_used_vj(response)), sorted([[], ['vjB']]))
        eq_(sorted(get_arrivals(response)), sorted(['20120614T080612', '20120614T180250']))

        # but with the wheelchair profile, if we explicitly accept non accessible solutions (not very
        # consistent, but anyway), we should take the non accessible bus that arrive at 08h
        response = self.query_region(journey_basic_query + "&traveler_type=wheelchair&wheelchair=False")
        assert(len(response['journeys']) == 2)
        eq_(sorted(get_used_vj(response)), sorted([['vjA'], []]))
        eq_(sorted(get_arrivals(response)), sorted(['20120614T080250', '20120614T080612']))

    @skip("temporarily disabled")
    def test_not_existent_filtering(self):
        """if we filter with a real type but not present, we don't get any journey, but we got a nice error"""

        response = self.query_region("{query}&type=car".
                                     format(query=journey_basic_query))

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert 'error' in response
        assert response['error']['id'] == 'no_solution'
        assert response['error']['message'] == 'No journey found, all were filtered'

    @skip("temporarily disabled")
    def test_other_filtering(self):
        """the basic query return a non pt walk journey and a best journey. we test the filtering of the non pt"""

        response = self.query_region("{query}&type=non_pt_walk".
                                     format(query=journey_basic_query))

        assert len(response['journeys']) == 1
        assert response['journeys'][0]["type"] == "non_pt_walk"

    @skip("temporarily disabled")
    def test_sp_to_sp(self):
        """
        Test journeys from stop point to stop point without street network
        """
        query = "journeys?from=stop_point:uselessA&to=stop_point:B&datetime=20120615T080000"

        # with street network desactivated
        response = self.query_region(query + "&max_duration_to_pt=0")
        assert('journeys' not in response)

        # with street network activated
        response = self.query_region(query + "&max_duration_to_pt=1")
        eq_(len(response['journeys']), 1)
        eq_(response['journeys'][0]['sections'][0]['from']['id'], 'stop_point:uselessA')
        eq_(response['journeys'][0]['sections'][0]['to']['id'], 'A')
        eq_(response['journeys'][0]['sections'][0]['type'], 'crow_fly')
        eq_(response['journeys'][0]['sections'][0]['mode'], 'walking')
        eq_(response['journeys'][0]['sections'][0]['duration'], 0)

    @skip("temporarily disabled")
    def test_speed_factor_direct_path(self):
        """We test the coherence of the non pt walk solution with a speed factor"""

        response = self.query_region("{query}&type=non_pt_walk&walking_speed=1.5".
                                     format(query=journey_basic_query))

        assert len(response['journeys']) == 1
        assert response['journeys'][0]["type"] == "non_pt_walk"
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['duration'] == response['journeys'][0]['sections'][0]['duration']
        assert response['journeys'][0]['duration'] == 205
        assert response['journeys'][0]['durations']['total'] == 205
        assert response['journeys'][0]['durations']['walking'] == 205

        assert response['journeys'][0]['departure_date_time'] == response['journeys'][0]['sections'][0]['departure_date_time']
        assert response['journeys'][0]['departure_date_time'] == '20120614T080000'
        assert response['journeys'][0]['arrival_date_time'] == response['journeys'][0]['sections'][0]['arrival_date_time']
        assert response['journeys'][0]['arrival_date_time'] == '20120614T080325'

    @skip("temporarily disabled")
    def test_traveler_type(self):
        q_fast_walker = journey_basic_query + "&traveler_type=fast_walker"
        response_fast_walker = self.query_region(q_fast_walker)
        basic_response = self.query_region(journey_basic_query)

        def bike_in_journey(fast_walker):
            return any(sect_fast_walker["mode"] == 'bike' for sect_fast_walker in fast_walker['sections'])

        def no_bike_in_journey(journey):
            return all(section['mode'] != 'bike' for section in journey['sections'] if 'mode' in section)

        assert any(bike_in_journey(journey_fast_walker) for journey_fast_walker in response_fast_walker['journeys'])
        assert all(no_bike_in_journey(journey) for journey in basic_response['journeys'])

    def test_crow_fly_sections(self):
        """
        When the departure is a stop_area...
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa='stopA', to_sa='stopB', datetime="20120614T080000")
        response = self.query("v1/coverage/main_routing_test/" + query)
        check_journeys(response)
        jrnys = response['journeys']
        assert len(jrnys) == 2
        section_0 = jrnys[0]['sections'][0]
        assert section_0['type'] == 'crow_fly'
        assert section_0['from']['id'] == 'stopA'
        assert section_0['to']['id'] == 'stop_point:stopA'

        section_2 = jrnys[0]['sections'][2]
        assert section_2['type'] == 'crow_fly'
        assert section_2['from']['id'] == 'stop_point:stopB'
        assert section_2['to']['id'] == 'stopB'

    def test_max_duration_to_pt_equals_to_0(self):
        query = journey_basic_query + \
            "&first_section_mode[]=bss" + \
            "&first_section_mode[]=walking" + \
            "&first_section_mode[]=bike" + \
            "&first_section_mode[]=car" + \
            "&debug=true"
        response = self.query_region(query)
        check_journeys(response)
        eq_(len(response['journeys']), 4)

        query += "&max_duration_to_pt=0"
        response = self.query_region(query)
        check_journeys(response)
        # the pt journey is eliminated
        eq_(len(response['journeys']), 3)

        # first is bike
        assert('bike' in response['journeys'][0]['tags'])
        eq_(len(response['journeys'][0]['sections']), 1)

        # second is car
        assert('car' in response['journeys'][1]['tags'])
        eq_(len(response['journeys'][1]['sections']), 3)

        # last is walking
        assert('walking' in response['journeys'][-1]['tags'])
        eq_(len(response['journeys'][-1]['sections']), 1)

    def test_max_duration_to_pt_equals_to_0_from_stop_point(self):
        query = "journeys?from=stop_point%3AstopA&to=stop_point%3AstopC&datetime=20120614T080000" \
                "&_override_scenario=experimental"
        response = self.query_region(query)
        check_journeys(response)
        eq_(len(response['journeys']), 2)

        query += "&max_duration_to_pt=0"
        response = self.query_region(query)
        check_journeys(response)
        eq_(len(response['journeys']), 2)

    def test_max_duration_equals_to_0(self):
        query = journey_basic_query + \
            "&first_section_mode[]=bss" + \
            "&first_section_mode[]=walking" + \
            "&first_section_mode[]=bike" + \
            "&first_section_mode[]=car" + \
            "&debug=true"
        response = self.query_region(query)
        check_journeys(response)
        eq_(len(response['journeys']), 4)

        query += "&max_duration=0"
        response = self.query_region(query)
        check_journeys(response)
        # the pt journey is eliminated
        eq_(len(response['journeys']), 3)

        # first is bike
        assert('bike' in response['journeys'][0]['tags'])
        eq_(response['journeys'][0]['debug']['internal_id'], 'dp_43-0')
        eq_(len(response['journeys'][0]['sections']), 1)

        # second is car
        assert('car' in response['journeys'][1]['tags'])
        eq_(response['journeys'][1]['debug']['internal_id'], "dp_44-0")
        eq_(len(response['journeys'][1]['sections']), 3)

        # last is walking
        assert('walking' in response['journeys'][-1]['tags'])
        eq_(response['journeys'][-1]['debug']['internal_id'], "dp_42-0")
        eq_(len(response['journeys'][-1]['sections']), 1)

    def test_crow_fly_sections(self):
        """
        When the departure is a stop_area...
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa='stopA', to_sa='stopB', datetime="20120614T080000")
        response = self.query("v1/coverage/main_routing_test/" + query)
        check_journeys(response)
        jrnys = response['journeys']
        assert len(jrnys) == 2
        section_0 = jrnys[0]['sections'][0]
        assert section_0['type'] == 'crow_fly'
        assert section_0['from']['id'] == 'stopA'
        assert section_0['to']['id'] == 'stop_point:stopA'

        section_2 = jrnys[0]['sections'][2]
        assert section_2['type'] == 'crow_fly'
        assert section_2['from']['id'] == 'stop_point:stopB'
        assert section_2['to']['id'] == 'stopB'

    def test_journey_stop_area_to_stop_point(self):
        """
        When the departure is stop_area:A and the destination is stop_point:B belonging to stop_area:B
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa='stopA', to_sa='stop_point:stopB', datetime="20120614T080000")
        response = self.query("v1/coverage/main_routing_test/" + query)
        check_journeys(response)
        jrnys = response['journeys']

        j = next((j for j in jrnys if j['type'] == 'non_pt_walk'), None)
        assert j
        assert j['sections'][0]['from']['id'] == 'stopA'
        assert j['sections'][0]['to']['id'] == 'stop_point:stopB'
        assert 'walking' in j['tags']

@config({"scenario": "experimental"})
class TestExperimentalJourneysWithPtref(JourneysWithPtref, AbstractTestFixture):
    pass


@dataset({"basic_routing_test": {"scenario": "new_default"}})
class TestNewDefaultOnBasicRouting(OnBasicRouting, AbstractTestFixture):

    @skip("temporarily disabled")
    def test_journeys_with_show_codes(self):
        '''
        Test journeys api with show_codes = false.
        The API's response contains the codes
        '''
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&show_codes=false"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        eq_(len(response['journeys'][0]['sections']), 4)
        first_section = response['journeys'][0]['sections'][0]
        eq_(first_section['from']['stop_point']['codes'][0]['type'], 'external_code')
        eq_(first_section['from']['stop_point']['codes'][0]['value'], 'stop_point:A')
        eq_(first_section['from']['stop_point']['codes'][1]['type'], 'source')
        eq_(first_section['from']['stop_point']['codes'][1]['value'], 'Ain')
        eq_(first_section['from']['stop_point']['codes'][2]['type'], 'source')
        eq_(first_section['from']['stop_point']['codes'][2]['value'], 'Aisne')

    @skip("temporarily disabled")
    def test_journeys_without_show_codes(self):
        '''
        Test journeys api without show_codes.
        The API's response contains the codes
        '''
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        eq_(len(response['journeys'][0]['sections']), 4)
        first_section = response['journeys'][0]['sections'][0]
        eq_(first_section['from']['stop_point']['codes'][0]['type'], 'external_code')
        eq_(first_section['from']['stop_point']['codes'][0]['value'], 'stop_point:A')
        eq_(first_section['from']['stop_point']['codes'][1]['type'], 'source')
        eq_(first_section['from']['stop_point']['codes'][1]['value'], 'Ain')
        eq_(first_section['from']['stop_point']['codes'][2]['type'], 'source')
        eq_(first_section['from']['stop_point']['codes'][2]['value'], 'Aisne')

    @skip("temporarily disabled")
    def test_novalidjourney_on_first_call(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        logging.getLogger(__name__).info("arrival date: {}".format(response['journeys'][0]['arrival_date_time']))
        eq_(response['journeys'][0]['arrival_date_time'],  "20120614T160000")
        eq_(response['journeys'][0]['type'], "best")

        assert len(response["disruptions"]) == 0
        feed_publishers = response["feed_publishers"]
        assert len(feed_publishers) == 1
        for feed_publisher in feed_publishers:
            is_valid_feed_publisher(feed_publisher)

        feed_publisher = feed_publishers[0]
        assert (feed_publisher["id"] == "base_contributor")
        assert (feed_publisher["name"] == "base contributor")
        assert (feed_publisher["license"] == "L-contributor")
        assert (feed_publisher["url"] == "www.canaltp.fr")

    @skip("temporarily disabled")
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
        eq_(len(response['journeys']), 2)
        eq_(response['journeys'][0]['arrival_date_time'], "20120614T150000")
        eq_(response['journeys'][0]['type'], "")
        eq_(response['journeys'][1]['arrival_date_time'], "20120614T160000")
        eq_(response['journeys'][1]['type'], "best")
