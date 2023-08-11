#  Copyright (c) 2001-2023, Hove and/or its affiliates. All rights reserved.
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

# Possible values implemented. Full languages within the doc:
# https://valhalla.readthedocs.io/en/latest/api/turn-by-turn/api-reference/#supported-language-tags
# Be careful, the syntax has to be exact

from __future__ import absolute_import, print_function, unicode_literals, division
import logging
from enum import Enum
import pybreaker
import six
import requests as requests
import ujson
import json

from jormungandr import app
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathKey
from jormungandr.ptref import FeedPublisher
from jormungandr.exceptions import HandimapTechnicalError, InvalidArguments, UnableToParse
from jormungandr.utils import get_pt_object_coord, mps_to_kmph, decode_polyline, kilometers_to_meters
from navitiacommon import response_pb2

DEFAULT_HANDIMAP_FEED_PUBLISHER = {
    'id': 'handimap',
    'name': 'handimap',
    'license': 'Private',
    'url': 'https://www.handimap.fr',
}

LANGUAGE_TRANSFORMATION_LIST = {"en-US": "en-US", "en-GB": "en-US", "fr-FR": "fr-FR"}


class Handimap(AbstractStreetNetworkService):
    def __init__(
        self,
        instance,
        service_url,
        username="",
        password="",
        modes=None,
        id='handimap',
        timeout=10,
        feed_publisher=DEFAULT_HANDIMAP_FEED_PUBLISHER,
        **kwargs
    ):
        self.instance = instance
        self.sn_system_id = id
        if not service_url:
            raise ValueError('service_url {} is not a valid handimap url'.format(service_url))
        self.service_url = service_url
        self.log = logging.LoggerAdapter(
            logging.getLogger(__name__), extra={'streetnetwork_id': six.text_type(id)}
        )
        self.auth = (username, password)
        self.headers = {"Content-Type": "application/json", "Accept": "application/json"}
        self.timeout = timeout
        self.modes = modes if modes else ["walking"]
        self.language = self._get_language(kwargs.get('language'))
        self.verify = kwargs.get('verify', True)

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get(
                'circuit_breaker_max_fail', app.config.get('CIRCUIT_BREAKER_MAX_HANDIMAP_FAIL', 4)
            ),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config.get('CIRCUIT_BREAKER_HANDIMAP_TIMEOUT_S', 60)
            ),
        )
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None
        self.check_handimap_modes(self.modes)

    def check_handimap_modes(self, modes):
        if len(modes) != 1 or "walking" not in modes:
            self.log.error('Handimap, mode(s) {} not implemented'.format(modes))
            raise InvalidArguments('Handimap, mode(s) {} not implemented'.format(modes))

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
        }

    def _get_language(self, language_value):
        language = LANGUAGE_TRANSFORMATION_LIST.get(language_value)
        if not language:
            self.log.error('Handimap parameter language={} Invalid - fallback to english'.format(language_value))
            language = "en-US"
        return language

    def get_language_parameter(self, request):
        language = request.get('language')
        return self.language if not language else self._get_language(language)

    @staticmethod
    def _make_request_arguments_walking_details(request, language):
        walking_speed_km = mps_to_kmph(request["walking_speed"])
        wheelchair = request.get("wheelchair", request.get("traveler_type") == "wheelchair")
        costing_value = "wheelchair" if wheelchair else "walking"
        costing_options = {
            "walking": {"walking_speed": walking_speed_km},
            "wheelchair": {"travel_speed": walking_speed_km},
        }
        return {
            "costing": costing_value,
            "costing_options": {costing_value: costing_options.get(costing_value)},
            "directions_options": {"units": "kilometers", "language": language},
        }

    @staticmethod
    def _format_coord(pt_object):
        coord = get_pt_object_coord(pt_object)
        return {"lat": coord.lat, "lon": coord.lon}

    @classmethod
    def _make_request_arguments_direct_path(cls, origin, destination, request, language):
        walking_details = cls._make_request_arguments_walking_details(request, language)
        params = {
            'locations': [
                cls._format_coord(origin),
                cls._format_coord(destination),
            ]
        }
        params.update(walking_details)
        return params

    def matrix_payload(self, origins, destinations, request, language):
        walking_details = self._make_request_arguments_walking_details(request, language)

        params = {
            "sources": [self._format_coord(o) for o in origins],
            "targets": [self._format_coord(d) for d in destinations],
        }
        params.update(walking_details)

        return params

    def post_matrix_request(self, origins, destinations, request, language):
        post_data = self.matrix_payload(origins, destinations, request, language)
        response = self._call_handimap(
            'sources_to_targets',
            post_data,
        )
        return self.check_response_and_get_json(response)

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at
        the beginning, the end of journey or a direct path without PT also matters especially for car (to know if we
        park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and it's meaning on the
        fallback period)
        Nota: period_extremity is not taken into consideration so far because we assume that a
        direct path from A to B remains the same even the departure time are different (no realtime)
        """
        return StreetNetworkPathKey(mode, orig_uri, dest_uri, streetnetwork_path_type, None)

    def _create_matrix_response(self, json_response, origins, destinations, max_duration):
        sources_to_targets = json_response.get('sources_to_targets', [])
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        row = sn_routing_matrix.rows.add()
        for i_o in range(len(origins)):
            for i_d in range(len(destinations)):
                sources_to_target = sources_to_targets[i_o][i_d]
                duration = int(round(sources_to_target["time"]))
                routing = row.routing_response.add()
                if duration <= max_duration:
                    routing.duration = duration
                    routing.routing_status = response_pb2.reached
                else:
                    routing.duration = -1
                    routing.routing_status = response_pb2.unreached
        return sn_routing_matrix

    def check_content_response(self, json_respons, origins, destinations):
        len_origins = len(origins)
        len_destinations = len(destinations)
        sources_to_targets = json_respons.get("sources_to_targets", [])
        check_content = (len_destinations == len(resp) for resp in sources_to_targets)
        if len_origins != len(sources_to_targets) or not all(check_content):
            self.log.error('Handimap nb response != nb requested')
            raise UnableToParse('Handimap nb response != nb requested')

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
    ):
        language = self.get_language_parameter(request)
        resp_json = self.post_matrix_request(origins, destinations, request, language)

        self.check_content_response(resp_json, origins, destinations)
        return self._create_matrix_response(resp_json, origins, destinations, max_duration)

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
        self.check_handimap_modes([mode])
        language = self.get_language_parameter(request)

        params = self._make_request_arguments_direct_path(
            pt_object_origin, pt_object_destination, request, language
        )

        response = self._call_handimap('route', params)
        json_response = self.check_response_and_get_json(response)
        return self._get_response(json_response, pt_object_origin, pt_object_destination, fallback_extremity)

    @staticmethod
    def _get_response(json_response, pt_object_origin, pt_object_destination, fallback_extremity):
        '''
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        '''
        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND
        handimap_trip = json_response["trip"]
        journey = resp.journeys.add()
        journey.duration = int(round(handimap_trip['summary']["time"]))
        datetime, represents_start_fallback = fallback_extremity
        if represents_start_fallback:
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
        else:
            journey.departure_date_time = datetime - journey.duration
            journey.arrival_date_time = datetime
        journey.durations.total = journey.duration
        journey.durations.walking = journey.duration

        journey.distances.walking = kilometers_to_meters(handimap_trip['summary']["length"])

        previous_section_endtime = journey.departure_date_time
        for index, handimap_leg in enumerate(handimap_trip['legs']):
            section = journey.sections.add()
            section.type = response_pb2.STREET_NETWORK
            section.duration = int(round(handimap_leg["summary"]['time']))
            section.begin_date_time = previous_section_endtime
            section.end_date_time = section.begin_date_time + section.duration
            previous_section_endtime = section.end_date_time

            section.id = 'section_{}'.format(index)
            section.length = kilometers_to_meters(handimap_leg["summary"]['length'])

            section.street_network.duration = section.duration
            section.street_network.length = section.length
            section.street_network.mode = response_pb2.Walking
            for handimap_instruction in handimap_leg['maneuvers']:
                path_item = section.street_network.path_items.add()
                if handimap_instruction.get("street_names", []):
                    path_item.name = handimap_instruction["street_names"][0]
                path_item.instruction = handimap_instruction["instruction"]
                path_item.length = kilometers_to_meters(handimap_instruction["length"])
                path_item.duration = int(round(handimap_instruction["time"]))
            shape = decode_polyline(handimap_leg['shape'])
            for sh in shape:
                section.street_network.coordinates.add(lon=sh[0], lat=sh[1])
        journey.sections[0].origin.CopyFrom(pt_object_origin)
        journey.sections[-1].destination.CopyFrom(pt_object_destination)

        return resp

    def _call_handimap(self, api, data):
        self.log.debug('Handimap routing service , call url : {}'.format(self.service_url))
        try:
            return self.breaker.call(
                requests.post,
                '{url}/{api}'.format(url=self.service_url, api=api),
                data=json.dumps(data),
                timeout=self.timeout,
                auth=self.auth,
                headers=self.headers,
                verify=self.verify,
            )
        except pybreaker.CircuitBreakerError as e:
            self.log.error('Handimap routing service unavailable (error: {})'.format(str(e)))
            self.record_external_failure('circuit breaker open')
            raise HandimapTechnicalError('Handimap routing service unavailable, Circuit breaker is open')
        except requests.Timeout as t:
            self.log.error('Handimap routing service unavailable (error: {})'.format(str(t)))
            self.record_external_failure('timeout')
            raise HandimapTechnicalError('Handimap routing service unavailable: Timeout')
        except Exception as e:
            self.log.exception('Handimap routing error: {}'.format(str(e)))
            self.record_external_failure(str(e))
            raise HandimapTechnicalError('Handimap routing has encountered unknown error')

    def check_response_and_get_json(self, response):
        if response.status_code != 200:
            self.log.error('Handimap service unavailable, response code: {}'.format(response.status_code))
            raise HandimapTechnicalError('Handimap service unavailable, impossible to query')
        try:
            return ujson.loads(response.text)
        except Exception as e:
            msg = 'Handimap unable to parse response, error: {}'.format(str(e))
            self.log.error(msg)
            raise UnableToParse(msg)

    def feed_publisher(self):
        return self._feed_publisher
