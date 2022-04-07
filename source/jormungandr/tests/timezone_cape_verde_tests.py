# Copyright (c) 2001-2017, Hove and/or its affiliates. All rights reserved.
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
import logging

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


@dataset({"timezone_cape_verde_test": {}})
class TestTimezoneCapeVerde(AbstractTestFixture):
    def test_double_pass_midnight(self):
        r = self.query_region('vehicle_journeys/vehicle_journey:vj:1:1')

        assert len(get_not_null(r, 'vehicle_journeys')) == 1
        stop_times = r['vehicle_journeys'][0]['stop_times']
        assert len(stop_times) == 7
        assert stop_times[0]['arrival_time'] == '220000'
        assert stop_times[0]['departure_time'] == '220000'
        assert stop_times[1]['arrival_time'] == '223000'
        assert stop_times[1]['departure_time'] == '223000'
        assert stop_times[2]['arrival_time'] == '230000'
        assert stop_times[2]['departure_time'] == '230000'
        assert stop_times[3]['arrival_time'] == '233000'
        assert stop_times[3]['departure_time'] == '233000'
        assert stop_times[4]['arrival_time'] == '000000'
        assert stop_times[4]['departure_time'] == '000000'
        assert stop_times[5]['arrival_time'] == '003000'
        assert stop_times[5]['departure_time'] == '003000'
        assert stop_times[6]['arrival_time'] == '010000'
        assert stop_times[6]['departure_time'] == '010000'

    def test_local_pass_midnight(self):
        r = self.query_region('vehicle_journeys/vehicle_journey:vj:1:2')

        assert len(get_not_null(r, 'vehicle_journeys')) == 1
        stop_times = r['vehicle_journeys'][0]['stop_times']
        assert len(stop_times) == 5
        assert stop_times[0]['arrival_time'] == '230000'
        assert stop_times[0]['departure_time'] == '230000'
        assert stop_times[1]['arrival_time'] == '233000'
        assert stop_times[1]['departure_time'] == '233000'
        assert stop_times[2]['arrival_time'] == '000000'
        assert stop_times[2]['departure_time'] == '000000'
        assert stop_times[3]['arrival_time'] == '003000'
        assert stop_times[3]['departure_time'] == '003000'
        assert stop_times[4]['arrival_time'] == '010000'
        assert stop_times[4]['departure_time'] == '010000'
