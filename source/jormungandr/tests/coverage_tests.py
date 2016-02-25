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
import logging


@dataset({"main_routing_test":{}, "null_status_test":{}})
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
        assert get_valid_datetime(response['regions'][0]["last_load_at"])
        assert 'name' in response['regions'][0]
        assert response['regions'][0]['name'] == 'routing api data'

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
