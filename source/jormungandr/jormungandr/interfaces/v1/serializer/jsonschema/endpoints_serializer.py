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

import serpy
import re

TYPE_MAP = {
    'unicode': {
        'type': 'string',
    },
    'str': {
        'type': 'string',
    },
    'float': {
        'type': 'number'
    },
    'int': {
        'type': 'integer'
    },
    'boolean': {
        'type': 'string',
        'enum': ['true', 'false']
    },
}

class JsonSchemaEndpointsSerializer(serpy.Serializer):
    links = serpy.MethodField()
    # Weird name to ensure it will be processed at the end
    _definitions = serpy.MethodField('get_definitions', display_none=False, label='definitions')

    def __init__(self, instance=None, many=False, data=None, context=None, **kwargs):
        super(JsonSchemaEndpointsSerializer, self).__init__(instance, many, data, context, **kwargs)
        self.definitions = []

    def get_definitions(self, app):
        properties = {}
        definitions = self.definitions

        for definition in definitions:
            if hasattr(app.url_map.converters[definition], 'num_convert'):
                type = app.url_map.converters[definition].num_convert.__name__
            else:
                type = 'str'

            properties[definition] = TYPE_MAP.get(type)

        return properties

    def get_links(self, app):
        links = []

        for endpoint, rules in app.url_map._rules_by_endpoint.items():
            link = {
                'method': 'GET',
                'rel': 'self',
                'title': endpoint,
                'schema': self.get_input_definitions(app.view_functions, endpoint),
            }
            for rule in rules:
                rule_with_definitions = self.set_definitions_in_rule(rule.rule)
                link.update({
                    'href': rule_with_definitions,
                    'targetSchema': {
                        '$ref': rule_with_definitions,
                        'method': 'OPTIONS'
                    }
                })
                links.append(link)

        return links

    def definition_repl(self, matchobject):
        name = matchobject.group('name')
        if not self.definitions.__contains__(name):
            self.definitions.append(name)
        return '{(%23%2Fdefinitions%2F' + name + ')}'

    def set_definitions_in_rule(self, rule):
        return re.sub(r'<(?P<name>.*?):.*?>', self.definition_repl, rule)

    def get_input_definitions(self, endpoints, endpoint):
        definitions = {}
        view = endpoints.get(endpoint)
        view_class = view.view_class()
        if hasattr(view_class, 'parsers'):
            parser_get = getattr(view_class, 'parsers').get('get')
            if parser_get:
                for argument in parser_get.args:
                    if not argument.hidden:
                        schema = {
                            'description': argument.description,
                        }

                        schema_type = getattr(argument, 'schema_type') if hasattr(argument, 'schema_type') else None
                        schema_metadata = getattr(argument, 'schema_metadata') if hasattr(argument, 'schema_metadata') else {}

                        schema_type = schema_type or argument.type
                        type_name = schema_type.__name__

                        if TYPE_MAP.__contains__(type_name):
                            type = TYPE_MAP.get(type_name)
                            schema.update(type)
                        elif not schema_metadata:
                            pass
                            # raise ValueError('unsupported type "%s" for argument "%s"' % (type_name, argument.name))

                        if argument.action == 'append':
                            schema.update(type='array')

                        if len(argument.choices) > 0:
                            schema.update(enum=argument.choices)
                            schema.pop('type')

                        if not argument.default:
                            default = argument.default
                            if type_name == 'boolean':
                                default = 'true' if argument.default else 'false'

                            schema.update(default=default)

                        if schema_metadata:
                            schema.update(schema_metadata)

                        definitions[argument.name] = schema

        return definitions


