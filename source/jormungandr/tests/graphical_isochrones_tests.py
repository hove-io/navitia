# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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

from .tests_mechanism import AbstractTestFixture, dataset, config, NewDefaultScenarioAbstractTestFixture
from .check_utils import *
from jormungandr import app
from shapely.geometry import asShape, Point
from six.moves import range
from six.moves import zip


@dataset({"main_routing_test": {}})
class TestGraphicalIsochrone(AbstractTestFixture):
    """
    Test the structure of the isochrones response
    """

    def test_from_graphical_isochrone_coord(self):
        query = "v1/coverage/main_routing_test/isochrones?from={}&datetime={}&max_duration={}"
        query = query.format(s_coord, "20120614T080000", "3600")
        response = self.query(query)
        origin = Point(0.0000898312, 0.0000898312)
        max_duration = response['isochrones'][0]['max_duration']
        requested_datetime = response['isochrones'][0]['requested_date_time']
        min_datetime = response['isochrones'][0]['min_date_time']
        max_datetime = response['isochrones'][0]['max_date_time']
        d = response['isochrones'][0]['geojson']
        multi_poly = asShape(d)

        assert requested_datetime == '20120614T080000'
        assert min_datetime == '20120614T080000'
        assert max_datetime == '20120614T090000'
        assert max_duration == 3600
        assert multi_poly.contains(origin)
        is_valid_graphical_isochrone(response, self.tester, query)
        self.check_context(response)

    def test_to_graphical_isochrone_coord(self):
        query = "v1/coverage/main_routing_test/isochrones?to={}&datetime={}&max_duration={}"
        query = query.format(s_coord, "20120614T080000", "3600")
        response = self.query(query)
        destination = Point(0.0000898312, 0.0000898312)
        d = response['isochrones'][0]['geojson']
        multi_poly = asShape(d)

        assert multi_poly.contains(destination)
        is_valid_graphical_isochrone(response, self.tester, query)

    def test_graphical_isochrones_from_stop_point(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', 'stopA', '3600')
        response = self.query(q)
        stopA = Point(0.000718649585564, 0.00107797437835)
        d = response['isochrones'][0]['geojson']
        multi_poly = asShape(d)

        assert multi_poly.contains(stopA)
        is_valid_graphical_isochrone(response, self.tester, q)

    def test_graphical_isochrones_to_stop_point(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&to={}&max_duration={}"
        q = q.format('20120614T080000', 'stopA', '3600')
        response = self.query(q)
        stopA = Point(0.000718649585564, 0.00107797437835)
        d = response['isochrones'][0]['geojson']
        multi_poly = asShape(d)

        assert multi_poly.contains(stopA)
        is_valid_graphical_isochrone(response, self.tester, q)

    def test_graphical_isochrones_no_datetime(self):
        current_datetime = '20120614T080000'
        q_no_dt = "v1/coverage/main_routing_test/isochrones?from={}&max_duration={}&_current_datetime={}"
        q_no_dt = q_no_dt.format(s_coord, '3600', current_datetime)
        response_no_dt = self.query(q_no_dt)

        excepted_context = {'current_datetime': current_datetime, 'timezone': 'UTC'}
        self.check_context(response_no_dt)

        for key, value in excepted_context.items():
            assert response_no_dt['context'][key] == value

        q_dt = "v1/coverage/main_routing_test/isochrones?from={}&datetime={}&max_duration={}"
        q_dt = q_dt.format(s_coord, '20120614T080000', '3600')
        isochrone = self.query(q_dt)

        is_valid_graphical_isochrone(response_no_dt, self.tester, q_no_dt)
        assert len(response_no_dt['isochrones']) == len(isochrone['isochrones'])

        for isochrone_no_dt, isochrone in zip(response_no_dt['isochrones'], isochrone['isochrones']):
            multi_poly_no_datetime = asShape(isochrone_no_dt['geojson'])
            multi_poly = asShape(isochrone['geojson'])

            assert multi_poly_no_datetime.equals(multi_poly)

    def test_graphical_isochrones_no_seconds_in_datetime(self):
        q_no_s = "v1/coverage/main_routing_test/isochrones?from={}&max_duration={}&datetime={}"
        q_no_s = q_no_s.format(s_coord, '3600', '20120614T0800')
        response_no_s = self.query(q_no_s)
        q_s = "v1/coverage/main_routing_test/isochrones?from={}&datetime={}&max_duration={}"
        q_s = q_s.format(s_coord, '20120614T080000', '3600')
        isochrone = self.query(q_s)

        is_valid_graphical_isochrone(response_no_s, self.tester, q_no_s)
        assert len(response_no_s['isochrones']) == len(isochrone['isochrones'])

        for isochrone_no_s, isochrone in zip(response_no_s['isochrones'], isochrone['isochrones']):
            multi_poly_no_s = asShape(isochrone_no_s['geojson'])
            multi_poly = asShape(isochrone['geojson'])

            assert multi_poly_no_s.equals(multi_poly)

    def test_graphical_isochrones_speed_factor(self):
        q_speed_2 = (
            "v1/coverage/main_routing_test/isochrones?from={}&max_duration={}&datetime={}&walking_speed={}"
        )
        q_speed_2 = q_speed_2.format(s_coord, '3600', '20120614T0800', 2)
        response_speed_2 = self.query(q_speed_2)
        q_speed_1 = "v1/coverage/main_routing_test/isochrones?from={}&datetime={}&max_duration={}"
        q_speed_1 = q_speed_1.format(s_coord, '20120614T080000', '3600')
        isochrone = self.query(q_speed_1)

        is_valid_graphical_isochrone(response_speed_2, self.tester, q_speed_2)

        for isochrone_speed_2, isochrone in zip(response_speed_2['isochrones'], isochrone['isochrones']):
            multi_poly_speed_2 = asShape(isochrone_speed_2['geojson'])
            multi_poly = asShape(isochrone['geojson'])

            assert multi_poly_speed_2.contains(multi_poly)

    def test_graphical_isochrones_min_duration(self):
        q_min_1200 = (
            "v1/coverage/main_routing_test/isochrones?from={}&max_duration={}&datetime={}&min_duration={}"
        )
        q_min_1200 = q_min_1200.format(s_coord, '3600', '20120614T0800', '1200')
        response_min_1200 = self.query(q_min_1200)

        q_max_1200 = "v1/coverage/main_routing_test/isochrones?from={}&max_duration={}&datetime={}"
        q_max_1200 = q_max_1200.format(s_coord, '1200', '20120614T0800')
        response_max_1200 = self.query(q_max_1200)

        is_valid_graphical_isochrone(response_min_1200, self.tester, q_min_1200)

        for isochrone_min_1200, isochrone_max_1200 in zip(
            response_min_1200['isochrones'], response_max_1200['isochrones']
        ):
            multi_poly_min_1200 = asShape(isochrone_min_1200['geojson'])
            multi_poly_max_1200 = asShape(isochrone_max_1200['geojson'])

            assert not multi_poly_min_1200.contains(multi_poly_max_1200)
            assert not multi_poly_max_1200.contains(multi_poly_min_1200)

        q_max_3600 = "v1/coverage/main_routing_test/isochrones?from={}&max_duration={}&datetime={}"
        q_max_3600 = q_max_3600.format(s_coord, '3600', '20120614T0800')
        response_max_3600 = self.query(q_max_3600)
        for isochrone_min_1200, isochrone_max_3600 in zip(
            response_min_1200['isochrones'], response_max_3600['isochrones']
        ):
            multi_poly_min_1200 = asShape(isochrone_min_1200['geojson'])
            multi_poly_max_3600 = asShape(isochrone_max_3600['geojson'])

            assert not multi_poly_min_1200.contains(multi_poly_max_3600)
            assert multi_poly_max_3600.contains(multi_poly_min_1200)

    def test_graphical_isochrone_traveler_type(self):
        q_slow_walker = "v1/coverage/main_routing_test/" + isochrone_basic_query + "&traveler_type=slow_walker"
        q_standard_walker = "v1/coverage/main_routing_test/" + isochrone_basic_query + "&traveler_type=standard"
        q_fast_walker = "v1/coverage/main_routing_test/" + isochrone_basic_query + "&traveler_type=fast_walker"
        response_slow_walker = self.query(q_slow_walker)
        response_standard_walker = self.query(q_standard_walker)
        response_fast_walker = self.query(q_fast_walker)

        is_valid_graphical_isochrone(response_slow_walker, self.tester, q_slow_walker)
        is_valid_graphical_isochrone(response_standard_walker, self.tester, q_standard_walker)
        is_valid_graphical_isochrone(response_fast_walker, self.tester, q_fast_walker)

        for isochrone_slow_walker, isochrone_standard_walker, isochrone_fast_walker in zip(
            response_slow_walker['isochrones'],
            response_standard_walker['isochrones'],
            response_fast_walker['isochrones'],
        ):
            multi_poly_slow_walker = asShape(isochrone_slow_walker['geojson'])
            multi_poly_standard_walker = asShape(isochrone_standard_walker['geojson'])
            multi_poly_fast_walker = asShape(isochrone_fast_walker['geojson'])

            assert multi_poly_standard_walker.contains(multi_poly_slow_walker)
            assert multi_poly_fast_walker.contains(multi_poly_standard_walker)

    def test_graphical_isochrone_section_mode(self):
        q_section_mode = (
            "v1/coverage/main_routing_test/" + isochrone_basic_query + "&first_section_mode[]=bike"
            "&last_section_mode[]=bss"
        )
        q_basic = "v1/coverage/main_routing_test/" + isochrone_basic_query
        response_section_mode = self.query(q_section_mode)
        response_basic = self.query(q_basic)

        is_valid_graphical_isochrone(response_section_mode, self.tester, q_section_mode)
        is_valid_graphical_isochrone(response_basic, self.tester, q_basic)

        for isochrone_basic, isochrone_section_mode in zip(
            response_basic['isochrones'], response_section_mode['isochrones']
        ):
            multi_poly_basic = asShape(isochrone_basic['geojson'])
            multi_poly_section_mode = asShape(isochrone_section_mode['geojson'])

            assert not multi_poly_basic.contains(multi_poly_section_mode)

    def test_graphical_isochrone_multi_isochrone(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}"
        q = q.format('20120614T080000', r_coord)
        for i in range(8):
            q += "&boundary_duration[]={}"
            q = q.format(i * 60)
        response = self.query(q)
        is_valid_graphical_isochrone(response, self.tester, q)

        assert len(response['isochrones']) == 7

    def test_graphical_isochrones_forbidden_uris(self):
        basic_query = "v1/coverage/main_routing_test/isochrones?from={}&datetime={}&max_duration={}"
        basic_query = basic_query.format(s_coord, "20120614T080000", "300")
        basic_response = self.query(basic_query)
        q_forbidden_uris = basic_query + "&forbidden_uris[]=A"
        response_forbidden_uris = self.query(q_forbidden_uris)

        is_valid_graphical_isochrone(basic_response, self.tester, basic_query)
        is_valid_graphical_isochrone(response_forbidden_uris, self.tester, basic_query)

        for basic_isochrone, isochrone_forbidden_uris in zip(
            basic_response['isochrones'], response_forbidden_uris['isochrones']
        ):
            basic_multi_poly = asShape(basic_isochrone['geojson'])
            multi_poly_forbidden_uris = asShape(isochrone_forbidden_uris['geojson'])

            assert not multi_poly_forbidden_uris.contains(basic_multi_poly)
            assert basic_multi_poly.area > multi_poly_forbidden_uris.area

    def test_graphical_isochrones_allowed_id(self):
        basic_query = "v1/coverage/main_routing_test/isochrones?from={}&datetime={}&max_duration={}"
        basic_query = basic_query.format(s_coord, "20120614T080000", "300")
        basic_response = self.query(basic_query)
        q_allowed_id = basic_query + "&allowed_id[]=B"
        response_allowed_id = self.query(q_allowed_id)

        is_valid_graphical_isochrone(basic_response, self.tester, basic_query)
        is_valid_graphical_isochrone(response_allowed_id, self.tester, basic_query)

        for basic_isochrone, isochrone_allowed_id in zip(
            basic_response['isochrones'], response_allowed_id['isochrones']
        ):
            basic_multi_poly = asShape(basic_isochrone['geojson'])
            multi_poly_allowed_id = asShape(isochrone_allowed_id['geojson'])

            assert not multi_poly_allowed_id.contains(basic_multi_poly)
            assert basic_multi_poly.area > multi_poly_allowed_id.area

    def test_graphical_isochrones_no_arguments(self):
        q = "v1/coverage/main_routing_test/isochrones"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you should provide a \'from\' or a \'to\' argument'

    def test_graphical_isochrones_no_region(self):
        q = "v1/coverage/isochrones"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 403
        assert 'message' in normal_response
        assert (
            normal_response['message']
            == "You don't have the permission to access the requested resource. It is either read-protected or not readable by the server."
        )

    def test_graphical_isochrones_invalid_duration(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', s_coord, '-3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert (
            'Unable to evaluate, invalid unsigned int\n'
            'max_duration description: Maximum duration of journeys in seconds' in normal_response['message']
        )

        p = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        p = p.format('20120614T080000', s_coord, 'toto')
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        assert (
            'Unable to evaluate, invalid literal for int() with base 10: \'toto\'' in normal_response['message']
        )

    def test_graphical_isochrones_null_speed(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}&walking_speed=0"
        q = q.format('20120614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert (
            'The walking_speed argument has to be in range [0.01, 4], you gave : 0' in normal_response['message']
        )

    def test_graphical_isochrones_no_duration(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}"
        q = q.format('20120614T080000', s_coord)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert (
            normal_response['message']
            == "you should provide a 'boundary_duration[]' or a 'max_duration' argument"
        )

    def test_graphical_isochrones_date_out_of_bound(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        q = q.format('20050614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['id'] == 'date_out_of_bounds'
        assert normal_response['error']['message'] == 'date is not in data production period'

    def test_graphical_isochrones_invalid_datetime(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        q = q.format('2005061', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert 'Unable to parse datetime, year' in normal_response['message']

        p = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        p = p.format('toto', s_coord, 3600)
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        assert 'unable to parse datetime, unknown string format' in normal_response['message'].lower()

    def test_graphical_isochrones_no_isochrones(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', 'stop_area:OIF:SA:toto', '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['id'] == "unknown_object"
        assert normal_response['error']['message'] == 'The entry point: stop_area:OIF:SA:toto is not valid'

    def test_grapical_isochrone_with_from_and_to(self):
        q = "v1/coverage/main_routing_test/" + isochrone_basic_query + "&to={}"
        q = q.format(r_coord)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you cannot provide a \'from\' and a \'to\' argument'

    def test_grapical_isochrone_more_than_10_duration(self):
        q = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}"
        q = q.format('20120614T080000', r_coord)
        for i in range(20):
            q += "&boundary_duration[]={}".format(i * 60)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you cannot provide more than 10 \'boundary_duration[]\''


@dataset({"main_routing_test": {"scenario": "new_default"}})
class TestGraphicalIsochroneTaxiNewDefault(NewDefaultScenarioAbstractTestFixture):
    def test_graphical_isochrone_taxi_with_new_default(self):
        query = "isochrones?from={}&datetime={}&max_duration={}&first_section_mode[]=taxi"
        query = query.format(s_coord, "20120614T080000", "3600")
        response = self.query_region(query, check=False)
        print(response)
        assert response[1] == 400
        assert "taxi is not available with new_default scenario" in response[0]['message']


@config({"scenario": "distributed"})
class TestGraphicalIsochroneDistributed(TestGraphicalIsochrone, NewDefaultScenarioAbstractTestFixture):
    pass


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestGraphicalIsochroneTaxiDistributed(NewDefaultScenarioAbstractTestFixture):
    def test_graphical_isochrone_taxi_with_new_default(self):
        # We should have the same response as "test_graphical_isochrone_section_mode"
        q_section_mode = (
            "v1/coverage/main_routing_test/" + isochrone_basic_query + "&first_section_mode[]=taxi"
            "&last_section_mode[]=taxi&taxi_speed=4.1"
        )
        q_basic = "v1/coverage/main_routing_test/" + isochrone_basic_query
        response_section_mode = self.query(q_section_mode)
        response_basic = self.query(q_basic)

        is_valid_graphical_isochrone(response_section_mode, self.tester, q_section_mode)
        is_valid_graphical_isochrone(response_basic, self.tester, q_basic)

        for isochrone_basic, isochrone_section_mode in zip(
            response_basic['isochrones'], response_section_mode['isochrones']
        ):
            multi_poly_basic = asShape(isochrone_basic['geojson'])
            multi_poly_section_mode = asShape(isochrone_section_mode['geojson'])

            assert not multi_poly_basic.contains(multi_poly_section_mode)
