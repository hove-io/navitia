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


@dataset({"basic_routing_test": {}})
class TestIsochrone(AbstractTestFixture):
    """
    Test the structure of the journeys response
    """

    def test_from_isochrone_coord(self):
        #NOTE: we query /v1/coverage/basic_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = "v1/coverage/basic_routing_test/journeys?from={}&datetime={}"
        query = query.format(s_coord, "20120614T080000")
        self.query(query)

    def test_stop_point_isochrone_coord(self):
        #NOTE: we query /v1/coverage/basic_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = "v1/coverage/basic_routing_test/stop_points/A/journeys?max_duration=400&datetime=20120614T080000"
        response = self.query(query, display=True)
        is_valid_isochrone_response(response, self.tester, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["duration"] == 300
        assert response["journeys"][0]["to"]["stop_point"]["id"] == "B"
        assert response["journeys"][0]["from"]["id"] == "A"
        query = "v1/coverage/basic_routing_test/stop_points/A/journeys?max_duration=25500&datetime=20120614T080000"
        response = self.query(query)
        is_valid_isochrone_response(response, self.tester, query)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["duration"] == 300
        assert response["journeys"][0]["to"]["stop_point"]["id"] == "B"
        assert response["journeys"][0]["from"]["id"] == "A"
        assert response["journeys"][1]["duration"] == 25200
        assert response["journeys"][1]["to"]["stop_point"]["id"] == "D"
        assert response["journeys"][1]["from"]["id"] == "A"

    def test_stop_point_isochrone_coord_no_transfers(self):
        #same query as the test_stop_point_isochrone_coord test, but this time we forbid to do a transfers
        #we should be able to touch only 'B'
        query = "v1/coverage/basic_routing_test/stop_points/A/journeys?" \
                "max_duration=25500&datetime=20120614T080000" \
                "&max_nb_transfers=0"
        response = self.query(query)
        is_valid_isochrone_response(response, self.tester, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["duration"] == 300
        assert response["journeys"][0]["to"]["stop_point"]["id"] == "B"
        assert response["journeys"][0]["from"]["id"] == "A"

    def test_to_isochrone_coord(self):
        query = "v1/coverage/basic_routing_test/journeys?from={}&datetime={}"
        query = query.format(s_coord, "20120614T080000")
        response = self.query(query)

    def test_from_isochrone_sa(self):
        query = "v1/coverage/basic_routing_test/journeys?from={}&datetime={}"
        query = query.format("stopA", "20120614T080000")
        response = self.query(query)

    def test_to_isochrone_sa(self):
        query = "v1/coverage/basic_routing_test/journeys?to={}&datetime={}&datetime_represents=arrival"
        query = query.format("stopA", "20120614T080000")
        response = self.query(query)

    def test_reverse_isochrone_coord(self):
        q = "v1/coverage/basic_routing_test/journeys?max_duration=100000" \
            "&datetime=20120615T200000&datetime_represents=arrival&to=D"
        normal_response = self.query(q, display=True)
        is_valid_isochrone_response(normal_response, self.tester, q)
        assert len(normal_response["journeys"]) == 2

    def test_reverse_isochrone_coord_clockwise(self):
        q = "v1/coverage/basic_routing_test/journeys?datetime=20120614T080000&to=A"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'reverse isochrone works only for anti-clockwise request'

    def test_isochrone_non_clockwise(self):
        q = "v1/coverage/basic_routing_test/journeys?datetime=20120614T080000&from=A&datetime_represents=arrival"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'isochrone works only for clockwise request'

    def test_isochrone_count(self):
        query = "v1/coverage/basic_routing_test/stop_points/A/journeys?max_duration=25500&datetime=20120614T080000"
        response = self.query(query)
        assert len(response["journeys"]) == 2
        is_valid_isochrone_response(response, self.tester, query)
        query += "&count=1"
        response = self.query(query)
        assert len(response["journeys"]) == 1
        is_valid_isochrone_response(response, self.tester, query)
