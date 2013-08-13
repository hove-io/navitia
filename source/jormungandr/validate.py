import re
import sys
from werkzeug.routing import Rule

class Validation_Response : 
    def __init__(self):
        self.valid = True
        self.details = {}
        self.arguments = {}
        self.givenByUser_ = []
    
    def givenByUser(self):
        result = {}
        for k in self.givenByUser_:
            if k in self.arguments.keys():
                result[k] = self.arguments[k]
        return result


class Argument : 
    required = True,
    repeated = False,
    validator = None,
    defaultValue = None,
    order = 50,
    allowableValues = None

    def __init__(self, desc, validator, required = False, repeated = False,
                 defaultValue = None, order = 50, allowableValues = None, hidden=False) :
        self.description = desc
        self.required = required
        self.repeated = repeated
        self.validator = validator
        self.defaultValue = defaultValue
        self.order = order
        self.allowableValues = allowableValues
        self.hidden = hidden
        if(self.validator == None) : 
            print "A validator is required"

class ValidationTypeException(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)



def boolean(value):
    if(value.lower() == "true") :
        return True
    elif(value.lower()=="false"):
         return False
    else:
        raise ValidationTypeException("boolean")

def time(value):
    m = re.match(r"T(?P<hour>(\d){1,2})(?P<minutes>(\d){2})", value)
    if(m) :
        if(int(m.groupdict()['hour']) < 0 or int(m.groupdict()['hour']) > 24 or
           int(m.groupdict()['minutes']) < 0 or int(m.groupdict()['minutes']) > 60):
            raise ValidationTypeException("time1")
        else:
            return value
    else:
            raise ValidationTypeException("time2")

    return value

def datetime_validator(value):
    m = re.match(r"^(\d){4}(?P<month>(\d){2})(?P<day>(\d){2})(?P<hour>T(\d){3,4})", value)
    if(m) :
        if(int(m.groupdict()['month']) < 1 or int(m.groupdict()['month']) > 12 or
           int(m.groupdict()['day']) < 1 or int(m.groupdict()['day']) > 31):
            raise Exception("validation_error")
        else:
            time(m.groupdict()['hour'])
            return value
    else:
        raise ValidationTypeException("datetime")

    return value


def entrypoint(valid_types = None):
    if valid_types == None:
        types = ("validity_pattern", "line", "route", "vehicle_journey",
        "stop_point", "stop_area", "stop_time", "network", "mode", "commercial_mode",
        "city", "connection", "route_point", "district", "department", "company", " vehicle", "country", "way", "coord", "address")
    else:
        types  = valid_types
    return lambda value : entrypoint_validator(value, types)


def entrypoint_validator(value, valid_types=None):
    m = re.match(r"^(?P<type>(\w)+):", value)
    if not m or m.groupdict()["type"] not in valid_types:
        raise ValidationTypeException("entrypoint")
    return value

def filter(value):
    return unicode(value)


def validate_arguments(request, validation_dict) :
    response = Validation_Response()
    for key, value in request.iteritems() :
        if not (key in validation_dict) :
            response.details[key] = {"status" : "ignored", "value":str(value)}
        else:
            if not validation_dict[key].repeated and len(value) > 1:
                response.details[key] = {"status" : "multiple", "value":str(value)}
            for val in value :
                try :
                    parsed_val = validation_dict[key].validator(val)
                    response.details[key] = {"status" : "valid", "value": val}
                    if not(validation_dict[key].repeated) :
                        if not(validation_dict[key].allowableValues) or parsed_val in validation_dict[key].allowableValues :
                            response.arguments[key] = parsed_val
                            response.givenByUser_.append(key)
                        else:
                            response.valid = False
                            response.details[key] = {"status" : "not in allowable values", "value":parsed_val}
                    else:
                        if not(validation_dict[key].allowableValues) or parsed_val in validation_dict[key].allowableValues :
                            if not(key in response.arguments):
                                response.arguments[key] = []
                                response.givenByUser_.append(key)
                            response.arguments[key].append(parsed_val)
                        else:
                            response.valid=False
                            response.details[key] = {"status" : "not in allowable values", "value":parsed_val}
                except:
                    response.valid = False
                    response.details[key] = {"status" : "notvalid", "value" : val }
    for key, value in validation_dict.iteritems():
        if not(key in request) :
            if value.required:
                response.valid = False
                response.details[key] = {"status": "missing"}
            else:
                response.arguments[key] = value.defaultValue

    return response

class RuleArguments(Rule):
    arguments = {},
    def __init__(self, string, defaults=None, subdomain=None, methods=None,
                 build_only=False, endpoint=None, strict_slashes=None,
                 redirect_to=None, alias=False, host=None, arguments=None):
        Rule.__init__(self, string, defaults, subdomain, methods, build_only,
                      endpoint, strict_slashes, redirect_to, alias, host)
        self.arguments = arguments

def validate_apis(apis) :
    response = Validation_Response() 
    for name, api in apis.iteritems():
        response.details[name] = [] 
        
        if not "arguments" in api:
            response.valid = False
            response.details[name].append("No arguments in the api ")
        else:
            for argname, argument in api["arguments"].iteritems():
                if not(argument.required) and argument.defaultValue == None:
                    response.details[name].append(argname + " doesn't have any defaultValue and is not required ")
                    response.valid = False


    return response

