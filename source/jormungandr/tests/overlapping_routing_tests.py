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
from navitiacommon import models

from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


class MockKraken:
    def __init__(self, kraken_instance, is_free):
        self.is_free = is_free
        self.kraken_instance = kraken_instance

@dataset(["main_routing_test", "empty_routing_test"])
class TestOverlappingCoverage(AbstractTestFixture):
    """
    Test the answer if 2 coverages are overlapping
    """
    def setup(self):
        from jormungandr import i_manager
        self.instance_map = {
            'main_routing_test': MockKraken(i_manager.instances['main_routing_test'], True),
            #the bad one is the non free one, so it will be chosen first
            'empty_routing_test': MockKraken(i_manager.instances['empty_routing_test'], False),
        }
        self.real_method = models.Instance.get_by_name

        models.Instance.get_by_name = self.mock_get_by_name

    def mock_get_by_name(self, name):
        return self.instance_map[name]

    def teardown(self):
        models.Instance.get_by_name = self.real_method

    def test_journeys(self):
        """
        journey query with 2 overlapping coverage.
        main_routing_test is free
        empty_routing_test is not free but without data (and with the same bounding shape than main_routing_test)

        the empty region should be chosen first and after having returned no journey the real region should be called
        ==> ie we must have a journey in the end
        """
        response = self.query("/v1/{q}".format(q=journey_basic_query))

        is_valid_journey_response(response, self.tester, journey_basic_query)

        assert len(response['feed_publishers']) == 1
        assert response['feed_publishers'][0]['name'] == u'routing api data'

        # with the initial response we cannot specifically check that the empty_routing_test region
        # has been called, so we call it back with debug and then we can check the region called field
        debug_query = "/v1/{q}&debug=true".format(q=journey_basic_query)
        response = self.query(debug_query)
        is_valid_journey_response(response, self.tester, debug_query)
        assert set(response['debug']['regions_called']) == {"main_routing_test", "empty_routing_test"}

    def test_journeys_on_empty(self):
        """
        explicit call to the empty region should not work
        """
        response, error_code = self.query_no_assert("/v1/coverage/empty_routing_test/{q}".format(q=journey_basic_query), display=False)

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert error_code == 404
        assert 'error' in response
        #impossible to project the starting/ending point
        assert response['error']['id'] == 'no_origin_nor_destination'

    def test_journeys_on_same_error(self):
        """
        if all region gives the same error we should get the error

        there we ask for a journey in 1980, both region should return a date_out_of_bounds
        """
        journey_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
                    to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
                    datetime="19800614T080000")
        response, error_code = self.query_no_assert("/v1/{q}".format(q=journey_query), display=False)

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert error_code == 404
        assert 'error' in response
        assert response['error']['id'] == 'date_out_of_bounds'

    def test_journeys_on_different_error(self):
        """
        if all region gives a different error we should get a default error

        there we limit the walking duration no to find the destination
        so the empty region will say that it cannot find a origin/destination
        and the real region will say that it cannot find a destination

        we call the api with debug=true so we must get a 'debug' node with the error by regions
        """
        response, error_code = self.query_no_assert("v1/{query}&max_duration_to_pt=20&debug=true".
                                                    format(query=journey_basic_query), display=False)

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert error_code == 200  # no solution is 200
        assert 'error' in response
        assert response['error']['id'] == 'no_solution'
        assert response['error']['message'] == 'No journey found'

        assert 'debug' in response
        assert 'errors_by_region' in response['debug']
        assert response['debug']['errors_by_region']['empty_routing_test'] == 'no origin point nor destination point'
        assert response['debug']['errors_by_region']['main_routing_test'] == 'no destination point'

        # we also should have the region called in the debug node
        # (and the empty one should be first since it has been called first)
        assert 'regions_called' in response['debug']
        assert response['debug']['regions_called'] == ['empty_routing_test', 'main_routing_test']

    def test_journeys_no_debug(self):
        """no debug in query, no debug in answer"""
        response = self.query("/v1/{q}".format(q=journey_basic_query), display=False)

        assert not 'debug' in response

    def test_coord(self):
        response = self.query("/v1/coord/0.000001;0.000898311281954")
        is_valid_address(response['address'])
        assert(response['address']['label'] == "42 rue kb (Condom)")

    def test_not_found_coord(self):
        """
        -0.9;-0.9 is in the two regions, but there is no addresses at
        200m, thus it should return no solution after trying the two
        region. empty_routing_test is tested first as it is non free.
        """
        response, error_code = self.query_no_assert("/v1/coord/-0.9;-0.9", display=True)
        assert error_code == 404
        assert response['regions'] == ["empty_routing_test", "main_routing_test"]
