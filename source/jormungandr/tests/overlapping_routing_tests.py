# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import, print_function, unicode_literals, division
import logging
from navitiacommon import models

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


@dataset({"main_routing_test": {'is_free': False}, "empty_routing_test": {'is_free': True}})
class TestOverlappingBestCoverage(AbstractTestFixture):
    """
    Test the answer if 2 coverages are overlapping by choosing the best one according to the instances comparator (non-free first)
    """

    def test_place_by_coords_id(self):
        response = self.query("/v1/coverage/0.000001;0.000898311281954/places/0.000001;0.000898311281954")
        places = get_not_null(response, 'places')
        assert len(places) == 1
        assert places[0]['embedded_type'] == 'address'
        assert places[0]['name'] == '42 rue kb (Condom)'
        assert places[0]['address']['label'] == "42 rue kb (Condom)"
        assert places[0]['address']['house_number'] == 42


@dataset({"main_routing_test": {'is_free': True}, "empty_routing_test": {'is_free': False}})
class TestOverlappingCoverage(AbstractTestFixture):
    """
    Test the answer if 2 coverages are overlapping
    """

    def test_journeys(self):
        """
        journey query with 2 overlapping coverage.
        main_routing_test is free
        empty_routing_test is not free but without data (and with the same bounding shape than main_routing_test)

        the empty region should be chosen first and after having returned no journey the real region should be called
        ==> ie we must have a journey in the end
        """
        response = self.query("/v1/{q}".format(q=journey_basic_query))

        self.is_valid_journey_response(response, journey_basic_query)

        assert len(response['feed_publishers']) == 1
        assert response['feed_publishers'][0]['name'] == u'routing api data'

        # with the initial response we cannot specifically check that the empty_routing_test region
        # has been called, so we call it back with debug and then we can check the region called field
        debug_query = "/v1/{q}&debug=true".format(q=journey_basic_query)
        response = self.query(debug_query)
        self.is_valid_journey_response(response, debug_query)
        assert response['debug']['regions_called'][0]['name'] == 'empty_routing_test'
        assert response['debug']['regions_called'][0]['scenario'] == 'new_default'
        assert response['debug']['regions_called'][1]['name'] == 'main_routing_test'
        assert response['debug']['regions_called'][1]['scenario'] == 'new_default'

    def test_journeys_on_empty(self):
        """
        explicit call to the empty region should not work
        """
        response, error_code = self.query_no_assert(
            "/v1/coverage/empty_routing_test/{q}".format(q=journey_basic_query), display=False
        )

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert error_code == 404
        assert 'error' in response
        # impossible to project the starting/ending point
        assert response['error']['id'] == 'no_origin_nor_destination'

    def test_journeys_on_same_error(self):
        """
        if all region gives the same error we should get the error

        there we ask for a journey in 1980, both region should return a date_out_of_bounds
        """
        journey_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="19800614T080000",
        )
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
        response, error_code = self.query_no_assert(
            "v1/{query}&max_duration_to_pt=20&debug=true".format(query=journey_basic_query), display=False
        )

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert error_code == 200  # no solution is 200
        assert 'error' in response
        assert response['error']['id'] == 'no_solution'
        assert response['error']['message'] == 'No journey found'

        assert 'debug' in response
        assert 'errors_by_region' in response['debug']
        assert (
            response['debug']['errors_by_region']['empty_routing_test']
            == 'Public transport is not reachable from origin nor destination'
        )
        assert (
            response['debug']['errors_by_region']['main_routing_test']
            == 'Public transport is not reachable from destination'
        )

        # we also should have the region called in the debug node
        # (and the empty one should be first since it has been called first)
        assert 'regions_called' in response['debug']
        assert response['debug']['regions_called'][0]['name'] == 'empty_routing_test'
        assert response['debug']['regions_called'][0]['scenario'] == 'new_default'
        assert response['debug']['regions_called'][1]['name'] == 'main_routing_test'
        assert response['debug']['regions_called'][1]['scenario'] == 'new_default'

    def test_journeys_no_debug(self):
        """no debug in query, no debug in answer"""
        response = self.query("/v1/{q}".format(q=journey_basic_query), display=False)

        assert not 'debug' in response

    def test_coord(self):
        response = self.query("/v1/coord/0.000001;0.000898311281954")
        is_valid_address(response['address'])
        assert response['address']['label'] == "42 rue kb (Condom)"

    def test_coord_geo_inv(self):
        response = self.query("/v1/coverage/0.000001;0.000898311281954/coords/0.000001;0.000898311281954")
        is_valid_address(response['address'])
        assert response['address']['label'] == "42 rue kb (Condom)"

    def test_not_found_coord(self):
        """
        -0.9;-0.9 is in the two regions, but there is no addresses at
        200m, thus it should return no solution after trying the two
        region. empty_routing_test is tested first as it is non free.
        """
        response, error_code = self.query_no_assert("/v1/coord/-0.9;-0.9", display=True)
        assert error_code == 404
        assert set(response['regions']) == {"empty_routing_test", "main_routing_test"}
