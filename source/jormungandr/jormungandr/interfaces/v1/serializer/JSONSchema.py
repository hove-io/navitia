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
from jormungandr.interfaces.v1.serializer.base import LiteralField

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
        'type': 'number'
    },
    'int': {
        'type': 'integer'
    },
    'bool': {
        'type': 'boolean',
    },
}

class JSONSchema(Serializer):
    properties = fields.MethodField()
    type = LiteralField('object')
    required = fields.MethodField()
    # Weird name to ensure it will be processed a the end
    _definitions = fields.MethodField('get_definitions', display_none=False, label='definitions')

    def __init__(self, instance=None, many=False, data=None, context=None, root=False, **kwargs):
        super(JSONSchema, self).__init__(instance, many, data, context, **kwargs)
        self.root = root
        self.definitions = {}

    def get_definitions(self, obj):
        if self.root:
            properties = {}
            definitions = self.definitions

            while definitions:
                field_name, field = definitions.popitem()
                schema, nestedDefinitions = self._from_nested_schema(field)
                properties[field_name] = schema
                self.add_new_definitions(definitions, nestedDefinitions, properties.keys())

            return properties
        else:
            return None

    def add_new_definitions(self, definitionsToProcessed, newDefinitions, processedDefinitions):
        reallyNewDefinitions = set(newDefinitions.keys()) - set(processedDefinitions)
        for k in reallyNewDefinitions:
            definitionsToProcessed.update({k: newDefinitions.get(k)})

    def get_properties(self, obj):
        mapping = {}
        mapping[fields.StrField] = 'str'
        mapping[fields.IntField] = 'int'
        mapping[fields.FloatField] = 'float'
        mapping[fields.BoolField] = 'bool'
        # Map the 2 next fields to avoid exceptions for now
        mapping[fields.Field] = 'str'
        # mapping[fields.MethodField] = 'method'
        properties = {}

        for field_name, field in obj._field_map.items():
            searchObj = field
            preTypeMethodName = '_jsonschema_pre_type_mapping'
            if isinstance(field, fields.MethodField):
                method = field.as_getter(field_name, obj.__class__)
                preTypeMethodName = method.__name__ + preTypeMethodName
                searchObj = obj
            if hasattr(searchObj, preTypeMethodName):
                field = getattr(searchObj, preTypeMethodName)()

            if hasattr(field, '_jsonschema_type_mapping'):
                schema = field._jsonschema_type_mapping()
            elif field.__class__ in mapping:
                pytype = mapping[field.__class__]
                schema = self._from_python_type(pytype)
            elif isinstance(field, Serializer):
                self.definitions.update({
                    field.__class__.__name__: field
                })
                schema, definitions = self._from_nested_schema(field, onlyRef=True)
            else:
                raise ValueError('unsupported field type %s for attr %s in object %s' % (field, field_name, obj.__class__.__name__))

            name = field.label or field_name
            properties[name] = schema

        return properties

    def get_required(self, obj):
        return [field.label or field_name for field_name, field in obj._field_map.items() if field.required]

    @classmethod
    def _from_python_type(cls, pytype):
        json_schema = {}

        for key, val in TYPE_MAP[pytype].items():
            json_schema[key] = val

        return json_schema

    @classmethod
    def _from_nested_schema(cls, field, onlyRef=False):
        serializer = cls(field)
        if onlyRef:
            schema = {
                '$ref': '#/definitions/' + field.__class__.__name__
            }
        else:
            schema = serializer.data
        definitions = serializer.definitions

        if field.many:
            schema = {
                'type': ["array"] if field.required else ['array', 'null'],
                'items': schema,
            }

        return schema, definitions
