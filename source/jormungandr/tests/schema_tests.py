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

import json
import logging
import os
import jsonschema
import re
import flex
from flex.exceptions import ValidationError

from tests.tests_mechanism import dataset, AbstractTestFixture, mock_bss_providers, mock_car_park_providers
from itertools import chain
from six.moves import filter
import six


def get_params(schema):
    return {p['name']: p for p in schema.get('get', {}).get('parameters', [])}


def collect_all_errors(validation_error):
    """
    collect all errors from a flex ValidationError

    the ValidationError is an aggregate of dict and list to have structured errors,
    we destructure them to get a simple dict with a 'path' to the error and the error
    """

    def _collect(err, key):
        errors = {}
        if isinstance(err, dict):
            for k, v in err.items():
                if k == 'default_factory':
                    continue
                errors.update(_collect(v, key='{prev}.{k}'.format(prev=key, k=k)))
            return errors

        if isinstance(err, list):
            for i, e in enumerate(err):
                errors.update(_collect(e, key='{k}[{i}]'.format(k=key, i=i)))
            return errors

        return {key: err}

    return _collect(validation_error.messages, key='.')


def additional_properties_false_adder(dict_var):
    # 'additional_properties=False' will forbid fields that are not specified in schema
    if dict_var.get('type') == 'object' and 'additionalProperties' not in dict_var:
        dict_var['additionalProperties'] = False

    for v in dict_var.values():
        if isinstance(v, dict):
            additional_properties_false_adder(v)
        if isinstance(v, list):
            for list_v in v:
                if isinstance(list_v, dict):
                    additional_properties_false_adder(list_v)


class SchemaChecker:
    def get_schema(self):
        """Since the schema is quite long to get we cache it"""
        if not hasattr(self, '_schema'):
            self._schema = self.query('v1/schema')
            additional_properties_false_adder(self._schema)
        return self._schema

    def _check_schema(self, url, hard_check=True):
        schema = self.get_schema()

        raw_response = self.tester.get(url)

        req = flex.http.Request(url=url, method='get')
        resp = flex.http.Response(
            request=req,
            content=raw_response.data,
            url=url,
            status_code=raw_response.status_code,
            content_type='application/json',
        )

        obj = json.loads(raw_response.data)

        try:
            flex.core.validate_api_call(schema, req, resp)
            return obj
        except ValidationError as e:
            logging.exception('validation error')
            if hard_check:
                raise
            return obj, collect_all_errors(e)

    def check_schema_parameters_structure(self, url):
        """
        Test the schema's parameters types
        """
        response = self.get_api_schema(url)

        params = get_params(response)

        for name, param in params.items():
            assert 'name' in param
            assert 'description' in param, (
                "API parameter '" + param['name'] + "' should have a description in the schema!"
            )
            assert 'type' in param

            assert type(param['name']) is six.text_type
            assert type(param['description']) is six.text_type

            assert any(
                param['type'] == t
                for t in ['integer', 'number', 'float', 'string', 'unicode', 'boolean', 'array']
            )


