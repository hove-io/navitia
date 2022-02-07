#  Copyright (c) 2001-2020, Canal TP and/or its affiliates. All rights reserved.
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

import datetime

from navitiacommon import response_pb2, default_values
import logging
import pybreaker
import requests as requests
from jormungandr import app
from jormungandr.exceptions import TechnicalError
from jormungandr.utils import get_pt_object_coord, PeriodExtremity, decode_polyline
import flexpolyline as fp
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathKey
from jormungandr.ptref import FeedPublisher
from jormungandr.fallback_modes import FallbackModes as fm
from jormungandr.utils import is_coord, get_lon_lat
from six import text_type
from enum import Enum
import itertools
import json
import time

# this limit is fixed by Here Api. See avoidAreas parameter
# https://developer.here.com/documentation/routing/dev_guide/topics/resource-calculate-route.html
HERE_MAX_LIMIT_AVOID_AREAS = 20

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


# Possible engine type
# https://developer.here.com/documentation/routing/dev_guide/topics/resource-param-type-vehicle-type.html
class EngineType(Enum):
    diesel = "diesel"
    gasoline = "gasoline"
    electric = "electric"
    unknown = "unknown"


# Possible values to active/deactivate realtime traffic
class RealTimeTraffic(Enum):
    enabled = "enabled"
    disabled = "disabled"
    unknown = "unknown"


def _convert_realtime_traffic(flag):
    if flag == True:
        return RealTimeTraffic.enabled
    elif flag == False:
        return RealTimeTraffic.disabled
    else:
        return RealTimeTraffic.unknown


# Possible values to choose a matrix method
# simple_matrix: call the Here matrix API (one call for multiple origins/destinations)
# multi_direct_path: call several Here direct path API step by step (one call for each origin/destination)
class MatrixType(Enum):
    simple_matrix = "simple_matrix"
    multi_direct_path = "multi_direct_path"
    unknown = "unknown"


