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


@dataset(["basic_routing_test"])
class TestIsochrone(AbstractTestFixture):
    """
    Test the structure of the journeys response
    """

    def test_from_isochrone_coord(self):
        #NOTE: we query /v1/coverage/basic_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = "v1/coverage/basic_routing_test/journeys?from={}&datetime={}"
        query = query.format(s_coord, "20120614T080000")
        self.query(query, display=False)

    def test_stop_point_isochrone_coord(self):
        #NOTE: we query /v1/coverage/basic_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = "v1/coverage/basic_routing_test/stop_points/A/journeys?max_duration=400&datetime=20120614T080000"
        response = self.query(query, display=False)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["duration"] == 300
        assert response["journeys"][0]["to"]["stop_point"]["id"] == "B"
        query = "v1/coverage/basic_routing_test/stop_points/A/journeys?max_duration=25500&datetime=20120614T080000"
        response = self.query(query, display=False)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["duration"] == 300
        assert response["journeys"][0]["to"]["stop_point"]["id"] == "B"
        assert response["journeys"][1]["duration"] == 25200
        assert response["journeys"][1]["to"]["stop_point"]["id"] == "D"

    def test_to_isochrone_coord(self):
        #NOTE: we query /v1/coverage/basic_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = "v1/coverage/basic_routing_test/journeys?from={}&datetime={}&datetime_represents=arrival"
        query = query.format(s_coord, "20120614T080000")
        self.query(query, display=False)

    def test_from_isochrone_sa(self):
        #NOTE: we query /v1/coverage/basic_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = "v1/coverage/basic_routing_test/journeys?from={}&datetime={}"
        query = query.format("stopA", "20120614T080000")
        self.query(query, display=False)

    def test_to_isochrone_sa(self):
        #NOTE: we query /v1/coverage/basic_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        query = "v1/coverage/basic_routing_test/journeys?from={}&datetime={}&datetime_represents=arrival"
        query = query.format("stopA", "20120614T080000")
        self.query(query, display=False)
