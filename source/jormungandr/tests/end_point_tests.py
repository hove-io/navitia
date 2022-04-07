# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from .tests_mechanism import AbstractTestFixture, dataset, dataset, mock_equipment_providers
from .check_utils import *
import pytest


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
                # we want one and only one 'current' api version
                assert not current_found, "we can have only one current version of the api"
                current_found = True

            # all non templated links must be valid
            # TODO use get_links_dict to ensure the same link format
            # check_links(v, self.tester)

        assert current_found, "we must have one current version of the api"


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


@dataset(
    {
        'main_routing_test': {'instance_config': {'zmq_socket_type': 'transient'}},
        'main_ptref_test': {'instance_config': {'zmq_socket_type': 'transient'}},
    }
)
class TestEndPoint(AbstractTestFixture):
    """
    Test the end point with 2 regions loaded
    """

    def test_coverage(self):
        json_response = self.query("/v1/coverage")

        check_links(json_response, self.tester)

        assert json_response['context']['timezone'] == 'Africa/Abidjan'
        regions = get_not_null(json_response, 'regions')

        assert len(regions) == 2, "with 2 kraken loaded, we should have 2 regions"

        for region in regions:
            assert is_valid_date(get_not_null(region, 'start_production_date')), "no start production date"
            assert is_valid_date(get_not_null(region, 'end_production_date')), "no end production date"

            get_not_null(region, 'status')

            # shapes are not filled in dataset for the moment
            # shape = get_not_null(region, 'shape')
            # TODO check the shape with regexp ?

            region_id = get_not_null(region, 'id')

            assert region_id in ['main_routing_test', 'main_ptref_test']

    @pytest.mark.xfail(
        strict=False, reason="github action doesn't have the version number", raises=AssertionError
    )
    def test_technical_status(self):
        json_response = self.query("/v1/status")

        jormun_version = get_not_null(json_response, 'jormungandr_version')

        assert is_valid_navitia_version_number(jormun_version)
        assert json_response['context']['timezone'] == 'Africa/Abidjan'

        # we also must have an empty regions list
        all_status = get_not_null(json_response, 'regions')
        assert len(all_status) == 2

        all_status_dict = {s['region_id']: s for s in all_status}

        main_status = get_not_null(all_status_dict, 'main_routing_test')

        is_valid_region_status(main_status)
        assert get_not_null(main_status, 'region_id') == "main_routing_test"
        assert get_not_null(main_status, 'is_open_data') is False
        assert get_not_null(main_status, 'is_open_service') is False
        assert get_not_null(main_status, 'status') == 'running'

        assert 'warnings' in json_response
        assert len(json_response['warnings']) == 1
        assert json_response['warnings'][0]['id'] == 'beta_endpoint'

    def test_one_status(self):
        json_response = self.query("/v1/coverage/main_routing_test/status")

        is_valid_region_status(get_not_null(json_response, "status"))
        self.check_context(json_response)
        assert json_response['context']['timezone'] == 'Africa/Abidjan'
        assert json_response["status"]["is_open_data"] is False
        assert json_response["status"]["is_open_service"] is False
        assert 'realtime_contributors' in json_response['status']
        assert 'realtime_proxies' in json_response['status']

        assert 'warnings' in json_response
        assert len(json_response['warnings']) == 1
        assert json_response['warnings'][0]['id'] == 'beta_endpoint'

        assert 'street_networks' in json_response['status']

    def test_geo_status(self):
        response = self.query('/v1/coverage/main_routing_test/_geo_status', display=True)
        self.check_context(response)
        assert response['context']['timezone'] == 'Africa/Abidjan'
        assert response['geo_status']
        assert response['geo_status']['nb_ways'] > 0
        assert response['geo_status']['nb_pois'] > 0
        assert response['geo_status']['nb_admins'] > 0
        assert response['geo_status']['nb_addresses'] > 0
        assert response['geo_status']['nb_admins_from_cities'] == 0
        assert response['geo_status']['street_network_sources'] == []
        assert response['geo_status']['poi_sources'] == []

    def test_companies(self):
        response = self.query('/v1/coverage/main_ptref_test/companies', display=True)
        self.check_context(response)
        assert len(response['companies']) == 2

        # Company 1
        assert response['companies'][0]['name'] == 'CMP1'
        assert response['companies'][0]['id'] == 'CMP1'

        # Codes for Company 1
        assert len(response['companies'][0]['codes']) == 3
        for idx, code in enumerate(response['companies'][0]['codes']):
            assert code['type'] == 'cmp1_code_key_' + str(idx)
            assert code['value'] == 'cmp1_code_value_' + str(idx)

        # company 2
        assert response['companies'][1]['name'] == 'CMP2'
        assert response['companies'][1]['id'] == 'CMP2'

        # Codes for Company 2
        assert len(response['companies'][1]['codes']) == 1
        assert response['companies'][1]['codes'][0]['type'] == 'cmp2_code_key_0'
        assert response['companies'][1]['codes'][0]['value'] == 'cmp2_code_value_0'

    def test_lines_context(self):
        response = self.query('/v1/coverage/main_routing_test/lines', display=True)
        self.check_context(response)
        assert response['context']['timezone'] == 'UTC'

    def test_lines_head(self):
        """
        check that HEAD request works
        """
        response = self.tester.head('/v1/coverage/main_routing_test/lines')
        assert response.status_code == 200
        assert len(response.data) == 0

    def test_equipment_reports_context(self):
        mock_equipment_providers(
            equipment_provider_manager=self.equipment_provider_manager("main_routing_test"),
            data={},
            code_types_list=["TCL_ASCENSEUR", "TCL_ESCALIER"],
        )
        response = self.query('/v1/coverage/main_routing_test/equipment_reports', display=True)
        self.check_context(response)
        assert response.get("equipment_reports", None) is not None
        assert response['context']['timezone'] == 'UTC'

    def test_region_coord_addresses(self):
        resp = [
            self.tester.get('v1/coverage/main_routing_test/coord/0.001077974378345651;0.0005839027882705609'),
            self.tester.get('v1/coverage/main_routing_test/coords/0.001077974378345651;0.0005839027882705609'),
            self.tester.get(
                'v1/coverage/main_routing_test/addresses/0.001077974378345651;0.0005839027882705609'
            ),
            self.tester.get(
                'v1/coverage/main_routing_test/coord/0.001077974378345651;0.0005839027882705609/addresses'
            ),
            self.tester.get(
                'v1/coverage/main_routing_test/coords/0.001077974378345651;0.0005839027882705609/addresses'
            ),
        ]
        assert all(r.status_code < 300 for r in resp)
        assert all('address' in r.get_json() for r in resp)

        address_name = resp[0].get_json()['address']['name']
        assert all(
            r.get_json()['address']['name'] == address_name for r in resp
        ), 'All requests should return the same result...'

    def test_coord_without_region_shouldnt_call_places_nearby(self):
        r1 = self.tester.get('v1/coverage/main_routing_test/coord/0.001077974378345651;0.0005839027882705609')
        r2 = self.tester.get('v1/coord/0.001077974378345651;0.0005839027882705609')
        assert r1.get_json()['address'] == r2.get_json()['address']
