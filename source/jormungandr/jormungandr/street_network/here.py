#  Copyright (c) 2001-2020, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

import datetime

from navitiacommon import response_pb2
import logging
import pybreaker
import requests as requests
from jormungandr import app
from jormungandr.exceptions import TechnicalError
from jormungandr.utils import get_pt_object_coord
import flexpolyline as fp
from jormungandr.street_network.street_network import (
    AbstractStreetNetworkService,
    StreetNetworkPathKey,
    StreetNetworkPathType,
)
from jormungandr.ptref import FeedPublisher
from jormungandr.fallback_modes import FallbackModes as fm
from jormungandr.utils import is_coord, get_lon_lat
from six import text_type
from enum import Enum
from retrying import retry
import itertools
import ujson

# this limit is fixed by Here Api. See avoidAreas parameter
# https://developer.here.com/documentation/routing/dev_guide/topics/resource-calculate-route.html
HERE_MAX_LIMIT_AVOID_AREAS = 20

# time beetween calls to fetch matrix result with GET method
DEFAULT_LAPSE_TIME_MATRIX_TO_RETRY = 0.1  # in secs

# max nb points for matrix request
MAX_MATRIX_POINTS_VALUES = 100

# CO2 coeff
CO2_ESTIMATION_COEFF_1 = 1.35
CO2_ESTIMATION_COEFF_2 = 184

# Possible values implemented. Full languages within the doc:
# https://developer.here.com/documentation/routing/dev_guide/topics/resource-param-type-languages.html#languages
# Be careful, the syntax has to be exact
class Languages(Enum):
    afrikaans = "af"
    arabic = "ar-sa"
    chinese = "zh-cn"
    dutch = "nl-nl"
    english = "en-gb"
    french = "fr-fr"
    german = "de-de"
    hebrew = "he"
    hindi = "hi"
    italian = "it-it"
    japanese = "ja-jp"
    nepali = "ne-np"
    portuguese = "pt-pt"
    russian = "ru-ru"
    spanish = "es-es"


DEFAULT_HERE_FEED_PUBLISHER = {
    'id': 'here',
    'name': 'here',
    'license': 'Private',
    'url': 'https://developer.here.com/terms-and-conditions',
}


def _get_coord(pt_object):
    coord = get_pt_object_coord(pt_object)
    return '{lat},{lon}'.format(lat=coord.lat, lon=coord.lon)


def _get_coord_for_matrix(pt_object):
    coord = get_pt_object_coord(pt_object)
    return {"lat": coord.lat, "lng": coord.lon}


def get_here_mode(mode):
    map_mode = {
        fm.walking.name: 'pedestrian',
        fm.bike.name: 'bicycle',
        fm.car.name: 'car',
        fm.car_no_park.name: 'car',
        fm.taxi.name: 'car',
        fm.ridesharing.name: 'car',
    }
    try:
        return map_mode[mode]
    except KeyError:
        raise TechnicalError('HERE does not handle the mode {}'.format(mode))


def _str_to_dt(timestamp):
    dt = datetime.datetime.utcfromtimestamp(timestamp)
    return dt.strftime("%Y-%m-%dT%H:%M:%SZ")


