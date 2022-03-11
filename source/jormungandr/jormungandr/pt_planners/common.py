# Copyright (c) 2001-2022, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import, print_function, unicode_literals, division

import logging
import pybreaker
import zmq
import time
from contextlib import contextmanager
from collections import deque
from datetime import datetime, timedelta
import flask
import six
from threading import Lock
from abc import abstractmethod, ABCMeta

from jormungandr import app
from jormungandr.exceptions import DeadSocketException
from navitiacommon import response_pb2, type_pb2, request_pb2
from jormungandr import utils


class ZmqSocket(six.with_metaclass(ABCMeta, object)):
    def __init__(self, zmq_context, zmq_socket, zmq_socket_type=None):
        self.zmq_socket = zmq_socket
        self.context = zmq_context
        self.sockets = deque()
        self.zmq_socket_type = zmq_socket_type
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config.get(str('CIRCUIT_BREAKER_MAX_INSTANCE_FAIL'), 5),
            reset_timeout=app.config.get(str('CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S'), 60),
        )
        self.is_initialized = False
        self.lock = Lock()

    @abstractmethod
    def name(self):
        pass

    @contextmanager
    def socket(self, context):
        try:
            socket, _ = self.sockets.pop()
        except IndexError:  # there is no socket available: lets create one
            socket = context.socket(zmq.REQ)
            socket.connect(self.zmq_socket)
        try:
            yield socket
        finally:
            if not socket.closed:
                self.sockets.append((socket, time.time()))

    def _send_and_receive(
        self, request, timeout=app.config.get('INSTANCE_TIMEOUT', 10000), quiet=False, **kwargs
    ):
        logger = logging.getLogger(__name__)
        deadline = datetime.utcnow() + timedelta(milliseconds=timeout)
        request.deadline = deadline.strftime('%Y%m%dT%H%M%S,%f')

        with self.socket(self.context) as socket:
            if 'request_id' in kwargs and kwargs['request_id']:
                request.request_id = kwargs['request_id']
            else:
                try:
                    request.request_id = flask.request.id
                except RuntimeError:
                    # we aren't in a flask context, so there is no request
                    if 'flask_request_id' in kwargs and kwargs['flask_request_id']:
                        request.request_id = kwargs['flask_request_id']

            socket.send(request.SerializeToString())
            if socket.poll(timeout=timeout) > 0:
                pb = socket.recv()
                resp = response_pb2.Response()
                resp.ParseFromString(pb)
                return resp
            else:
                socket.setsockopt(zmq.LINGER, 0)
                socket.close()
                if not quiet:
                    logger.error('request on %s failed: %s', self.zmq_socket, six.text_type(request))
                raise DeadSocketException(self.name, self.zmq_socket)

    def send_and_receive(self, *args, **kwargs):
        """
        encapsulate all call to kraken in a circuit breaker, this way we don't loose time calling dead instance
        """
        try:
            return self.breaker.call(self._send_and_receive, *args, **kwargs)
        except pybreaker.CircuitBreakerError:
            raise DeadSocketException(self.name, self.zmq_socket)


def create_journeys_request(origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt):
    req = request_pb2.Request()
    req.requested_api = type_pb2.pt_planner
    for stop_point_id, access_duration in origins.items():
        location = req.journeys.origin.add()
        location.place = stop_point_id
        location.access_duration = access_duration

    for stop_point_id, access_duration in destinations.items():
        location = req.journeys.destination.add()
        location.place = stop_point_id
        location.access_duration = access_duration

    req.journeys.night_bus_filter_max_factor = journey_parameters.night_bus_filter_max_factor
    req.journeys.night_bus_filter_base_factor = journey_parameters.night_bus_filter_base_factor

    req.journeys.datetimes.append(datetime)
    req.journeys.clockwise = clockwise
    req.journeys.realtime_level = utils.realtime_level_to_pbf(journey_parameters.realtime_level)
    req.journeys.max_duration = journey_parameters.max_duration
    req.journeys.max_transfers = journey_parameters.max_transfers
    req.journeys.wheelchair = journey_parameters.wheelchair
    if journey_parameters.max_extra_second_pass:
        req.journeys.max_extra_second_pass = journey_parameters.max_extra_second_pass

    for uri in journey_parameters.forbidden_uris:
        req.journeys.forbidden_uris.append(uri)

    for id in journey_parameters.allowed_id:
        req.journeys.allowed_id.append(id)

    if journey_parameters.direct_path_duration is not None:
        req.journeys.direct_path_duration = journey_parameters.direct_path_duration

    req.journeys.bike_in_pt = bike_in_pt

    if journey_parameters.min_nb_journeys:
        req.journeys.min_nb_journeys = journey_parameters.min_nb_journeys

    if journey_parameters.timeframe:
        req.journeys.timeframe_duration = int(journey_parameters.timeframe)

    if journey_parameters.depth:
        req.journeys.depth = journey_parameters.depth

    if journey_parameters.isochrone_center:
        req.journeys.isochrone_center.place = journey_parameters.isochrone_center
        req.journeys.isochrone_center.access_duration = 0
        req.requested_api = type_pb2.ISOCHRONE

    if journey_parameters.sn_params:
        sn_params_request = req.journeys.streetnetwork_params
        sn_params_request.origin_mode = journey_parameters.sn_params.origin_mode
        sn_params_request.destination_mode = journey_parameters.sn_params.destination_mode
        sn_params_request.walking_speed = journey_parameters.sn_params.walking_speed
        sn_params_request.bike_speed = journey_parameters.sn_params.bike_speed
        sn_params_request.car_speed = journey_parameters.sn_params.car_speed
        sn_params_request.bss_speed = journey_parameters.sn_params.bss_speed
        sn_params_request.car_no_park_speed = journey_parameters.sn_params.car_no_park_speed

    if journey_parameters.current_datetime:
        req._current_datetime = journey_parameters.current_datetime

    if journey_parameters.walking_transfer_penalty:
        req.journeys.walking_transfer_penalty = journey_parameters.walking_transfer_penalty

    if journey_parameters.arrival_transfer_penalty:
        req.journeys.arrival_transfer_penalty = journey_parameters.arrival_transfer_penalty

    return req


def create_graphical_isochrones_request(
    origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
):
    req = create_journeys_request(
        origins,
        destinations,
        datetime,
        clockwise,
        graphical_isochrones_parameters.journeys_parameters,
        bike_in_pt,
    )
    req.requested_api = type_pb2.graphical_isochrone
    req.journeys.max_duration = graphical_isochrones_parameters.journeys_parameters.max_duration
    if graphical_isochrones_parameters.boundary_duration:
        for duration in sorted(graphical_isochrones_parameters.boundary_duration, key=int, reverse=True):
            if graphical_isochrones_parameters.min_duration < duration < req.journeys.max_duration:
                req.isochrone.boundary_duration.append(duration)
    req.isochrone.boundary_duration.insert(0, req.journeys.max_duration)
    req.isochrone.boundary_duration.append(graphical_isochrones_parameters.min_duration)

    # We are consistent with new_default there.
    if req.journeys.origin:
        req.journeys.clockwise = True
    else:
        req.journeys.clockwise = False

    req.isochrone.journeys_request.CopyFrom(req.journeys)
    return req
