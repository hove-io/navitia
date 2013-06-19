from validate import *
from instance_manager import NavitiaManager
import copy

def convertType(validator):
    if validator == str :
        return "string"
    elif validator == datetime_validator:
        return "datetime"
    elif validator == time:
        return "time"
    elif validator == int:
        return "integer"
    elif validator == float:
        return "float"
    elif validator == boolean:
        return "boolean"
    elif validator == entrypoint():
        return '<a href="http://www.navitia.io/#entrypoints">entrypoint</a>'
    elif validator == filter:
        return '<a href="http://www.navitia.io/#filter">filter</a>'
    else:
        return "string"


def api_doc(apis, api = None) : 
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
                if not item[1].hidden:
                    param = {}
                    param['name'] = item[0]
                    param['paramType'] = 'query'
                    param['description'] = item[1].description
                    param['dataType'] = convertType(item[1].validator)
                    param['required'] = item[1].required
                    param['allowMultiple'] = item[1].repeated
                    if item[1].allowableValues:
                        param['allowableValues'] = {"valueType" : "LIST", "values" :
                                                    item[1].allowableValues}
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
                for key in NavitiaManager().instances.keys():
                    regions['allowableValues']["values"].append(key)
                params.append(regions)
                path += "{region}/"
            response['resourcePath'] = api

            if api == "regions":
                path += "regions.json"
            else:
                path += api+".{content-type}"
                formats = {}
                formats['name'] = 'content-type'
                formats['paramType'] = 'path'
                formats['description'] = 'The type of data you want to have'
                formats['dataType'] = 'String'
                formats['required'] = 'True'
                formats['allowMultiple'] = False
                formats['allowableValues'] = {'valueType':'LIST', "values":
                                              ['json', 'xml', 'pb']}
                params.append(formats)

            
            response['apis'].append({
                    "path" : path,
                    "description" : apis[api]["description"] if "description" in apis[api] else "",
                    "operations" : [{
                            "httpMethod" : "GET",
                            "summary" : apis[api]["description"] if "description" in apis[api] else "",
                            "nickname" : api,
                            "responseClass" : "void",
                            "parameters" : params,
                            }]
            })
            if "universal" in apis[api] and apis[api]["universal"]:
                params_universal = copy.deepcopy(params)
                i = 0
                for param in params_universal : 
                    #if param['dataType'] == convertType(entrypoint):
                    #    param['description'] += " (can only be a coord)"
                    if param['name'] == 'region':
                        del params_universal[i]
                    i+=1                
                response['apis'].append({
                        "path" : "/{version}/"+api+".{content-type}",
                        "description" : apis[api]["description"] if "description" in apis[api] else "",
                        "operations" : [{
                            "httpMethod" : "GET",
                            "summary" : apis[api]["description"] if "description" in apis[api] else "",
                            "nickname" : api+"_universal",
                            "responseClass" : "void",
                            "parameters" : params_universal,
                            }]
                })

    else:
        list_apis = apis.items()
        list_apis.sort(key = lambda x : x[1]['order'] if "order" in x[1] else 50)
        for l in list_apis :
            key = l[0]
            if "hidden" not in apis[key]  or not apis[key]["hidden"]:
                response['apis'].append({"path":"/doc.{format}/"+key, "description" :apis[key]["description"] if "description" in apis[key] else  ""})


    return response

