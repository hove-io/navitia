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

from serpy import fields, Serializer

TYPE_MAP = {
    'unknown': {
        'type': 'unknown',
    },
    'method': {
        'type': 'method',
    },
    'str': {
        'type': 'string',
    },
    'float': {
        'type': 'number',
        'format': 'float',
    },
    'int': {
        'type': 'number',
        'format': 'integer',
    },
    'bool': {
        'type': 'boolean',
    },
}

class JSONSchema(Serializer):
    properties = fields.MethodField('get_properties')
    type = fields.MethodField('get_constant_object')
    required = fields.MethodField('get_required')

    def get_constant_object(self, obj):
        return 'object'

    def get_properties(self, obj):
        mapping = {}
        mapping[fields.StrField] = 'str'
        mapping[fields.IntField] = 'int'
        mapping[fields.FloatField] = 'float'
        mapping[fields.BoolField] = 'bool'
        # Map the 2 next fields to avoid exceptions for now
        mapping[fields.Field] = 'unknown'
        mapping[fields.MethodField] = 'method'
        properties = {}

        for field_name, field in sorted(obj._field_map.items()):
            if hasattr(field, '_jsonschema_type_mapping'):
                schema = field._jsonschema_type_mapping()
            elif field.__class__ in mapping:
                pytype = mapping[field.__class__]
                schema = self.__class__._from_python_type(pytype)
            elif isinstance(field, Serializer):
                schema = self.__class__._from_nested_schema(field)
            else:
                raise ValueError('unsupported field type %s' % field)

            properties[field_name] = schema

        return properties

    def get_required(self, obj):
        required = []

        for field_name, field in sorted(obj._field_map.items()):
            if field.required:
                required.append(field_name)

        return required

    @classmethod
    def _from_python_type(cls, pytype):
        json_schema = {}

        for key, val in TYPE_MAP[pytype].items():
            json_schema[key] = val

        return json_schema

    @classmethod
    def _from_nested_schema(cls, field):
        schema = cls(field).data

        if field.many:
            schema = {
                'type': ["array"] if field.required else ['array', 'null'],
                'items': schema,
            }

        return schema
