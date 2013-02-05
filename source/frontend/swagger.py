from werkzeug.wrappers import Response
import json
from validate import *

def convertType(validator):
    if validator == str :
        return "string"
    elif validator == datetime:
        return "datetime"
    elif validator == time:
        return "time"
    elif validator == int:
        return "integer"
    elif validator == float:
        return "float"
    elif validator == boolean:
        return "boolean"
    elif validator == entrypoint:
        return '<a href="#">entrypoint</a>'
    elif validator == filter:
        return "filter"
    else:
        return "string"


def api_doc(apis, instance_manager, api = None) : 
    response = {}
    response['apiVersion'] = "0.2"
    response['swaggerVersion'] = "1.1"
    response['basePath'] = ""
    response['apis'] = []
    
    if api:
        if api in apis:
            params = []
            list_params = apis[api]['arguments'].items()
            list_params.sort(key = lambda x : x[1].order)

            for item in list_params:
                param = {}
                param['name'] = item[0]
                param['paramType'] = 'query'
                param['description'] = item[1].description
                param['dataType'] = convertType(item[1].validator)
                param['required'] = item[1].required
                param['allowMultiple'] = item[1].repeated
                params.append(param)
            path = "/"
            version = {}
            version['name'] = 'version'
            version['paramType'] = 'path'
            version['description'] = 'Version of the Api'
            version['dataType'] = 'String'
            version['allowableValues'] = {"values" : ["v0"],
                                          "valueType":"LIST"}
            version['required'] = True
            version['allowMultiple'] = False
            params.append(version)
            path += "{version}/"
            
            if not("regions" in apis[api]) or apis[api]["regions"]:
                regions = {}
                regions['name'] = 'region'
                regions['paramType'] = 'path'
                regions['description'] = 'The region you want to query'
                regions['dataType'] = 'String'
                regions['required'] = True
                regions['allowMultiple'] = False
                regions['allowableValues'] = {"valueType":"LIST", "values":[]}
                for key, val in instance_manager.keys_navitia.iteritems():
                    regions['allowableValues']["values"].append(key)
                params.append(regions)
                path += "{region}/"
            response['resourcePath'] = api
            
            response['apis'].append({
                    "path" : path+api+".{format}",
                    "description" : apis[api]["description"] if "description" in apis[api] else "",
                    "operations" : [{
                            "httpMethod" : "GET",
                            "summary" : apis[api]["description"] if "description" in apis[api] else "",
                            "nickname" : api,
                            "responseClass" : "void",
                            "parameters" : params,
                            #"example" : apis[api]["example"] if "example" in apis[api] else None
                            }
                            ]
                                    })

    else:
        list_apis = apis.items()
        list_apis.sort(key = lambda x : x[1]['order'] if "order" in x[1] else 50)
        for l in list_apis :
            key = l[0]
            response['apis'].append({"path":"/doc.{format}/"+key, "description" :apis[key]["description"] if "description" in apis[key] else  ""})


    return response

