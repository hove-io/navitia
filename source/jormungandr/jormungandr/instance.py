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
from navitiacommon import response_pb2, request_pb2, type_pb2
import logging
from .exceptions import DeadSocketException
from navitiacommon import models
from importlib import import_module
type_to_pttype = {
      "stop_area" : request_pb2.PlaceCodeRequest.StopArea,
      "network" : request_pb2.PlaceCodeRequest.Network,
      "company" : request_pb2.PlaceCodeRequest.Company,
      "line" : request_pb2.PlaceCodeRequest.Line,
      "route" : request_pb2.PlaceCodeRequest.Route,
      "vehicle_journey" : request_pb2.PlaceCodeRequest.VehicleJourney,
      "stop_point" : request_pb2.PlaceCodeRequest.StopPoint,
      "calendar" : request_pb2.PlaceCodeRequest.Calendar
}

class Instance(object):

    def __init__(self, context, name):
        self.geom = None
        self._sockets = Queue.Queue()
        self.socket_path = None
        self._scenario = None
        self.nb_created_socket = 0
        self.lock = Lock()
        self.context = context
        self.name = name
        self.timezone = None  # timezone will be fetched from the kraken
        self.publication_date = -1

    @property
    def scenario(self):
        if not self._scenario:
            instance_db = models.Instance.get_by_name(self.name)
            if instance_db:
                logger.info('loading of scenario {} for instance {}', instance_db.scenario, self.name)
                module = import_module("jormungandr.scenarios.{}".format(instance_db.scenario))
                self._scenario = module.Scenario()
            else:
                logger = logging.getLogger(__name__)
                logger.warn('instance %s not found in db, we use the default script', self.name)
                module = import_module("jormungandr.scenarios.default")
                self._scenario = module.Scenario()
        return self._scenario

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

    def send_and_receive(self, request, timeout=10000, quiet=False):
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
                if not quiet:
                    logging.error("La requete : " + request.SerializeToString() + " a echoue " + self.socket_path)
                raise DeadSocketException(self.name, self.socket_path)

    def has_id(self, id_):
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = id_
        response = self.send_and_receive(req)
        return len(response.places) > 0


    def has_external_code(self, type_, id_):
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_code
        if type_ not in type_to_pttype:
            raise ValueError("Can't call pt_code API with type: {}".format(type_))
        req.place_code.type = type_to_pttype[type_]
        req.place_code.type_code = "external_code"
        req.place_code.code = id_
        response = self.send_and_receive(req)
        print response
        return len(response.places) > 0
