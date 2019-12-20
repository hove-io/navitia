#!/usr/bin/env python
# coding=utf-8

# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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

import zmq
import time
import gevent
from gevent import queue
from contextlib import contextmanager
import logging
import six
from typing import Dict


class TransientSocket(object):
    """
    With this class, sockets will be shut down and reopened if the TTL run out.
    This is useful especially for services that are hosted on AWS and accesses by Zmq.

    Why?

    Because jormungandr creates sockets via the AWS's autobalancer, when a new instance is popped by autoscaling,
    despite the autobalancer, sockets created previously will still stick to the old instance. We have to close the
    socket and reopen one so that traffic will be lead to new instances.

    """

    _sockets = dict()  # type: Dict[object, queue.PriorityQueue]
    _sockets_run_out_of_ttl = queue.Queue(maxsize=None)
    _reaper_interval = 10

    def __init__(self, name, zmq_context, socket_path, socket_ttl, *args, **kwargs):
        super(TransientSocket, self).__init__(*args, **kwargs)
        self.name = name
        self._zmq_context = zmq_context
        self._socket_path = socket_path
        self.ttl = socket_ttl
        self._sockets[self] = queue.PriorityQueue(maxsize=None)

    @contextmanager
    def socket(self):
        # We don't want to waste time to close sockets in this function since the performance is critical
        # The cleaning job is done in another greenlet or uwsgi's signal-triggered task.
        try:
            while True:
                t, socket = self._sockets[self].get_nowait()
                now = time.time()
                # why do we need to check the ttl and another queue here?
                # What may happen WITHOUT this queue is that every time the reaper is started, a request may arrive
                # and pick the available socket in the self._sockets[self], so the reaper will consider that there is
                # nothing to clean and quit.
                if now - t > self.ttl:
                    self._sockets_run_out_of_ttl.put((socket, self.name))
                else:
                    break
        except queue.Empty:  # there is no socket available: let's create one
            logging.getLogger(__name__).info("opening one socket for %s", self.name)

            socket = self._zmq_context.socket(zmq.REQ)
            socket.connect(self._socket_path)
            t = time.time()
        try:
            yield socket
        finally:
            if not socket.closed:
                self._sockets[self].put((t, socket))

    @staticmethod
    def close_socket(socket, name):
        logging.getLogger(__name__).info("closing one socket for %s", name)
        socket.setsockopt(zmq.LINGER, 0)
        socket.close()

    @staticmethod
    def _reaper(sockets, name, ttl):
        try:
            while True:
                now = time.time()
                t, socket = sockets.get_nowait()
                if now - t > ttl:
                    TransientSocket.close_socket(socket, name)
                else:
                    sockets.put((t, socket))
                    return  # remaining socket are still in "keep alive" state
        except queue.Empty:
            pass

    @classmethod
    def _reap_sockets(cls):
        logging.getLogger(__name__).info("reaping sockets")
        for o, sockets in six.iteritems(cls._sockets):
            cls._reaper(sockets, o.name, o.ttl)

        try:
            while True:
                socket, name = cls._sockets_run_out_of_ttl.get_nowait()
                TransientSocket.close_socket(socket, name)
        except queue.Empty:
            return

    @classmethod
    def gevent_reap_sockets(cls):
        while True:
            cls._reap_sockets()
            gevent.idle(-1)
            gevent.sleep(cls._reaper_interval)

    @classmethod
    def init_socket_reaper(cls, config):
        cls._reaper_interval = config['ASGARD_ZMQ_SOCKET_REAPER_INTERVAL']
        # start a greenlet that handle connection closing when idle
        logging.getLogger(__name__).info("spawning a socket reaper with gevent")
        gevent.spawn(cls.gevent_reap_sockets)
