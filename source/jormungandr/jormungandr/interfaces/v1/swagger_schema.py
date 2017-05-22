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


class SwaggerMethod(object):
    def __init__(self, summary='', parameters=None):
        self.summary = summary
        self.parameters = parameters or []
        # self.output_type = output


class Swagger(object):
    definitions = []
    methods = {}

    def add_method(self, method_name, resource):
        method_name = method_name.lower()  # a bit hacky, but we want a lower case http verb

        if method_name == 'options':
            return  # we don't want to document options
        swagger_method = SwaggerMethod()

        request_parser = resource.parsers.get(method_name)
        if request_parser:
            for argument in request_parser.args:
                swagger_params = SwaggerParam.make_from_flask_arg(argument)
                # several swagger args can be created for one flask arg
                swagger_method.parameters += swagger_params

        path_params = SwaggerParam.make_from_flask_route(resource, method_name)
        swagger_method.parameters += path_params

        self.methods[method_name] = swagger_method


def make_schema(resource):
    schema = Swagger()

    for method_name in resource.methods:
        schema.add_method(method_name, resource)

    return schema