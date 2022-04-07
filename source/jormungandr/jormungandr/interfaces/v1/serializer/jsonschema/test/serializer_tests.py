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

from __future__ import absolute_import
import serpy
import pytest

from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.serializer.base import LambdaField
from jormungandr.interfaces.v1.serializer.jsonschema.fields import (
    StrField,
    BoolField,
    FloatField,
    IntField,
    Field,
    MethodField,
)
from jormungandr.interfaces.v1.swagger_schema import get_schema, SwaggerParam


def serpy_supported_serialization_test():
    """
    Supported serpy fields
    """

    class SerpySupportedType(serpy.Serializer):
        serpyStrField = StrField(display_none=True)
        serpyBoolField = BoolField(display_none=True)
        serpyFloatField = FloatField()
        serpyIntField = IntField()

    schema, external_definitions = get_schema(SerpySupportedType)
    assert schema.get('type') == 'object'
    assert len(schema.get('required')) == 2
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

    class SerpyUnsupportedMethodFieldType(serpy.Serializer):
        serpyMethodField = MethodField()

        def get_serpyMethodField(self, obj):
            pass

    with pytest.raises(ValueError):
        get_schema(SerpyUnsupportedMethodFieldType)


def serpy_extended_supported_serialization_test():
    """
    Supported custom serpy children fields
    """

    class Custom(serpy.Serializer):
        bob = jsonschema.IntField()

    class JsonchemaSupportedType(serpy.Serializer):
        jsonschemaStrField = StrField(required=False)
        jsonschemaBoolField = BoolField(required=True, display_none=True)
        jsonschemaFloatField = FloatField(required=True, display_none=True)
        jsonschemaIntField = IntField()
        jsonschemaField = Field(schema_type=int)
        jsonschemaMethodField = MethodField(schema_type=str)
        lambda_schema = LambdaField(method=lambda **kw: None, schema_type=Custom())
        list_lambda_schema = LambdaField(method=lambda **kw: None, schema_type=Custom(many=True))

        def get_jsonschemaMethodField(self, obj):
            pass

    schema, external_definitions = get_schema(JsonchemaSupportedType)
    assert schema.get('type') == 'object'
    assert set(schema.get('required')) == {'jsonschemaBoolField', 'jsonschemaFloatField'}
    properties = schema.get('properties', {})
    assert len(properties) == 8
    assert properties.get('jsonschemaStrField', {}).get('type') == 'string'
    assert properties.get('jsonschemaBoolField', {}).get('type') == 'boolean'
    assert properties.get('jsonschemaFloatField', {}).get('type') == 'number'
    assert properties.get('jsonschemaIntField', {}).get('type') == 'integer'
    assert properties.get('jsonschemaField', {}).get('type') == 'integer'
    assert properties.get('jsonschemaMethodField', {}).get('type') == 'string'
    assert properties.get('lambda_schema', {}).get('$ref') == '#/definitions/Custom'
    assert properties.get('list_lambda_schema', {}).get('type') == 'array'
    assert properties.get('list_lambda_schema').get('items').get('$ref') == '#/definitions/Custom'

    # we must find the 'CustomSerializer' in the definitions
    assert next(iter(d for d in external_definitions if d.__class__ == Custom), None)


def schema_type_test():
    """
    Custom fields metadata
    """

    class JsonchemaMetadata(serpy.Serializer):
        metadata = jsonschema.Field(schema_metadata={"type": "string", "description": "meta"})

    class JsonchemaType(serpy.Serializer):
        primitive = jsonschema.Field(schema_type=str)
        function = jsonschema.MethodField(schema_type=int, display_none=False)
        serializer = jsonschema.Field(schema_type=JsonchemaMetadata)

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
    assert nested_data.get('type') == 'array'
    assert nested_data.get('items', {}).get('$ref') == '#/definitions/NestedType'
    assert len(external_definitions) == 1


def param_test():
    from navitiacommon.parser_args_type import OptionValue

    flask_arg = ArgumentDoc(
        "kind", type=OptionValue(['bob', 'bobette', 'bobitto']), default=['bob', 'bobette'], help="kind of bob"
    )

    swagger_args = SwaggerParam.make_from_flask_arg(flask_arg)

    assert len(swagger_args) == 1
    swagger_arg = swagger_args[0]
    assert swagger_arg.name == 'kind'
    assert swagger_arg.type == 'string'
    assert swagger_arg.required is False
    assert swagger_arg.location == 'query'
    assert swagger_arg.enum == ['bob', 'bobette', 'bobitto']
    assert swagger_arg.description == 'kind of bob'
    assert swagger_arg.default == ['bob', 'bobette']
    assert swagger_arg.format is None  # no additional format provided
    assert swagger_arg.collection_format is None


def param_list_test():
    flask_arg = ArgumentDoc("pouet", type=str, action="append")

    swagger_args = SwaggerParam.make_from_flask_arg(flask_arg)

    assert len(swagger_args) == 1
    swagger_arg = swagger_args[0]
    assert swagger_arg.name == 'pouet'
    assert swagger_arg.type == 'array'
    assert swagger_arg.items is not None
    assert swagger_arg.items.type == 'string'
    assert swagger_arg.collection_format == 'multi'
