#  Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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


class SwaggerDefinitions(object):
    pass

TYPE_MAP = {
    'unicode': {
        'type': 'string',
    },
    'str': {
        'type': 'string',
    },
    'float': {
        'type': 'number',
        'format': 'float'
    },
    'int': {
        'type': 'integer'
    },
    'boolean': {
        'type': 'boolean',
    },
}


def convert_to_swagger_type(type):
    swagger_type = TYPE_MAP.get(type, {}).get('type')
    swagger_format = TYPE_MAP.get(type, {}).get('format')

    return swagger_type or type, swagger_format


class SwaggerParam(object):
    def __init__(self, description=None, name=None, required=None, type=None,
                 default=None, enum=None, format=None, location=None):
        self.description = description
        self.name = name
        self.required = required
        self.type = type
        self.default = default
        self.enum = enum
        self.format = format
        self.location = location  # called 'in' in swagger spec

    @classmethod
    def make_from_flask_arg(cls, argument):
        if argument.hidden:
            return []

        args = []
        for location in argument.location:
            # flask call 'values' parameters in the query (called 'query' in swagger)
            if location == 'values':
                location = 'query'

            type = getattr(argument, 'schema_type', None) or argument.type

            swagger_type, swagger_format = convert_to_swagger_type(type)

            # TODO handle arrays
            # if argument.action == 'append':
            #     schema.update(type='array')
            enum = argument.choices if len(argument.choices) > 0 else None

            args.append(SwaggerParam(name=argument.name,
                                     description=argument.description or argument.help,
                                     # TODO refactor to remove argument.description and always use help
                                     type=swagger_type,
                                     format=swagger_format,
                                     default=argument.default,
                                     location=location,
                                     enum=enum,
                                     required=argument.required))

        return args

    @classmethod
    def make_from_flask_route(cls, ressource, method_name):
        import inspect
        associated_function = getattr(ressource, method_name, None)
        if not associated_function:
            return []

        params = inspect.getargspec(associated_function)

        print('youhou: {}'.format(params))

        args = []

        from jormungandr import app
        for param_name in params.args:
            # we'll try to guess the type of the params based on the flask converters
            # it's a bit hacky, we consider that the type and the variable have the same name

            converter = app.url_map.converters.get(param_name)
            if not converter:
                continue

            format = getattr(converter, 'regex', None)
            args.append(SwaggerParam(name=param_name,
                                     description=converter.__doc__,
                                     type=converter.type_,
                                     format=format,
                                     location='path',
                                     required=False))

        return args


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


def _from_nested_schema(field):
    """
    return reference and a list of nested definitions from a Serializer
    or schema depending on param onlyRef
    """
    schema = {
        '$ref': '#/definitions/' + field.__class__.__name__
    }
    if field.many:
        schema = {
            'type': ["array"] if field.required else ['array', 'null'],
            'items': schema,
        }

    return schema, field


def get_schema(serializer):
    # TODO remove this mapping and add a type in the fields
    mapping = {
        str: 'str',
        int: 'int',
        float: 'float',
        bool: 'bool',
        serpy.StrField: 'str',
        serpy.IntField: 'int',
        serpy.FloatField: 'float',
        serpy.BoolField: 'bool'
    }
    external_definitions = []
    properties = {}
    for field_name, field in serializer._field_map.items():
        schema = {}
        schema_type = getattr(field, 'schema_type', None)
        schema_metadata = getattr(field, 'schema_metadata', None)

        if isinstance(schema_type, basestring) and hasattr(serializer, schema_type):
            if hasattr(serializer, '__name__'):
                serializer = serializer()
            schema_type = getattr(serializer, schema_type)
            # Get method result or attribute value
            schema_type = schema_type() if callable(schema_type) else schema_type

        rendered_field = schema_type() if callable(schema_type) else schema_type or field

        if rendered_field.__class__ in mapping:
            pytype = mapping[rendered_field.__class__]
            schema = _from_python_type(pytype)
        elif isinstance(rendered_field, serpy.Serializer):
            # complex types are stored in the `definition` list and referenced
            schema, definition = _from_nested_schema(rendered_field)
            external_definitions.append(definition)
        elif not schema_metadata:
            raise ValueError('unsupported field type %s for attr %s in object %s' % (
                rendered_field, field_name, serializer.__class__.__name__))

        if schema_metadata:
            schema.update(schema_metadata)
        name = field.label if hasattr(field, 'label') and field.label else field_name
        properties[name] = schema

    return properties, external_definitions


class SwaggerMethod(object):
    def __init__(self, summary='', parameters=None):
        self.summary = summary
        self.parameters = parameters or []
        self.output_type = None
        self.definitions = []  # used to store complex type definitions

    def define_output_schema(self, resource):
        """
        return schema and a list of nested definitions from a Serializer

        :param obj: serpy.Serializer
        :return: the list of referenced definitions
        """
        obj = resource.output_type_serializer

        self.output_type, external_definitions = get_schema(obj)

        return external_definitions


def get_parameters(resource, method_name):
    """
    get all parameter for a given HTTP method of a flask resource
    
    get the path parameters and the query parameters
    
    http://api.navitia.io/v1/coverage/bob/places?q=toto
                                       |             |
                                       v             |
                                    path parameter   |
                                                     v
                                              query parameter
    """
    params = []
    request_parser = resource.parsers.get(method_name)
    if request_parser:
        for argument in request_parser.args:
            swagger_params = SwaggerParam.make_from_flask_arg(argument)
            # several swagger args can be created for one flask arg
            params += swagger_params

    path_params = SwaggerParam.make_from_flask_route(resource, method_name)
    params += path_params
    return params


class Swagger(object):
    definitions = {}
    methods = {}

    def add_method(self, method_name, resource):
        method_name = method_name.lower()  # a bit hacky, but we want a lower case http verb

        if method_name == 'options':
            return []  # we don't want to document options
        swagger_method = SwaggerMethod()
        self.methods[method_name] = swagger_method

        swagger_method.parameters += get_parameters(resource, method_name)

        external_definitions = swagger_method.define_output_schema(resource)

        return external_definitions

    def add_definitions(self, external_definitions):
        while external_definitions:
            field = external_definitions.pop()
            field_name = field.__class__.__name__
            if field_name in self.definitions:
                # we already got this definition, we can skip
                continue
            schema, nested_definitions = get_schema(field)
            self.definitions[field_name] = schema

            # the new schema can have it's lot of definition, we add them
            external_definitions.update(nested_definitions)


def make_schema(resource):
    schema = Swagger()

    external_definitions = set()
    for method_name in resource.methods:
        external_definitions.update(schema.add_method(method_name, resource))

    schema.add_definitions(external_definitions)

    return schema
