# coding: utf-8

#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from navitiacommon import request_pb2, type_pb2
import logging

# Mapping to know with response field to consider for a given pb_type
PT_TYPE_RESPONSE_MAPPING = {
    type_pb2.STOP_AREA: 'stop_areas',
    type_pb2.STOP_POINT: 'stop_points',
    type_pb2.POI: 'pois',
    type_pb2.ADMINISTRATIVE_REGION: 'administrative_regions',
    type_pb2.LINE: 'lines',
    type_pb2.NETWORK: 'networks',
    type_pb2.COMMERCIAL_MODE: 'commercial_modes',
    type_pb2.ROUTE: 'routes',
    type_pb2.LINE_GROUP: 'line_groups',
}


class PTRefException(Exception):
    pass


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
        if not all([code_key, code_value]):
            logging.getLogger(__name__).warning(
                'PtRef, not valid filter you give: code_key = {},  code_value= {}'.format(code_key, code_value)
            )
            return None
        req.ptref.filter = 'stop_point.has_code("{code_key}", "{code_value}")'.format(
            code_key=code_key, code_value=code_value
        )
        if line_uri:
            req.ptref.filter = req.ptref.filter + ' and line.uri="{}"'.format(line_uri)

        result = self.instance.send_and_receive(req)
        if len(result.stop_points) == 0:
            logging.getLogger(__name__).info(
                'PtRef, Unable to find stop_point with filter {}'.format(req.ptref.filter)
            )
            return None
        if len(result.stop_points) == 1:
            return result.stop_points[0]

        logging.getLogger(__name__).info(
            'PtRef, Multiple stop_points found with filter {}'.format(req.ptref.filter)
        )
        return None

    def get_objs(self, pb_type, filter='', odt_level=None):
        """
        iterator on all navitia objects that match a filter
        """
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL
        req.ptref.requested_type = pb_type
        req.ptref.count = 20
        req.ptref.start_page = 0
        req.ptref.depth = 1
        req.ptref.filter = filter
        if odt_level:
            req.ptref.odt_level = odt_level

        while True:
            result = self.instance.send_and_receive(req)

            objects = getattr(result, PT_TYPE_RESPONSE_MAPPING[pb_type])
            if not objects:
                # when no more objects are yielded we stop
                return

            for o in objects:
                yield o

            req.ptref.start_page += 1
            if req.ptref.count > result.pagination.itemsOnPage:
                # we did not get as much results as planned, we can stop
                return

    def get_matching_routes(self, line_uri, start_sp_uri, destination_code, destination_code_key):
        req = request_pb2.Request()
        req.requested_api = type_pb2.matching_routes
        req.matching_routes.line_uri = line_uri
        req.matching_routes.start_stop_point_uri = start_sp_uri
        req.matching_routes.destination_code = destination_code
        req.matching_routes.destination_code_key = destination_code_key

        result = self.instance.send_and_receive(req)

        if result.HasField('error'):
            raise PTRefException('impossible to find matching routes because {}'.format(result.error.message))

        return result.routes


class FeedPublisher(object):
    def __init__(self, id, name, license='', url=''):
        self.id = id
        self.name = name
        self.license = license
        self.url = url
