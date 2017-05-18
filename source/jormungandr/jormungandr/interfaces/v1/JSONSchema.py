# coding=utf-8

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
import re

from flask.ext.restful import Resource
from serpy.fields import MethodField
from flask import request
from jormungandr import app
from jormungandr.interfaces.v1.serializer import api, jsonschema
import serpy

from jormungandr.interfaces.v1.serializer.base import LiteralField, LambdaField

BASE_PATH = 'v1'


def set_definitions_in_rule(self, rule):
    return re.sub(r'<(?P<name>.*?):.*?>', self.definition_repl, rule)


args_regexp = re.compile(r'<(?P<name>.*?):.*?>')


def format_args(rule):
    formated_rule = args_regexp.sub(lambda m: '{' + m.group('name') + '}', rule)
    return formated_rule

base_path_regexp = re.compile('^/{base}'.format(base=BASE_PATH))


def get_all_described_paths():
    paths = []
    for endpoint, rules in app.url_map._rules_by_endpoint.items():
        for rule in rules:
            if 'OPTIONS' not in rule.methods or rule.provide_automatic_options:
                continue

            formated_rule = format_args(rule.rule)

            # we trim the base path
            formated_rule = base_path_regexp.sub('', formated_rule)

            paths.append(formated_rule)

    return paths


def make_schema_link(path):
    link = '{root}{base}{path}'.format(root=request.url_root, base=BASE_PATH, path=path)
    return {
        '$ref': link,
        'method': 'OPTIONS'
    }


class JsonSchemaEndpointsSerializer(serpy.Serializer):
    basePath = LiteralField('/' + BASE_PATH)
    swagger = LiteralField('2.0')
    host = LambdaField(lambda *args: request.url_root)
    paths = MethodField()

    def get_paths(self, obj):
        return {
            p: make_schema_link(p) for p in obj
        }


class Schema(Resource):
    def __init__(self, **kwargs):
        Resource.__init__(self, **kwargs)

    def get(self):

        path = get_all_described_paths()

        return JsonSchemaEndpointsSerializer(path).data, 200
