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
import queue
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
            t, socket = self._sockets[self].get_nowait()
        except queue.Empty:  # there is no socket available: let's create one
            socket = self._zmq_context.socket(zmq.REQ)
            socket.connect(self._socket_path)
            t = time.time()
        try:
            yield socket
        finally:
            if not socket.closed:
                self._sockets[self].put((t, socket))

    @staticmethod
    def _reaper(sockets, name, ttl):
        while True:
            try:
                now = time.time()
                t, socket = sockets.get_nowait()
                if now - t > ttl:
                    logging.getLogger(__name__).info("closing one socket for %s", name)
                    socket.setsockopt(zmq.LINGER, 0)
                    socket.close()
                else:
                    sockets.put((t, socket))
                    break  # remaining socket are still in "keep alive" state
            except queue.Empty:
                break

    @classmethod
    def _reap_sockets(cls):
        logging.getLogger(__name__).info("reaping sockets")
        for o, sockets in six.iteritems(cls._sockets):
            cls._reaper(sockets, o.name, o.ttl)

    @classmethod
    def uwsgi_reap_sockets(cls, signal):
        cls._reap_sockets()

    @classmethod
    def gevent_reap_sockets(cls):
        while True:
            cls._reap_sockets()
            gevent.idle(-1)
            gevent.sleep(cls._reaper_interval)

    @classmethod
    def init_socket_reaper(cls, config):
        cls._reaper_interval = config['ZMQ_SOCKET_REAPER_INTERVAL']
        # start a greenlet that handle connection closing when idle
        logging.getLogger(__name__).info("spawning a socket reaper with gevent")
        gevent.spawn(cls.gevent_reap_sockets)

        try:
            # Use uwsgi timer if we are running in uwsgi without gevent.
            # When we are using uwsgi without gevent, idle workers won't run the greenlet, it will only
            # be scheduled when waiting for a response of an external service (kraken mostly)
            import uwsgi

            # In gevent mode we stop, no need to add a timer, the greenlet will be scheduled while waiting
            # for incomming request.
            if 'gevent' in uwsgi.opt:
                return

            logging.getLogger(__name__).info("spawning a socket reaper with  uwsgi timer")

            # Register a signal handler for the signal 2 of uwsgi
            # this signal will trigger the socket reaper and can be on the first available worker
            uwsgi.register_signal(2, 'worker', cls.uwsgi_reap_sockets)
            # Add a timer that trigger this signal every reaper_interval second
            uwsgi.add_timer(2, cls._reaper_interval)
        except (ImportError, ValueError):
            # ImportError is triggered if we aren't in uwsgi
            # ValueError is raise if there is no more timer availlable: only 64 timers can be created
            # workers that didn't create a timer can still run the signal handler
            # if uwsgi dispatch the signal to them
            # signal are dispatched randomly to workers (not round robbin :()
            logging.getLogger(__name__).info(
                "No more uwsgi timer available or not running in uwsgi, only gevent will be used"
            )
