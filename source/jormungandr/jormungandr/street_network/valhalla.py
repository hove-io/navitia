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

from __future__ import absolute_import, print_function, unicode_literals, division
from navitiacommon import request_pb2, response_pb2, type_pb2
import logging
import pybreaker
import requests as requests
from jormungandr import app
import json
from flask_restful import abort
from exceptions import NotImplementedError
from jormungandr.exceptions import UnableToParse, TechnicalError
from flask import g


class Valhalla(object):

    def __init__(self, instance, service_url, directions_options, costing_options, api_key=None):
        self.instance = instance
        self.service_url = service_url
        self.directions_options = directions_options
        self.costing_options = costing_options
        self.api_key = api_key
        # kilometres is default units
        if 'units' not in self.directions_options:
            self.directions_options['units'] = 'kilometers'
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_VALHALLA_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S'])

    def __to_metre(self, distance):
        return distance * 1000.0

    def get(self, mode, origin, destination, datetime, clockwise):
        return self.__direct_path(mode, origin, destination, datetime, clockwise)

    def __call_valhalla(self, url):
        logging.getLogger(__name__).debug('Valhalla routing service , call url : {}'.format(url))
        try:
            return self.breaker.call(requests.get, url, timeout=10)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Valhalla routing service dead (error: {})'.format(e))
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Valhalla routing service dead (error: {})'.format(t))
        except:
            logging.getLogger(__name__).exception('Valhalla routing error')
        return None

    def __format_coord(self, coords):
        coord = coords.split(':')
        if len(coord) == 3:
            return {"lat": coord[2], "lon": coord[1], "type": "break"}
        else:
            logging.getLogger(__name__).error('Invalid coord: {}'.format(coords))
            UnableToParse("Unable to parse coords {} ".format(coords))

    def __decode(self, encoded):
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

    def __get_speed(self, valhalla_mode):
        map_speed = {
            "pedestrian": self.instance.walking_speed * 3.6,
            "bicycle": self.instance.bike_speed * 3.6
        }
        return map_speed.get(valhalla_mode)

    def __get_costing_options(self, valhalla_mode):
        costing_options = self.costing_options.get(valhalla_mode, None)
        if not costing_options:
            return None
        if valhalla_mode == 'pedestrian':
            costing_options['walking_speed'] = self.__get_speed(valhalla_mode)
        if valhalla_mode == 'bicycle':
            costing_options['cycling_speed'] = self.__get_speed(valhalla_mode)
        return costing_options

    def __get_response(self, json_resp, datetime, mode):
        map_mode = {
            "walking": response_pb2.Walking,
            "car": response_pb2.Car,
            "bike": response_pb2.Bike
        }
        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND
        locations = json_resp['trip']['locations']

        for leg in json_resp['trip']['legs']:
            shape = self.__decode(leg['shape'])
            journey = resp.journeys.add()
            journey.duration = leg['summary']['time']
            journey.departure_date_time = datetime
            journey.arrival_date_time = datetime + journey.duration
            journey.durations.total = journey.duration

            section = journey.sections.add()
            section.type = response_pb2.STREET_NETWORK

            section.begin_date_time = journey.departure_date_time
            section.end_date_time = journey.arrival_date_time

            section.id = 'section_0'
            section.duration = journey.duration
            section.length = int(self.__to_metre(leg['summary']['length']))

            section.origin.CopyFrom(g.requested_origin)
            section.destination.CopyFrom(g.requested_destination)

            section.street_network.length = self.__to_metre(leg['summary']['length'])
            section.street_network.duration = leg['summary']['time']
            section.street_network.mode = map_mode[mode]
            for maneuver in leg['maneuvers']:
                path_item = section.street_network.path_items.add()
                if 'street_names' in maneuver and len(maneuver['street_names']) > 0:
                    path_item.name = maneuver['street_names'][0]
                path_item.length = self.__to_metre(maneuver['length'])
                path_item.duration = maneuver['time']
                # TODO: calculate direction
                path_item.direction = 0

            for sh in shape:
                coord = section.street_network.coordinates.add()
                coord.lon = sh[0]
                coord.lat = sh[1]

        return resp

    def __format_url(self, mode, origin, destination):
        map_mode = {
            "walking": "pedestrian",
            "car": "auto",
            "bike": "bicycle"
        }
        if mode not in map_mode:
            logging.getLogger(__name__).error('Valhalla, mode {} not implemented'.format(mode))
            raise NotImplementedError()
        valhalla_mode = map_mode.get(mode)
        args = {
            "locations": [self.__format_coord(origin), self.__format_coord(destination)],
            "costing": valhalla_mode,
            "directions_options": self.directions_options
        }
        if valhalla_mode in self.costing_options:
            costing_options = self.__get_costing_options(valhalla_mode)
            if costing_options and len(costing_options) > 0:
                args["costing_options"] = costing_options

        return '{}/route?json={}&api_key={}'.format(self.service_url, json.dumps(args), self.api_key)

    def __direct_path(self, mode, origin, destination, datetime, clockwise):
        url = self.__format_url(mode, origin, destination)
        r = self.__call_valhalla(url)
        if not r:
            raise TechnicalError('impossible to access valhalla service')
        resp_json = r.json()
        if r.status_code != 200:
            logging.getLogger(__name__).error('Valhalla service unavailable, impossible to query : {}'.format(r.url))
            resp = response_pb2.Response()
            resp.status_code = r.status_code
            resp.error.message = resp_json['trip']['status_message']
            return resp
        return self.__get_response(resp_json, datetime, mode)
