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
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import get_not_null, is_valid_places


def access_point_is_present(access_point_list, access_point_name):
    for ap in access_point_list:
        if ap['name'] == access_point_name:
            return True
    return False


@dataset({"access_points_test": {}})
class TestAccessPoints(AbstractTestFixture):
    def test_access_points_with_stop_points(self):
        r = self.query_region('stop_points?depth=2')
        assert len(get_not_null(r, 'stop_points')) == 7

        for sp in r['stop_points']:
            # spA
            if sp['name'] == 'spA':
                assert len(get_not_null(sp, 'access_points')) == 2
                assert access_point_is_present(sp['access_points'], 'AP1')
                assert access_point_is_present(sp['access_points'], 'AP2')
                for ap in sp['access_points']:
                    if ap['name'] == 'AP2':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == False
                        assert ap['length'] == 13
                        assert ap['traversal_time'] == 26
                        assert ap['access_point']['access_point_code'] == "access_point_code_2"
                    if ap['name'] == 'AP1':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == True
                        assert ap['length'] == 10
                        assert ap['traversal_time'] == 23
                        assert ap['access_point']['access_point_code'] == "access_point_code_1"
            # spC
            if sp['name'] == 'spC':
                assert len(get_not_null(sp, 'access_points')) == 2
                assert access_point_is_present(sp['access_points'], 'AP2')
                assert access_point_is_present(sp['access_points'], 'AP3')
                for ap in sp['access_points']:
                    if ap['name'] == 'AP3':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == False
                        assert ap['length'] == 12
                        assert ap['traversal_time'] == 36
                        assert ap['access_point']['access_point_code'] == "access_point_code_3"
                    if ap['name'] == 'AP2':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == False
                        assert ap['length'] == 13
                        assert ap['traversal_time'] == 26
                        assert ap['access_point']['access_point_code'] == "access_point_code_4"

        # without depth=2
        r = self.query_region('stop_points')
        assert len(get_not_null(r, 'stop_points')) == 7

        for sp in r['stop_points']:
            # spA
            if sp['name'] == 'spA':
                assert "access_points" not in sp
            # spB
            if sp['name'] == 'spC':
                assert "access_points" not in sp

    def test_access_points_with_stop_points_depth_1(self):
        r = self.query_region('stop_points?depth=1')
        assert len(get_not_null(r, 'stop_points')) == 7

        for sp in r['stop_points']:
            assert "access_points" not in sp

    def test_access_points_with_places_nearby(self):
        response = self.query_region("coords/2.362795;48.872871/places_nearby?depth=2")
        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])

        for pn in response['places_nearby']:
            # spA
            if pn['embedded_type'] == 'stop_point' and pn['name'] == 'spA':
                assert len(get_not_null(pn['stop_point'], 'access_points')) == 2
                assert access_point_is_present(pn['stop_point']['access_points'], 'AP1')
                assert access_point_is_present(pn['stop_point']['access_points'], 'AP2')
                for ap in pn['stop_point']['access_points']:
                    if ap['name'] == 'AP2':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == False
                        assert ap['length'] == 13
                        assert ap['traversal_time'] == 26
                    if ap['name'] == 'AP1':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == True
                        assert ap['length'] == 10
                        assert ap['traversal_time'] == 23

    def test_access_points_with_places_nearby_depth_1(self):
        response = self.query_region("coords/2.362795;48.872871/places_nearby?depth=1")
        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])
        sp_places_nearby = [sp for sp in response['places_nearby'] if sp["embedded_type"] == "stop_point"]
        assert len(sp_places_nearby) == 6
        for pn in sp_places_nearby:
            assert "access_points" not in pn['stop_point']

    def test_access_points_api(self):
        r = self.query_region('access_points')
        assert len(get_not_null(r, 'access_points')) == 3

        for ap in r['access_points']:
            if ap['name'] == 'AP1':
                assert ap['access_point_code'] == "access_point_code_1"
                assert ap['id'] == "AP1"
                assert ap['coord'] == {'lon': '2.364795', 'lat': '48.874871'}
            if ap['name'] == 'AP2':
                assert ap['access_point_code'] == "access_point_code_2"
                assert ap['id'] == "AP2"
                assert ap['coord'] == {'lat': '48.875871', 'lon': '2.365795'}
            if ap['name'] == 'AP3':
                assert ap['access_point_code'] == "access_point_code_3"
                assert ap['id'] == "AP3"
                assert ap['coord'] == {'lat': '48.876871', 'lon': '2.366795'}

    def test_access_points_api_with_filter(self):
        r = self.query_region('stop_points/spA/access_points')
        assert len(get_not_null(r, 'access_points')) == 2

        for ap in r['access_points']:
            if ap['name'] == 'AP1':
                assert ap['access_point_code'] == "access_point_code_1"
                assert ap['id'] == "AP1"
                assert ap['coord'] == {'lon': '2.364795', 'lat': '48.874871'}
            if ap['name'] == 'AP2':
                assert ap['access_point_code'] == "access_point_code_2"
                assert ap['id'] == "AP2"
                assert ap['coord'] == {'lat': '48.875871', 'lon': '2.365795'}

        r = self.query_region('stop_points/spB/access_points')
        assert len(r['access_points']) == 0
