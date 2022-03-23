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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
import logging
from jormungandr.exceptions import TechnicalError
from jormungandr import app
from jormungandr.transient_socket import TransientSocket
from jormungandr.fallback_modes import FallbackModes
from jormungandr.street_network.kraken import Kraken
from jormungandr.utils import get_pt_object_coord
from jormungandr.street_network.utils import make_speed_switcher, crowfly_distance_between

from navitiacommon import response_pb2, type_pb2
from zmq import green as zmq
import six
from enum import Enum
import pybreaker


# Possible values implemented. Full languages within the doc:
# https://valhalla.readthedocs.io/en/latest/api/turn-by-turn/api-reference/#supported-language-tags
# Be careful, the syntax has to be exact
class Languages(Enum):
    bulgarian = "bg-BG"
    catalan = "ca-ES"
    czech = "cs-CZ"
    danish = "da-DK"
    german = "de-DE"
    greek = "el-GR"
    english_gb = "en-GB"
    english_pirate = "en-US-x-pirate"
    english_us = "en-US"
    spanish = "es-ES"
    estonian = "et-EE"
    finnish = "fi-FI"
    french = "fr-FR"
    hindi = "hi-IN"
    hungarian = "hu-HU"
    italian = "it-IT"
    japanese = "ja-JP"
    bokmal = "nb-NO"
    dutch = "nl-NL"
    polish = "pl-PL"
    portuguese_br = "pt-BR"
    portuguese_pt = "pt-PT"
    romanian = "ro-RO"
    russian = "ru-RU"
    slovak = "sk-SK"
    slovenian = "sl-SI"
    swedish = "sv-SE"
    turkish = "tr-TR"
    ukrainian = "uk-UA"


class Asgard(TransientSocket, Kraken):
    def __init__(
        self,
        instance,
        service_url,
        asgard_socket,
        modes=None,
        id=None,
        timeout=10,
        api_key=None,
        socket_ttl=app.config['ASGARD_ZMQ_SOCKET_TTL_SECONDS'],
        **kwargs
    ):
        super(Asgard, self).__init__(
            name=instance.name,
            zmq_context=instance.context,
            socket_path=asgard_socket,
            socket_ttl=socket_ttl,
            instance=instance,
            service_url=service_url,
            modes=modes or [],
            id=id or 'asgard',
            timeout=timeout,
            api_key=api_key,
            **kwargs
        )
        self.asgard_socket = asgard_socket
        self.timeout = timeout

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_ASGARD_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_ASGARD_TIMEOUT_S'],
        )
        self.socket_ttl = socket_ttl
        self.logger = logging.getLogger(__name__)

    def status(self):
        return {
            'id': six.text_type(self.sn_system_id),
            'class': self.__class__.__name__,
            'modes': self.modes,
            'timeout': self.timeout,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
            'zmq_socket_ttl': self.socket_ttl,
            'language': self.instance.asgard_language,
        }

    def make_location(self, obj):
        coord = get_pt_object_coord(obj)
        return type_pb2.LocationContext(
            place="", access_duration=0, lon=round(coord.lon, 5), lat=round(coord.lat, 5)
        )

    def get_language_parameter(self, request):
        language = request.get('_asgard_language', "english_us")
        language_tag = getattr(Languages, language, Languages.english_us).value
        return language_tag

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, mode, max_duration, request, request_id, **kwargs
    ):
        speed_switcher = make_speed_switcher(request)

        req = self._create_sn_routing_matrix_request(
            origins, destinations, mode, max_duration, speed_switcher, request, **kwargs
        )

        # asgard doesn't know what to do with 'car_no_park'...
        if mode == FallbackModes.car_no_park.name:
            req.sn_routing_matrix.mode = FallbackModes.car.name

        res = self._call_asgard(req)
        self._check_for_error_and_raise(res)
        return res.sn_routing_matrix

    def _add_cycle_lane_length(self, response):
        def _is_cycle_lane(path):
            if path.HasField(str("cycle_path_type")):
                return path.cycle_path_type != response_pb2.NoCycleLane

            return False

        # We have one journey and several sections in direct path
        for section in response.journeys[0].sections:
            path_items = section.street_network.path_items
            cycle_lane_length = sum((path.length for path in path_items if _is_cycle_lane(path)))
            # Since path.length are doubles and we want an int32 in the proto
            section.cycle_lane_length = int(cycle_lane_length)

        return response

    def _hanlde_car_no_park_modes(self, mode):
        if mode in (FallbackModes.ridesharing.name, FallbackModes.taxi.name, FallbackModes.car_no_park.name):
            return FallbackModes.car.name
        return mode

    def _direct_path(
        self,
        instance,
        mode,
        pt_object_origin,
        pt_object_destination,
        fallback_extremity,
        request,
        direct_path_type,
        request_id,
    ):
        # if the crowfly distance between origin and destination is too large, there is no need to call asgard
        crowfly_distance = crowfly_distance_between(
            get_pt_object_coord(pt_object_origin), get_pt_object_coord(pt_object_destination)
        )

        # if the crowfly distance between origin and destination is
        # bigger than max_{mode}_direct_path_distance don't compute direct_path
        if crowfly_distance > int(request['max_{mode}_direct_path_distance'.format(mode=mode)]):
            return response_pb2.Response()

        if (
            crowfly_distance / float(request['{mode}_speed'.format(mode=mode)])
            > request['max_{mode}_direct_path_duration'.format(mode=mode)]
        ):
            return response_pb2.Response()

        language = self.get_language_parameter(request)

        req = self._create_direct_path_request(
            mode, pt_object_origin, pt_object_destination, fallback_extremity, request, language
        )

        response = self._call_asgard(req)

        # car_no_park is interpreted as car for Asgard, we need to overwrite the streetnetwork mode here
        if mode == "car_no_park":
            try:
                response.journeys[0].sections[0].street_network.mode = response_pb2.CarNoPark
            except AttributeError:
                pass
            except Exception as e:
                raise e

        if response and mode in (FallbackModes.bike.name, FallbackModes.bss.name):
            response = self._add_cycle_lane_length(response)

        return response

    def get_uri_pt_object(self, pt_object):
        return 'coord:{c.lon}:{c.lat}'.format(c=get_pt_object_coord(pt_object))

    def _call_asgard(self, request):
        def _request():
            with self.socket() as socket:
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
