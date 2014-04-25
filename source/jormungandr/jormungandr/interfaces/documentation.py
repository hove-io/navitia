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

from flask.ext.restful import Resource
from flask import current_app, request, abort, url_for
import inspect
import re


class Documentation(Resource):

    """ Make the documentation """

    def __init__(self, prefix="", api_version=0, *args, **kwargs):
        super(Documentation, self).__init__(*args, **kwargs)
        self.prefix = prefix
        self.endpoints = []
        self.api_version = 0
        self.base_path = None
        self.method_decorators = []

    def get(self, api=None):
        if api is None:
            return self.index()
        else:
            return self.get_api(api)
        return {"endpoint": api}

    def get_api(self, api):
        response = {
            "apiVersion": repr(self.api_version),
            "swaggerVersion": "1.2",
            "basePath": request.url_root,
            "resourcePath": "/" + api,
            "apis": []
        }
        rule_list = []
        all_rules = current_app.url_map._rules_by_endpoint
        endpoint = "v"+str(self.api_version)+"."+api
        if endpoint in all_rules:
            for rule in all_rules[endpoint]:
                rule_list.append((rule, endpoint))
        endpoint += ".collection"
        if endpoint in all_rules:
            for rule in all_rules[endpoint]:
                rule_list.append((rule, endpoint))
        if len(rule_list) == 0:
            abort(404, {"message": "api not found"})
        response["apis"] = self.get_endpoint(rule_list)
        return response

    def get_path(self, rule):
        path = rule.rule
        for argument in rule.arguments:
            path = re.sub(r'<(\w+):' + argument + '>', '{' + argument + '}',
                          path)
        return path

    def get_parameters(self, rule, parser):
        parameters = []
        for argument in rule.arguments:
            name = argument
            converter = rule._converters[name]
            description = converter.__doc__
            type_ = 'string'
            if 'type_' in dir(converter):
                type_ = converter.type_
            parameters.append({
                "paramType": "path",
                "name": name,
                "description": description,
                "type": type_,
                "required": True,
                "dataType": "string"
            })
        for argument in parser.args:
            if "description" in dir(argument) and\
                    argument.description and\
                    not argument.hidden:
                param = {"paramType": "query",
                         "name": argument.name,
                         "dataType": "string",
                         "required": argument.required,
                         "allowMultiple": argument.action == "append",
                         "description": argument.description,
                         }
                if len(argument.choices) > 0:
                    values = [list(i) for i in argument.choices]
                    param["allowableValues"] = {
                        "valueType": "LIST",
                        "values": values
                    }
                parameters.append(param)
        return parameters

    def get_endpoint(self, rules):
        apis = []
        for (rule, endpoint) in rules:
            path = self.get_path(rule)
            api = {"path": path,
                   "operations": []
                   }
            for method in rule.methods:
                if not method in ['OPTIONS', 'HEAD']:
                    view_func = current_app.view_functions[endpoint]
                    func = view_func.func_dict['view_class']
                    parameters = []
                    func_instance = func()
                    if "parsers" in dir(func_instance) and\
                            method.lower() in func_instance.parsers.keys():
                        parser = func_instance.parsers[method.lower()]
                        parameters = self.get_parameters(rule, parser)
                    summary = None
                    notes = None
                    if func.__doc__:
                        doc = func.__doc__.splitlines()
                        summary = doc[0]
                        notes = "" if len(doc) == 1\
                            else func.__doc__[len(summary) + 1:]
                    api["operations"].append({
                        "httpMethod": method.upper(),
                        "summary": summary,
                        "notes": notes,
                        "nickname": endpoint.replace(".", "_") + method,
                        "parameters": parameters
                    })
            apis.append(api)
        return apis

    def index(self):
        response = {
            "apiVersion": repr(self.api_version),
            "swaggerVersion": "1.2",
            "apis": []
        }
        apis_names = []
        for rule in current_app.url_map.iter_rules():
            splitted_endpoint = rule.endpoint.split(".")
            if self.prefix and len(splitted_endpoint) == 1:
                continue
            api_name = splitted_endpoint[1]
            if rule.rule[:len(request.url_rule.rule)] != request.url_rule.rule:
                splitted_endpoint = rule.endpoint.split(".")
                if not self.prefix or(
                    splitted_endpoint[0] == self.prefix) and\
                        not api_name in apis_names:
                    apis_names.append(api_name)
                    func = current_app.view_functions[rule.endpoint]
                    description = None
                    if "view_class" in func.func_dict.keys() and\
                            func.func_dict['view_class'].__doc__ is not None:
                        description = func.func_dict['view_class'].__doc__
                        description = description.splitlines()[0]
                    url = "/" + splitted_endpoint[1]
                    api = {"path": url}
                    if description:
                        api["description"] = description
                    response["apis"].append(api)
        return response


class V0Documentation(Documentation):

    def __init__(self, prefix=None, *args, **kwargs):
        super(V0Documentation, self).__init__(*args, **kwargs)
        self.prefix = "v0"
        self.api_version = 0


def v0_documentation(api):
    api.add_resource(V0Documentation,
                     "/v0/doc/<string:api>",
                     "/v0/doc/",
                     endpoint="v0.documentation")


class V1Documentation(Documentation):

    def __init__(self, prefix=None, *args, **kwargs):
        super(V1Documentation, self).__init__(*args, **kwargs)
        self.prefix = "v1"
        self.api_version = 1


def v1_documentation(api):
    api.add_resource(V1Documentation,
                     "/v1/doc/<string:api>",
                     "/v1/doc/",
                     endpoint="v1.documentation")
