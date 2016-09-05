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
from navitiacommon import response_pb2, type_pb2
import logging
import pybreaker
import requests as requests
from jormungandr import app
import json
from flask_restful import abort
from jormungandr.exceptions import UnableToParse, TechnicalError, InvalidArguments
from flask import g
from jormungandr.utils import is_url, kilometers_to_metres
from copy import deepcopy


class Valhalla(object):

    def __init__(self, instance, url, timeout=10, api_key=None, **kwargs):
        self.instance = instance
        if not is_url(url):
            raise ValueError('service_url is invalid, you give {}'.format(url))
        self.service_url = url
        self.api_key = api_key
        self.timeout = timeout
        self.costing_options = kwargs.get('costing_options', None)
        # kilometres is default units
        self.directions_options = {
            'units': 'kilometers'
        }
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_VALHALLA_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S'])

    def _call_valhalla(self, url):
        logging.getLogger(__name__).debug('Valhalla routing service , call url : {}'.format(url))
        try:
            return self.breaker.call(requests.get, url, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Valhalla routing service dead (error: {})'.format(e))
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Valhalla routing service dead (error: {})'.format(t))
        except:
            logging.getLogger(__name__).exception('Valhalla routing error')
        return None

    def _format_coord(self, pt_object):
        if not isinstance(pt_object, type_pb2.PtObject):
            logging.getLogger(__name__).error('Invalid pt_object')
            raise InvalidArguments('Invalid pt_object')
        map_coord = {
            type_pb2.STOP_POINT: pt_object.stop_point.coord,
            type_pb2.STOP_AREA: pt_object.stop_area.coord,
            type_pb2.ADDRESS: pt_object.address.coord,
            type_pb2.ADMINISTRATIVE_REGION: pt_object.administrative_region.coord
        }
        coord = map_coord.get(pt_object.embedded_type, None)
        if not coord:
            logging.getLogger(__name__).error('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
            raise UnableToParse('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
        return {"lat": coord.lat, "lon": coord.lon, "type": "break"}

    def _decode(self, encoded):
        # See: https://mapzen.com/documentation/mobility/decoding/#python
        inv = 1.0 / 1e6
        decoded = []
        previous = [0, 0]
        i = 0
        #for each byte
        while i < len(encoded):
            #for each coord (lat, lon)
            ll = [0, 0]
            for j in [0, 1]:
                shift = 0
                byte = 0x20
                #keep decoding bytes until you have this coord
                while byte >= 0x20:
                    byte = ord(encoded[i]) - 63
                    i += 1
                    ll[j] |= (byte & 0x1f) << shift
                    shift += 5
                #get the final value adding the previous offset and remember it for the next
                ll[j] = previous[j] + (~(ll[j] >> 1) if ll[j] & 1 else (ll[j] >> 1))
                previous[j] = ll[j]
            #scale by the precision and chop off long coords also flip the positions so
            # #its the far more standard lon,lat instead of lat,lon
            decoded.append([float('%.6f' % (ll[1] * inv)), float('%.6f' % (ll[0] * inv))])
            #hand back the list of coordinates
        return decoded

    def _get_speed(self, valhalla_mode):
        map_speed = {
            "pedestrian": self.instance.walking_speed * 3.6,
            "bicycle": self.instance.bike_speed * 3.6
        }
        return map_speed.get(valhalla_mode)

    def _get_costing_options(self, valhalla_mode):
        costing_options = deepcopy(self.costing_options)
        if valhalla_mode not in ['pedestrian', 'bicycle']:
            return costing_options
        if not costing_options:
            costing_options = {}
        if valhalla_mode == 'pedestrian':
            if 'pedestrian' not in costing_options:
                costing_options['pedestrian'] = {}
            costing_options['pedestrian']['walking_speed'] = self._get_speed(valhalla_mode)
        if valhalla_mode == 'bicycle':
            if 'bicycle' not in costing_options:
                costing_options['bicycle'] = {}
            costing_options['bicycle']['cycling_speed'] = self._get_speed(valhalla_mode)
        return costing_options

    def _get_response(self, json_resp, mode, pt_object_origin, pt_object_destination, datetime):
        map_mode = {
            "walking": response_pb2.Walking,
            "car": response_pb2.Car,
            "bike": response_pb2.Bike
        }
        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND

        for leg in json_resp['trip']['legs']:
            journey = resp.journeys.add()
            journey.duration = leg['summary']['time']
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
            journey.durations.total = journey.duration

            if mode == 'walking':
                journey.durations.walking = journey.duration

            section = journey.sections.add()
            section.type = response_pb2.STREET_NETWORK

            section.begin_date_time = journey.departure_date_time
            section.end_date_time = journey.arrival_date_time

            section.id = 'section_0'
            section.duration = journey.duration
            section.length = int(kilometers_to_metres(leg['summary']['length']))

            section.origin.CopyFrom(pt_object_origin)
            section.destination.CopyFrom(pt_object_destination)

            section.street_network.length = kilometers_to_metres(leg['summary']['length'])
            section.street_network.duration = leg['summary']['time']
            section.street_network.mode = map_mode[mode]
            for maneuver in leg['maneuvers']:
                path_item = section.street_network.path_items.add()
                if 'street_names' in maneuver and len(maneuver['street_names']) > 0:
                    path_item.name = maneuver['street_names'][0]
                path_item.length = kilometers_to_metres(maneuver['length'])
                path_item.duration = maneuver['time']
                # TODO: calculate direction
                path_item.direction = 0

            shape = self._decode(leg['shape'])
            for sh in shape:
                coord = section.street_network.coordinates.add()
                coord.lon = sh[0]
                coord.lat = sh[1]

        return resp

    def _format_url(self, mode, pt_object_origin, pt_object_destination):
        map_mode = {
            "walking": "pedestrian",
            "car": "auto",
            "bike": "bicycle"
        }
        if mode not in map_mode:
            logging.getLogger(__name__).error('Valhalla, mode {} not implemented'.format(mode))
            raise InvalidArguments('Valhalla, mode {} not implemented'.format(mode))
        valhalla_mode = map_mode.get(mode)
        args = {
            "locations": [self._format_coord(pt_object_origin), self._format_coord(pt_object_destination)],
            "costing": valhalla_mode,
            "directions_options": self.directions_options
        }

        costing_options = self._get_costing_options(valhalla_mode)
        if costing_options and len(costing_options) > 0:
            args["costing_options"] = costing_options

        return '{}/route?json={}&api_key={}'.format(self.service_url, json.dumps(args), self.api_key)

    def direct_path(self, mode, pt_object_origin, pt_object_destination, datetime, clockwise):
        url = self._format_url(mode, pt_object_origin, pt_object_destination)
        r = self._call_valhalla(url)
        if r == None:
            raise TechnicalError('impossible to access valhalla service')
        if r.status_code != 200:
            logging.getLogger(__name__).error('Valhalla service unavailable, impossible to query : {}'.format(r.url))
            resp = response_pb2.Response()
            resp.status_code = r.status_code
            resp.error.message = 'Valhalla service unavailable, impossible to query : {}'.format(r.url)
            return resp
        resp_json = r.json()
        return self._get_response(resp_json, mode, pt_object_origin, pt_object_destination, datetime)