def _convert_matrix_type(mt):
    if mt == "simple_matrix":
        return MatrixType.simple_matrix
    elif mt == "multi_direct_path":
        return MatrixType.multi_direct_path
    else:
        return MatrixType.unknown


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
    coord_for_matrix = {}
    coord_for_matrix['lat'] = coord.lat
    coord_for_matrix['lng'] = coord.lon
    return coord_for_matrix


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
        matrix_type=MatrixType.simple_matrix.value,
        max_matrix_points=default_values.here_max_matrix_points,
        realtime_traffic=True,
        language="english",
        engine_type="diesel",
        engine_average_consumption=7,
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
        self.matrix_type = self._get_matrix_type(matrix_type)
        self.max_matrix_points = self._get_max_matrix_points(max_matrix_points)
        self.realtime_traffic = self._get_realtime_traffic(realtime_traffic)
        self.language = self._get_language(language.lower())
        self.engine_type = self._get_engine_type(engine_type.lower())
        self.engine_average_consumption = engine_average_consumption
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_HERE_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_HERE_TIMEOUT_S'],
        )

        self.log.debug(
            'Here, load configuration max_matrix_points={} - matrix_type={} - realtime_traffic={} - language={} - engine_type={} - engine_average_consumption={}'.format(
                self.max_matrix_points,
                self.matrix_type.value,
                self.realtime_traffic.value,
                self.language.value,
                self.engine_type,
                self.engine_average_consumption,
            )
        )
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None

    def status(self):
        return {
            'id': text_type(self.sn_system_id),
            'class': self.__class__.__name__,
            'modes': self.modes,
            'timeout': self.timeout,
            'matrix_type': self.matrix_type.value,
            'max_matrix_points': self.max_matrix_points,
            'realtime_traffic': self.realtime_traffic.value,
            'language': self.language.value,
            'engine_type': self.engine_type.value,
            'engine_average_consumption': self.engine_average_consumption,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
        }

    def _call_here(self, url, params={}, mode=requests.get, data={}, headers={}):
        self.log.debug('Here routing service, url: {}'.format(url))
        try:
            r = self.breaker.call(mode, url, timeout=self.timeout, params=params, data=data, headers=headers)
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
    def _read_response(response, origin, destination, mode, fallback_extremity, request):
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

        # co2 emission "kEC"
        co2_emission = 0
        # co2_emission = float(route.get('summary', {}).get('co2Emission', 0)) * 1000
        # journey.co2_emission.unit = 'gEC'
        # journey.co2_emission.value = co2_emission

        # durations
        travel_time = here_section.get('summary', {}).get('duration', 0)
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

        datetime, represents_start_fallback = fallback_extremity
        if represents_start_fallback:
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
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

        section = journey.sections.add()
        section.type = response_pb2.STREET_NETWORK

        section.duration = travel_time
        section.begin_date_time = journey.departure_date_time
        section.end_date_time = section.begin_date_time + section.duration

        # since HERE can give us the base time, we display it.
        # we do not try to do clever stuff (like taking the 'clockwise' into account) here
        section.base_begin_date_time = section.begin_date_time
        section.base_end_date_time = section.base_begin_date_time + travel_time

        section.id = 'section_0'
        section.length = length

        section.co2_emission.unit = 'gEC'
        section.co2_emission.value = co2_emission

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

        # instruction
        i = 0
        for maneuver in here_section.get('actions', []):
            path_item = section.street_network.path_items.add()
            path_item.id = i
            path_item.length = maneuver.get('length', 0)
            path_item.duration = maneuver.get('duration', 0)
            path_item.instruction = maneuver.get('instruction', '')
            path_item.name = ''
            path_item.direction = _convert_direction(maneuver.get('direction', ''))
            offset = maneuver.get('offset', 0)
            if offset < shape_len:
                path_item.instruction_start_coordinate.lat = float(shape[offset][0])
                path_item.instruction_start_coordinate.lon = float(shape[offset][1])
            i = i + 1

        return resp

    def get_exclusion_areas(self, request):
        """
        Retreive and adapt exclusion parameters for Here API.
        See the Here doc for avoidAreas option
        https://developer.here.com/documentation/routing/dev_guide/topics/resource-calculate-matrix.html
        """
        _exclusion_areas = request.get('_here_exclusion_area[]', None)
        if _exclusion_areas == None:
            return None
        else:
            boxes = ""
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
                        # The superior latitude has to be the first coord for Here API
                        # https://developer.here.com/documentation/routing/dev_guide/topics/resource-param-type-bounding-box.html
                        if lat_1 > lat_2:
                            boxes += "{},{};{},{}".format(lat_1, lon_1, lat_2, lon_2)
                        else:
                            boxes += "{},{};{},{}".format(lat_2, lon_2, lat_1, lon_1)
                        if idx < (len(_exclusion_areas) - 1):
                            boxes += "!"
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
            return {"avoidAreas": boxes}

    def _get_language(self, language):
        try:
            return Languages[language]
        except KeyError:
            self.log.error('Here parameters language={} not exist - force to english'.format(language))
            return Languages.english

    def _get_engine_type(self, engine_type):
        try:
            return EngineType[engine_type]
        except KeyError:
            self.log.error('Here parameters engine_type={} not exist - force to diesel'.format(engine_type))
            return EngineType.diesel

    def get_language_parameter(self, request):
        _language = request.get('_here_language', None)
        if _language == None:
            return self.language
        else:
            return self._get_language(_language.lower())

    def _get_realtime_traffic(self, rt_traffic):
        realtime_traffic = _convert_realtime_traffic(rt_traffic)
        if realtime_traffic == RealTimeTraffic.unknown:
            self.log.error(
                'Here parameters _here_realtime_traffic={} is badly formatted (option True/False) - force to enabled'.format(
                    rt_traffic
                )
            )
            return RealTimeTraffic.enabled
        return realtime_traffic

    def get_realtime_traffic_parameter(self, request):
        _realtime_traffic = request.get('_here_realtime_traffic', None)
        if _realtime_traffic == None:
            return self.realtime_traffic
        else:
            return self._get_realtime_traffic(_realtime_traffic)

    def _get_max_matrix_points(self, max_matrix_points):
        if max_matrix_points > default_values.here_max_matrix_points:
            self.log.error(
                'Here confifguration max_matrix_points={} is > 100. Force to 100'.format(max_matrix_points)
            )
            return default_values.here_max_matrix_points
        return max_matrix_points

    def _get_matrix_type(self, mt):
        matrix_type = _convert_matrix_type(mt)
        if matrix_type == MatrixType.unknown:
            self.log.error(
                'Here parameters _here_matrix_type={} is badly formatted (option simple_matrix/multi_direct_path) - force to simple_matrix'.format(
                    mt
                )
            )
            return MatrixType.simple_matrix
        return matrix_type

    def get_matrix_parameters(self, request):
        # max_matrix_ppints
        _max_matrix_points = request.get('_here_max_matrix_points', None)
        if _max_matrix_points == None:
            max_matrix_points = self.max_matrix_points
        else:
            max_matrix_points = self._get_max_matrix_points(_max_matrix_points)

        # matrix_type
        _matrix_type = request.get('_here_matrix_type', None)
        if _matrix_type == None:
            matrix_type = self.matrix_type
        else:
            matrix_type = self._get_matrix_type(_matrix_type)
        return max_matrix_points, matrix_type

    def get_direct_path_params(
        self,
        origin,
        destination,
        mode,
        fallback_extremity,
        request,
        realtime_traffic,
        language,
        engine_type,
        engine_average_consumption,
    ):
        datetime, clockwise = fallback_extremity
        params = {
            'apiKey': self.apiKey,
            'origin': _get_coord(origin),
            'destination': _get_coord(destination),
            'return': 'summary,polyline,instructions,actions',
            'spans': 'dynamicSpeedInfo',
            'transportMode': 'car',
            'lang': '{language}'.format(language=language.value),
            # 'vehicletype': '{engine_type},{engine_average_consumption}'.format(
            # engine_type=engine_type.value, engine_average_consumption=engine_average_consumption
            # ),
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
        realtime_traffic = self.get_realtime_traffic_parameter(request)
        language = self.get_language_parameter(request)

        params = self.get_direct_path_params(
            pt_object_origin,
            pt_object_destination,
            mode,
            fallback_extremity,
            request,
            realtime_traffic,
            language,
            self.engine_type,
            self.engine_average_consumption,
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
            json, pt_object_origin, pt_object_destination, mode, fallback_extremity, request
        )

    def _create_matrix_response(self, json_response, origins, destinations):
        travel_times = json_response.get('matrix', {}).get('travelTimes')

        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        row = sn_routing_matrix.rows.add()
        num_destinations = len(destinations)
        for i_o, _ in enumerate(origins):
            for i_d, _ in enumerate(destinations):
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

    def get_post_matrix_payload(
        self, origins, destinations, mode, max_duration, request, realtime_traffic, max_matrix_points
    ):
        here_origins = []
        for i, o in enumerate(itertools.islice(origins, int(max_matrix_points))):
            here_origins.append(_get_coord_for_matrix(o))

        here_destinations = []
        for i, o in enumerate(itertools.islice(destinations, int(max_matrix_points))):
            here_destinations.append(_get_coord_for_matrix(o))

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

        return json.dumps(payload)

    def _get_sn_routing_matrix_with_simple_matrix(
        self, origins, destinations, mode, max_duration, request, realtime_traffic, max_matrix_points
    ):

        # Post the resquest
        post_data = self.get_post_matrix_payload(
            origins, destinations, mode, max_duration, request, realtime_traffic, max_matrix_points
        )
        params = {'apiKey': self.apiKey}
        headers = {'Content-Type': 'application/json'}
        post_resp = self._call_here(
            self.matrix_service_url, params=params, mode=requests.post, data=post_data, headers=headers
        )
        post_resp.raise_for_status()

        if post_resp.status_code != 202:
            return None

        t = 0
        lapse_time_to_retry = 0.1
        step_time = self.timeout / lapse_time_to_retry
        while t < self.timeout * 1000:
            self.log.debug('Here, try to fetch matrix data with a get')
            matrix_id = post_resp.json().get('matrixId', None)
            if matrix_id == None:
                raise TechnicalError('Here, invalid matrixId inside matrix POST response')
            time.sleep(lapse_time_to_retry)
            get_url = self.matrix_service_url + '/' + str(matrix_id) + '?apiKey=' + str(self.apiKey)
            matrix_resp = requests.request("GET", get_url, headers=headers)
            if matrix_resp.status_code == 200:
                return self._create_matrix_response(matrix_resp.json(), origins, destinations)
            t = t + step_time

        raise TechnicalError('Here, impossible to retreive matrix data, timeout')

    def _get_sn_routing_matrix_with_multi_direct_path(
        self, origins, destinations, mode, max_duration, request, realtime_traffic, max_matrix_points
    ):
        sn_routing_matrix = response_pb2.StreetNetworkRoutingMatrix()
        row = sn_routing_matrix.rows.add()
        language = self.get_language_parameter(request)
        for origin in itertools.islice(origins, int(max_matrix_points)):
            for destination in itertools.islice(destinations, int(max_matrix_points)):
                params = self.get_direct_path_params(
                    origin,
                    destination,
                    mode,
                    PeriodExtremity(datetime=request['datetime'], represents_start=request['clockwise']),
                    request,
                    realtime_traffic,
                    language,
                    self.engine_type,
                    self.engine_average_consumption,
                )
                r = self._call_here(self.routing_service_url, params=params, mode=requests.get)
                r.raise_for_status()
                self._create_matrix_response_with_direct_path(r.json(), row)
        return sn_routing_matrix

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, mode, max_duration, request, request_id, **kwargs
    ):
        realtime_traffic = self.get_realtime_traffic_parameter(request)
        max_matrix_points, matrix_type = self.get_matrix_parameters(request)

        # call tha matrix API (one call for multiple origins/destinations)
        if matrix_type == MatrixType.simple_matrix:
            return self._get_sn_routing_matrix_with_simple_matrix(
                origins, destinations, mode, max_duration, request, realtime_traffic, max_matrix_points
            )
        # call several direct path API step by step (one call for each origin/destination)
        # TODO: add coroutine/future to parallelize computation
        elif matrix_type == MatrixType.multi_direct_path:
            return self._get_sn_routing_matrix_with_multi_direct_path(
                origins, destinations, mode, max_duration, request, realtime_traffic, max_matrix_points
            )
        else:
            self.log.error('Here, invalid matrix_type : {}, impossible to query'.format(self.matrix_type.value))
            raise TechnicalError(
                'Here, invalid matrix_type : {}, impossible to query'.format(self.matrix_type.value)
            )

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
