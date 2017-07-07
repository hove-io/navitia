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

import json
import logging

from tests.tests_mechanism import dataset, AbstractTestFixture
import flex


def get_params(schema):
    return {p['name']: p for p in schema.get('get', {}).get('parameters', [])}


@dataset({"main_routing_test": {}, "main_autocomplete_test": {}})
class TestSwaggerSchema(AbstractTestFixture):
    """
    Test swagger schema
    """

    def get_schema(self):
        """Since the schema is quite long to get we cache it"""
        if not hasattr(self, '_schema'):
            self._schema = self.query('v1/schema')
        return self._schema

    def test_swagger(self):
        """
        Test the global schema
        """
        response = self.get_schema()
        flex.core.validate(response)
        for typename in response.get('definitions'):
            assert typename  # we should newer have empty names

    def _check_schema(self, url):
        schema = self.get_schema()

        raw_response = self.tester.get(url)

        req = flex.http.Request(url=url, method='get')
        resp = flex.http.Response(
            request=req,
            content=raw_response.data,
            url=url,
            status_code=raw_response.status_code,
            content_type='application/json')
        flex.core.validate_api_call(schema, req, resp)
        return json.loads(raw_response.data)

    def test_coverage_schema(self):
        """
        Test the coverage schema
        """
        self._check_schema('/v1/coverage/')

    def get_api_schema(self, url):
        response = self.tester.options(url)

        assert response, "response for url {} is null".format(url)
        assert(response.status_code == 200)
        data = json.loads(response.data, encoding='utf-8')

        # the schema should not be empty and should be valid
        assert 'get' in data
        flex.core.validate(data)

        # the response should also have the 'allow' headers
        assert response.allow.as_set() == {'head', 'options', 'get'}
        return data

    def test_options_coverage_schema(self):
        """
        Test the partial coverage schema
        """
        response = self.get_api_schema('/v1/coverage?schema=true')
        assert len(get_params(response)) == 1 and 'disable_geojson' in get_params(response)

    def test_no_schema_by_default(self):
        """
        Test the 'OPTIONS' method without the 'schema' arg. In this case we do not return the schema
        """
        response = self.tester.options('/v1/coverage')
        assert(response.status_code == 200)
        assert response.allow.as_set() == {'head', 'options', 'get'}
        assert response.data == ''  # no schema dumped

    def test_places_schema(self):
        """
        Test the autocomplete schema schema
        we use a easy query ('e') to get lots of different results
        """
        r = self._check_schema('/v1/coverage/main_autocomplete_test/places?q=e')

        # we check that the result contains different type (to be sure to test everything)
        assert any((o for o in r.get('places', []) if o.get('embedded_type') == 'administrative_region'))
        assert any((o for o in r.get('places', []) if o.get('embedded_type') == 'stop_area'))
        assert any((o for o in r.get('places', []) if o.get('embedded_type') == 'address'))

        # we also check an adress with a house number
        r = self._check_schema('/v1/coverage/main_routing_test/places?q=2 rue')
        assert any((o for o in r.get('places', []) if o.get('embedded_type') == 'address'))


    def test_stop_points_schema(self):
        """
        Test the stop_points schema
        """
        self._check_schema('/v1/coverage/main_routing_test/stop_points')

    def test_stop_areas_schema(self):
        self._check_schema('/v1/coverage/main_routing_test/stop_areas')

    def test_lines_schema(self):
        self._check_schema('/v1/coverage/main_routing_test/lines')

    def test_routes_schema(self):
        self._check_schema('/v1/coverage/main_routing_test/routes')

    def test_networks_schema(self):
        self._check_schema('/v1/coverage/main_routing_test/networks')
