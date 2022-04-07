#  Copyright (c) 2001-2017, Hove and/or its affiliates. All rights reserved.
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
import copy
import re
import serpy
import inspect
import six

from jormungandr.interfaces.v1.serializer.jsonschema.fields import CustomSchemaType


class SwaggerDefinitions(object):
    pass


TYPE_MAP = {
    str: {'type': 'string'},
    six.text_type: {'type': 'string'},
    int: {'type': 'integer'},
    float: {'type': 'number', 'format': 'float'},
    bool: {'type': 'boolean'},
}


def convert_to_swagger_type(type_):
    swagger_type = TYPE_MAP.get(type_, {}).get('type')
    swagger_format = TYPE_MAP.get(type_, {}).get('format')

    return swagger_type or type_, swagger_format


class SwaggerParam(object):
    def __init__(
        self,
        description=None,
        name=None,
        required=None,
        type=None,
        pattern=None,
        default=None,
        enum=None,
        minimum=None,
        maximum=None,
        format=None,
        location=None,
        items=None,
        collection_format=None,
    ):
        self.description = description
        self.name = name
        self.required = required
        self.type = type
        self.default = default
        self.format = format
        self.location = location  # called 'in' in swagger spec
        self.enum = enum
        self.minimum = minimum
        self.maximum = maximum
        self.items = items
        self.pattern = pattern
        self.collection_format = collection_format

    @classmethod
    def make_from_flask_arg(cls, argument):
        if argument.hidden:
            return []

        args = []
        for location in argument.location:
            # flask call 'args' parameters in the query (called 'query' in swagger)
            if location == 'args':
                location = 'query'
            # flask call 'values' parameters in the query and in the body.
            # for the moment we consider that values is only for query parameters
            if location == 'values':
                location = 'query'

            param_type = getattr(argument, 'schema_type', None)
            metadata = {}

            # we check if the flask's type checker can give a description
            if isinstance(argument.type, CustomSchemaType):
                ts = argument.type.schema()
                param_type = ts.type  # overwrite if already set
                if ts.metadata:
                    metadata.update(ts.metadata)

            if param_type is None:
                param_type = argument.type

            # we try to autodetect the type from the default argument
            if param_type is None and argument.default is not None:
                param_type = TYPE_MAP.get(type(argument.default), {}).get('type')

            param_metadata = getattr(argument, 'schema_metadata', None)
            if param_metadata:
                # we merge it with the other metadata we can have
                metadata.update(param_metadata)

            swagger_type, swagger_format = convert_to_swagger_type(param_type)
            if swagger_format and 'format' not in metadata:
                metadata['format'] = swagger_format

            if argument.default and 'default' not in metadata:
                metadata['default'] = argument.default

            desc_meta = metadata.get('description', None)
            if argument.help:
                metadata['description'] = argument.help
                if desc_meta:
                    metadata['description'] += '\n\n'
            if desc_meta:
                metadata['description'] += desc_meta

            items = None
            collection_format = None
            if argument.action == 'append':
                items = SwaggerParam(
                    type=swagger_type, format=metadata.pop('format', None), enum=metadata.pop('enum', None)
                )
                swagger_type = 'array'
                collection_format = 'multi'

            args.append(
                SwaggerParam(
                    name=argument.name,
                    type=swagger_type,
                    location=location,
                    required=argument.required,
                    items=items,
                    collection_format=collection_format,
                    **metadata
                )
            )

        return args

    @classmethod
    def make_from_flask_route(cls, rule_converters):
        args = []
        for name, converter in (rule_converters or {}).items():
            swagger_type, swagger_format = convert_to_swagger_type(converter.type_)
            custom_format = getattr(converter, 'regex', None)
            if custom_format:
                swagger_format = custom_format
            args.append(
                SwaggerParam(
                    name=name,
                    description=converter.__doc__,
                    type=swagger_type,
                    format=swagger_format,
                    location='path',
                    required=True,
                )
            )

        return args


def _from_nested_schema(field):
    """
    return reference and a list of nested definitions from a Serializer
    or schema depending on param onlyRef
    """
    schema = {'$ref': '#/definitions/' + get_serializer_name(field)}

    return schema, field


def get_schema(serializer):
    properties_schema, definitions = get_schema_properties(serializer)
    required_properties = [
        field.label or field_name for field_name, field in serializer._field_map.items() if field.required
    ]
    schema = {"type": "object", "properties": properties_schema}
    if required_properties:  # swagger is a bit rigid not to have empty required properties
        schema['required'] = required_properties
    return schema, definitions


