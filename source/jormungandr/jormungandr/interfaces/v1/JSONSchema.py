# coding=utf-8

# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import
import re

from flask_restful import Resource
from serpy.fields import MethodField
from flask import request
from jormungandr import app, _version
import serpy

from jormungandr.interfaces.v1.serializer.base import LiteralField, LambdaField
from jormungandr.interfaces.v1.serializer.jsonschema.serializer import SwaggerPathSerializer
from jormungandr.interfaces.v1.swagger_schema import make_schema, Swagger, ARGS_REGEXP

BASE_PATH = 'v1'


def set_definitions_in_rule(self, rule):
    return re.sub(r'<(?P<name>.*?):.*?>', self.definition_repl, rule)


def format_args(rule):
    """format argument like swagger : {arg1}&{arg2}"""
    formatted_rule = ARGS_REGEXP.sub(lambda m: '{' + m.group('name') + '}', rule)
    return formatted_rule


base_path_regexp = re.compile('^/{base}'.format(base=BASE_PATH))


def get_all_described_paths():
    """
    fetch the description of all api routes that have an 'OPTIONS' endpoint
    """
    swagger = Swagger()
    for endpoint, rules in app.url_map._rules_by_endpoint.items():
        for rule in rules:
            if 'OPTIONS' not in rule.methods or rule.provide_automatic_options:
                continue

            if rule.hide:
                # we might want to hide some rule
                continue

            view_function = app.view_functions.get(endpoint)
            if view_function is not None:
                view_class = view_function.view_class
                resource = view_class()
                schema_path = make_schema(resource=resource, rule=rule)

                # the definitions are stored aside and referenced in the response
                swagger.definitions.update(schema_path.definitions)

                formated_rule = format_args(rule.rule)

                # we trim the base path
                formated_rule = base_path_regexp.sub('', formated_rule)

                swagger.paths[formated_rule] = schema_path

    return swagger


class JsonSchemaInfo(serpy.Serializer):
    title = LiteralField('navitia')
    version = LiteralField(_version.__version__)
    description = LiteralField(
        """
    navitia.io is the open API for building cool stuff with mobility data. It provides the following services

    * journeys computation
    * line schedules
    * next departures
    * exploration of public transport data / search places
    * and sexy things such as isochrones

    navitia is a HATEOAS API that returns JSON formated results
    """
    )
    contact = LiteralField(
        {'name': 'Navitia', 'url': 'https://www.navitia.io/', 'email': 'navitia@googlegroups.com'}
    )
    license = LiteralField({'name': 'license', 'url': 'https://www.navitia.io/api-term-of-use'})


class SecurityDefinitionsSerializer(serpy.Serializer):
    basicAuth = LiteralField({'type': 'basic'})


class JsonSchemaEndpointsSerializer(serpy.Serializer):
    basePath = LiteralField('/' + BASE_PATH)
    swagger = LiteralField('2.0')
    host = LambdaField(lambda *args: request.url_root.replace('http://', '').replace('https://', '').rstrip('/'))
    paths = MethodField()
    definitions = serpy.Field()
    info = LambdaField(lambda s, o: JsonSchemaInfo(o).data)
    securityDefinitions = LambdaField(lambda s, o: SecurityDefinitionsSerializer(o).data)
    security = LiteralField([{'basicAuth': []}])

    def get_paths(self, obj):
        return {k: SwaggerPathSerializer(v).data for k, v in obj.paths.items()}


class Schema(Resource):
    def __init__(self, **kwargs):
        Resource.__init__(self, **kwargs)

    def get(self):
        """
        endpoint to get the swagger schema of Navitia
        """
        path = get_all_described_paths()

        return JsonSchemaEndpointsSerializer(path).data, 200