class Here(AbstractStreetNetworkService):
    def __init__(
        self,
        instance,
        service_base_url,
        modes=[],
        id='here',
        timeout=10,
        apiKey=None,
        max_matrix_points=MAX_MATRIX_POINTS_VALUES,
        lapse_time_matrix_to_retry=DEFAULT_LAPSE_TIME_MATRIX_TO_RETRY,
        language="english",
        feed_publisher=DEFAULT_HERE_FEED_PUBLISHER,
        **kwargs
    ):
        self.instance = instance
        self.sn_system_id = id
        if not service_base_url:
            raise ValueError('service_url {} is not a valid HERE url'.format(service_base_url))
        service_base_url = service_base_url.rstrip('/')
        self.routing_service_url = 'https://{base_url}/routes'.format(base_url=service_base_url)
        self.matrix_service_url = 'https://matrix.{base_url}/matrix'.format(base_url=service_base_url)
        self.log = logging.LoggerAdapter(logging.getLogger(__name__), extra={'streetnetwork_id': text_type(id)})
        self.apiKey = apiKey
        self.modes = modes
        self.timeout = timeout
        self.max_matrix_points = self._get_max_matrix_points(max_matrix_points)
        self.lapse_time_matrix_to_retry = self._get_lapse_time_matrix_to_retry(lapse_time_matrix_to_retry)
        self.language = self._get_language(language.lower())
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_HERE_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_HERE_TIMEOUT_S'],
        )

        self.log.debug(
            'Here, load configuration max_matrix_points={} - timeout={} - lapse_time_matrix_to_retry={} - language={}'.format(
                self.max_matrix_points, self.timeout, self.lapse_time_matrix_to_retry, self.language.value
            )
        )
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None

    def status(self):
        return {
            'id': text_type(self.sn_system_id),
            'class': self.__class__.__name__,
            'modes': self.modes,
            'timeout': self.timeout,
            'max_matrix_points': self.max_matrix_points,
            'lapse_time_matrix_to_retry': self.lapse_time_matrix_to_retry,
            'language': self.language.value,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
        }

    def _call_here(self, url, params={}, http_method=requests.get, data={}, headers={}):
        self.log.debug('Here routing service, url: {}'.format(url))
        try:
            r = self.breaker.call(
                http_method, url, timeout=self.timeout, params=params, data=data, headers=headers
            )
            self.record_call('ok')
            return r
        except pybreaker.CircuitBreakerError as e:
            self.log.error('Here routing service dead (error: {})'.format(e))
            self.record_call('failure', reason='circuit breaker open')
            raise TechnicalError('HERE service not available')
        except requests.Timeout as t:
            self.log.error('Here routing service dead (error: {})'.format(t))
            self.record_call('failure', reason='timeout')
            raise TechnicalError('impossible to access HERE service, timeout reached')
        except Exception as e:
            self.log.exception('Here routing error')
            self.record_call('failure', reason=str(e))
            raise TechnicalError('impossible to access Here service')

    @staticmethod
    def _read_response(response, origin, destination, mode, fallback_extremity, request, direct_path_type):
        resp = response_pb2.Response()
        resp.status_code = 200

        # routes
        routes = response.get('routes', [])
        if not routes:
            resp.response_type = response_pb2.NO_SOLUTION
            return resp
        route = routes[0]

        # sections
        here_sections = route.get('sections', [])
        if not here_sections:
            resp.response_type = response_pb2.NO_SOLUTION
            return resp
        here_section = here_sections[0]

        resp.response_type = response_pb2.ITINERARY_FOUND

        journey = resp.journeys.add()
        # durations
        travel_time = here_section.get('summary', {}).get('duration', 0)
        base_duration = here_section.get('summary', {}).get('baseDuration', 0)
        journey.duration = travel_time
        journey.durations.total = travel_time
        if mode == 'walking':
            journey.durations.walking = travel_time
        elif mode == 'bike':
            journey.durations.bike = travel_time
        elif mode == 'car' or mode == 'car_no_park':
            journey.durations.car = travel_time
        else:
            journey.durations.taxi = travel_time

        datetime, clockwise = fallback_extremity
        if (
            direct_path_type == StreetNetworkPathType.BEGINNING_FALLBACK
            or direct_path_type == StreetNetworkPathType.DIRECT
        ):
            if clockwise == True:
                journey.departure_date_time = datetime
                journey.arrival_date_time = datetime + journey.duration
            else:
                journey.departure_date_time = datetime - journey.duration
                journey.arrival_date_time = datetime
        else:
            journey.departure_date_time = datetime - journey.duration
            journey.arrival_date_time = datetime

        journey.requested_date_time = request['datetime']

        # distances
        length = here_section.get('summary', {}).get('length', 0)
        if mode == 'walking':
            journey.distances.walking = length
        elif mode == 'bike':
            journey.distances.bike = length
        elif mode == 'car' or mode == 'car_no_park':
            journey.distances.car = length
        else:
            journey.distances.taxi = length

        # co2 emission
        journey.co2_emission.unit = 'gEC'
        journey.co2_emission.value = CO2_ESTIMATION_COEFF_1 * length / 1000.0 * CO2_ESTIMATION_COEFF_2

        section = journey.sections.add()
        section.type = response_pb2.STREET_NETWORK

        section.duration = travel_time
        section.begin_date_time = journey.departure_date_time
        section.end_date_time = journey.arrival_date_time

        # base duration impacts the base arrival and departure datetime
        if (
            direct_path_type == StreetNetworkPathType.BEGINNING_FALLBACK
            or direct_path_type == StreetNetworkPathType.DIRECT
        ):
            section.base_begin_date_time = section.begin_date_time + (travel_time - base_duration)
            section.base_end_date_time = section.end_date_time
        else:
            section.base_begin_date_time = section.begin_date_time
            section.base_end_date_time = section.end_date_time - (travel_time - base_duration)

        section.id = 'section_0'
        section.length = length

        section.co2_emission.unit = 'gEC'
        section.co2_emission.value = journey.co2_emission.value

        section.origin.CopyFrom(origin)
        section.destination.CopyFrom(destination)

        section.street_network.length = section.length
        section.street_network.duration = section.duration
        map_mode = {
            fm.walking.name: response_pb2.Walking,
            fm.car.name: response_pb2.Car,
            fm.bike.name: response_pb2.Bike,
            fm.car_no_park.name: response_pb2.CarNoPark,
        }
        section.street_network.mode = map_mode[mode]

        # shape
        shape = fp.decode(here_section.get('polyline', ''))
        shape_len = len(shape)
        for sh in shape:
            coord = section.street_network.coordinates.add()
            coord.lon = sh[1]
            coord.lat = sh[0]

        # handle maneuvers to fill Path field
        def _convert_direction(direction):
            sentence = direction.lower()
            if "right" in sentence:
                return 90
            elif "left" in sentence:
                return -90
            else:
                return 0

        # dynamic speed
        for span in here_section.get('spans', []):
            dynamic_speed = section.street_network.dynamic_speeds.add()
            dynamic_speed.geojson_offset = span.get('offset', 0)
            dynamic_speed_info = span.get('dynamicSpeedInfo', {})
            dynamic_speed.base_speed = int(dynamic_speed_info.get('baseSpeed', 0))
            dynamic_speed.traffic_speed = int(dynamic_speed_info.get('trafficSpeed', 0))

        # instruction
        for idx, maneuver in enumerate(here_section.get('actions', [])):
            path_item = section.street_network.path_items.add()
            path_item.id = idx
            path_item.length = maneuver.get('length', 0)
            path_item.duration = maneuver.get('duration', 0)
            path_item.instruction = maneuver.get('instruction', '')
            path_item.name = ''
            path_item.direction = _convert_direction(maneuver.get('direction', ''))
            offset = maneuver.get('offset', 0)
            if offset < shape_len:
                path_item.instruction_start_coordinate.lat = float(shape[offset][0])
                path_item.instruction_start_coordinate.lon = float(shape[offset][1])

        return resp

    def get_exclusion_areas(self, request):
        """
        Retreive and adapt exclusion parameters for Here API.
        See the Here doc for avoid option
        https://developer.here.com/documentation/matrix-routing-api/api-reference-swagger.html
        """
        _exclusion_areas = request.get('_here_exclusion_area[]', None)
        if _exclusion_areas == None:
            return None
        else:
            boxes = 'bbox:'
            if len(_exclusion_areas) > HERE_MAX_LIMIT_AVOID_AREAS:
                self.log.error(
                    'Here parameters _here_exclusion_area[] is limited to 20 exclusion areas. truncate list !'
                )
            for idx, exclusion_area in enumerate(_exclusion_areas[:HERE_MAX_LIMIT_AVOID_AREAS]):
                if exclusion_area.count('!') == 1:
                    coord_1, coord_2 = exclusion_area.split('!')
                    if is_coord(coord_1) and is_coord(coord_2):
                        lon_1, lat_1 = get_lon_lat(coord_1)
                        lon_2, lat_2 = get_lon_lat(coord_2)
                        # The superior latitude has to be the second coord for Here API
                        if lat_1 < lat_2:
                            boxes += "{},{},{},{}".format(lon_1, lat_1, lon_2, lat_2)
                        else:
                            boxes += "{},{},{},{}".format(lon_2, lat_2, lon_1, lat_1)
                        if idx < (len(_exclusion_areas) - 1):
                            boxes += "|"
                    else:
                        self.log.error(
                            'Here parameters _here_exclusion_area[]={} is badly formated. Exclusion box is skipped'.format(
                                exclusion_area
                            )
                        )
                else:
                    self.log.error(
                        'Here parameters _here_exclusion_area[]={} is badly formated. Exclusion box is skipped'.format(
                            exclusion_area
                        )
                    )
            return {'avoid': boxes}

    def _get_language(self, language):
        try:
            return Languages[language]
        except KeyError:
            self.log.error('Here parameters language={} not exist - force to english'.format(language))
            return Languages.english

    def get_language_parameter(self, request):
        _language = request.get('_here_language', None)
        if _language == None:
            return self.language
        else:
            return self._get_language(_language.lower())

    def _get_max_matrix_points(self, max_matrix_points):
        if max_matrix_points > MAX_MATRIX_POINTS_VALUES:
            self.log.error(
                'Here configuration max_matrix_points={} is > 100. Force to 100'.format(max_matrix_points)
            )
            return MAX_MATRIX_POINTS_VALUES
        return max_matrix_points

    def _get_lapse_time_matrix_to_retry(self, lapse_time_matrix_to_retry):
        if lapse_time_matrix_to_retry > self.timeout:
            self.log.error(
                'Here configuration lapse_time_matrix_to_retry={} is > timeout={}'.format(
                    lapse_time_matrix_to_retry, self.timeout
                )
            )
            return self.timeout
        return lapse_time_matrix_to_retry

    def get_matrix_parameters(self, request):
        # max_matrix_points
        _max_matrix_points = request.get('_here_max_matrix_points', None)
        if _max_matrix_points == None:
            max_matrix_points = self.max_matrix_points
        else:
            max_matrix_points = self._get_max_matrix_points(_max_matrix_points)

        return max_matrix_points

    def get_direct_path_params(self, origin, destination, fallback_extremity, request, language):
        datetime, _ = fallback_extremity
        params = {
            'apiKey': self.apiKey,
            'origin': _get_coord(origin),
            'destination': _get_coord(destination),
            'return': 'summary,polyline,instructions,actions',
            'spans': 'dynamicSpeedInfo',
            'transportMode': 'car',
            'lang': '{language}'.format(language=language.value),
            'departureTime': _str_to_dt(datetime),
        }
        exclusion_areas = self.get_exclusion_areas(request)
        if exclusion_areas != None:
            params.update(exclusion_areas)

        return params

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
        language = self.get_language_parameter(request)

        params = self.get_direct_path_params(
            pt_object_origin, pt_object_destination, fallback_extremity, request, language
        )
        r = self._call_here(self.routing_service_url, params=params)
        if r.status_code != 200:
            self.log.debug(
                'impossible to find a path between {o} and {d}, response ({code}) {r}'.format(
                    o=pt_object_origin, d=pt_object_destination, code=r.status_code, r=r.text
                )
            )
            resp = response_pb2.Response()
            resp.status_code = 200
            resp.response_type = response_pb2.NO_SOLUTION
            return resp

        json = r.json()

        self.log.debug('here response = {}'.format(json))

        return self._read_response(
            json, pt_object_origin, pt_object_destination, mode, fallback_extremity, request, direct_path_type
        )

    def _create_matrix_response(self, json_response, origins, destinations):
        travel_times = json_response.get('matrix', {}).get('travelTimes')

        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        row = sn_routing_matrix.rows.add()
        num_destinations = len(destinations)
        for i_o, _ in enumerate(itertools.islice(origins, int(self.max_matrix_points))):
            for i_d, _ in enumerate(itertools.islice(destinations, int(self.max_matrix_points))):
                routing = row.routing_response.add()
                # the idx algorithm is describe inside the doc
                # https://developer.here.com/documentation/matrix-routing-api/api-reference-swagger.html
                idx = num_destinations * i_o + i_d
                if idx < self.max_matrix_points:
                    routing.duration = travel_times[idx]
                    routing.routing_status = response_pb2.reached
                else:
                    # since we limit the number of points given to HERE, we say that all the others cannot be
                    # reached (and not response_pb2.unkown since we don't want a crow fly section)
                    routing.duration = -1
                    routing.routing_status = response_pb2.unreached

        return sn_routing_matrix

    def _create_matrix_response_with_direct_path(self, json_response, row):
        routing = row.routing_response.add()
        routes = json_response.get('response', {}).get('route', [])
        if not routes:
            return row
        route = routes[0]
        travel_time = route.get('summary', {}).get('travelTime')
        if travel_time is not None:
            routing.duration = travel_time
            routing.routing_status = response_pb2.reached
        else:
            routing.duration = -1
            routing.routing_status = response_pb2.unreached

        return row

    def matrix_payload(self, origins, destinations, request):
        here_origins = []
        for _, o in enumerate(itertools.islice(origins, int(self.max_matrix_points))):
            here_origins.append(_get_coord_for_matrix(o))

        here_destinations = []
        for _, d in enumerate(itertools.islice(destinations, int(self.max_matrix_points))):
            here_destinations.append(_get_coord_for_matrix(d))

        payload = {
            'routingMode': 'fast',
            'regionDefinition': {'type': 'world'},
            'origins': here_origins,
            'destinations': here_destinations,
        }

        exclusion_areas = self.get_exclusion_areas(request)
        if exclusion_areas != None:
            payload.update(exclusion_areas)

        # for the ending fallback matrix (or the beginning of non clockwise query), we do not know the
        # precise departure/arrival (as it depend on the public transport taken)
        datetime = request['datetime']
        payload['departureTime'] = _str_to_dt(datetime)

        return ujson.dumps(payload)

    def post_matrix_request(self, origins, destinations, request):
        post_data = self.matrix_payload(origins, destinations, request)
        params = {'apiKey': self.apiKey}
        headers = {'Content-Type': 'application/json'}
        post_resp = self._call_here(
            self.matrix_service_url, params=params, http_method=requests.post, data=post_data, headers=headers
        )
        post_resp.raise_for_status()

        if post_resp.status_code != 202:
            raise TechnicalError('Here, post return status code != 202')

        return post_resp.json()

    def get_matrix_response(self, origins, destinations, post_resp):

        headers = {'Content-Type': 'application/json'}
        matrix_id = post_resp.get('matrixId', None)
        if matrix_id == None:
            raise TechnicalError('Here, invalid matrixId inside matrix POST response')

        # Here just expose a get to retreive matrix but you have to wait for the result to be available.
        # It is your own waiting loop !

        get_url = self.matrix_service_url + '/' + str(matrix_id) + '?apiKey=' + str(self.apiKey)

        @retry(stop_max_delay=self.timeout * 1000, wait_fixed=self.lapse_time_matrix_to_retry * 1000)
        def _get_matrix_response(get_url):
            matrix_resp = requests.request("GET", get_url, headers=headers)
            if matrix_resp.status_code == 200:
                return self._create_matrix_response(matrix_resp.json(), origins, destinations)
            else:
                raise TechnicalError('Here, impossible to get matrix data. Should retry')

        return _get_matrix_response(get_url)

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, mode, max_duration, request, request_id, **kwargs
    ):
        # post the matrix resquest (Post method)
        post_resp = self.post_matrix_request(origins, destinations, request)

        # fetch matrix response (Get method)
        return self.get_matrix_response(origins, destinations, post_resp)

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at
        the beginning, the end of journey or a direct path without PT also matters especially for car (to know if we
        park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and it's meaning on the
        fallback period)
        Nota: period_extremity is taken into consideration so far because we assume that a
        direct path from A to B change with the realtime
        """
        return StreetNetworkPathKey(mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity)

    def feed_publisher(self):
        return self._feed_publisher
