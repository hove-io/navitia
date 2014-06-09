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
from jormungandr import i_manager
from jormungandr.exceptions import RegionNotFound
from nose.tools import *
from jormungandr.interfaces.v1.Journeys import compute_regions


class MockInstance:
    def __init__(self, is_free):
        self.is_free = is_free


class TestMultiCoverage:
    #TODO change that with real mock and change nose to py.test to be able to use test generator
    def __init__(self):
        #we mock a query
        self.args = {
            'origin': 'paris',
            'destination': 'lima'
        }
        #for each test, we mock the instance filtering

        #and we will use a list of instances
        self.regions = {
            'equador': MockInstance(True),
            'france': MockInstance(False),
            'peru': MockInstance(False),
            'bolivia': MockInstance(True)
        }

    def _mock_function(self, paris_region, lima_region):
        """
        small helper, mock i_manager.key_of_id
        """
        def mock_key_by_id(object_id, only_one=True):
            if object_id == 'paris':
                if not paris_region:
                    raise RegionNotFound('paris')
                return [self.regions[r] for r in paris_region]
            if object_id == 'lima':
                if not lima_region:
                    raise RegionNotFound('lima')
                return [self.regions[r] for r in lima_region]

        i_manager.key_of_id = mock_key_by_id

    def test_multi_coverage_simple(self):
        """Simple test, paris and lima are in the same region, should be easy"""
        self._mock_function(['france'], ['france'])

        region = compute_regions(self.args)

        assert region == self.regions['france']

    @raises(RegionNotFound)
    def test_multi_coverage_diff_region(self):
        """orig and dest are in different region"""
        self._mock_function(['france'], ['peru'])

        compute_regions(self.args)

    @raises(RegionNotFound)
    def test_multi_coverage_no_region(self):
        """no orig """
        self._mock_function(None, ['peru'])

        compute_regions(self.args)

    @raises(RegionNotFound)
    def test_multi_coverage_no_region(self):
        """no orig not dest"""
        self._mock_function(None, None)

        compute_regions(self.args)

    @raises(RegionNotFound)
    def test_multi_coverage_diff_multiple_region(self):
        """orig and dest are in different multiple regions"""
        self._mock_function(['france', 'bolivia'], ['peru', 'equador'])

        compute_regions(self.args)

    def test_multi_coverage_overlap(self):
        """orig as 2 possible region and dest one"""
        self._mock_function(['france', 'peru'], ['peru'])

        region = compute_regions(self.args)

        assert region == self.regions['peru']

    def test_multi_coverage_overlap_chose_non_free(self):
        """orig as 2 possible region and destination 3, we want to chose the france because equador is free"""
        self._mock_function(['france', 'equador'], ['france', 'peru', 'equador'])

        region = compute_regions(self.args)

        assert region == self.regions['france']

    def test_multi_coverage_overlap_chose_non_free2(self):
        """all regions and overlaping, we have to return one of the non free region (we don't know which one)"""
        self._mock_function(['france', 'equador', 'peru', 'bolivia'], ['france', 'equador', 'peru', 'bolivia'])

        region = compute_regions(self.args)

        assert region in [self.regions['france'], self.regions['peru']]