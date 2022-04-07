# coding=utf-8

#  Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
import navitiacommon.response_pb2 as response_pb2
from functools import wraps
from flask import request

'''

                                Gestion des erreurs :
            ------------------------------------------------------------
            |    error_id                       |           http_code   |
            ------------------------------------------------------------
            |date_out_of_bounds					|  404                  |
            |no_origin							|  404                  |
            |no_destination						|  404                  |
            |no_origin_nor_destination			|  404                  |
            |unknown_object						|  404                  |
            |                                   |                       |
            |no_solution                        |  200                  |
            |                                   |                       |
            |bad_filter                         |  400                  |
            |unknown_api                        |  400                  |
            |unable_to_parse                    |  400                  |
            |bad_format                         |  400                  |
            |internal_error                     |  500                  |
            -------------------------------------------------------------

'''


class ManageError(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            response = f(*args, **kwargs)
            code = 200
            errors = {
                response_pb2.Error.service_unavailable: 503,
                response_pb2.Error.internal_error: 500,
                response_pb2.Error.date_out_of_bounds: 404,
                response_pb2.Error.no_origin: 404,
                response_pb2.Error.no_destination: 404,
                response_pb2.Error.no_origin_nor_destination: 404,
                response_pb2.Error.unknown_object: 404,
                response_pb2.Error.unable_to_parse: 400,
                response_pb2.Error.bad_filter: 400,
                response_pb2.Error.unknown_api: 400,
                response_pb2.Error.bad_format: 400,
                response_pb2.Error.no_solution: 200,
            }
            if response.HasField(str("error")) and response.error.id in errors:
                code = errors[response.error.id]
                if code == 400 and "filter" not in request.args:
                    response.error.id = response_pb2.Error.unknown_object
                    code = 404

            else:
                response.ClearField(str("error"))
            return response, code

        return wrapper
