# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from flask import request
from werkzeug.exceptions import HTTPException
import logging
from jormungandr.new_relic import record_exception


__all__ = ["RegionNotFound", "DeadSocketException", "ApiNotFound", "InvalidArguments"]


def format_error(code, message):
    error = {"error": {"id": code, "message": message}, "message": message}
    return error


class RegionNotFound(HTTPException):
    def __init__(self, region=None, lon=None, lat=None, object_id=None, custom_msg=None):
        super(RegionNotFound, self).__init__()
        self.code = 404
        if custom_msg:
            self.data = format_error("unknown_object", custom_msg)
            return
        if object_id:
            if object_id.count(";") == 1:
                lon, lat = object_id.split(";")
                object_id = None
            elif object_id[:6] == "coord:":
                lon, lat = object_id[6:].split(":")
                object_id = None
        if not any([region, lon, lat, object_id]):
            self.data = format_error("unknown_object", "No region nor " "coordinates given")
        elif region and not any([lon, lat, object_id]):
            self.data = format_error("unknown_object", "The region {0} " "doesn't exists".format(region))
        elif not any([region, object_id]) and lon and lat:
            self.data = format_error(
                "unknown_object",
                "No region available for the coordinates:" "{lon}, {lat}".format(lon=lon, lat=lat),
            )
        elif region == lon == lat is None and object_id:
            self.data = format_error("unknown_object", "Invalid id : {id}".format(id=object_id))
        else:
            self.data = format_error("unknown_object", "Unable to parse region")

    def __str__(self):
        return repr(self.data['message'])


class DeadSocketException(HTTPException):
    def __init__(self, region, path):
        super(DeadSocketException, self).__init__()
        error = 'The region {} is dead'.format(region)
        self.data = format_error("dead_socket", error)
        self.code = 503


class ApiNotFound(HTTPException):
    def __init__(self, api):
        super(ApiNotFound, self).__init__()
        error = 'The api {} doesn\'t exist'.format(api)
        self.data = format_error("unknown_object", error)
        self.code = 404


class UnknownObject(HTTPException):
    def __init__(self, msg):
        super(UnknownObject, self).__init__()
        error = 'The object {} doesn\'t exist'.format(msg)
        self.data = format_error("unknown_object", error)
        self.code = 404


class InvalidArguments(HTTPException):
    def __init__(self, arg):
        super(InvalidArguments, self).__init__()
        self.data = format_error("unknown_object", "Invalid arguments " + arg)
        self.code = 400


class UnableToParse(HTTPException):
    def __init__(self, msg):
        super(UnableToParse, self).__init__()
        self.data = format_error("unable_to_parse", msg)
        self.code = 400


class TechnicalError(HTTPException):
    def __init__(self, msg):
        super(TechnicalError, self).__init__()
        self.data = format_error("technical_error", msg)
        self.code = 500


class ConfigException(Exception):
    def __init__(self, arg):
        super(ConfigException, self).__init__(arg)
        self.data = format_error("config_exception", "Invalid config " + arg)
        self.code = 400


def log_exception(sender, exception, **extra):
    logger = logging.getLogger(__name__)
    message = ""
    if hasattr(exception, "data") and "message" in exception.data:
        message = exception.data['message']
    error = '{} {} {}'.format(exception.__class__.__name__, message, request.url)

    if isinstance(exception, (HTTPException, RegionNotFound)):
        logger.debug(error)
        if exception.code >= 500:
            record_exception()
    else:
        logger.exception(error)
        record_exception()
