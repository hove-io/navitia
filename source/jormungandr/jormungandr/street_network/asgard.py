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
from jormungandr import app
from jormungandr.street_network.kraken import Kraken
from jormungandr.utils import get_pt_object_coord
from jormungandr.street_network.utils import make_speed_switcher

from contextlib import contextmanager
import queue
from navitiacommon import response_pb2
from zmq import green as zmq
import six
import pybreaker


class Asgard(Kraken):
    def __init__(
        self, instance, service_url, asgard_socket, modes=None, id=None, timeout=10, api_key=None, **kwargs
    ):
        super(Asgard, self).__init__(
            instance, service_url, modes or [], id or 'asgard', timeout, api_key, **kwargs
        )
        self.asgard_socket = asgard_socket
        self.timeout = timeout
        self._sockets = queue.Queue()

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_ASGARD_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_ASGARD_TIMEOUT_S'],
        )
        self.logger = logging.getLogger(__name__)

    def status(self):
        return {
            'id': unicode(self.sn_system_id),
            'class': self.__class__.__name__,
            'modes': self.modes,
            'asgard_socket': self.asgard_socket,
        }

    def get_street_network_routing_matrix(self, origins, destinations, mode, max_duration, request, **kwargs):
        speed_switcher = make_speed_switcher(request)

        req = self._create_sn_routing_matrix_request(
            origins, destinations, mode, max_duration, speed_switcher, **kwargs
        )

        res = self._call_asgard(req)
        self._check_for_error_and_raise(res)
        return res.sn_routing_matrix

    def _direct_path(
        self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type
    ):
        req = self._create_direct_path_request(
            mode, pt_object_origin, pt_object_destination, fallback_extremity, request
        )
        return self._call_asgard(req)

    def get_uri_pt_object(self, pt_object):
        return 'coord:{c.lon}:{c.lat}'.format(c=get_pt_object_coord(pt_object))

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
        def _request():
            with self.socket(self.instance.context) as socket:
                socket.send(request.SerializeToString())
                # timeout is in second, we need it on millisecond
                if socket.poll(timeout=self.timeout * 1000) > 0:
                    pb = socket.recv()
                    resp = response_pb2.Response()
                    resp.ParseFromString(pb)
                    return resp
                else:
                    socket.setsockopt(zmq.LINGER, 0)
                    socket.close()
                    self.logger.error('request on %s failed: %s', self.asgard_socket, six.text_type(request))
                    raise TechnicalError('asgard on {} failed'.format(self.asgard_socket))

        try:
            return self.breaker.call(_request)
        except pybreaker.CircuitBreakerError as e:
            self.logger.error('Asgard routing service dead (error: {})'.format(e))
            self.record_external_failure('circuit breaker open')
            raise
        except Exception as e:
            self.logger.exception('Asgard routing error')
            self.record_external_failure(str(e))
            raise
