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
from shapely.geometry import asShape, Point


@dataset({"main_routing_test": {}})
class TestGraphicalIsochron(AbstractTestFixture):
    """
    Test the structure of the isochrons response
    """

    def test_from_graphical_isochron_coord(self):
        query = "v1/coverage/main_routing_test/isochrons?from={}&datetime={}&duration={}"
        query = query.format(s_coord, "20120614T080000", "3600")
        response = self.query(query)
        origin = Point(0.0000898312, 0.0000898312)
        d = response['isochrons'][0]['geojson']
        multi_poly = asShape(d)

        assert (multi_poly.contains(origin))
        is_valid_graphical_isochron(response, self.tester, query)

    def test_to_graphical_isochron_coord(self):
        query = "v1/coverage/main_routing_test/isochrons?to={}&datetime={}&duration={}&datetime_represents=arrival"
        query = query.format(s_coord, "20120614T080000", "3600")
        response = self.query(query)
        destination = Point(0.0000898312, 0.0000898312)
        d = response['isochrons'][0]['geojson']
        multi_poly = asShape(d)

        assert (multi_poly.contains(destination))
        is_valid_graphical_isochron(response, self.tester, query)

    def test_graphical_isochrons_from_stop_point(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}"
        q = q.format('20120614T080000', 'stopA', '3600')
        response = self.query(q)
        stopA = Point(0.000718649585564, 0.00107797437835)
        d = response['isochrons'][0]['geojson']
        multi_poly = asShape(d)

        assert (multi_poly.contains(stopA))
        is_valid_graphical_isochron(response, self.tester, q)

    def test_graphical_isochrons_to_stop_point(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&to={}&duration={}&datetime_represents=arrival"
        q = q.format('20120614T080000', 'stopA', '3600')
        response = self.query(q)
        stopA = Point(0.000718649585564, 0.00107797437835)
        d = response['isochrons'][0]['geojson']
        multi_poly = asShape(d)

        assert (multi_poly.contains(stopA))
        is_valid_graphical_isochron(response, self.tester, q)

    def test_graphical_isochrons_no_datetime(self):
        q = "v1/coverage/main_routing_test/isochrons?from={}&duration={}&_current_datetime={}"
        q = q.format(s_coord, '3600', '20120614T080000')
        response = self.query(q)
        query = "v1/coverage/main_routing_test/isochrons?from={}&datetime={}&duration={}"
        query = query.format(s_coord, '20120614T080000', '3600')
        resp = self.query(query)

        is_valid_graphical_isochron(response, self.tester, q)
        assert len(response['isochrons']) == len(resp['isochrons'])

        for i in range(0, len(response['isochrons'])):
            multi_poly_no_datetime = asShape(response['isochrons'][i]['geojson'])
            multi_poly = asShape(resp['isochrons'][i]['geojson'])

            assert multi_poly_no_datetime.equals(multi_poly)

    def test_graphical_isochrons_no_seconds_in_datetime(self):
        q = "v1/coverage/main_routing_test/isochrons?from={}&duration={}&datetime={}"
        q = q.format(s_coord, '3600', '20120614T0800')
        response = self.query(q)
        query = "v1/coverage/main_routing_test/isochrons?from={}&datetime={}&duration={}"
        query = query.format(s_coord, '20120614T080000', '3600')
        resp = self.query(query)

        is_valid_graphical_isochron(response, self.tester, q)
        assert len(response['isochrons']) == len(resp['isochrons'])

        for i in range(0, len(response['isochrons'])):
            multi_poly_no_seconds = asShape(response['isochrons'][i]['geojson'])
            multi_poly = asShape(resp['isochrons'][i]['geojson'])

            assert multi_poly_no_seconds.equals(multi_poly)

    def test_grapical_isochrons_speed_factor(self):
        q = "v1/coverage/main_routing_test/isochrons?from={}&duration={}&datetime={}&walking_speed={}"
        q = q.format(s_coord, '3600', '20120614T0800', 2)
        response = self.query(q)
        query = "v1/coverage/main_routing_test/isochrons?from={}&datetime={}&duration={}"
        query = query.format(s_coord, '20120614T080000', '3600')
        resp = self.query(query)

        is_valid_graphical_isochron(response, self.tester, q)

        for i in range(0, len(response['isochrons'])):
            multi_poly_speed = asShape(response['isochrons'][i]['geojson'])
            multi_poly = asShape(resp['isochrons'][i]['geojson'])

            assert multi_poly_speed.contains(multi_poly)

    def test_reverse_graphical_isochrons_coord_clockwise(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&to={}&duration={}"
        q = q.format('20120614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'reverse isochrone works only for anti-clockwise request'

    def test_graphical_isochrons_coord_clockwise(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}&datetime_represents=arrival"
        q = q.format('20120614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'isochrone works only for clockwise request'

    def test_graphical_isochrons_no_arguments(self):
        q = "v1/coverage/main_routing_test/isochrons"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you should provide a \'from\' or a \'to\' argument'

    def test_graphical_isochrons_no_region(self):
        q = "v1/coverage/isochrons"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'The region isochrons doesn\'t exists'

    def test_graphical_isochrons_invald_duration(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}"
        q = q.format('20120614T080000', s_coord, '-3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'Unable to evaluate, invalid positive int'

        p = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}"
        p = p.format('20120614T080000', s_coord, 'toto')
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        assert normal_response['message'] == 'Unable to evaluate, invalid literal for int() with base 10: \'toto\''

    def test_graphical_isochrons_null_speed(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}&walking_speed=0"
        q = q.format('20120614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'The walking_speed argument has to be > 0, you gave : 0'

    def test_graphical_isochrons_no_duration(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}"
        q = q.format('20120614T080000', s_coord)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you should provide a \'duration\' argument'

    def test_graphical_isochrons_date_out_of_bound(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}"
        q = q.format('20050614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['id'] == 'date_out_of_bounds'
        assert normal_response['error']['message'] == 'date is not in data production period'

    def test_graphical_isochrons_invalid_datetime(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}"
        q = q.format('2005061', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'Unable to parse datetime, year is out of range'

        p = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}"
        p = p.format('toto', s_coord, 3600)
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        assert normal_response['message'] == 'Unable to parse datetime, unknown string format'

    def test_graphical_isochros_no_isochrons(self):
        q = "v1/coverage/main_routing_test/isochrons?datetime={}&from={}&duration={}"
        q = q.format('20120614T080000', '90;0', '3600')
        response = self.query(q)

        assert 'isochrons' not in response
