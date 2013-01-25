import re
from werkzeug.routing import Rule

class Validation_Response : 
    valid = True,
    details = {}

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
    if(value == "0") :
        return False
    elif(value=="1"):
         return True
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

def datetime(value): 
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


def validate_arguments(request, validation_dict) :
    response = Validation_Response() 
    for key, value in request.args.iteritems() :
        s = re.split(r"\[\]$", key)
        splitted_key = s[0]
        if(not (splitted_key in validation_dict)) : 
            response.details[key] = {"status" : "ignored", "value":value}
        else:
            if not(validation_dict[splitted_key].repeated) and (len(s)>1 or len(request.args.getlist(splitted_key))> 2):
                response.details[key] = {"status" : "multiple", "value":value}

            for val in request.args.getlist(key) :
                try :
                    v = validation_dict[splitted_key].validator(val)
                    response.details[key] = {"status" : "valid", "value": val}
                except:
                    if(validation_dict[splitted_key].required):
                        response.valid = False
                    response.details[key] = {"status" : "notvalid", "value" : val }
    for key, value in validation_dict.iteritems():
        if value.required:
            if not(key in request.args) :
                response.valid = False
                response.details[key] = {"status", "missing"}



    return response

class RuleArguments(Rule):
    arguments = {},
    def __init__(self, string, defaults=None, subdomain=None, methods=None,
                 build_only=False, endpoint=None, strict_slashes=None,
                 redirect_to=None, alias=False, host=None, arguments=None):
        Rule.__init__(self, string, defaults, subdomain, methods, build_only,
                      endpoint, strict_slashes, redirect_to, alias, host)
        self.arguments = arguments

