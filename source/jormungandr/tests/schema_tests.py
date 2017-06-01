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

from tests.tests_mechanism import dataset, AbstractTestFixture
from swagger_spec_validator import validator20
import flex


@dataset({"main_autocomplete_test": {}})
class TestSwaggerSchema(AbstractTestFixture):
    """
    Test swagger schema
    """

    def test_swagger(self):
        """
        Test the global schema
        """
        response = self.query('v1/schema')
        flex.core.validate(response)
        validator20.validate_spec(response)

    def _check_schema(self, url):
        schema = self.query('v1/schema')

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
        r = self._check_schema('/v1/coverage/main_autocomplete_test/places?q=2 rue')
        assert any((o for o in r.get('places', []) if o.get('embedded_type') == 'address'))
