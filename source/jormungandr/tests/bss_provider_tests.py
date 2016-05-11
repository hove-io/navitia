# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
import mock
from mock import PropertyMock
from jormungandr.parking_space_availability.bss.bss_provider import BssProvider
from jormungandr.parking_space_availability.bss.stands import Stands
from tests.check_utils import is_valid_poi, get_not_null
from tests.tests_mechanism import AbstractTestFixture, dataset


class MockBssProvider(BssProvider):
    def support_poi(self, poi):
        # TODO
        return True

    def get_informations(self, poi):
        # TODO
        return Stands(available_places=13, available_bikes=3)


def mock_bss_providers():
    providers = [MockBssProvider()]
    return mock.patch('jormungandr.parking_space_availability.bss.BssProviderManager._get_providers',
                      new_callable=PropertyMock, return_value=lambda: providers)

@dataset({"main_routing_test": {}})
class TestBssProvider(AbstractTestFixture):

    def pois_test(self):
        with mock_bss_providers():
            r = self.query_region('pois', display=True)

            pois = get_not_null(r, 'pois')
            for p in pois:
                is_valid_poi(p)

            assert 'stands' in pois[0]
