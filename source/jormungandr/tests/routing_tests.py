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
from check_utils import *


@dataset(["main_routing_test"])
class TestJourneys(AbstractTestFixture):
    """
    Test the structure of the journeys response
    """

    def test_journeys(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        response = self.query_region(journey_basic_query, display=False)

        is_valid_journey_response(response, self.tester, journey_basic_query)

    def test_error_on_journeys(self):
        """ if we got an error with kraken, an error should be returned"""

        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20110614T080000")  # 2011 should not be in the production period

        response, status = self.query_no_assert("v1/coverage/main_routing_test/" + query_out_of_production_bound)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "date_out_of_bounds"
        assert response['error']['message'] == "date is not in data production period"

        #and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_best_filtering(self):
        """Filter to get the best journey, we should have only one journey, the best one"""
        query = "{query}&type=best".format(query=journey_basic_query)
        response = self.query_region(query, display=False)

        is_valid_journey_response(response, self.tester, query)
        assert len(response['journeys']) == 1

        assert response['journeys'][0]["type"] == "best"

    def test_other_filtering(self):
        """the basic query return a non pt walk journey and a best journey. we test the filtering of the non pt"""

        response = self.query_region("{query}&type=non_pt_walk".
                                     format(query=journey_basic_query), display=False)

        assert len(response['journeys']) == 1
        assert response['journeys'][0]["type"] == "non_pt_walk"

    def test_not_existent_filtering(self):
        """if we filter with a real type but not present, we don't get any journey"""

        response = self.query_region("{query}&type=car".
                                     format(query=journey_basic_query), display=False)

        assert not 'journeys' in response or len(response['journeys']) == 0

    def test_dumb_filtering(self):
        """if we filter with non existent type, we get an error"""

        response, status = self.query_region("{query}&type=sponge_bob".format(query=journey_basic_query), check=False, display=True)

        assert status == 400, "the response should not be valid"

        assert response['message'].startswith("The type argument must be in list")


@dataset([])
class TestJourneysNoRegion(AbstractTestFixture):
    """
    If no region loaded we must have a polite error while asking for a journey
    """

    def test_with_region(self):
        response, status = self.query_no_assert("v1/coverage/non_existent_region/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"
        assert response['error']['message'] == "The region non_existent_region doesn't exists"

    def test_no_region(self):
        response, status = self.query_no_assert("v1/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"

        error_regexp = re.compile('^No region available for the coordinates.*')
        assert error_regexp.match(response['error']['message'])


@dataset(["basic_routing_test"])
class TestLongWaitingDurationFilter(AbstractTestFixture):
    """
    Test if the filter on long waiting duration is working
    """

    def test_novalidjourney_on_first_call(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        assert(len(response['journeys']) == 1)
        logging.getLogger(__name__).info("arrival date: {}".format(response['journeys'][0]['arrival_date_time']))
        assert(response['journeys'][0]['arrival_date_time'] == "20120614T160000")
        assert(response['journeys'][0]['type'] == "best")

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
        assert(len(response['journeys']) == 2)
        assert(response['journeys'][0]['arrival_date_time'] == "20120614T150000")
        assert(response['journeys'][0]['type'] == "")
        assert(response['journeys'][1]['arrival_date_time'] == "20120614T160000")
        assert(response['journeys'][1]['type'] == "best")


    def test_remove_one_journey_from_batch(self):
        """
        Kraken returns two journeys, the earliest arrival one returns a too
        long waiting duration, therefore it must be deleted.
        The second one must be returned
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        assert(len(response['journeys']) == 1)
        assert(response['journeys'][0]['arrival_date_time'] == u'20120615T151000')
        assert(response['journeys'][0]['type'] == "best")


    def test_max_attemps(self):
        """
        Kraken always retrieves journeys with non_pt_duration > max_non_pt_duration
        No journeys should be typed, but get_journeys should stop quickly
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="E", to_sa="H", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        assert(not "journeys" in response or len(response['journeys']) == 0)


    def test_max_attemps_debug(self):
        """
        Kraken always retrieves journeys with non_pt_duration > max_non_pt_duration
        No journeys should be typed, but get_journeys should stop quickly
        We had the debug argument, hence a non-typed journey is returned
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"\
            .format(from_sa="E", to_sa="H", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        assert(len(response['journeys']) == 1)
