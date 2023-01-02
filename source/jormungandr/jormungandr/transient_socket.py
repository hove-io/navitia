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
import gevent
from contextlib import contextmanager
import logging
import six
from typing import Dict
from gevent.lock import BoundedSemaphore
from collections import defaultdict, namedtuple
from sortedcontainers import SortedList
from jormungandr.exceptions import DeadSocketException
import ujson


class NoAliveSockets(Exception):
    pass


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
        self.ttl = socket_ttl
        self.semaphore = BoundedSemaphore(1)
        self._sockets = SortedList([], key=lambda s: -s.t)

    def make_new_socket(self):
        start = time.time()
        socket = self._zmq_context.socket(zmq.REQ)
        socket.connect(self._zmq_socket)
        self._logger.info(
            "it took %s ms to open a socket of %s",
            '%.2e' % ((time.time() - start) * 1000),
            self.name,
        )
        t = time.time()
        return TransientSocket.TimedSocket(t, socket)

    def get_socket(self):
        if not self._sockets:
            return self.make_new_socket()

        newest_timed_socket = self._sockets[0]
        now = time.time()

        if now - newest_timed_socket.t < self.ttl:
            # we find an alive socket! we move the ownership to this greenlet and use it!
            with self.semaphore:
                self._sockets.pop(0)

            return newest_timed_socket
        else:
            sockets_to_be_closed = self._sockets[:]
            with self.semaphore:
                self._sockets.clear()

            for s in sockets_to_be_closed:
                self.close_socket(s.socket)

            return self.make_new_socket()

    @contextmanager
    def socket(self):
        # We don't want to waste time to close sockets in this function since the performance is critical
        # The cleaning job is done in another greenlet.
        timed_socket = self.get_socket()

        try:
            yield timed_socket.socket
        except DeadSocketException as e:
            raise e
        except:
            self._logger.exception("")

        finally:
            if not timed_socket.socket.closed:
                now = time.time()
                if now - timed_socket.t >= self.ttl:
                    self.close_socket(timed_socket.socket)
                else:
                    with self.semaphore:
                        self._sockets.add(timed_socket)

    def close_socket(self, socket):
        try:
            start = time.time()
            socket.setsockopt(zmq.LINGER, 0)
            socket.close()
            self._logger.info(
                "it took %s ms to close a socket of %s",
                '%.2e' % ((time.time() - start) * 1000),
                self.name,
            )
        except:
            self._logger.exception("")
