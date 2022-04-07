# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from .check_utils import *
import logging


@dataset({"main_routing_test": {}, "null_status_test": {}})
class TestNullStatus(AbstractTestFixture):
    """
    Test with an empty coverage
    """

    def test_null_status_is_hidden(self):
        """
        There are two regions (main_routing_test, null_status_test),
        null_status_test must be hidden
        """
        response = self.query("/v1/coverage", display=False)
        logging.info(response['regions'])
        assert 'regions' in response
        assert len(response['regions']) == 1
        assert response['regions'][0]['id'] == 'main_routing_test'
        assert 'last_load_at' in response['regions'][0]
        assert response['regions'][0]['dataset_created_at'] == 'not-a-date-time'
        assert response['regions'][0]['last_load_at'] == 'not-a-date-time'
        assert 'name' in response['regions'][0]
        assert response['regions'][0]['name'] == 'routing api data'
        self.check_context(response)
        context = response['context']
        assert context['timezone'] == 'Africa/Abidjan'

    def test_null_status(self):
        """
        If we ask directly for the status of an hidden region we are able
        to see it.
        """
        response = self.query("/v1/coverage/null_status_test", display=False)
        logging.info(response['regions'])
        assert 'regions' in response
        assert len(response['regions']) == 1
        assert response['regions'][0]['id'] == 'null_status_test'
        self.check_context(response)
        context = response['context']
        assert context['timezone'] == 'Africa/Abidjan'

    def test_street_network_backend_status(self):
        """
        There are two regions (main_routing_test, null_status_test),
        null_status_test must be hidden
        """
        response = self.query("/v1/coverage/main_routing_test/status", display=False)
        sn_backends = response['status']['street_networks']
        modes = [mode for sn in sn_backends for mode in sn['modes']]
        assert len(modes) == 7
        assert set(modes) == set(['walking', 'bike', 'bss', 'car', 'car_no_park', 'ridesharing', 'taxi'])

    # @raises(AssertionError)
    # def test_couverage_with_headers(self):
    #     """test coverage with headers"""
    #     headers = {'content-type': 'application/json'}
    #     self.query("v1/coverage", headers=headers)
