from flask import current_app, request

__all__ = ["RegionNotFound", "DeadSocketException", "ApiNotFound",
            "InvalidArguments"]


class RegionNotFound(Exception):
    def __init__(self, region=None, lon=None, lat=None, object_id=None):
        self.data = {}
        if region == lon == lat == None == object_id:
            self.data['message'] = "No region nor coordinates given"
        elif region and lon == lat == object_id == None:
            self.data['message'] = "The region {0} doesn't exists".format(region)
        elif region == object_id == None and lon and lat:
            self.data['message'] = "No region available for the coordinates \
                    : {lon}, {lat}".format(lon=lon, lat=lat)
        elif region == lon == lat == None and object_id:
            self.data['message'] = "Invalid id : {id}".format(id=object_id)
        else:
            self.data['message'] = "Unable to parse region"
        self.code = 404
        current_app.logger.error('RegionNotFound : ' + self.data['message']+\
                " "+request.url)

    def __str__(self):
        return repr(self.data['message'])


class DeadSocketException(Exception):
    def __init__(self, region, path):
        self.data['message'] = "The region " + region + " is dead"
        self.code = 503
        current_app.logger.error('DeadSocketException : ' + self.data['message']+\
                " "+request.url)


class ApiNotFound(Exception):
    def __init__(self, api):
        self.data['message'] = "The api " + api + " doesn't exist"
        self.code = 404
        current_app.logger.error('ApiNotFound : ' + self.data['message']+\
                " "+request.url)


class InvalidArguments(Exception):
    def __init__(self, arg):
        self.data['message'] = "Invalid arguments " + arg
        self.code = 400
        current_app.logger.error('InvalidArguments : ' + self.data['message']+\
                " "+request.url)
