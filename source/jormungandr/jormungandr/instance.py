# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from contextlib import contextmanager
import Queue
from threading import Lock
import zmq
from navitiacommon import response_pb2
import logging
from .exceptions import DeadSocketException


class Instance(object):

    def __init__(self, context, name):
        self.geom = None
        self._sockets = Queue.Queue()
        self.socket_path = None
        self.script = None
        self.nb_created_socket = 0
        self.lock = Lock()
        self.context = context
        self.name = name
        self.timezone = None  # timezone will be fetched from the kraken


    @contextmanager
    def socket(self, context):
        socket = None
        try:
            socket = self._sockets.get(block=False)
        except Queue.Empty:
            socket = context.socket(zmq.REQ)
            socket.connect(self.socket_path)
            self.lock.acquire()
            self.nb_created_socket += 1
            self.lock.release()
        try:
            yield socket
        finally:
            if not socket.closed:
                self._sockets.put(socket)

    def send_and_receive(self, request, timeout=10000):
        with self.socket(self.context) as socket:
            socket.send(request.SerializeToString())
            if socket.poll(timeout=timeout) > 0:
                pb = socket.recv()
                resp = response_pb2.Response()
                resp.ParseFromString(pb)
                return resp
            else:
                socket.setsockopt(zmq.LINGER, 0)
                socket.close()
                logging.error("La requete : " + request.SerializeToString()
                              + " a echoue " + self.socket_path)
                raise DeadSocketException(self.name, self.socket_path)
