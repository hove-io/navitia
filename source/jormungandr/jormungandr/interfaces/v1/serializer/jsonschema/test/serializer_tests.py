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
import pytest

from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.swagger_schema import get_schema


def serpy_supported_serialization_test():
    """
    Supported serpy fields
    """

    class SerpySupportedType(serpy.Serializer):
        serpyStrField = serpy.StrField(required=False)
        serpyBoolField = serpy.BoolField()
        serpyFloatField = serpy.FloatField()
        serpyIntField = serpy.IntField()

    schema, external_definitions = get_schema(SerpySupportedType)
    assert schema.get('type') == 'object'
    assert len(schema.get('required')) == 3
    properties = schema.get('properties', {})
    assert len(properties) == 4
    assert properties.get('serpyStrField', {}).get('type') == 'string'
    assert properties.get('serpyBoolField', {}).get('type') == 'boolean'
    assert properties.get('serpyFloatField', {}).get('type') == 'number'
    assert properties.get('serpyIntField', {}).get('type') == 'integer'


def serpy_unsupported_serialization_test():
    """
    Unsupported serpy fields
    """

    class SerpyUnsupportedFieldType(serpy.Serializer):
        serpyField = serpy.Field()

    class SerpyUnsupportedMethodFieldType(serpy.Serializer):
        serpyMethodField = serpy.MethodField()

        def get_serpyMethodField(self, obj):
            pass

    with pytest.raises(ValueError):
        get_schema(SerpyUnsupportedFieldType)

    with pytest.raises(ValueError):
        get_schema(SerpyUnsupportedMethodFieldType)


def serpy_extended_supported_serialization_test():
    """
    Supported custom serpy children fields
    """

    class JsonchemaSupportedType(serpy.Serializer):
        jsonschemaStrField = jsonschema.StrField(required=False)
        jsonschemaBoolField = jsonschema.BoolField()
        jsonschemaFloatField = jsonschema.FloatField()
        jsonschemaIntField = jsonschema.IntField()
        jsonschemaField = jsonschema.Field(schema_type=int)
        jsonschemaMethodField = jsonschema.MethodField(schema_type=str)

        def get_jsonschemaMethodField(self, obj):
            pass

    schema, external_definitions = get_schema(JsonchemaSupportedType)
    assert schema.get('type') == 'object'
    assert len(schema.get('required')) == 5
    properties = schema.get('properties', {})
    assert len(properties) == 6
    assert properties.get('jsonschemaStrField', {}).get('type') == 'string'
    assert properties.get('jsonschemaBoolField', {}).get('type') == 'boolean'
    assert properties.get('jsonschemaFloatField', {}).get('type') == 'number'
    assert properties.get('jsonschemaIntField', {}).get('type') == 'integer'
    assert properties.get('jsonschemaField', {}).get('type') == 'integer'
    assert properties.get('jsonschemaMethodField', {}).get('type') == 'string'


def schema_type_test():
    """
    Custom fields metadata
    """

    class JsonchemaMetadata(serpy.Serializer):
        metadata = jsonschema.Field(schema_metadata={
            "type": "string",
            "description": "meta"
        })

    class JsonchemaType(serpy.Serializer):
        primitive = jsonschema.Field(schema_type=str)
        function = jsonschema.MethodField(schema_type='get_bibu', display_none=False)
        serializer = jsonschema.Field(schema_type=JsonchemaMetadata)

        def get_bibu(self):
            return int

        def get_function(self):
            pass

    schema, external_definitions = get_schema(JsonchemaType)
    assert schema.get('type') == 'object'
    properties = schema.get('properties', {})
    assert len(properties) == 3
    assert properties.get('primitive', {}).get('type') == 'string'
    assert properties.get('function', {}).get('type') == 'integer'
    assert properties.get('serializer', {}).get('type') is None
    assert properties.get('serializer', {}).get('$ref') == '#/definitions/JsonchemaMetadata'

    schema, external_definitions = get_schema(JsonchemaMetadata)
    properties = schema.get('properties', {})
    assert len(properties) == 1
    assert properties.get('metadata', {}).get('type') == 'string'
    assert properties.get('metadata', {}).get('description') == 'meta'


def nested_test():
    """
    Nested Serialization
    """

    class NestedType(serpy.Serializer):
        id = jsonschema.StrField()

    class JsonchemaType(serpy.Serializer):
        nested = NestedType()

    class JsonchemaManyType(serpy.Serializer):
        nested = NestedType(many=True)

    schema, external_definitions = get_schema(JsonchemaType)
    assert schema.get('type') == 'object'
    properties = schema.get('properties', {})
    assert len(properties) == 1
    nested_data = properties.get('nested', {})
    assert nested_data.get('type') is None
    assert nested_data.get('$ref') == '#/definitions/NestedType'
    assert len(external_definitions) == 1

    schema, external_definitions = get_schema(JsonchemaManyType)
    assert schema.get('type') == 'object'
    properties = schema.get('properties', {})
    assert len(properties) == 1
    nested_data = properties.get('nested', {})
    assert nested_data.get('type') == ['array']
    assert nested_data.get('items', {}).get('$ref') == '#/definitions/NestedType'
    assert len(external_definitions) == 1
