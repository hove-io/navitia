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
from jormungandr.routing.abstract_routing import AbstractRouting
import pybreaker
import requests as requests
from jormungandr import app
import json
from flask_restful import abort
from exceptions import NotImplementedError
from jormungandr.exceptions import UnableToParse, TechnicalError


class Valhalla(AbstractRouting):

    def __init__(self, instance, **kwargs):
        self.instance = instance
        self.args = kwargs
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_VALHALLA_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_VALHALLA_TIMEOUT_S'])
        self.convert_distances = {'kilometers': 1000.0, 'miles': 1609.34}

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
            logging.getLogger(__name__).error('Invalide coord: {}'.format(coords))
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

    def __get_response(self, json_resp, datetime, mode):
        map_mode = {
            "walking": response_pb2.Walking,
            "car": response_pb2.Car,
            "bicycle": response_pb2.Bike
        }
        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND
        locations = json_resp['trip']['locations']
        coefficient = self.convert_distances.get(json_resp['trip']['units'])
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
            section.length = int(leg['summary']['length'] * coefficient)
            section.origin.name = leg['maneuvers'][0]['street_names'][0]
            section.origin.uri = '{};{}'.format(locations[0]['lon'], locations[0]['lat'])
            section.origin.embedded_type = type_pb2.ADDRESS
            section.origin.address.coord.lon = locations[0]['lon']
            section.origin.address.coord.lat = locations[0]['lat']
            section.origin.address.label = section.destination.name

            section.destination.name = leg['maneuvers'][len(leg['maneuvers'])-2]['street_names'][0]
            section.destination.uri = '{};{}'.format(locations[1]['lon'], locations[1]['lat'])
            section.destination.embedded_type = type_pb2.ADDRESS
            section.destination.address.coord.lon = locations[1]['lon']
            section.destination.address.coord.lat = locations[1]['lat']
            section.destination.address.label = section.destination.name

            section.street_network.length = leg['summary']['length'] * coefficient
            section.street_network.duration = leg['summary']['time']
            section.street_network.mode = map_mode[mode]
            for maneuver in leg['maneuvers']:
                path_item = section.street_network.path_items.add()
                if 'street_names' in maneuver and len(maneuver['street_names']) > 0:
                    path_item.name = maneuver['street_names'][0]
                path_item.length = maneuver['length'] * coefficient
                path_item.duration = maneuver['time']

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
        url = {
            "locations": [self.__format_coord(origin), self.__format_coord(destination)],
            "costing": map_mode.get(mode),
            "directions_options": self.args.get('service_args')
        }
        return '{}/route?json={}'.format(self.args.get('service_url'), json.dumps(url))

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
