# coding=utf-8

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

import zmq
import time
from contextlib import contextmanager
import logging
import six
import flask
from gevent.lock import BoundedSemaphore
from collections import namedtuple
from sortedcontainers import SortedList
from jormungandr.exceptions import DeadSocketException
from datetime import datetime, timedelta
from navitiacommon import response_pb2

zmq_version = [int(n) for n in zmq.zmq_version().split(".")[:2]]

# ZMQ_HANDSHAKE_IVL is available from 4.1.x
SET_ZMQ_HANDSHAKE_IVL = zmq_version[0] > 4 or (zmq_version[0] == 4 and zmq_version[1] >= 2)


class TransientSocket(object):
    """
    With this class, sockets will be shut down and reopened if the TTL run out.
    This is useful especially for services that are hosted on AWS and accesses by Zmq.

    Why?

    Because jormungandr creates sockets via the AWS's autobalancer, when a new instance is popped by auto scaling,
    despite the auto balancer, sockets created previously will still stick to the old instance. We have to close the
    socket and reopen one so that traffic will be lead to new instances.

    """

    # TODO: use dataclass in python > 3.7
    TimedSocket = namedtuple('TimedSocket', ['t', 'socket'])

    # _sockets is a map of TransientSocket vs a sorted list of tuple of created time and tcp sockets
    # the sorted list is arranged in a way that the first element is the most recent one and the last element is oldest
    # one.
    _logger = logging.getLogger(__name__)

    def __init__(self, name, zmq_context, zmq_socket, socket_ttl, *args, **kwargs):
        super(TransientSocket, self).__init__(*args, **kwargs)
        self.name = name
        self._zmq_context = zmq_context
        self._zmq_socket = zmq_socket
        self._ttl = socket_ttl
        self._semaphore = BoundedSemaphore(1)
        self._sockets = SortedList([], key=lambda s: -s.t)

    def make_new_socket(self):
        start = time.time()
        socket = self._zmq_context.socket(zmq.REQ)
        # During the migration to debian 11, the socket was closed by server because of HANDSHAKE_FAILED_NO_DETAIL
        # exception.
        # This is very likely due to the fact that jormungandr(client) and kraken(server) are not using the same version
        # of libzmq.
        # In the recent version (4.3.x), client need to HANDSHAKE with the server the exchange the configuration information
        # of the socket.
        # Search for ZMQ_HANDSHAKE_IVL on this page: http://api.zeromq.org/4-3:zmq-setsockopt

        # When it comes to debian8, on which runs kraken, this option is not existent:
        # https://manpages.debian.org/jessie/libzmq-dev/zmq_setsockopt.3.en.html

        # As to Asgard, no HANDSHAKE_FAILED_NO_DETAIL exception has been encountered since Asgard runs on ubuntu20,
        # which has a recent version of libzmq

        # The workaround:
        # since jormungandr renew the zmq socket at the interval of ZMQ_SOCKET_TTL_SECONDS,
        # we just need to make sure that the socket is renewed before HANDSHAKE_FAILED_NO_DETAIL is raised, hence
        # setsockopt(zmq.HANDSHAKE_IVL, 0)
        if SET_ZMQ_HANDSHAKE_IVL:
            socket.setsockopt(zmq.HANDSHAKE_IVL, 0)

        socket.connect(self._zmq_socket)
        self._logger.debug(
            "it took %s ms to open a socket of %s",
            '%.2e' % ((time.time() - start) * 1000),
            self.name,
        )
        t = time.time()
        return TransientSocket.TimedSocket(t, socket)

    def get_socket(self):

        with self._semaphore:
            if not self._sockets:
                return self.make_new_socket()
            # since _sockets is a sorted list, the first element is always the newest socket.
            newest_timed_socket = self._sockets[0]
            now = time.time()

            if now - newest_timed_socket.t < self._ttl:
                # we find an alive socket! we move the ownership to this greenlet and use it!
                self._sockets.pop(0)
                return newest_timed_socket
            else:
                # copy the sockets to be closed
                sockets_to_be_closed = self._sockets[:]
                self._sockets.clear()
                start = time.time()
                for s in sockets_to_be_closed:
                    self.close_socket(s.socket)
                self._logger.debug(
                    "it took %s ms to close %s sockets of %s",
                    '%.2e' % ((time.time() - start) * 1000),
                    len(sockets_to_be_closed),
                    self.name,
                )
        return self.make_new_socket()

    def call(self, content, timeout, debug_cb=lambda: "", quiet=False):
        timed_socket = self.get_socket()

        try:
            timed_socket.socket.send(content)
            if timed_socket.socket.poll(timeout=timeout * 1000) > 0:
                pb = timed_socket.socket.recv()
                return pb
            else:
                if not quiet:
                    self._logger.error('request on %s failed: %s', self._zmq_socket, debug_cb())
                raise DeadSocketException(self.name, self._zmq_socket)

        except DeadSocketException as e:
            self.close_socket(timed_socket.socket)
            raise e
        except:
            self.close_socket(timed_socket.socket)
            self._logger.exception("")

        finally:
            if not timed_socket.socket.closed:
                now = time.time()
                if now - timed_socket.t >= self._ttl:
                    self.close_socket(timed_socket.socket)
                else:
                    with self._semaphore:
                        self._sockets.add(timed_socket)

    def close_socket(self, socket):
        try:
            socket.setsockopt(zmq.LINGER, 0)
            socket.close()
        except:
            self._logger.exception("")
