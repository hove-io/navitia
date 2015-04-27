# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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

from tests_mechanism import dataset
from routing_tests import TestJourneys
from check_utils import *
from nose.tools import eq_
import jormungandr.scenarios.new_default
from jormungandr.instance import Instance

@dataset(["main_routing_test"])
class TestJourneysNewDefault(TestJourneys):
    """
    Test the new default scenario
    All the tests are defined in "TestJourneys" class, we only change the scenario
    """

    def setup(self):
        logging.debug('setup for new default')
        from jormungandr import i_manager
        dest_instance = i_manager.instances['main_routing_test']
        self.old_scenario = dest_instance._scenario
        dest_instance._scenario = jormungandr.scenarios.new_default.Scenario()

    def teardown(self):
        from jormungandr import i_manager
        i_manager.instances['main_routing_test']._scenario = self.old_scenario


