# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from jormungandr.pt_planners.common import ZmqSocket, get_crow_fly, get_odt_stop_points, get_stop_points_from_uri
from jormungandr import utils, app
from .pt_planner import AbstractPtPlanner
from navitiacommon import type_pb2


class Kraken(ZmqSocket, AbstractPtPlanner):
    def __init__(
        self,
        name,
        zmq_context,
        zmq_socket,
        zmq_socket_type,
        timeout=app.config.get(str('INSTANCES_TIMEOUT'), 10),
    ):
        super(Kraken, self).__init__(
            "pt_planner_kraken_{}".format(name), zmq_context, zmq_socket, zmq_socket_type, timeout
        )

    def journeys(self, origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt, request_id):
        req = utils.create_journeys_request(
            origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt
        )
        return self.send_and_receive(req, request_id=request_id)

    def graphical_isochrones(
        self, origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
    ):
        req = utils.create_graphical_isochrones_request(
            origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
        )
        return self.send_and_receive(req)

    def get_access_points(self, pt_object, access_point_filter, request_id):
        stop_points = get_stop_points_from_uri(self, pt_object.uri, request_id, depth=2)
        if not stop_points:
            return None

        return [
            type_pb2.PtObject(name=ap.name, uri=ap.uri, embedded_type=type_pb2.ACCESS_POINT, access_point=ap)
            for ap in stop_points[0].access_points
            if access_point_filter(ap)
        ]

    def get_crow_fly(
        self,
        origin,
        streetnetwork_mode,
        max_duration,
        max_nb_crowfly,
        object_type=type_pb2.STOP_POINT,
        filter=None,
        stop_points_nearby_duration=300,
        request_id=None,
        depth=2,
        forbidden_uris=[],
        allowed_id=[],
        **kwargs
    ):
        return get_crow_fly(
            self,
            origin,
            streetnetwork_mode,
            max_duration,
            max_nb_crowfly,
            object_type,
            filter,
            stop_points_nearby_duration,
            request_id,
            depth,
            forbidden_uris,
            allowed_id,
            **kwargs
        )

    def get_odt_stop_points(self, coord, request_id):
        return get_odt_stop_points(self, coord, request_id)
