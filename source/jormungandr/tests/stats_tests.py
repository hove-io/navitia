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
from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *
from jormungandr import stat_manager
from jormungandr.stat_manager import StatManager, get_posix_date_time
#from mock import patch


class MockWrapper:

    def __init__(self):
        self.called = False #TODO: use mock!

    def mock_journey_stat(self, stat):
        self.check_stat_journey_to_publish(stat)
        self.called = True

    def mock_coverage_stat(self, stat):
        self.check_stat_coverage_to_publish(stat)
        self.called = True

    def check_stat_coverage_to_publish(self, stat):
        #Verify request elements
        assert stat.user_id != ""
        assert stat.api == "v1.coverage"
        assert len(stat.parameters) == 0
        assert len(stat.coverages) == 1
        #Here we verify id of region in request.view_args of the request.
        assert stat.coverages[0].region_id == ""
        assert len(stat.journeys) == 0
        #Verify elements of request.error
        assert stat.error.id == ""

    def mock_places_stat(self, stat):
        self.check_stat_places_to_publish(stat)

    def check_stat_journey_to_publish(self, stat):
        #Verify request elements
        assert stat.user_id != ""
        assert stat.api == "v1.journeys"

        epsilon = 1e-5
        #Verify elements of request.parameters
        assert len(stat.parameters) == 3
        assert stat.parameters[0].key == "to"
        assert stat.parameters[0].value == "0.00188646;0.00071865"
        assert stat.parameters[1].key == "from"
        assert stat.parameters[1].value == "0.0000898312;0.0000898312"

        #Verify elements of request.coverages
        assert len(stat.coverages) == 1
        assert stat.coverages[0].region_id == "main_routing_test"

        #Verify elements of request.error
        assert stat.error.id == ""

        #Verify elements of request.journeys
        assert len(stat.journeys) == 2

#commented for the moment, will need to update that, but it is for another commit
#        assert stat.journeys[0].requested_date_time == get_posix_date_time("20120614T080000") #1339653600
        assert stat.journeys[0].departure_date_time == get_posix_date_time("20120614T080042") #1339653642
        assert stat.journeys[0].arrival_date_time == get_posix_date_time("20120614T080222") #1339653742
        assert stat.journeys[0].duration == 99
        assert stat.journeys[0].type == "best"
        assert stat.journeys[0].nb_transfers == 0

        #Verify elements of request.journeys.sections
        assert len(stat.journeys[0].sections) == 3
        assert stat.journeys[0].sections[1].departure_date_time == get_posix_date_time("20120614T080100") #1339653660
        assert stat.journeys[0].sections[1].arrival_date_time == get_posix_date_time("20120614T080102") #1339653662
        assert stat.journeys[0].sections[1].duration == 2
        assert stat.journeys[0].sections[1].from_embedded_type == "stop_area"
        assert stat.journeys[0].sections[1].from_id == "stopB"
        assert stat.journeys[0].sections[1].from_name == "stopB"
        assert stat.journeys[0].sections[1].mode == ""
        assert stat.journeys[0].sections[1].network_id == "base_network"
        assert stat.journeys[0].sections[1].network_name == "base_network"
        assert abs(stat.journeys[0].sections[1].from_coord.lat - 0.000269493594586) < epsilon
        assert abs(stat.journeys[0].sections[1].from_coord.lon - 8.98311981955e-05) < epsilon
        assert stat.journeys[0].sections[1].to_embedded_type == "stop_area"
        assert stat.journeys[0].sections[1].to_id == "stopA"
        assert stat.journeys[0].sections[1].to_name == "stopA"

        assert abs(stat.journeys[0].sections[1].to_coord.lat - 0.000718649585564) < epsilon
        assert abs(stat.journeys[0].sections[1].to_coord.lon - 0.00107797437835) < epsilon
        assert len(stat.journeys[1].sections) == 1
        assert stat.journeys[1].sections[0].departure_date_time == get_posix_date_time("20120614T080000") #1339653600
        assert stat.journeys[1].sections[0].arrival_date_time == get_posix_date_time("20120614T080435") #1339653875
        assert stat.journeys[1].sections[0].duration == 275
        assert stat.journeys[1].sections[0].from_embedded_type == "address"
        assert stat.journeys[1].sections[0].from_id == ""
        assert stat.journeys[1].sections[0].from_name == "rue bs"
        assert stat.journeys[1].sections[0].mode == "walking"
        assert stat.journeys[1].sections[0].to_embedded_type == "address"
        assert stat.journeys[1].sections[0].to_id == ""
        assert stat.journeys[1].sections[0].to_name == "rue ar"
        assert stat.journeys[1].sections[0].type == "street_network"

    def check_stat_places_to_publish(self, stat):
        assert stat.user_id != ""
        assert stat.api == "v1.place_uri"
        assert len(stat.parameters) == 0
        assert len(stat.coverages) == 1
        #Here we verify id of region in request.view_args of the request.
        assert stat.coverages[0].region_id == "main_ptref_test"
        assert stat.error.id == ""

        self.called = True


@dataset(["main_routing_test"])
class TestStatJourneys(AbstractTestFixture):

    def setup(self):
        """
        We save the original publish_method to be able to put it back after the tests
        each test will override this method with it's own mock check method
        """
        self.real_publish_request_method = StatManager.publish_request
        self.old_save_val = stat_manager.save_stat
        stat_manager.save_stat = True

    def teardown(self):
        """
        Here we put back the original method to stat manager.
        """
        StatManager.publish_request = self.real_publish_request_method
        stat_manager.save_stat = self.old_save_val

    def test_simple_journey_query_with_stats(self):
        """
        test on a valid journey (tested in routing_tests.py).
        stat activated to test objects filled by stat manager
        """
        # we override the stat method with a mock method to test the journeys
        mock = MockWrapper()
        StatManager.publish_request = mock.mock_journey_stat
        response = self.query_region(journey_basic_query, display=False)

        assert mock.called

    def test_simple_test_coverage_with_stats(self):
        """
        here  we test stat objects for api coverage filled by stat manager
        """
        # we override the stat method with a mock method to test the coverage
        mock = MockWrapper()
        StatManager.publish_request = mock.mock_coverage_stat
        json_response = self.query("/v1/coverage", display=False)

        assert mock.called


@dataset(["main_ptref_test"])
class TestStatPlaces(AbstractTestFixture):

    def setup(self):
        """
        We save the original publish_method to be able to put it back after the tests
        each test will override this method with it's own mock check method
        """
        self.real_publish_request_method = StatManager.publish_request
        self.old_save_val = stat_manager.save_stat
        stat_manager.save_stat = True

    def teardown(self):
        """
        Here we put back the original method to stat manager.
        """
        StatManager.publish_request = self.real_publish_request_method
        stat_manager.save_stat = self.old_save_val

    def test_simple_test_places_with_stats(self):
        """
        here  we test stat objects for api place_uri filled by stat manager
        """
        # we override the stat method with a mock method to test the place_uri
        mock = MockWrapper()
        StatManager.publish_request = mock.check_stat_places_to_publish
        response = self.query_region("places/stop_area:stop1", display=False)
        assert mock.called