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

class JsonSchemaSerializer(serpy.Serializer):
    properties = serpy.MethodField()
    type = serpy.MethodField()
    required = serpy.MethodField(display_none=False)
    # Weird name to ensure it will be processed a the end
    _definitions = serpy.MethodField('get_definitions', display_none=False, label='definitions')

    def __init__(self, instance=None, many=False, data=None, context=None, root=False, **kwargs):
        super(JsonSchemaSerializer, self).__init__(instance, many, data, context, **kwargs)
        self.root = root
        self.definitions = {}

    def get_type(self, *args):
        return "object"

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
        mapping[str] = 'str'
        mapping[int] = 'int'
        mapping[float] = 'float'
        mapping[bool] = 'bool'
        mapping[serpy.StrField] = 'str'
        mapping[serpy.IntField] = 'int'
        mapping[serpy.FloatField] = 'float'
        mapping[serpy.BoolField] = 'bool'
        # Map the 2 next fields.py to avoid exceptions for now
        # mapping[serpy.Field] = 'str'
        # mapping[fields.py.MethodField] = 'method'
        properties = {}

        for field_name, field in obj._field_map.items():
            schema = {}
            schema_type = getattr(field, 'schema_type') if hasattr(field, 'schema_type') else None
            schema_metadata = getattr(field, 'schema_metadata') if hasattr(field, 'schema_metadata') else {}

            if isinstance(schema_type, basestring) and hasattr(obj, schema_type):
                if hasattr(obj, '__name__'):
                    obj = obj()
                schema_type = getattr(obj, schema_type)
                # Get method result or attribute value
                schema_type = schema_type() if callable(schema_type) else schema_type

            rendered_field = schema_type() if callable(schema_type) else schema_type or field

            if rendered_field.__class__ in mapping:
                pytype = mapping[rendered_field.__class__]
                schema = self._from_python_type(pytype)
            elif isinstance(rendered_field, serpy.Serializer):
                self.definitions.update({
                    rendered_field.__class__.__name__: rendered_field
                })
                schema, definitions = self._from_nested_schema(rendered_field, onlyRef=True)
            elif not schema_metadata:
                raise ValueError('unsupported field type %s for attr %s in object %s' % (rendered_field, field_name, obj.__class__.__name__))

            if schema_metadata:
                schema.update(schema_metadata)
            name = field.label if hasattr(field, 'label') and field.label else field_name
            properties[name] = schema

        return properties

    def get_required(self, obj):
        required = [field.label or field_name for field_name, field in obj._field_map.items() if field.required]
        return required if len(required) > 0 else None

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

        if onlyRef and field.many:
            schema = {
                'type': ["array"] if field.required else ['array', 'null'],
                'items': schema,
            }

        return schema, definitions
