import re
from werkzeug.routing import Rule

class Validation_Response : 
    def __init__(self):
        self.valid = True
        self.details = {}
        self.arguments = {}

class Argument : 
    required = True,
    repeated = False,
    validator = None,
    defaultValue = None,
    order = 50

    def __init__(self, desc, validator, required = False, repeated = False,
                 defaultValue = None, order = 50) :
        self.description = desc
        self.required = required
        self.repeated = repeated
        self.validator = validator
        self.defaultValue = defaultValue
        self.order = order
        if(self.validator == None) : 
            print "A validator is required"


def boolean(value):
    if(value.lower() == "true") :
        return True
    elif(value.lower()=="false"):
         return False
    else:
        raise Exception("validation_error")

def time(value): 
    m = re.match(r"T(?P<hour>(\d){1,2})(?P<minutes>(\d){2})", value)
    if(m) :
        if(int(m.groupdict()['hour']) < 0 or int(m.groupdict()['hour']) > 24 or
           int(m.groupdict()['minutes']) < 0 or int(m.groupdict()['minutes']) > 60):
            raise Exception("validation_error")
        else:
            return value
    else:
            raise Exception("validation_error")

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
        raise Exception("validation_error")

    return value
valid_types = ("validity_pattern", "line", "route", "vehicle_journey",
        "stop_point", "stop_area", "stop_time", "network", "mode", "mode_type",
        "city", "connection", "route_point", "district", "department", "company", " vehicle", "country", "way", "coord", "address")

def entrypoint(value):
    m = re.match(r"^(?P<type>(\w)+):", value)
    if not m or m.groupdict()["type"] not in valid_types:
        raise Exception("validation_error")
    return value

def filter(value):
    return value


def validate_arguments(request, validation_dict) :
    response = Validation_Response() 
    for key, value in request.args.iteritems() :
        if not (key in validation_dict) : 
            response.details[key] = {"status" : "ignored", "value":value}
        else:
            if not validation_dict[key].repeated and len(request.args.getlist(key)) > 1:
                response.details[key] = {"status" : "multiple", "value":value}

            for val in request.args.getlist(key) :
                try :
                    parsed_val = validation_dict[key].validator(val)
                    response.details[key] = {"status" : "valid", "value": val}
                    if not(validation_dict[key].repeated) :
                        response.arguments[key] = parsed_val
                    else:
                        if not(key in response.arguments):
                            response.arguments[key] = []
                        response.arguments[key].append(parsed_val)
                except:
                    if validation_dict[key].required:
                        response.valid = False
                    response.details[key] = {"status" : "notvalid", "value" : val }
    for key, value in validation_dict.iteritems():
        if not(key in request.args) :
            if value.required:
                response.valid = False
                response.details[key] = {"status", "missing"}
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

