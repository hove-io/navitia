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

from jormungandr.interfaces.v1.serializer.base import LiteralField

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


def process_method(serializer):
    """
    return a swaggerMethod object and schema of the associated definitions
    for a Serializer
    
    :param serializer: serpy.Serializer
    :return: (SwaggerMethod, dict)
    """
    (schema, definitions_to_process) = get_schema(serializer)

    swagger_method = SwaggerMethod()
    swagger_method.output_type.update({
        "200": {
            "schema": schema
        }
    })

    # recursive definition process
    definitions = process_definitions(definitions_to_process)

    return swagger_method, definitions


def process_definitions(definitions_to_process):
    """
    recursively convert definitions to schema
    
    :param definitions_to_process: dict
    :return: dict
    """
    properties = {}

    while definitions_to_process:
        field_name, field = definitions_to_process.popitem()
        schema, nested_definitions = _from_nested_schema(field)
        properties[field_name] = schema
        definitions_to_process = add_new_definitions(definitions_to_process, nested_definitions, properties.keys())

    return properties


def add_new_definitions(definitions_to_process, new_definitions, processed_definitions):
    """
    Add new definitions to the list of definition to process
    
    :param definitions_to_process: dict
    :param new_definitions: dict
    :param processed_definitions: list
    :return: dict
    """
    really_new_definitions = set(new_definitions.keys()) - set(processed_definitions)
    for k in really_new_definitions:
        definitions_to_process.update({k: new_definitions.get(k)})

    return definitions_to_process


def _from_python_type(pytype):
    """
    return schema for python type
    
    :param pytype: 
    :return: dict
    """
    json_schema = {}

    for key, val in TYPE_MAP[pytype].items():
        json_schema[key] = val

    return json_schema


def _from_nested_schema(field, only_ref=False):
    """
    return reference and a list of nested definitions from a Serializer
    or schema depending on param onlyRef
    
    :param field: serpy.Serializer
    :param only_ref: bool
    :return: (dict, dict)
    """
    definitions = {}

    if only_ref:
        schema = {
            '$ref': '#/definitions/' + field.__class__.__name__
        }
        if field.many:
            schema = {
                'type': ["array"] if field.required else ['array', 'null'],
                'items': schema,
            }

        definitions.update({
            field.__class__.__name__: field
        })

    else:
        schema, definitions = get_schema(field)

    return schema, definitions


def get_schema(obj):
    (properties_schema, definitions) = get_schema_properties(obj)
    required_properties = [field.label or field_name for field_name, field in obj._field_map.items() if field.required]
    schema = {
        "type": "object",
        "properties": properties_schema,
        "required": required_properties
    }

    return schema, definitions


def get_schema_properties(obj):
    """
    return schema and a list of nested definitions from a Serializer
    
    :param obj: serpy.Serializer
    :return: (dict, dict)
    """
    mapping = {}
    mapping[str] = 'str'
    mapping[int] = 'int'
    mapping[float] = 'float'
    mapping[bool] = 'bool'
    mapping[serpy.StrField] = 'str'
    mapping[serpy.IntField] = 'int'
    mapping[serpy.FloatField] = 'float'
    mapping[serpy.BoolField] = 'bool'
    schema = {}
    definitions = {}

    for field_name, field in obj._field_map.items():
        property_schema = {}
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
            property_schema = _from_python_type(pytype)
        elif isinstance(rendered_field, serpy.Serializer):
            property_schema, definitions = _from_nested_schema(rendered_field, only_ref=True)
        elif not schema_metadata:
            raise ValueError('unsupported field type %s for attr %s in object %s' % (
                rendered_field, field_name, obj.__class__.__name__))

        if schema_metadata:
            property_schema.update(schema_metadata)
        name = field.label if hasattr(field, 'label') and field.label else field_name
        schema[name] = property_schema

    return schema, definitions


class SwaggerPathDumper(object):
    def __init__(self, serializer=None):
        """
        Parse the serializer to schema
        
        :param serializer: serpy.Serializer
        """
        super(SwaggerPathDumper, self).__init__()

        (swagger_method, definitions) = process_method(serializer)

        swagger_path = Swagger()
        swagger_path.definitions = definitions
        swagger_path.get = swagger_method

        self.swagger_path = swagger_path

    def get_datas(self):
        """
        Serialize schema for rendering
        
        :return: dict
        """
        return SwaggerPathSerializer(self.swagger_path).data


class Swagger(object):
    definitions = []  # [{}]
    get = None  # SwaggerMethod()


class SwaggerMethod(object):
    description = ''
    parameters = []
    output_type = {}  # schema CoveragesSerializer()
    summary = ''


class SwaggerParam(object):
    description = serpy.StrField()
    location = serpy.MethodField(label='in')
    name = serpy.StrField()
    required = serpy.MethodField()
    type = serpy.MethodField()
    default = serpy.StrField(display_none=False)
    enum = serpy.Field(attr='choices', display_none=False)


class SwaggerResponseSerializer(serpy.DictSerializer):
    success = serpy.Field(attr='200', label='200')


class SwaggerMethodSerializer(serpy.Serializer):
    consumes = LiteralField('["application/json"]')
    produces = LiteralField('["application/json"]')
    responses = SwaggerResponseSerializer(attr='output_type')
    description = serpy.Field()
    summary = serpy.Field()


class SwaggerPathSerializer(serpy.Serializer):
    definitions = serpy.Field(display_none=False)
    get = SwaggerMethodSerializer()
