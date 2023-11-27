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
from __future__ import absolute_import, print_function, unicode_literals, division
import logging
from enum import Enum
import pybreaker
import six
import requests as requests
import ujson
import json
from shapely.geometry import Point, Polygon


from jormungandr import utils
from jormungandr import app
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathKey
from jormungandr.ptref import FeedPublisher
from jormungandr.exceptions import AndyamoTechnicalError, InvalidArguments, UnableToParse
from jormungandr.utils import get_pt_object_coord, mps_to_kmph, decode_polyline, kilometers_to_meters
from navitiacommon import response_pb2

DEFAULT_ANDYAMO_FEED_PUBLISHER = {
    'id': 'andyamo',
    'name': 'andyamo',
    'license': 'Private',
    'url': 'https://www.andyamo.fr',
}


class Andyamo(AbstractStreetNetworkService):
    def __init__(
        self,
        instance,
        service_url,
        service_backup,
        zone,
        token="",
        modes=None,
        id='andyamo',
        timeout=10,
        feed_publisher=DEFAULT_ANDYAMO_FEED_PUBLISHER,
        **kwargs
    ):
        self.instance = instance
        self.sn_system_id = id
        self.token = token
        self.zone = zone
        self.polygon_zone = None
        if zone != "":
            self.polygon_zone = Polygon(zone)

        if not service_backup:
            raise ValueError('service_backup {} is not define cant forward to asgard'.format(service_backup))

        service_backup["args"]["instance"] = instance
        if 'service_url' not in service_backup['args']:
            service_backup['args'].update({'service_url': None})

        self.service_backup = utils.create_object(service_backup)

        if not service_url:
            raise ValueError('service_url {} is not a valid andyamo url'.format(service_url))
        self.service_url = service_url
        self.log = logging.LoggerAdapter(
            logging.getLogger(__name__), extra={'streetnetwork_id': six.text_type(id)}
        )

        self.headers = {"Content-Type": "application/json", "Accept": "application/json"}
        if self.token:
            self.headers['Authorization'] = 'apiKey {}'.format(self.token)

        self.timeout = timeout
        self.modes = modes if modes else ["walking"]
        self.verify = kwargs.get('verify', True)

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get(
                'circuit_breaker_max_fail', app.config.get('CIRCUIT_BREAKER_MAX_ANDYAMO_FAIL', 4)
            ),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config.get('CIRCUIT_BREAKER_ANDYAMO_TIMEOUT_S', 60)
            ),
        )
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None
        self.check_andyamo_modes(self.modes)

    def are_both_points_inside_zone(self, from_point, to_point):
        from_coords = self._format_coord(from_point)
        to_coords = self._format_coord(to_point)

        from_point_coords = (from_coords["lon"], from_coords["lat"])
        to_point_coords = (to_coords["lon"], to_coords["lat"])

        shapely_from_point = Point(from_point_coords)
        shapely_to_point = Point(to_point_coords)
        return self.polygon_zone.contains(shapely_from_point) and self.polygon_zone.contains(shapely_to_point)

    def not_in_andyamo_zone(self, from_point, to_point):
        return not self.are_both_points_inside_zone(from_point, to_point)

    def mapping_inside_outside(self, from_point, to_point):
        inside_zone = False

        for f_point in from_point:
            for t_point in to_point:
                if self.are_both_points_inside_zone(f_point, t_point):
                    inside_zone = True
                    break
            if inside_zone:
                break

        if inside_zone:
            inside_zone_combinations = [(f, t) for f in from_point for t in to_point]
            outside_zone_combinations = []
        else:
            outside_zone_combinations = [(f, t) for f in from_point for t in to_point]
            inside_zone_combinations = []

        return inside_zone_combinations, outside_zone_combinations

    def get_unic_objects(self, list_object):
        used = set()
        result = []
        for obj in list_object:
            if obj.uri in used:
                continue
            result.append(obj)
            used.add(obj.uri)
        return result

    def dispatch(self, origins, destinations, wheelchair):
        inside_zone_combinations, outside_zone_combinations = self.mapping_inside_outside(origins, destinations)

        andyamo = {
            'origins': self.get_unic_objects([pair[0] for pair in inside_zone_combinations]),
            'destinations': self.get_unic_objects([pair[1] for pair in inside_zone_combinations]),
        }

        asgard = {
            'origins': self.get_unic_objects([pair[0] for pair in outside_zone_combinations]),
            'destinations': self.get_unic_objects([pair[1] for pair in outside_zone_combinations]),
        }

        if not wheelchair and len(andyamo['origins']) > 0:
            # reverse in wheelchair
            return {'andyamo': asgard, 'asgard': andyamo}

        return {'andyamo': andyamo, 'asgard': asgard}

    def check_andyamo_modes(self, modes):
        if len(modes) != 1 or "walking" not in modes:
            self.log.error('Andyamo, mode(s) {} not implemented'.format(modes))
            raise InvalidArguments('Andyamo, mode(s) {} not implemented'.format(modes))

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

    def get_wheelchair_parameter(self, request):
        if request is None:
            return False
        return request.get('wheelchair', False)

    @staticmethod
    def _format_coord(pt_object):
        coord = get_pt_object_coord(pt_object)
        return {"lat": coord.lat, "lon": coord.lon}

    @classmethod
    def _make_request_arguments_direct_path(cls, origin, destination, request):
        source = cls._format_coord(origin)
        target = cls._format_coord(destination)

        params = {
            "sourceLatitude": source["lat"],
            "sourceLongitude": source["lon"],
            "targetLatitude": target["lat"],
            "targetLongitude": target["lon"],
        }

        return params

    def matrix_payload(self, origins, destinations, request):
        params = {
            "sources": [self._format_coord(o) for o in origins],
            "targets": [self._format_coord(d) for d in destinations],
        }

        return params

    def post_matrix_request(self, origins, destinations, request):
        post_data = self.matrix_payload(origins, destinations, request)
        response = self._call_andyamo('matrix', post_data)
        return self.check_response_and_get_json(response)

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        return StreetNetworkPathKey(mode, orig_uri, dest_uri, streetnetwork_path_type, None)

    def _create_matrix_response(self, json_response, origins, destinations, max_duration):
        sources_to_targets = json_response.get('sources_to_targets', [])
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        row = sn_routing_matrix.rows.add()
        for st in sources_to_targets:
            duration = int(round(st["time"]))
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
        check_content = (isinstance(resp, dict) for resp in sources_to_targets)
        locations = json_respons.get("locations", {})
        if (
            len_origins != len(locations.get("sources"))
            or len_destinations != len(locations.get("targets"))
            or not all(check_content)
        ):
            self.log.error('Andyamo nb response != nb requested')
            raise UnableToParse('Andyamo nb response != nb requested')

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
    ):
        wheelchair = self.get_wheelchair_parameter(request)
        result = self.dispatch(origins, destinations, wheelchair)
        andyamo = result['andyamo']
        asgard = result['asgard']

        if len(andyamo["origins"]) == 0:
            asgard_output = self.service_backup._get_street_network_routing_matrix(
                instance,
                asgard['origins'],
                asgard['destinations'],
                street_network_mode,
                max_duration,
                request,
                request_id,
                **kwargs
            )
            return asgard_output

        resp_json = self.post_matrix_request(andyamo['origins'], andyamo['destinations'], request)
        self.check_content_response(resp_json, andyamo['origins'], destinations)

        andyamo_output = self._create_matrix_response(
            resp_json, andyamo['origins'], andyamo['destinations'], max_duration
        )

        return andyamo_output

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
        wheelchair = self.get_wheelchair_parameter(request)
        if self.not_in_andyamo_zone(pt_object_origin, pt_object_destination) or not wheelchair:
            return self.service_backup._direct_path(
                instance,
                mode,
                pt_object_origin,
                pt_object_destination,
                fallback_extremity,
                request,
                direct_path_type,
                request_id,
            )

        self.check_andyamo_modes([mode])

        params = self._make_request_arguments_direct_path(pt_object_origin, pt_object_destination, request)
        response = self._call_andyamo('route', params)
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
        andyamo_trip = json_response["trip"]
        journey = resp.journeys.add()
        journey.duration = int(round(andyamo_trip['summary']["time"]))
        datetime, represents_start_fallback = fallback_extremity
        if represents_start_fallback:
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
        else:
            journey.departure_date_time = datetime - journey.duration
            journey.arrival_date_time = datetime
        journey.durations.total = journey.duration
        journey.durations.walking = journey.duration

        journey.distances.walking = kilometers_to_meters(andyamo_trip['summary']["length"])

        previous_section_endtime = journey.departure_date_time
        for index, andyamo_leg in enumerate(andyamo_trip['legs']):
            section = journey.sections.add()
            section.type = response_pb2.STREET_NETWORK
            section.duration = int(round(andyamo_leg["summary"]['time']))
            section.begin_date_time = previous_section_endtime
            section.end_date_time = section.begin_date_time + section.duration
            previous_section_endtime = section.end_date_time

            section.id = 'section_{}'.format(index)
            section.length = kilometers_to_meters(andyamo_leg["summary"]['length'])

            section.street_network.duration = section.duration
            section.street_network.length = section.length
            section.street_network.mode = response_pb2.Walking
            for andyamo_instruction in andyamo_leg['maneuvers']:
                path_item = section.street_network.path_items.add()
                path.direction = andyamo_instruction.get("type", 0)
                try:
                    # Attempt to cast to an int
                    path.direction = int(path.direction)
                except (ValueError, TypeError):
                    # If casting fails, default to 0
                    path.direction = 0

                if path.direction == 10:  ## means right turn as defined in Valhalla (See link above)
                    path.direction = 90 
                elif path.direction == 15: ## means left turn as defined in Valhalla (See link above)
                    path.direction = -90 

                if andyamo_instruction.get("street_names", []):
                    path_item.name = andyamo_instruction["street_names"][0]
                path_item.instruction = andyamo_instruction["instruction"]
                path_item.length = kilometers_to_meters(andyamo_instruction["length"])
                path_item.duration = int(round(andyamo_instruction["time"]))
            shape = decode_polyline(andyamo_leg['shape'])
            for sh in shape:
                section.street_network.coordinates.add(lon=sh[0], lat=sh[1])
        journey.sections[0].origin.CopyFrom(pt_object_origin)
        journey.sections[-1].destination.CopyFrom(pt_object_destination)

        return resp

    def _call_andyamo(self, api, data):
        self.log.debug('Andyamo routing service , call url : {}'.format(self.service_url))
        try:
            return self.breaker.call(
                requests.post,
                '{url}/{api}'.format(url=self.service_url, api=api),
                data=json.dumps(data),
                timeout=self.timeout,
                headers=self.headers,
                verify=self.verify,
            )
        except pybreaker.CircuitBreakerError as e:
            self.log.error('Andyamo routing service unavailable (error: {})'.format(str(e)))
            self.record_external_failure('circuit breaker open')
            raise AndyamoTechnicalError('Andyamo routing service unavailable, Circuit breaker is open')
        except requests.Timeout as t:
            self.log.error('Andyamo routing service unavailable (error: {})'.format(str(t)))
            self.record_external_failure('timeout')
            raise AndyamoTechnicalError('Andyamo routing service unavailable: Timeout')
        except Exception as e:
            self.log.exception('Andyamo routing error: {}'.format(str(e)))
            self.record_external_failure(str(e))
            raise AndyamoTechnicalError('Andyamo routing has encountered unknown error')

    def check_response_and_get_json(self, response):
        if response.status_code != 200:
            self.log.error('Andyamo service unavailable, response code: {}'.format(response.status_code))
            raise AndyamoTechnicalError('Andyamo service unavailable, impossible to query')
        try:
            return ujson.loads(response.text)
        except Exception as e:
            msg = 'Andyamo unable to parse response, error: {}'.format(str(e))
            self.log.error(msg)
            raise UnableToParse(msg)

    def feed_publisher(self):
        return self._feed_publisher
