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
    # Weird name to ensure it will be processed at the end
    z_definitions = serpy.MethodField('get_definitions', display_none=False, label='definitions')
    properties = serpy.MethodField()
    type = serpy.MethodField()
    required = serpy.MethodField(display_none=False)

    def __init__(self, instance=None, many=False, data=None, context=None, root=False, **kwargs):
        super(JsonSchemaSerializer, self).__init__(instance, many, data, context, **kwargs)
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
        mapping[str] = 'str'
        mapping[int] = 'int'
        mapping[float] = 'float'
        mapping[bool] = 'bool'
        mapping[serpy.StrField] = 'str'
        mapping[serpy.IntField] = 'int'
        mapping[serpy.FloatField] = 'float'
        mapping[serpy.BoolField] = 'bool'
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

    def get_type(self, *args):
        return "object"

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
        serializer = JsonSchemaSerializer(field)
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


class InputArgumentSerializer(serpy.Serializer):
    description = serpy.StrField()
    location = serpy.MethodField(label='in')
    name = serpy.StrField()
    required = serpy.MethodField()
    type = serpy.MethodField()
    default = serpy.StrField(display_none=False)
    enum = serpy.Field(attr='choices', display_none=False)

    def __init__(self, schema_metadata={}, **kwargs):
        super(InputArgumentSerializer, self).__init__(**kwargs)
        self.schema_metadata = schema_metadata

    def to_value(self, instance):
        if not instance.hidden:
            data = super(InputArgumentSerializer, self).to_value(instance)
            if self.schema_metadata:
                data.update(self.schema_metadata)

            return data

    def get_location(self, obj):
        return 'query'

    def get_type(self, obj):
        type = None
        schema_type = getattr(obj, 'schema_type') or obj.type
        type_name = schema_type.__name__

        if TYPE_MAP.__contains__(type_name):
            type = TYPE_MAP.get(type_name).get('type')
        elif not self.schema_metadata:
            type = 'not handle : %s' % type_name
            # raise ValueError('unsupported type "%s" for argument "%s"' % (type_name, argument.name))

        if obj.action == 'append':
            type = 'array'

        return type

    def get_required(self, obj):
        return obj.default is None

class SwaggerPathSerializer(serpy.Serializer):
    definitions = serpy.MethodField(display_none=False)
    get = serpy.MethodField()

    def __init__(self, instance=None, endpoint=None, **kwargs):
        super(SwaggerPathSerializer, self).__init__(instance, **kwargs)
        self.endpoint = endpoint
        self.has_serialized_object = False
        self.serialized_object = None

    def serialize(self, field):
        if not self.has_serialized_object:
            self.serializer = JsonSchemaSerializer(field)
            self.serialized_object = JsonSchemaSerializer(field).data
            self.has_serialized_object = True

        return self.serialized_object

    def get_get(self, obj):
        schema = self.serialize(obj)

        return {
            'consumes': ['application/json'],
            'description': '',
            'parameters': self.get_parameters(),
            'produces': ['application/json'],
            'responses': {
                '200': schema
            },
            'summary': ''
        }

    def get_parameters(self):
        definitions = []
        if self.endpoint and hasattr(self.endpoint, 'parsers'):
            parser_get = getattr(self.endpoint, 'parsers').get('get')
            if parser_get:
                for argument in parser_get.args:
                    schema_metadata = getattr(argument, 'schema_metadata')
                    schema = InputArgumentSerializer(instance=argument, schema_metadata=schema_metadata).data
                    if schema:
                        definitions.append(schema)

        return definitions

    def get_definitions(self, obj):
        self.serialize(obj)
        self.serializer.root = True
        definitions = self.serializer.get_definitions(None)
        return definitions

"""
class Swagger(object):
    definitions = []
    get = SwaggerMethod()

class SwaggerDefinitions(object):
    pass

class SwaggerMethod(object):
    description = ''
    parameters=  [SwaggerParam]
    output_type = CoveragesSerializer
    summary = ''

class SwaggerParam(object):
    description = serpy.StrField()
    location = serpy.MethodField(label='in')
    name = serpy.StrField()
    required = serpy.MethodField()
    type = serpy.MethodField()
    default = serpy.StrField(display_none=False)
    enum = serpy.Field(attr='choices', display_none=False)
"""

