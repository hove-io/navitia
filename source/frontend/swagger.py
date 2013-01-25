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
            for key, val in apis[api]['arguments'].iteritems():
                param = {}
                param['name'] = key
                param['paramType'] = 'query'
                param['description'] = val.description
                param['dataType'] = convertType(val.validator)
                param['required'] = val.required
                param['allowMultiple'] = val.repeated
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
            response['resourcePath'] = "/"+api
            
            response['apis'].append({
                    "path" : path+api+".{format}",
                    "description" : apis[api]["description"] if "description" in apis[api] else "",
                    "operations" : [{
                            "httpMethod" : "GET",
                            "summary" : apis[api]["description"] if "description" in apis[api] else "",
                            "nickname" : api,
                            "responseClass" : "void",
                            "parameters" : params
                            }
                            ]
                                    })

    else:
        for key, val in apis.iteritems() :
            response['apis'].append({"path":"/doc.{format}/"+key, "description" :apis[key]["description"] if "description" in apis[key] else  ""})


    return response

