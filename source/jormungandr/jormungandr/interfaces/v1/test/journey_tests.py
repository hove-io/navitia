# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
import pytest
from jormungandr import i_manager
from jormungandr.exceptions import RegionNotFound
from jormungandr.interfaces.v1.journey_common import compute_regions, sort_regions
from navitiacommon import models


class MockInstance:
    def __init__(self, is_free, name, priority):
        self.is_free = is_free
        self.name = name
        self.priority = priority


class TestMultiCoverage:
    # TODO change that with real mock
    def setup_method(self, method):
        # we mock a query
        self.args = {'origin': 'paris', 'destination': 'lima'}
        # for each test, we mock the instance filtering

        # and we will use a list of instances
        self.regions = {
            'equador': MockInstance(True, 'equador', 0),
            'france': MockInstance(False, 'france', 0),
            'peru': MockInstance(False, 'peru', 0),
            'bolivia': MockInstance(True, 'bolivia', 0),
            'netherlands': MockInstance(False, 'netherlands', 25),
            'germany': MockInstance(False, 'germany', 50),
            'brazil': MockInstance(True, 'brazil', 100),
        }

    def _mock_function(self, paris_region, lima_region):
        """
        small helper, mock i_manager.get_region
        """

        def mock_get_instances(region_str=None, lon=None, lat=None, object_id=None, api='ALL', only_one=True):
            if object_id == 'paris':
                if not paris_region:
                    raise RegionNotFound('paris')
                return [self.regions[r] for r in paris_region]
            if object_id == 'lima':
                if not lima_region:
                    raise RegionNotFound('lima')
                return [self.regions[r] for r in lima_region]

        i_manager.get_instances = mock_get_instances
        # we also need to mock the ptmodel cache
        class weNeedMock:
            @classmethod
            def get_by_name(cls, i):
                return i

        models.Instance.get_by_name = weNeedMock.get_by_name

    def test_multi_coverage_simple(self):
        """Simple test, paris and lima are in the same region, should be easy"""
        self._mock_function(['france'], ['france'])

        regions = compute_regions(self.args)

        assert len(regions) == 1

        assert regions[0] == self.regions['france'].name

    def test_multi_coverage_diff_region(self):
        """orig and dest are in different region"""
        self._mock_function(['france'], ['peru'])

        with pytest.raises(RegionNotFound):
            compute_regions(self.args)

    def test_multi_coverage_no_region_peru(self):
        """no orig """
        self._mock_function(None, ['peru'])

        with pytest.raises(RegionNotFound):
            compute_regions(self.args)

    def test_multi_coverage_no_region(self):
        """no orig not dest"""
        self._mock_function(None, None)

        with pytest.raises(RegionNotFound):
            compute_regions(self.args)

    def test_multi_coverage_diff_multiple_region(self):
        """orig and dest are in different multiple regions"""
        self._mock_function(['france', 'bolivia'], ['peru', 'equador'])

        with pytest.raises(RegionNotFound):
            compute_regions(self.args)

    def test_multi_coverage_overlap(self):
        """orig as 2 possible region and dest one"""
        self._mock_function(['france', 'peru'], ['peru'])

        regions = compute_regions(self.args)

        assert len(regions) == 1

        assert regions[0] == self.regions['peru'].name

    def test_multi_coverage_overlap_chose_non_free(self):
        """orig as 2 possible region and destination 3, we want to have france first because equador is free"""
        self._mock_function(['france', 'equador'], ['france', 'peru', 'equador'])

        regions = compute_regions(self.args)

        assert len(regions) == 2
        print("regions ==> {}".format(regions))

        assert regions[0] == self.regions['france'].name
        assert regions[1] == self.regions['equador'].name

    def test_multi_coverage_overlap_chose_non_free2_with_priority_zero(self):
        """
        all regions are overlaping,
        we have to have the non free first then the free (but we don't know which one)
        """
        self._mock_function(['france', 'equador', 'peru', 'bolivia'], ['france', 'equador', 'peru', 'bolivia'])

        regions = compute_regions(self.args)

        assert len(regions) == 4
        print("regions ==> {}".format(regions))

        assert set([regions[0], regions[1]]) == set([self.regions['france'].name, self.regions['peru'].name])
        assert set([regions[2], regions[3]]) == set([self.regions['equador'].name, self.regions['bolivia'].name])

    def test_multi_coverage_overlap_chose_with_non_free_and_priority(self):
        """
        all regions are overlaping,
        regions are sorted by priority desc
        """
        self._mock_function(['france', 'netherlands', 'germany'], ['france', 'netherlands', 'germany'])

        regions = compute_regions(self.args)

        assert len(regions) == 3
        print("regions ==> {}".format(regions))

        assert regions[0] == self.regions['germany'].name
        assert regions[1] == self.regions['netherlands'].name
        assert regions[2] == self.regions['france'].name

    def test_multi_coverage_overlap_chose_with_priority(self):
        """
        4 regions are overlaping,
        regions are sorted by priority desc
        """
        self._mock_function(
            ['france', 'netherlands', 'brazil', 'bolivia', 'germany'],
            ['france', 'netherlands', 'brazil', 'bolivia'],
        )

        regions = compute_regions(self.args)

        assert len(regions) == 4
        print("regions ==> {}".format(regions))

        assert regions[0] == self.regions['brazil'].name
        assert regions[1] == self.regions['netherlands'].name
        assert regions[2] == self.regions['france'].name
        assert regions[3] == self.regions['bolivia'].name

    def test_sorted_regions(self):
        """
        Test that the regions are sorted correctly according to the comparator criteria (priority > is_free=False > is_free=True)
        """
        # Remove the 2 following instances as they have same configuration as others
        self.regions.pop('peru')
        self.regions.pop('bolivia')

        regions = sort_regions(self.regions.values())
        assert len(regions) == 5
        assert regions[0].name == self.regions['brazil'].name
        assert regions[1].name == self.regions['germany'].name
        assert regions[2].name == self.regions['netherlands'].name
        assert regions[3].name == self.regions['france'].name
        assert regions[4].name == self.regions['equador'].name
