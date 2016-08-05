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

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *
from jormungandr import app
from nose.tools import eq_


@dataset({"main_routing_test": {}})
class TestHeatMap(AbstractTestFixture):
    """
    Test the structure of the heat_maps response
    """

    def test_from_heat_map_coord(self):
        query = "v1/coverage/main_routing_test/" + heat_map_basic_query
        response = self.query(query)

        is_valid_heat_maps(response, self.tester, query)

    def test_to_heat_map_coord(self):
        query = "v1/coverage/main_routing_test/heat_maps?to={}&datetime={}&max_duration={}"
        query = query.format(s_coord, "20120614T080000", "3600")
        response = self.query(query)

        is_valid_heat_maps(response, self.tester, query)

    def test_heat_maps_from_stop_point(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', 'stopA', '3600')
        response = self.query(q)

        is_valid_heat_maps(response, self.tester, q)

    def test_heat_maps_to_stop_point(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&to={}&max_duration={}"
        q = q.format('20120614T080000', 'stopA', '3600')
        response = self.query(q)

        is_valid_heat_maps(response, self.tester, q)

    def test_heat_maps_no_datetime(self):
        q_no_dt = "v1/coverage/main_routing_test/heat_maps?from={}&max_duration={}&_current_datetime={}"
        q_no_dt = q_no_dt.format(s_coord, '3600', '20120614T080000')
        response_no_dt = self.query(q_no_dt)

        is_valid_heat_maps(response_no_dt, self.tester, q_no_dt)

    def test_heat_maps_no_seconds_in_datetime(self):
        q_no_s = "v1/coverage/main_routing_test/heat_maps?from={}&max_duration={}&datetime={}"
        q_no_s = q_no_s.format(s_coord, '3600', '20120614T0800')
        response_no_s = self.query(q_no_s)

        is_valid_heat_maps(response_no_s, self.tester, q_no_s)

    def test_heat_maps_no_arguments(self):
        q = "v1/coverage/main_routing_test/heat_maps"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you should provide a \'from\' or a \'to\' argument'


    def test_heat_maps_no_region(self):
        q = "v1/coverage/heat_maps"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'The region heat_maps doesn\'t exists'


    def test_heat_maps_invald_duration(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', s_coord, '-3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'Unable to evaluate, invalid positive int'

        p = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        p = p.format('20120614T080000', s_coord, 'toto')
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        assert normal_response['message'] == 'Unable to evaluate, invalid literal for int() with base 10: \'toto\''


    def test_heat_maps_null_speed(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}&walking_speed=0"
        q = q.format('20120614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'The walking_speed argument has to be > 0, you gave : 0'


    def test_heat_maps_no_duration(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}"
        q = q.format('20120614T080000', s_coord)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == "you should provide a 'max_duration' argument"


    def test_heat_maps_date_out_of_bound(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20050614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['id'] == 'date_out_of_bounds'
        assert normal_response['error']['message'] == 'date is not in data production period'


    def test_heat_maps_invalid_datetime(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('2005061', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'Unable to parse datetime, year is out of range'

        p = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        p = p.format('toto', s_coord, 3600)
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        eq_(normal_response['message'].lower(), 'unable to parse datetime, unknown string format')


    def test_graphical_heat_maps_no_heat_maps(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', '90;0', '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'no origin point nor destination point valid'


    def test_heat_maps_with_from_and_to(self):
        q = "v1/coverage/main_routing_test/" + heat_map_basic_query + "&to={}"
        q = q.format(r_coord)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you cannot provide a \'from\' and a \'to\' argument'