@dataset({"main_routing_test": {}, "main_autocomplete_test": {}})
class TestSwaggerSchema(AbstractTestFixture, SchemaChecker):
    """
    Test swagger schema
    """

    def test_swagger_schema_itself(self):
        """
        Test that the swagger schema is compliant with OpenAPI v2.0 (Swagger 2.0)
        """
        spec_file = os.path.join(os.path.dirname(__file__), 'swagger_schema', 'open_api_2.0.json')
        swagger_spec = json.loads(open(spec_file, "r").read())

        navitia_schema = self.query('v1/schema')  # original schema
        jsonschema.validate(navitia_schema, swagger_spec)

        navitia_hard_schema = self.get_schema()  # schema with forbidden additionalProperties
        jsonschema.validate(navitia_hard_schema, swagger_spec)

    def test_swagger(self):
        """
        Test the global schema
        """
        response = self.get_schema()
        flex.core.validate(response)
        for typename in response.get('definitions'):
            assert typename  # we should newer have empty names

        # we don't want to document /connections apis
        assert not any('connections' in p for p in response['paths'])
        # we also don't want this api, as we consider it deprecated
        assert not any('/coverage/{lon};{lat}/{uri}/journeys' in p for p in response['paths'])

    def test_coverage_schema(self):
        """
        Test the coverage schema
        """
        self._check_schema('/v1/coverage/')

    def get_api_schema(self, url):
        response = self.tester.options(url)

        assert response, "response for url {} is null".format(url)
        assert response.status_code == 200
        data = json.loads(response.data)

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
        assert response.status_code == 200
        assert response.allow.as_set() == {'head', 'options', 'get'}
        assert response.data == six.b('')  # no schema dumped

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

    def test_disruptions_schema(self):
        self._check_schema('/v1/coverage/main_routing_test/disruptions')

    def test_journeys_schema(self):
        self._check_schema(
            '/v1/coverage/main_routing_test/journeys?'
            'from=0.001527130369323005;0.0004491559909773545&'
            'to=poi:station_1&'
            'datetime=20120615T080000'
        )

    def test_vehicle_journeys(self):
        self._check_schema('/v1/coverage/main_routing_test/vehicle_journeys')

    def test_stop_schedule(self):
        """
        test the stop_schedule swagger

        the problem with this API is that we return sometime `null` field and swagger does not like this
        since the navitia SDK handle those and we haven't found a way around those (we can't use swagger3 yet)
        we consider those error acceptable
        """
        # Note: swagger does not handle '/' in parameters, so we cannot express our '<uris>'
        # so the '/' is urlencoded as %2F to be able to test the call

        obj, errors = self._check_schema(
            '/v1/coverage/main_routing_test/stop_areas%2FstopB/stop_schedules?' 'from_datetime=20120614T165200',
            hard_check=False,
        )

        pattern_error = re.compile(
            "Got value `None` of type `null`. Value must be of type\(s\): `\(u?'string',\)`"
        )
        # we have some errors, but only on additional_informations
        assert len(errors) == 4
        for k, e in errors.items():
            assert k.endswith('additional_informations[0].type[0]')
            assert pattern_error.match(e)

        # we check that the response is not empty
        assert any((o.get('date_times') for o in obj.get('stop_schedules', [])))

    def test_route_schedule(self):
        """
        test the route_schedule swagger

        the problem with this API is that we return sometime `null` field and swagger does not like this
        since the navitia SDK handle those and we haven't found a way around those (we can't use swagger3 yet)
        we consider those error acceptable
        """
        # Note: swagger does not handle '/' in parameters, so we cannot express our '<uris>'
        # so the '/' is urlencoded as %2F to be able to test the call

        _, errors = self._check_schema(
            '/v1/coverage/main_routing_test/routes%2FA:0/route_schedules?' 'from_datetime=20120614T165200',
            hard_check=False,
        )

        pattern_error = re.compile(
            "Got value `None` of type `null`. Value must be of type\(s\): `\(u?'string',\)`"
        )
        # we have some errors, but only on additional_informations
        assert len(errors) == 1
        for k, e in errors.items():
            assert k.endswith('additional_informations[0].type[0]')
            assert pattern_error.match(e)

    def test_departures(self):
        self._check_schema(
            '/v1/coverage/main_routing_test/stop_areas%2FstopB/departures?' 'from_datetime=20120614T165200'
        )

    def test_arrivals(self):
        self._check_schema(
            '/v1/coverage/main_routing_test/stop_areas%2FstopB/arrivals?' 'from_datetime=20120614T165200'
        )

    def test_traffic_reports(self):
        self._check_schema('/v1/coverage/main_routing_test/traffic_reports?' '_current_datetime=20120801T0000')

    def test_places_nearby(self):
        self._check_schema('/v1/coverage/main_routing_test/stop_areas%2FstopA/places_nearby')

    def test_pt_objects(self):
        self._check_schema('/v1/coverage/main_routing_test/pt_objects?q=1')
        self._check_schema('/v1/coverage/main_routing_test/pt_objects?q=stop')

    def test_isochrones(self):
        query = "/v1/coverage/main_routing_test/isochrones?from={}&datetime={}&max_duration={}"
        query = query.format("0.0000898312;0.0000898312", "20120614T080000", "3600")
        self._check_schema(query)

    def test_heatmaps(self):
        resolution = 50
        # test heat_map with <from>
        query = "/v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}&resolution={}"
        query = query.format('20120614T080100', 'stopB', '3600', resolution)

        _, errors = self._check_schema(query, hard_check=False)

        pattern = re.compile(
            ".*heat_maps.*items.*ref.*heat_matrix.*ref.*lines.*items.*ref.*duration.*items.*type"
        )
        pattern_error = re.compile(
            "Got value `None` of type `null`. Value must be of type\(s\): `\(u?'integer',\)`"
        )

        for k, e in errors.items():
            assert pattern.match(k)
            assert pattern_error.match(e)

    def test_geo_status(self):
        query = '/v1/coverage/main_routing_test/_geo_status'
        self._check_schema(query)

    def test_schema_parameters_sctructure(self):
        """
        Check for each endpoint the schema's structure
        """
        schema = self.get_schema()
        paths = schema['paths']

        for u in paths:
            url = '/v1' + u.format(region='main_routing_test', lon=0, lat=0, id=0, uri='_') + '?schema=true'
            self.check_schema_parameters_structure(url)


@dataset({"main_ptref_test": {}})
class TestSwaggerSchemaPtref(AbstractTestFixture, SchemaChecker):
    def test_calendars(self):
        self._check_schema('/v1/coverage/main_ptref_test/calendars')


@dataset({"departure_board_test": {}})
class TestSwaggerSchemaDepartureBoard(AbstractTestFixture, SchemaChecker):
    def test_departures(self):
        self._check_schema(
            '/v1/coverage/departure_board_test/routes%2Fline%3AA%3A0/departures?from_datetime=20120615T080000'
        )

    def test_stop_schedules(self):
        obj, errors = self._check_schema(
            '/v1/coverage/departure_board_test/networks%2Fbase_network/stop_schedules'
            '?from_datetime=20120615T080000&count=20&',
            hard_check=False,
        )

        # we have some errors, but only on additional_informations
        assert len(errors) == 9
        for k, e in errors.items():
            assert k.endswith('additional_informations[0].type[0]')
            assert "Got value `None` of type `null`. Value must be of type(s):" in e

        # we check that the response is not empty
        assert any((o.get('date_times') for o in obj.get('stop_schedules', [])))


@dataset({"main_routing_test": {}})
class TestSwaggerSchemaBssStands(AbstractTestFixture, SchemaChecker):
    def test_pois_with_stands_on_first_poi(self):
        with mock_bss_providers(pois_supported=[]):
            self._check_schema('/v1/coverage/main_routing_test/pois/poi:station_1')


@dataset({"main_routing_test": {}})
class TestSwaggerSchemaCarPark(AbstractTestFixture, SchemaChecker):
    def test_pois_with_car_park_on_first_poi(self):
        with mock_car_park_providers(pois_supported=[]):
            self._check_schema('/v1/coverage/main_routing_test/pois/poi:station_1')
