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
    _Socket = namedtuple('_Socket', ['t', 'socket'])

    # _sockets is a map of TransientSocket vs a sorted list of tuple of created time and tcp sockets
    # the sorted list is arranged in a way that the first element is the most recent one and the last element is oldest
    # one.
    _sockets = defaultdict(lambda: SortedList([], key=lambda s: -s.t))  # type: Dict[TransientSocket, SortedList]
    _reaper_interval = 5
    _logger = logging.getLogger(__name__)

    def __init__(self, name, zmq_context, zmq_socket, socket_ttl, *args, **kwargs):
        super(TransientSocket, self).__init__(*args, **kwargs)
        self.name = name
        self._zmq_context = zmq_context
        self._zmq_socket = zmq_socket
        self.ttl = socket_ttl

        self.semaphore = BoundedSemaphore(1)

    @contextmanager
    def socket(self):
        # We don't want to waste time to close sockets in this function since the performance is critical
        # The cleaning job is done in another greenlet.
        try:
            with self.semaphore:
                # self._sockets's first element is the most recent one
                t, socket = self._sockets[self][
                    0
                ]  # If self._sockets is empty, a IndexError exception will be raised
                now = time.time()
                if now - t < self.ttl:
                    # we find an alive socket! we move the ownership to this greenlet and use it!
                    self._logger.debug("using an alive socket for %s", self.name)
                    self._sockets[self].pop(0)
                else:
                    raise NoAliveSockets
        except (IndexError, NoAliveSockets):  # there is no socket available: let's create one
            self._logger.debug("opening one socket for %s", self.name)
            socket = self._zmq_context.socket(zmq.REQ)
            socket.connect(self._zmq_socket)
            t = time.time()

        try:
            yield socket
        except DeadSocketException as e:
            raise e
        except:
            self._logger.exception("")

        finally:
            if not socket.closed:
                if time.time() - t >= self.ttl:
                    self.close_socket(socket)
                else:
                    with self.semaphore:
                        self._sockets[self].add(TransientSocket._Socket(t, socket))

    @classmethod
    def close_socket(cls, socket):
        try:
            socket.setsockopt(zmq.LINGER, 0)
            socket.close()
        except:
            cls._logger.exception("")

    @classmethod
    def _reap(cls, o, sockets):
        oldest_creation_time = time.time() - o.ttl
        with o.semaphore:
            i = sockets.bisect_left(TransientSocket._Socket(oldest_creation_time, None))
            sockets_to_be_closed = sockets[i:]  # no worries, it's a copy
            del sockets[i:]
        nb_sockets = len(sockets_to_be_closed)
        for t, socket in sockets_to_be_closed:
            cls._logger.debug("closing socket: {}, life span: {}".format(o.name, time.time() - t))
            cls.close_socket(socket)
        return nb_sockets

    @classmethod
    def _reap_sockets(cls):
        cls._logger.debug("reaping sockets")
        report = defaultdict(int)
        for o, sockets in six.iteritems(cls._sockets):
            nb_sockets = TransientSocket._reap(o, sockets)
            if nb_sockets:
                report[o.name] = report[o.name] + nb_sockets
        if report:
            cls._logger.debug("reaped: {}".format(ujson.dumps(report)))

    @classmethod
    def gevent_reap_sockets(cls):
        while True:
            cls._reap_sockets()
            gevent.idle(-1)
            gevent.sleep(cls._reaper_interval)

    @classmethod
    def init_socket_reaper(cls, config):
        cls._reaper_interval = config.get(str('ZMQ_SOCKET_REAPER_INTERVAL'), 5)
        # start a greenlet that handle connection closing when idle
        cls._logger.info("spawning a socket reaper with gevent")
        gevent.spawn(cls.gevent_reap_sockets)
