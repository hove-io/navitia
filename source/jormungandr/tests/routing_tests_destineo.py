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

from tests_mechanism import AbstractTestFixture, dataset
from routing_tests import TestJourneys
from check_utils import *
from nose.tools import eq_
import jormungandr.scenarios.destineo
from jormungandr.instance import Instance

@dataset(["main_routing_test"])
class TestJourneysDestineo(TestJourneys):
    """
    Test the structure of the journeys response
    All the tests are defined in "TestJourneys" class, we only change the scenario
    if needed we can override the tests in this class
    """

    @classmethod
    def setup_class(cls):
        #we need to recopy the dataset else it won't find it
        AbstractTestFixture.data_sets = cls.data_sets
        logging.debug('setup for destineo')
        logging.debug(cls.data_sets)
        TestJourneys.setup_class()

        #we don't want to use the database for the scenario, so we mock the property of instance
        #but we want to test the scenario for destineo
        @property
        def mock_scenario(self):
            return jormungandr.scenarios.destineo.Scenario()
        Instance.scenario = mock_scenario

