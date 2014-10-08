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
        response = self.query_region("v1/vehicle_journeys", display=True)

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
