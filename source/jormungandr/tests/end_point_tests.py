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

@dataset([])
class TestEmptyEndPoint(AbstractTestFixture):
    """
    Test main entry points.
    Do not need running kraken
    """

    def test_index(self):
        json_response = self.query("/")

        versions = get_not_null(json_response, 'versions')

        assert len(versions) == 2

        current_found = False
        for v in versions:
            status = v["status"]
            assert status in ["deprecated", "current"]

            if v["status"] == "current":
                #we want one and only one 'current' api version
                assert not current_found, "we can have only one current version of the api"
                current_found = True

            #all non templated links must be valid
            #TODO use get_links_dict to ensure the same link format
            #check_links(v, self.tester)

        assert current_found, "we must have one current version of the api"

    def test_empty_coverage(self):
        #even with no loaded kraken, the response should be valid
        json_response = self.query("/v1/coverage")

        assert 'regions' in json_response
        assert not json_response['regions']

    def test_v1(self):
        json_response = self.query("/v1")
        # this entry point is just links
        check_links(json_response, self.tester)


@dataset(['main_routing_test', 'main_ptref_test'])
class TestEndPoint(AbstractTestFixture):
    """
    Test the end point with 2 regions loaded
    """
    def test_coverage(self):
        json_response = self.query("/v1/coverage")

        check_links(json_response, self.tester)

        regions = get_not_null(json_response, 'regions')

        assert len(regions) == 2, "with 2 kraken loaded, we should have 2 regions"

        for region in regions:
            assert is_valid_date(get_not_null(region, 'start_production_date')), "no start production date"
            assert is_valid_date(get_not_null(region, 'end_production_date')), "no end production date"

            get_not_null(region, 'status')

            #shapes are not filled in dataset for the moment
            #shape = get_not_null(region, 'shape')
            #TODO check the shape with regexp ?

            region_id = get_not_null(region, 'id')

            assert region_id in ['main_routing_test', 'main_ptref_test']