def get_schema_properties(serializer):
    """
    create the schema from a serpy serializer

    for each serializer field, we'll search for it's schema
    All complex fields (a field that is another serializer), we add the field's serializer to the external
    definitions
    """
    external_definitions = []
    properties = {}
    for field_name, field in serializer._field_map.items():
        schema = {}
        schema_type = getattr(field, 'schema_type', None)
        # schema_metadata are additional information blindly added to the schema
        schema_metadata = getattr(field, 'schema_metadata', None)

        # for some import order problem, the schema can be a function that returns the real schema
        # if a schema has not been defined, we consider directly the field
        rendered_field = schema_type() if callable(schema_type) else schema_type or field

        # if the field's class in a simple known type (str, bool, ...), we got it from TYPE_MAP
        if rendered_field.__class__ in TYPE_MAP:
            schema = copy.deepcopy(TYPE_MAP.get(rendered_field.__class__, {}))
        elif isinstance(rendered_field, serpy.Serializer):
            # complex types are stored in the `definition` list and referenced
            schema, definition = _from_nested_schema(rendered_field)
            external_definitions.append(definition)
        elif isinstance(rendered_field, CustomSchemaType):
            ts = rendered_field.schema()
            swagger_type, _ = convert_to_swagger_type(ts.type)
            schema = copy.deepcopy(ts.metadata)
            schema['type'] = swagger_type
        elif not schema_metadata:
            raise ValueError(
                'unsupported field type %s for attr %s in object %s'
                % (rendered_field, field_name, get_serializer_name(serializer))
            )

        if schema_metadata:
            if 'deprecated' in schema_metadata:
                # the 'deprecated' tag is handled only in swagger 3
                # since I think it's important to tag the deprecated fields, we do it
                # and remove the deprecated tag until the swagger 3 migration
                schema_metadata.pop('deprecated')
            schema.update(schema_metadata)
        name = field.label if hasattr(field, 'label') and field.label else field_name
        if getattr(field, 'many', False) or getattr(rendered_field, 'many', False):
            schema = {'type': "array", 'items': schema}
        properties[name] = schema

    return properties, external_definitions


def get_parameters(resource, method_name, rule_converters):
    """
    get all parameter for a given HTTP method of a flask resource

    get the path parameters and the query parameters

    http://api.navitia.io/v1/coverage/bob/places?q=toto
                                       |             |
                                       v             |
                                    path parameter   |
                                                     v
                                              query parameter

    the path parameters are retreived from the flask's rule_converters
    """
    params = []
    request_parser = resource.parsers.get(method_name)
    if request_parser:
        for argument in request_parser.args:
            swagger_params = SwaggerParam.make_from_flask_arg(argument)
            # several swagger args can be created for one flask arg
            params += swagger_params

    path_params = SwaggerParam.make_from_flask_route(rule_converters)
    params += path_params
    return params


def get_serializer_name(serializer):
    class_name = serializer.__name__ if inspect.isclass(serializer) else serializer.__class__.__name__
    # this replace is temporary, we need to find a better way to have good names there since they are now
    # exposed in the documentation / SDK
    name = class_name.replace('Serializer', '')
    return name


ARGS_REGEXP = re.compile(r'<(?P<name>.*?):.*?>')


def make_id(name, rule):
    """
    the operation id is a unique identifier for a swagger method

    rule is string representing a flask rule (like '/v1/coverage/<region:region>/places')
    name is the http method name (GET)
    >>> make_id('get', '/v1/coverage/<region:region>/places')
    'get_coverage__region__places'
    """
    # for the moment we do a dump concatenation with a bit of formating, we should see if we need something
    # better
    formated_rule = rule.replace('/v1/', '').replace('/', '_').replace(';', '')
    formated_rule = ARGS_REGEXP.sub(lambda m: '_' + m.group('name') + '_', formated_rule)
    return '{method}_{rule}'.format(rule=formated_rule, method=name)


class SwaggerMethod(object):
    def __init__(self, name=None, rule=None, summary='', resource=None, parameters=None):
        self.name = name
        if rule:
            self.id = make_id(name, rule)
        if resource:
            self.tags = [
                # in the tag we add the name of the Flask resources
                resource.__class__.__name__
            ]
        self.summary = summary
        self.parameters = parameters or []
        self.output_type = None

    def define_output_schema(self, resource):
        """
        return schema and a list of nested definitions from a Serializer

        :param obj: serpy.Serializer
        :return: the list of referenced definitions
        """
        obj = resource.output_type_serializer

        if not obj:
            return []

        output_type, external_definitions = get_schema(obj)

        # for the api responses, we add a title for better SDK integration
        output_type['title'] = get_serializer_name(obj)

        self.output_type = {"200": {"schema": output_type, 'description': resource.__doc__ or ''}}
        return external_definitions


class Swagger(object):
    def __init__(self):
        self.definitions = {}
        self.paths = {}


class SwaggerPath(object):
    def __init__(self):
        self.definitions = {}
        self.methods = {}
        self.tags = []

    def add_method(self, method_name, resource, rule):
        method_name = method_name.lower()  # a bit hacky, but we want a lower case http verb

        if method_name == 'options':
            return []  # we don't want to document options
        swagger_method = SwaggerMethod(name=method_name, rule=rule.rule, resource=resource)
        self.methods[method_name] = swagger_method

        swagger_method.parameters += get_parameters(resource, method_name, rule._converters)

        external_definitions = swagger_method.define_output_schema(resource)

        return external_definitions

    def add_definitions(self, external_definitions):
        while external_definitions:
            field = external_definitions.pop()
            field_name = get_serializer_name(field)
            if field_name in self.definitions:
                # we already got this definition, we can skip
                continue
            schema, nested_definitions = get_schema(field)
            self.definitions[field_name] = schema

            # the new schema can have it's lot of definition, we add them
            external_definitions.update(nested_definitions)


def make_schema(resource, rule=None):
    schema = SwaggerPath()

    external_definitions = set()
    for method_name in resource.methods:
        external_definitions.update(schema.add_method(method_name, resource, rule))

    schema.add_definitions(external_definitions)

    return schema
