#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
import requests as requests
import pybreaker
import ujson
import six

import itertools
import sys
from navitiacommon import response_pb2
from jormungandr import app
from jormungandr.exceptions import GeoveloTechnicalError, InvalidArguments, UnableToParse
from jormungandr.street_network.street_network import (
    AbstractStreetNetworkService,
    StreetNetworkPathKey,
    StreetNetworkPathType,
)
from jormungandr.utils import get_pt_object_coord, is_url, decode_polyline, mps_to_kmph
from jormungandr.street_network.utils import add_cycle_lane_length
from jormungandr.ptref import FeedPublisher

from navitiacommon.response_pb2 import StreetInformation

DEFAULT_GEOVELO_FEED_PUBLISHER = {
    'id': 'geovelo',
    'name': 'geovelo',
    'license': 'Private',
    'url': 'https://geovelo.fr/a-propos/cgu/',
}

# Filter matrix points with modes having priority order:
# Train', 'RapidTransit', 'Metro', 'Tramway', Car, Bus
DEFAULT_MODE_WEIGHT = {
    'physical_mode:Train': 1,
    'physical_mode:RapidTransit': 2,
    'physical_mode:Metro': 3,
    'physical_mode:Tramway': 4,
    'physical_mode:Car': 5,
    'physical_mode:Bus': 6,
}


