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
from tests.check_utils import journey_basic_query
from tests.tests_mechanism import dataset, AbstractTestFixture
from check_utils import *

@dataset(["main_ptref_test"])
class TestPtRef(AbstractTestFixture):
    """
    Test the structure of the ptref response
    """

    def test_vj_default_depth(self):
        """default depth is 1"""
        response = self.query_region("v1/vehicle_journeys")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=1)

        #we check afterward that we have the right data
        #we know there is only one vj in the dataset
        assert len(vjs) == 1
        vj = vjs[0]
        assert vj['id'] == 'vj1'

        assert len(vj['stop_times']) == 2
        assert vj['stop_times'][0]['arrival_time'] == '101500'
        assert vj['stop_times'][0]['departure_time'] == '101500'
        assert vj['stop_times'][1]['arrival_time'] == '111000'
        assert vj['stop_times'][1]['departure_time'] == '111000'

        #we added some comments on the vj, we should have them
        com = get_not_null(vj, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'standard'
        assert com[0]['value'] == 'hello'


    def test_vj_depth_0(self):
        """default depth is 1"""
        response = self.query_region("v1/vehicle_journeys?depth=0")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=0)

    def test_vj_depth_2(self):
        """default depth is 1"""
        response = self.query_region("v1/vehicle_journeys?depth=2")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=2)

    def test_vj_depth_3(self):
        """default depth is 1"""
        response = self.query_region("v1/vehicle_journeys?depth=3")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=3)

    def test_line(self):
        """test line formating"""
        response = self.query_region("v1/lines")

        lines = get_not_null(response, 'lines')

        assert len(lines) == 1

        l = lines[0]

        is_valid_line(l, depth_check=1)
        #we know we have a geojson for this test so we can check it
        geo = get_not_null(l, 'geojson')
        shape(geo)

        com = get_not_null(l, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'standard'
        assert com[0]['value'] == "I'm a happy comment"

        physical_modes = get_not_null(l, 'physical_modes')
        assert len(physical_modes) == 1

        is_valid_physical_mode(physical_modes[0], depth_check=1)

        assert physical_modes[0]['id'] == 'physical_mode:Car'
        assert physical_modes[0]['name'] == 'name physical_mode:Car'

        line_group = get_not_null(l, 'line_groups')
        assert len(line_group) == 1
        is_valid_line_group(line_group[0], depth_check=0)
        assert line_group[0]['name'] == 'A group'
        assert line_group[0]['id'] == 'group:A'

    def test_line_groups(self):
        """test line group formating"""
        # Test for each possible range to ensure main_line is always at a depth of 0
        for depth in range(0,3):
            response = self.query_region("line_groups?depth={0}".format(depth))

            line_groups = get_not_null(response, 'line_groups')

            assert len(line_groups) == 1

            lg = line_groups[0]

            is_valid_line_group(lg, depth_check=depth)

            if depth > 0:
                com = get_not_null(lg, 'comments')
                assert len(com) == 1
                assert com[0]['type'] == 'standard'
                assert com[0]['value'] == "I'm a happy comment"

        # test if line_groups are accessible through the ptref graph
        response = self.query_region("routes/line:A:0/line_groups")
        line_groups = get_not_null(response, 'line_groups')
        assert len(line_groups) == 1
        lg = line_groups[0]
        is_valid_line_group(lg)

    def test_line_codes(self):
        """test line formating"""
        response = self.query_region("v1/lines/line:A?show_codes=true")

        lines = get_not_null(response, 'lines')

        assert len(lines) == 1

        l = lines[0]

        codes = get_not_null(l, 'codes')

        assert len(codes) == 3

        is_valid_codes(codes)


    def test_route(self):
        """test line formating"""
        response = self.query_region("v1/routes")

        routes = get_not_null(response, 'routes')

        assert len(routes) == 1

        r = routes[0]
        is_valid_route(r, depth_check=1)

        #we know we have a geojson for this test so we can check it
        geo = get_not_null(r, 'geojson')
        shape(geo)

        com = get_not_null(r, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'standard'
        assert com[0]['value'] == "I'm a happy comment"

    def test_stop_areas(self):
        """test stop_areas formating"""
        response = self.query_region("v1/stop_areas")

        stops = get_not_null(response, 'stop_areas')

        assert len(stops) == 2

        s = next((s for s in stops if s['name'] == 'stop_area:stop1'))
        is_valid_stop_area(s, depth_check=1)

        com = get_not_null(s, 'comments')
        assert len(com) == 2
        assert com[0]['type'] == 'standard'
        assert com[0]['value'] == "comment on stop A"
        assert com[1]['type'] == 'standard'
        assert com[1]['value'] == "the stop is sad"

    def test_stop_points(self):
        """test stop_areas formating"""
        response = self.query_region("v1/stop_points")

        stops = get_not_null(response, 'stop_points')

        assert len(stops) == 2

        s = next((s for s in stops if s['name'] == 'stop_area:stop2'))
        is_valid_stop_area(s, depth_check=1)

        com = get_not_null(s, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'standard'
        assert com[0]['value'] == "hello bob"

    def test_company_default_depth(self):
        """default depth is 1"""
        response = self.query_region("v1/companies")

        companies = get_not_null(response, 'companies')

        for company in companies:
            is_valid_company(company, depth_check=1)

        #we check afterward that we have the right data
        #we know there is only one vj in the dataset
        assert len(companies) == 1
        company = companies[0]
        assert company['id'] == 'CMP1'

@dataset(["main_routing_test"])
class TestPtRefPlace(AbstractTestFixture):

    def test_with_coord(self):
        """test with a coord in the pt call, so a place nearby is actually called"""
        response = self.query_region("coords/{coord}/stop_areas".format(coord=r_coord))

        stops = get_not_null(response, 'stop_areas')

        for s in stops:
            is_valid_stop_area(s)

        #the default is the search for all stops within 200m, so we should have A and C
        eq_(len(stops), 2)
        assert ["stopA", "stopC"] == [s['name'] for s in stops]

    def test_with_coord_distance_different(self):
        """same as test_with_coord, but with 300m radius. so we find all stops"""
        response = self.query_region("coords/{coord}/stop_areas?distance=300".format(coord=r_coord))

        stops = get_not_null(response, 'stop_areas')

        for s in stops:
            is_valid_stop_area(s)

        eq_(len(stops), 3)
        assert ["stopA", "stopB", "stopC"] == [s['name'] for s in stops]

    def test_with_coord_and_filter(self):
        """
        we now test with a more complex query, we want all stops with a metro within 300m of r

        only A and C have a metro line
        Note: the metro is physical_mode:0x1
        """
        response = self.query_region("physical_modes/physical_mode:0x1/coords/{coord}/stop_areas?distance=300".format(coord=r_coord), display=True)

        stops = get_not_null(response, 'stop_areas')

        for s in stops:
            is_valid_stop_area(s)

        #the default is the search for all stops within 200m, so we should have all 3 stops
        #we should have 3 stops
        eq_(len(stops), 2)
        assert ["stopA", "stopC"] == [s['name'] for s in stops]
