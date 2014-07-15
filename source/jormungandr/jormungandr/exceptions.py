# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from flask import request
import logging


__all__ = ["RegionNotFound", "DeadSocketException", "ApiNotFound",
           "InvalidArguments"]


def format_error(code, message):
    error = {"error":
             {"id": code,
              "message": message
              }, "message": message}
    return error


class RegionNotFound(Exception):

    def __init__(self, region=None, lon=None, lat=None, object_id=None, custom_msg=None):
        if custom_msg:
            self.data = format_error("unknown_object", custom_msg)
        elif region == lon == lat is None == object_id:
            self.data = format_error("unknown_object", "No region nor "
                                     "coordinates given")
        elif region and lon == lat == object_id is None:
            self.data = format_error("unknown_object", "The region {0} "
                                     "doesn't exists".format(region))
        elif region == object_id is None and lon and lat:
            self.data = format_error("unknown_object",
                                     "No region available for the coordinates:"
                                     "{lon}, {lat}".format(lon=lon, lat=lat))
        elif region == lon == lat is None and object_id:
            self.data = format_error("unknown_object",
                                     "Invalid id : {id}".format(id=object_id))
        else:
            self.data = format_error("unknown_object",
                                     "Unable to parse region")
        self.code = 404

    def __str__(self):
        return repr(self.data['message'])


class DeadSocketException(Exception):

    def __init__(self, region, path):
        error = "The region " + region + " is dead"
        self.data = format_error("dead_socket", error)
        self.code = 503


class ApiNotFound(Exception):

    def __init__(self, api):
        error = "The api " + api + " doesn't exist"
        self.data = format_error("unknown_object", error)
        self.code = 404


class InvalidArguments(Exception):

    def __init__(self, arg):
        self.data = format_error("unknown_object", "Invalid arguments " + arg)
        self.code = 400


class TechnicalError(Exception):

    def __init__(self, msg):
        self.data = format_error("technical_error", msg)
        self.code = 400


def log_exception(sender, exception, **extra):
    message = ""
    if hasattr(exception, "data") and "message" in exception.data:
        message = exception.data['message']
    error = exception.__class__.__name__ + " " + message + " " + request.url
    logging.exception(error)
