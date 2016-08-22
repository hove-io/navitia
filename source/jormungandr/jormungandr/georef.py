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

class Kraken(object):

    def __init__(self, instance):
        self.instance = instance
    def get_stop_points(self, place, mode, max_duration, reverse=False, max_nb_crowfly=5000):
        # we use place_nearby of kraken at the first place to get stop_points around the place, then call the
        # one_to_many(or many_to_one according to the arg "reverse") service to take street network into consideration
        # TODO: reverse is not handled as so far
        places_crowfly = self.get_crow_fly(place, mode, max_duration, max_nb_crowfly)
        destinations = []
        for p in places_crowfly:
            destinations.append(p.uri)
        sn_routing_matrix = self.get_streetnetwork_routing_matrix([place],
                                                                  destinations,
                                                                  mode,
                                                                  max_duration)
        if not sn_routing_matrix.rows[0].duration:
            return {}
        import numpy as np
        durations = np.array(sn_routing_matrix.rows[0].duration)
        valid_duration_idx = np.argwhere((durations > -1) & (durations < max_duration)).flatten()
        return dict(zip([destinations[i] for i in valid_duration_idx],
                        durations[(durations > -1) & (durations < max_duration)].flatten()))

    def place(self, place):

        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = place
        response = self.instance.send_and_receive(req)
        return response.places[0]

    def direct_path(self, mode, origin, destination, datetime, clockwise):
        logger = logging.getLogger(__name__)
        req = request_pb2.Request()
        req.requested_api = type_pb2.direct_path
        req.direct_path.origin.place = origin
        req.direct_path.origin.access_duration = 0
        req.direct_path.destination.place = destination
        req.direct_path.destination.access_duration = 0
        req.direct_path.datetime = datetime
        req.direct_path.clockwise = clockwise
        req.direct_path.streetnetwork_params.origin_mode = mode
        req.direct_path.streetnetwork_params.destination_mode = mode
        req.direct_path.streetnetwork_params.walking_speed = self.instance.walking_speed
        req.direct_path.streetnetwork_params.max_walking_duration_to_pt = self.instance.max_walking_duration_to_pt
        req.direct_path.streetnetwork_params.bike_speed = self.instance.bike_speed
        req.direct_path.streetnetwork_params.max_bike_duration_to_pt = self.instance.max_bike_duration_to_pt
        req.direct_path.streetnetwork_params.bss_speed = self.instance.bss_speed
        req.direct_path.streetnetwork_params.max_bss_duration_to_pt = self.instance.max_bss_duration_to_pt
        req.direct_path.streetnetwork_params.car_speed = self.instance.car_speed
        req.direct_path.streetnetwork_params.max_car_duration_to_pt = self.instance.max_car_duration_to_pt
        return self.instance.send_and_receive(req)

    def get_car_co2_emission_on_crow_fly(self, origin, destination):
        logger = logging.getLogger(__name__)
        req = request_pb2.Request()
        req.requested_api = type_pb2.car_co2_emission
        req.car_co2_emission.origin.place = origin
        req.car_co2_emission.origin.access_duration = 0
        req.car_co2_emission.destination.place = destination
        req.car_co2_emission.destination.access_duration = 0

        response = self.instance.send_and_receive(req)
        if response.error and response.error.id == \
                response_pb2.Error.error_id.Value('no_solution'):
            logger.error("Cannot compute car co2 emission from {} to {}"
                         .format(origin, destination))
            return None
        return response.car_co2_emission

    def get_crow_fly(self, origin, streetnetwork_mode, max_duration, max_nb_crowfly):
        speed_switcher = {
            "walking": self.instance.walking_speed,
            "bike": self.instance.bike_speed,
            "car": self.instance.car_speed,
            "bss": self.instance.bss_speed,
        }
        # Getting stop_ponits or stop_areas using crow fly
        # the distance of crow fly is defined by the mode speed and max_duration
        req = request_pb2.Request()
        req.requested_api = type_pb2.places_nearby
        req.places_nearby.uri = origin

        req.places_nearby.distance = speed_switcher.get(streetnetwork_mode,
                                                        self.instance.walking_speed) * max_duration
        req.places_nearby.depth = 1
        req.places_nearby.count = max_nb_crowfly
        req.places_nearby.start_page = 0
        # we are only interested in public transports
        req.places_nearby.types.append(type_pb2.STOP_POINT)
        return self.instance.send_and_receive(req).places_nearby

    def get_streetnetwork_routing_matrix(self, origins, destinations, streetnetwork_mode, max_duration):
        # TODO: reverse is not handled as so far
        speed_switcher = {
            "walking": self.instance.walking_speed,
            "bike": self.instance.bike_speed,
            "car": self.instance.car_speed,
            "bss": self.instance.bss_speed,
        }
        req = request_pb2.Request()
        req.requested_api = type_pb2.street_network_routing_matrix
        for o in origins:
            orig = req.sn_routing_matrix.origins.add()
            orig.place = o
            orig.access_duration = 0
        for d in destinations:
            dest = req.sn_routing_matrix.destinations.add()
            dest.place = d
            dest.access_duration = 0

        req.sn_routing_matrix.mode = streetnetwork_mode
        req.sn_routing_matrix.speed = speed_switcher.get(streetnetwork_mode, self.instance.walking_speed)
        req.sn_routing_matrix.max_duration = max_duration
        return self.instance.send_and_receive(req).sn_routing_matrix
