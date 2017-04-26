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

from jormungandr.interfaces.v1.serializer import jsonschema
import serpy
import pytest


def serpy_supported_serialization_test():
    """
    Supported serpy fields
    """
    class SerpySupportedType(serpy.Serializer):
        serpyStrField = serpy.StrField(required=False)
        serpyBoolField = serpy.BoolField()
        serpyFloatField = serpy.FloatField()
        serpyIntField = serpy.IntField()

    serializer = jsonschema.JsonSchemaSerializer(SerpySupportedType, root=True)
    obj = serializer.data
    assert obj.get('type') == 'object'
    assert len(obj.get('required')) == 3
    properties = obj.get('properties', {})
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
        jsonschema.JsonSchemaSerializer(SerpyUnsupportedFieldType, root=True).data

    with pytest.raises(ValueError):
        jsonschema.JsonSchemaSerializer(SerpyUnsupportedMethodFieldType, root=True).data

def serpy_extended_supported_serialization_test():
    """
    Supported custom serpy children fields
    """
    class jsonchemaSupportedType(serpy.Serializer):
        jsonschemaStrField = jsonschema.StrField(required=False)
        jsonschemaBoolField = jsonschema.BoolField()
        jsonschemaFloatField = jsonschema.FloatField()
        jsonschemaIntField = jsonschema.IntField()
        jsonschemaField = jsonschema.Field(schema_type=int)
        jsonschemaMethodField = jsonschema.MethodField(schema_type=str)

        def get_jsonschemaMethodField(self, obj):
            pass

    serializer = jsonschema.JsonSchemaSerializer(jsonchemaSupportedType, root=True)
    obj = serializer.data
    assert obj.get('type') == 'object'
    assert len(obj.get('required')) == 5
    properties = obj.get('properties', {})
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
    class jsonchemaMetadata(serpy.Serializer):
        metadata = jsonschema.Field(schema_metadata={
           "type": "string",
           "description": "meta"
        })

    class jsonchemaType(serpy.Serializer):
        primitive = jsonschema.Field(schema_type=str)
        function = jsonschema.MethodField(schema_type='get_bibu', display_none=False)
        serializer = jsonschema.Field(schema_type=jsonchemaMetadata)

        def get_bibu(self):
            return int

        def get_function(self):
            pass

    serializer = jsonschema.JsonSchemaSerializer(jsonchemaType, root=True)
    obj = serializer.data
    assert obj.get('type') == 'object'
    properties = obj.get('properties', {})
    assert len(properties) == 3
    assert properties.get('primitive', {}).get('type') == 'string'
    assert properties.get('function', {}).get('type') == 'integer'
    assert properties.get('serializer', {}).get('type') is None
    assert properties.get('serializer', {}).get('$ref') == '#/definitions/jsonchemaMetadata'

    serializer = jsonschema.JsonSchemaSerializer(jsonchemaMetadata, root=True)
    obj = serializer.data
    properties = obj.get('properties', {})
    assert len(properties) == 1
    assert properties.get('metadata', {}).get('type') == 'string'
    assert properties.get('metadata', {}).get('description') == 'meta'

def nested_test():
    """
    Nested Serialization
    """
    class nestedType(serpy.Serializer):
        id = jsonschema.StrField()

    class jsonchemaType(serpy.Serializer):
        nested = nestedType()

    class jsonchemaManyType(serpy.Serializer):
        nested = nestedType(many=True)

    serializer = jsonschema.JsonSchemaSerializer(jsonchemaType, root=True)
    obj = serializer.data
    assert obj.get('type') == 'object'
    properties = obj.get('properties', {})
    assert len(properties) == 1
    nested_data = properties.get('nested', {})
    assert nested_data.get('type') is None
    assert nested_data.get('$ref') == '#/definitions/nestedType'
    definitions = obj.get('definitions', {})
    assert len(definitions) == 1
    nested_definition = definitions.get('nestedType', {})
    assert nested_definition.get('type') == 'object'
    assert nested_definition.get('properties', {}).get('id', {}).get('type') == 'string'

    serializer = jsonschema.JsonSchemaSerializer(jsonchemaManyType, root=True)
    obj = serializer.data
    assert obj.get('type') == 'object'
    properties = obj.get('properties', {})
    assert len(properties) == 1
    nested_data = properties.get('nested', {})
    assert nested_data.get('type') == ['array']
    assert nested_data.get('items', {}).get('$ref') == '#/definitions/nestedType'
    definitions = obj.get('definitions', {})
    assert len(definitions) == 1
    nested_definition = definitions.get('nestedType', {})
    assert nested_definition.get('type') == 'object'
    assert nested_definition.get('properties', {}).get('id', {}).get('type') == 'string'

def constant_value_test():
    """
    Always return 'object'
    """
    serializer = jsonschema.JsonSchemaSerializer()
    assert serializer.get_type() == 'object'