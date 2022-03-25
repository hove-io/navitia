# Copyright (c) 2001-2021, Canal TP and/or its affiliates. All rights reserved.
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
        r = self.query_region('stop_points?depth=3')
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
                        assert ap['access_point_code'] == "access_point_code_2"
                    if ap['name'] == 'AP1':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == True
                        assert ap['length'] == 10
                        assert ap['traversal_time'] == 23
                        assert ap['access_point_code'] == "access_point_code_1"
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
                        assert ap['access_point_code'] == "access_point_code_3"
                    if ap['name'] == 'AP2':
                        assert ap['is_entrance'] == True
                        assert ap['is_exit'] == False
                        assert ap['length'] == 13
                        assert ap['traversal_time'] == 26
                        assert ap['access_point_code'] == "access_point_code_4"

        # without depth=3
        r = self.query_region('stop_points')
        assert len(get_not_null(r, 'stop_points')) == 7

        for sp in r['stop_points']:
            # spA
            if sp['name'] == 'spA':
                assert "access_points" not in sp
            # spB
            if sp['name'] == 'spC':
                assert "access_points" not in sp

    def test_access_points_with_places_nearby(self):
        response = self.query_region("coords/2.362795;48.872871/places_nearby?depth=3")
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

    def test_access_points_api(self):
        r = self.query_region('access_points')
        assert len(get_not_null(r, 'access_points')) == 3

        for ap in r['access_points']:
            if ap['name'] == 'AP1':
                assert ap['is_entrance'] == True
                assert ap['is_exit'] == True
            if ap['name'] == 'AP2':
                assert ap['is_entrance'] == True
                assert ap['is_exit'] == False
            if ap['name'] == 'AP3':
                assert ap['is_entrance'] == True
                assert ap['is_exit'] == False

    def test_access_points_api_with_filter(self):
        r = self.query_region('stop_points/spA/access_points')
        assert len(get_not_null(r, 'access_points')) == 2

        for ap in r['access_points']:
            if ap['name'] == 'AP1':
                assert ap['is_entrance'] == True
                assert ap['is_exit'] == True
            if ap['name'] == 'AP2':
                assert ap['is_entrance'] == True
                assert ap['is_exit'] == False

        r = self.query_region('stop_points/spB/access_points')
        assert len(r['access_points']) == 0
