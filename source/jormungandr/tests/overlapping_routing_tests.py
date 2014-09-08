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
import logging
from jormungandr.instance_manager import InstanceManager
from navitiacommon import models

from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


class MockKraken:
    def __init__(self, kraken_instance, is_free):
        self.is_free = is_free
        self.kraken_instance = kraken_instance

@dataset(["main_routing_test", "empty_routing_test"])
class TestOverlappingCoverage(AbstractTestFixture):
    """
    Test the answer if 2 coverages are overlapping
    """
    def setup(self):
        from jormungandr import i_manager
        self.instance_map = {
            'main_routing_test': MockKraken(i_manager.instances['main_routing_test'], True),
            #the bad one is the non free one, so it will be chosen first
            'empty_routing_test': MockKraken(i_manager.instances['empty_routing_test'], False),
        }
        self.real_method = models.Instance.get_by_name

        models.Instance.get_by_name = self.mock_get_by_name

    def mock_get_by_name(self, name):
        return self.instance_map[name]

    def teardown(self):
        models.Instance.get_by_name = self.real_method

    def test_journeys(self):
        response = self.query("/v1/{q}".format(q=journey_basic_query), display=True)

        is_valid_journey_response(response, self.tester, journey_basic_query)
