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
from navitiacommon import response_pb2


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
