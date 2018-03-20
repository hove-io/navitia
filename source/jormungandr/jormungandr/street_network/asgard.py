#  Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
import logging
from jormungandr.exceptions import TechnicalError
from jormungandr.utils import get_pt_object_coord

from jormungandr.street_network.valhalla import Valhalla

from contextlib import contextmanager
import queue
from navitiacommon import response_pb2, request_pb2, type_pb2
from zmq import green as zmq
import six


class Asgard(Valhalla):
    def __init__(self, instance, service_url, asgard_socket, modes=[], id='asgard', timeout=10, api_key=None, **kwargs):
        super(Asgard, self).__init__(instance, service_url, modes, id, timeout, api_key, **kwargs)
        self.asgard_socket = asgard_socket
        self._sockets = queue.Queue()

    def get_street_network_routing_matrix(self, origins, destinations, mode, max_duration, request, **kwargs):
        speed_switcher = {
            "walking": request['walking_speed'],
            "bike": request['bike_speed'],
            "car": request['car_speed'],
            "bss": request['bss_speed'],
        }
        req = request_pb2.Request()
        req.requested_api = type_pb2.street_network_routing_matrix

        for o in origins:
            orig = req.sn_routing_matrix.origins.add()
            orig.place = 'coord:{c.lon}:{c.lat}'.format(c=get_pt_object_coord(o))
            orig.access_duration = 0
        for d in destinations:
            dest = req.sn_routing_matrix.destinations.add()
            dest.place = 'coord:{c.lon}:{c.lat}'.format(c=get_pt_object_coord(d))
            dest.access_duration = 0

        req.sn_routing_matrix.mode = mode
        req.sn_routing_matrix.speed = speed_switcher.get(mode, kwargs.get("walking"))
        req.sn_routing_matrix.max_duration = max_duration

        res = self._call_asgard(req)
        #TODO handle car park
        if res.HasField('error'):
            logging.getLogger(__name__).error('routing matrix query error {}'.format(res.error))
            raise TechnicalError('routing matrix fail')
        return res.sn_routing_matrix

    @contextmanager
    def socket(self, context):
        socket = None
        try:
            socket = self._sockets.get(block=False)
        except queue.Empty:
            socket = context.socket(zmq.REQ)
            socket.connect(self.asgard_socket)
        try:
            yield socket
        finally:
            if not socket.closed:
                self._sockets.put(socket)


    def _call_asgard(self, request):
        with self.socket(self.instance.context) as socket:
            socket.send(request.SerializeToString())
            #timeout is in second, we need it on millisecond
            if socket.poll(timeout=self.timeout*1000) > 0:
                pb = socket.recv()
                resp = response_pb2.Response()
                resp.ParseFromString(pb)
                return resp
            else:
                socket.setsockopt(zmq.LINGER, 0)
                socket.close()
                logger = logging.getLogger(__name__)
                logger.error('request on %s failed: %s', self.asgard_socket, six.text_type(request))
                raise TechnicalError('asgard on {} failed'.format(self.asgard_socket))
