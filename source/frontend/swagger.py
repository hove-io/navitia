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
    
    if(api) :
        if(api in apis) :
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
            version = {}
            version['name'] = 'version'
            version['paramType'] = 'path'
            version['description'] = 'Version of the Api'
            version['dataType'] = 'String'
            version['allowableValues'] = {"values" : ["v0"],
                                          "valueType":"LIST"}
            version['required'] = True
            version['allowMultiple'] = False
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
            params.append(version)
            params.append(regions)

            response['resourcePath'] = "/"+api
            response['apis'].append({
                    "path" : "/{version}/{region}/"+api+".{format}",
                    "description" : "",
                    "operations" : [{
                            "httpMethod" : "GET",
                            "summary" : "",
                            "nickname" : api,
                            "responseClass" : "void",
                            "parameters" : params
                            }
                            ]
                                    })

    else:
        for key, val in apis.iteritems() :
            response['apis'].append({"path":"/doc.{format}/"+key, "description" : ""})

    r = Response(json.dumps(response, ensure_ascii=False), mimetype='application/json')
    r.headers.add('Access-Control-Allow-Origin', '*')
    return r

