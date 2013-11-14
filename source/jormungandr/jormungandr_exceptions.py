from flask import request

__all__ = ["RegionNotFound", "DeadSocketException", "ApiNotFound",
            "InvalidArguments"]

def format_error(code, message):
    error = {"error":
                    {"id":code,
                     "message":message
                    },"message":message}
    return error

class RegionNotFound(Exception):
    def __init__(self, region=None, lon=None, lat=None, object_id=None):
        if region == lon == lat == None == object_id:
            self.data = format_error("unknown_object","No region nor coordinates given")
        elif region and lon == lat == object_id == None:
            self.data = format_error("unknown_object","The region {0} doesn't exists".format(region))
        elif region == object_id == None and lon and lat:
            self.data = format_error("unknown_object","No region available for the coordinates : {lon}, {lat}".format(lon=lon, lat=lat))
        elif region == lon == lat == None and object_id:
            self.data = format_error("unknown_object","Invalid id : {id}".format(id=object_id))
        else:
            self.data = format_error("unknown_object","Unable to parse region")
        self.code = 404

    def __str__(self):
        return repr(self.data['message'])


class DeadSocketException(Exception):
    def __init__(self, region, path):
        self.data = format_error("unknown_object","The region " + region + " is dead")
        self.code = 503


class ApiNotFound(Exception):
    def __init__(self, api):
        self.data = format_error("unknown_object","The api " + api + " doesn't exist" )
        self.code = 404


class InvalidArguments(Exception):
    def __init__(self, arg):
        self.data = format_error("unknown_object","Invalid arguments " + arg  )
        self.code = 400

def log_exception(sender, exception, **extra):
    message = ""
    if hasattr(exception, "data") and exception.data.has_key("message"):
        message = exception.data['message']
    sender.logger.error(exception.__class__.__name__ + " " + message + " " + request.url)
