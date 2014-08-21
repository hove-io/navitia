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
from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *
from jormungandr import stat_manager
from jormungandr.stat_manager import StatManager


class StatError(Exception):
    pass


def always_in_error(self, start_time, func_call):
    raise StatError()


@dataset(["main_routing_test"])
class TestNoStats(AbstractTestFixture):

    def setup(self):
        """
         change the persist stat method of the stat manager with a mock function that always fail
        """
        self.real_persist_stat_method = StatManager._manage_stat
        self.real_save_stat_val = stat_manager.save_stat
        StatManager._manage_stat = always_in_error

    def teardown(self):
        """
        set back the previous params
        """
        StatManager._manage_stat = self.real_persist_stat_method
        stat_manager.save_stat = self.real_save_stat_val

    def test_simple_journey_query_no_stats(self):
        """
        test on a valid journey (tested in routing_tests.py).
        stats deactivation should not alter results
        """
        stat_manager.save_stat = False # we disable the stats
        response = self.query_region(journey_basic_query, display=False)

        is_valid_journey_response(response, self.tester, journey_basic_query)

    def test_simple_journey_query_stats(self):
        """
        Even if the stat manager is failing we want the journey result.
        """
        response = self.query_region(journey_basic_query, display=False)
        is_valid_journey_response(response, self.tester, journey_basic_query)