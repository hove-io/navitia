#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
from navitiacommon import request_pb2, type_pb2
from flask import logging


class PtRef(object):
    def __init__(self, instance):
        self.instance = instance

    def get_stop_point(self, line_uri, code_key, code_value):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL
        req.ptref.requested_type = type_pb2.STOP_POINT
        req.ptref.count = 1
        req.ptref.start_page = 0
        req.ptref.depth = 1
        req.ptref.filter = "stop_point.has_code({code_key}, {code_value})".\
            format(code_key=code_key, code_value=code_value)
        if line_uri:
            req.ptref.filter = req.ptref.filter + ' and line.uri={}'.format(line_uri)

        result = self.instance.send_and_receive(req)
        if len(result.stop_points) == 0:
            logging.getLogger(__name__).info('PtRef, Unable to find stop_point with filter {}'.
                                             format(req.ptref.filter))
            return None
        if len(result.stop_points) == 1:
            return result.stop_points[0]

        logging.getLogger(__name__).info('PtRef, Multiple stop_points found with filter {}'.format(req.ptref.filter))
        return None