class Geovelo(AbstractStreetNetworkService):
    def __init__(
        self,
        instance,
        service_url,
        modes=[],
        id='geovelo',
        timeout=10,
        api_key=None,
        feed_publisher=DEFAULT_GEOVELO_FEED_PUBLISHER,
        verify=True,
        **kwargs
    ):
        self.instance = instance
        self.sn_system_id = id
        if not is_url(service_url):
            raise ValueError('service_url {} is not a valid url'.format(service_url))
        self.service_url = service_url
        self.api_key = api_key
        self.timeout = timeout
        self.modes = modes
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_GEOVELO_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_GEOVELO_TIMEOUT_S'],
        )
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None
        self.verify = verify
        self.mode_weight = kwargs.get("mode_weight") or DEFAULT_MODE_WEIGHT
        self.mode_weight_keys = set(self.mode_weight.keys())

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

    @classmethod
    def _pt_object_summary_isochrone(cls, pt_object):
        coord = get_pt_object_coord(pt_object)
        return [coord.lat, coord.lon, None]

    @classmethod
    def _make_request_arguments_bike_details(cls, bike_speed_mps):
        bike_speed = mps_to_kmph(bike_speed_mps)

        return {
            'profile': 'MEDIAN',  # can be BEGINNER, EXPERT
            'bikeType': 'TRADITIONAL',  # can be 'BSS'
            'averageSpeed': bike_speed,  # in km/h, BEGINNER sets it to 13
        }

    def sort_by_mode(self, points):
        def key_func(point):
            if len(point.stop_point.physical_modes) == 0:
                return float('inf')
            return min(
                (self.mode_weight.get(mode.uri, float('inf')) for mode in point.stop_point.physical_modes)
            )

        return sorted(points, key=key_func)

    @classmethod
    def _make_request_arguments_isochrone(cls, origins, destinations, bike_speed_mps=3.33):
        origins_coord = [cls._pt_object_summary_isochrone(o) for o in origins]
        destinations_coord = [cls._pt_object_summary_isochrone(o) for o in destinations]
        return {
            'starts': [o for o in origins_coord],
            'ends': [o for o in destinations_coord],
            'bikeDetails': cls._make_request_arguments_bike_details(bike_speed_mps),
            'transportMode': 'BIKE',
        }

    @classmethod
    def _make_request_arguments_direct_path(cls, origin, destination, bike_speed_mps=3.33):
        coord_orig = get_pt_object_coord(origin)
        coord_dest = get_pt_object_coord(destination)
        return {
            'waypoints': [
                {'latitude': coord_orig.lat, 'longitude': coord_orig.lon},
                {'latitude': coord_dest.lat, 'longitude': coord_dest.lon},
            ],
            'transportModes': ['BIKE'],
            'bikeDetails': cls._make_request_arguments_bike_details(bike_speed_mps),
        }

    def _call_geovelo(self, url, method=requests.post, data=None):
        logging.getLogger(__name__).debug('Geovelo routing service , call url : {}'.format(url))
        try:
            return self.breaker.call(
                method,
                url,
                timeout=self.timeout,
                data=data,
                headers={
                    'content-type': 'application/json',
                    'Api-Key': self.api_key,
                    'Accept-Encoding': 'gzip, br',
                },
                verify=self.verify,
            )
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Geovelo routing service unavailable (error: {})'.format(e))
            self.record_external_failure('circuit breaker open')
            raise GeoveloTechnicalError('Geovelo routing service unavailable')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Geovelo routing service unavailable (error: {})'.format(t))
            self.record_external_failure('timeout')
            raise GeoveloTechnicalError('Geovelo routing service unavailable')
        except Exception as e:
            logging.getLogger(__name__).exception('Geovelo routing error')
            self.record_external_failure(str(e))
            raise GeoveloTechnicalError('Geovelo routing has encountered unknown error')

    @staticmethod
    def get_geovelo_tag(geovelo_response):
        if geovelo_response['title'] == 'RECOMMENDED':
            return 'balanced'
        elif geovelo_response['title'] == 'FASTER':
            return 'shortest'
        elif geovelo_response['title'] == 'SAFER':
            return 'comfort'
        elif geovelo_response['title'] == 'BIS':
            return 'comfort'
        else:
            return 'geovelo_other'

    @staticmethod
    def get_geovelo_cycle_path_type(cyclability):
        if cyclability == 1:
            return response_pb2.NoCycleLane
        elif cyclability == 2:
            return response_pb2.SharedCycleWay
        elif cyclability in [3, 4]:
            return response_pb2.DedicatedCycleWay
        else:
            return response_pb2.SeparatedCycleWay

    @classmethod
    def _get_matrix(cls, json_response):
        '''
        build the 1-n response matrix from geovelo table
        each element is ["start", "end", duration] (first being the header)
        as it is ordered, we only consider the third
        '''
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        row = sn_routing_matrix.rows.add()
        # checking header of geovelo's response
        if json_response[0] != ["start_reference", "end_reference", "duration"]:
            logging.getLogger(__name__).error('Geovelo parsing error. Response: {}'.format(json_response))
            raise UnableToParse('Geovelo parsing error. Response: {}'.format(json_response))

        add_ = row.routing_response.add
        for e in itertools.islice(json_response, 1, sys.maxsize):
            duration, routing_status = (e[2], response_pb2.reached) if e[2] else (-1, response_pb2.unknown)
            add_(duration=duration, routing_status=routing_status)

        return sn_routing_matrix

    @classmethod
    def _check_response(cls, response):
        if response.status_code != 200:
            logging.getLogger(__name__).error(
                'Geovelo service unavailable, impossible to query : {}'
                ' with response : {}'.format(response.url, response.text)
            )
            raise GeoveloTechnicalError('Geovelo service unavailable, impossible to query')

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
    ):
        if street_network_mode != "bike":
            logging.getLogger(__name__).error('Geovelo, mode {} not implemented'.format(street_network_mode))
            raise InvalidArguments('Geovelo, mode {} not implemented'.format(street_network_mode))
        if len(origins) != 1 and len(destinations) != 1:
            logging.getLogger(__name__).error(
                'Geovelo, managing only 1-n in connector, requested {}-{}'.format(
                    len(origins), len(destinations)
                )
            )
            raise InvalidArguments(
                'Geovelo, managing only 1-n in connector, requested {}-{}'.format(
                    len(origins), len(destinations)
                )
            )

        data = self._make_request_arguments_isochrone(origins, destinations, request['bike_speed'])
        r = self._call_geovelo(
            '{}/{}'.format(self.service_url, 'api/v2/routes_m2m'), requests.post, ujson.dumps(data)
        )
        self._check_response(r)
        resp_json = ujson.loads(r.text)

        if len(resp_json) - 1 != len(data.get('starts', [])) * len(data.get('ends', [])):
            logging.getLogger(__name__).error('Geovelo nb response != nb requested')
            raise UnableToParse('Geovelo nb response != nb requested')

        return self._get_matrix(resp_json)

    @classmethod
    def _get_response(cls, json_response, pt_object_origin, pt_object_destination, fallback_extremity):
        '''
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        '''

        # map giving the change of direction (degrees) from geovelo indications
        # see: http://developers.geovelo.fr/#/documentation/compute
        map_instructions_direction = {
            'HEAD_ON': 0,
            'GO_STRAIGHT': 0,
            'REACHED_YOUR_DESTINATION': 0,
            'REACH_VIA_LOCATION': 0,
            'ELEVATOR': 0,
            'CROSSING': 0,
            'ENTER_ROUND_ABOUT': 90,
            'LEAVE_ROUND_ABOUT': 90,
            'STAY_ON_ROUND_ABOUT': 0,
            'ENTER_AGAINST_ALLOWED_DIRECTION': 0,
            'LEAVE_AGAINST_ALLOWED_DIRECTION': 0,
            'TURN_SLIGHT_RIGHT': 45,
            'TURN_SLIGHT_LEFT': -45,
            'TURN_RIGHT': 90,
            'TURN_LEFT': -90,
            'TURN_SHARP_RIGHT': 135,
            'TURN_SHARP_LEFT': -135,
            'U-TURN': 180,
        }

        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND

        for geovelo_response in json_response:
            journey = resp.journeys.add()
            journey.tags.append(cls.get_geovelo_tag(geovelo_response))
            journey.duration = int(geovelo_response['duration'])
            datetime, represents_start_fallback = fallback_extremity
            if represents_start_fallback:
                journey.departure_date_time = datetime
                journey.arrival_date_time = datetime + journey.duration
            else:
                journey.departure_date_time = datetime - journey.duration
                journey.arrival_date_time = datetime
            journey.durations.total = journey.duration
            journey.durations.bike = journey.duration

            journey.distances.bike = int(geovelo_response['distances']['total'])

            previous_section_endtime = journey.departure_date_time
            for index, geovelo_section in enumerate(geovelo_response['sections']):
                section = journey.sections.add()
                section.type = response_pb2.STREET_NETWORK

                section.duration = int(geovelo_section['duration'])
                section.begin_date_time = previous_section_endtime
                section.end_date_time = section.begin_date_time + section.duration
                previous_section_endtime = section.end_date_time

                section.id = 'section_{}'.format(index)
                section.length = int(geovelo_section['details']['distances']['total'])

                if index == 0:
                    section.origin.CopyFrom(pt_object_origin)
                if index == len(geovelo_response['sections']) - 1:
                    section.destination.CopyFrom(pt_object_destination)

                section.street_network.duration = section.duration
                section.street_network.length = section.length
                section.street_network.mode = response_pb2.Bike

                speed = section.length / section.duration if section.duration != 0 else 0

                for geovelo_instruction in itertools.islice(
                    geovelo_section['details']['instructions'], 1, sys.maxsize
                ):
                    path_item = section.street_network.path_items.add()
                    path_item.name = geovelo_instruction[1]
                    path_item.length = geovelo_instruction[2]
                    path_item.duration = round(path_item.length / speed) if speed != 0 else 0
                    path_item.direction = map_instructions_direction.get(geovelo_instruction[0], 0)
                    street_info = StreetInformation()
                    street_info.geojson_offset = geovelo_instruction[5]
                    street_info.length = geovelo_instruction[2]
                    street_info.cycle_path_type = cls.get_geovelo_cycle_path_type(geovelo_instruction[4])
                    section.street_network.street_information.append(street_info)

                shape = decode_polyline(geovelo_response['sections'][0]['geometry'])
                for sh in shape:
                    section.street_network.coordinates.add(lon=sh[0], lat=sh[1])

                elevations = geovelo_section.get('details', {}).get('elevations', []) or []
                for geovelo_elevation in itertools.islice(elevations, 1, sys.maxsize):
                    elevation = section.street_network.elevations.add()
                    elevation.distance_from_start = geovelo_elevation[0]
                    elevation.elevation = geovelo_elevation[1]
                    elevation.geojson_index = geovelo_elevation[2]

        if resp:
            resp = add_cycle_lane_length(resp)

        return resp

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
        if mode != "bike":
            logging.getLogger(__name__).error('Geovelo, mode {} not implemented'.format(mode))
            raise InvalidArguments('Geovelo, mode {} not implemented'.format(mode))

        data = self._make_request_arguments_direct_path(
            pt_object_origin, pt_object_destination, request['bike_speed']
        )
        single_result = True
        if (
            direct_path_type == StreetNetworkPathType.DIRECT
            and request['direct_path'] == 'only_with_alternatives'
        ):
            single_result = False
        r = self._call_geovelo(
            '{}/{}'.format(
                self.service_url,
                'api/v2/computedroutes?'
                'instructions=true&'
                'elevations=true&'
                'geometry=true&'
                'single_result={}&'
                'bike_stations=false&'
                'objects_as_ids=true&'.format(single_result),
            ),
            requests.post,
            ujson.dumps(data),
        )
        self._check_response(r)
        resp_json = ujson.loads(r.text)

        if len(resp_json) != 1 and single_result:
            logging.getLogger(__name__).error('Geovelo nb response != nb requested')
            raise UnableToParse('Geovelo nb response != nb requested')

        return self._get_response(resp_json, pt_object_origin, pt_object_destination, fallback_extremity)

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

    def feed_publisher(self):
        return self._feed_publisher

    def filter_places_isochrone(self, places_isochrone):
        ordered_isochrone = self.sort_by_mode(places_isochrone)
        result = []
        for p in ordered_isochrone[:50]:
            if self.mode_weight_keys & set((pm.uri for pm in p.stop_point.physical_modes)):
                result.append(p)
        return result
