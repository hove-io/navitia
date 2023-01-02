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

from jormungandr.pt_planners.common import ZmqSocket, get_crow_fly
from jormungandr import utils, app
from .pt_planner import AbstractPtPlanner
from navitiacommon import type_pb2, request_pb2
import logging


class Kraken(ZmqSocket, AbstractPtPlanner):
    def __init__(
        self, name, zmq_context, zmq_socket, zmq_socket_type, timeout=app.config.get(str('INSTANCE_TIMEOUT'), 10)
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
