from flask.ext.restful import Resource
from flask import current_app, request, abort, url_for

class Documentation(Resource):
    """ Make the documentation """
    def __init__(self, prefix="", api_version=0, *args, **kwargs):
        super(Documentation, self).__init__(*args, **kwargs)
        self.prefix = prefix
        self.endpoints = []
        self.api_version = 0
        self.base_path = None


    def get(self, endpoint=None):
        if endpoint is None:
            return self.index()
        else:
            return self.get_endpoint(endpoint)
        return {"endpoint" : endpoint}


    def get_endpoint(self, endpoint):
        if self.prefix:
            endpoint = self.prefix + endpoint
        if endpoint in current_app.url_map._rules_by_endpoint:
            response = {
                "apiVersion" : repr(self.api_version),
                "swaggerVersion": "1.2",
                "basePath" : request.url_root,
                "resourcePath" : url_for(endpoint, region="rennes"),
                "apis" : []
            }
            rules = current_app.url_map._rules_by_endpoint[endpoint]
            for rule in rules:
                arguments = []
                for argument in rule.arguments:
                    arguments.append((argument, '{'+argument+'}'))
                path = url_for(endpoint, **dict(arguments))
                method = None
                for m in rule.methods:
                    if not m in ['OPTIONS', 'HEAD']:
                        method = m
                view_func = current_app.view_functions[endpoint]
                func = view.func_dict['view_class']
                summary = None
                notes = None
                if func.__doc__:
                    doc = func.__doc__.splitlines()
                    summary = doc[0]
                    notes = "" if len(doc)==1 else func.__doc__[len(summary)+1:]
                 #response["apis"].append(
                 #       "path" : path,
                 #       "description" :

                #)
                print dir(func)
            return  response
        else:
            abort(404, {"message" : "endpoint not found"})


    def index(self):
        response = {
            "apiVersion" : repr(self.api_version),
            "swaggerVersion": "1.2",
            "apis" : []
        }
        for rule in current_app.url_map.iter_rules():
            if self.prefix and\
               rule.rule[:len(request.url_rule.rule)] != request.url_rule.rule:
                if self.prefix and\
                   rule.endpoint[:len(self.prefix)] == self.prefix:
                    func = current_app.view_functions[rule.endpoint]
                    description = None
                    if "view_class" in func.func_dict.keys() and\
                       func.func_dict['view_class'].__doc__ != None:
                        description = func.func_dict['view_class'].__doc__
                        description = description.splitlines()[0]
                    url = request.url_rule.rule
                    url += rule.endpoint[len(self.prefix):]
                    api = { "path" : url}
                    if description:
                        api["description"] = description
                    response["apis"].append(api)
        return response


class V0Documentation(Documentation):
    def __init__(self, prefix= None, *args, **kwargs):
        super(V0Documentation, self).__init__(*args, **kwargs)
        self.prefix = "v0."
        self.api_version = 0

def v0_documentation(api):
    #print dir(api.app.url_map)
    #for rule in api.app.url_map.iter_rules():
    #    if rule.endpoint[:len("v0")] == "v0":
    #        api.
    #        print rule.endpoint, rule.arguments
            #func = api.app.view_functions[rule.endpoint]
            #print dir(func.func_dict['view_class'])
    api.add_resource(V0Documentation,
                     "/v0/doc/<string:endpoint>",
                     "/v0/doc/",
                     endpoint="v0.documentation")
