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

from __future__ import absolute_import, print_function, unicode_literals, division
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *

@dataset({})
class TestEmptyEndPoint(AbstractTestFixture):
    """
    Test main entry points.
    Do not need running kraken
    """

    def test_index(self):
        json_response = self.query("/")

        versions = get_not_null(json_response, 'versions')

        assert len(versions) >= 1

        current_found = False
        for v in versions:
            status = v["status"]
            assert status in ["deprecated", "current", "testing"]

            if v["status"] == "current":
                #we want one and only one 'current' api version
                assert not current_found, "we can have only one current version of the api"
                current_found = True

            #all non templated links must be valid
            #TODO use get_links_dict to ensure the same link format
            #check_links(v, self.tester)

        assert current_found, "we must have one current version of the api"

"""
   TODO make this test OK, bug for the moment
 def test_status(self):
        json_response = self.query("/v1/status")

        status = get_not_null(json_response, 'jormungandr_version')

        assert is_valid_navitia_version_number(status)

        #we also must have an empty regions list
        assert 'regions' in json_response
        assert json_response['regions'] == []"""



@dataset({})
class TestHttps(AbstractTestFixture):
    """
    Test if https link are returned for forced hosts
    Do not need running kraken
    """

    def test_index(self):
        json_response = self.query("/")

        versions = get_not_null(json_response, 'versions')
        assert versions[0]['links'][0]['href'].startswith('http://')

    def test_index_with_scheme_https(self):
        json_response = self.query("/", headers={'X-Scheme': 'https'})

        versions = get_not_null(json_response, 'versions')
        assert versions[0]['links'][0]['href'].startswith('https://')

    def test_index_with_scheme_http(self):
        json_response = self.query("/", headers={'X-Scheme': 'http'})

        versions = get_not_null(json_response, 'versions')
        assert versions[0]['links'][0]['href'].startswith('http://')


@dataset({'main_routing_test':{},  'main_ptref_test': {}})
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

    def test_all_status(self):
        json_response = self.query("/v1/status")

        jormun_version = get_not_null(json_response, 'jormungandr_version')

        assert is_valid_navitia_version_number(jormun_version)

        #we also must have an empty regions list
        all_status = get_not_null(json_response, 'regions')
        assert len(all_status) == 2

        all_status_dict = {s['region_id']: s for s in all_status}

        main_status = get_not_null(all_status_dict, 'main_routing_test')

        is_valid_region_status(main_status)
        get_not_null(main_status, 'region_id')
        get_not_null(main_status, 'is_open_data') == False
        get_not_null(main_status, 'is_open_service') == False
        assert get_not_null(main_status, 'status') == 'running'

    def test_one_status(self):
        json_response = self.query("/v1/coverage/main_routing_test/status")

        is_valid_region_status(get_not_null(json_response, "status"))
        self.check_context(json_response)
        assert json_response["status"]["is_open_data"] == False
        assert json_response["status"]["is_open_service"] == False
        assert 'realtime_contributors' in json_response['status']
        assert 'realtime_proxies' in json_response['status']

    def test_geo_status(self):
        response = self.query('/v1/coverage/main_routing_test/_geo_status', display=True)
        self.check_context(response)
        assert response['context']['timezone'] == 'UTC'
        assert response['geo_status']
        assert response['geo_status']['nb_ways'] > 0
        assert response['geo_status']['nb_pois'] > 0
        assert response['geo_status']['nb_admins'] > 0
        assert response['geo_status']['nb_addresses'] > 0
        assert response['geo_status']['nb_admins_from_cities'] == 0
        assert response['geo_status']['street_network_sources'] == []
        assert response['geo_status']['poi_sources'] == []
