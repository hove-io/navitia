# encoding: utf-8

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

import math
from navitiacommon import request_pb2, type_pb2, response_pb2
from jormungandr.fallback_modes import FallbackModes
from jormungandr import app

N_DEG_TO_RAD = 0.01745329238
EARTH_RADIUS_IN_METERS = 6372797.560856


def crowfly_distance_between(start_coord, end_coord):
    lon_arc = (start_coord.lon - end_coord.lon) * N_DEG_TO_RAD
    lon_h = math.sin(lon_arc * 0.5)
    lon_h *= lon_h
    lat_arc = (start_coord.lat - end_coord.lat) * N_DEG_TO_RAD
    lat_h = math.sin(lat_arc * 0.5)
    lat_h *= lat_h
    tmp = math.cos(start_coord.lat * N_DEG_TO_RAD) * math.cos(end_coord.lat * N_DEG_TO_RAD)
    return EARTH_RADIUS_IN_METERS * 2.0 * math.asin(math.sqrt(lat_h + tmp * lon_h))


def get_manhattan_duration(distance, speed):
    return int((distance * math.sqrt(2)) / speed)


def make_speed_switcher(req):
    from jormungandr.fallback_modes import FallbackModes

    return {
        FallbackModes.walking.name: req['walking_speed'],
        FallbackModes.bike.name: req['bike_speed'],
        FallbackModes.car.name: req['car_speed'],
        FallbackModes.car_no_park.name: req['car_no_park_speed'],
        FallbackModes.bss.name: req['bss_speed'],
        FallbackModes.ridesharing.name: req['car_no_park_speed'],
        FallbackModes.taxi.name: req['taxi_speed'],
    }


PARK_RIDE_VALUES = {'yes'}


def pick_up_park_ride_car_park(pt_objects):
    from navitiacommon import type_pb2
    from jormungandr import utils

    def is_poi(pt_object):
        return pt_object.embedded_type == type_pb2.POI

    def is_park_ride(pt_object):
        return any(
            prop.type == 'park_ride' and prop.value.lower() in PARK_RIDE_VALUES
            for prop in pt_object.poi.properties
        )

    park_ride_poi_filter = utils.ComposedFilter().add_filter(is_poi).add_filter(is_park_ride).compose_filters()

    return list(park_ride_poi_filter(pt_objects))


def create_kraken_direct_path_request(
    connector, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, language
):
    req = request_pb2.Request()
    req.requested_api = type_pb2.direct_path

    req.direct_path.origin.CopyFrom(connector.make_location(pt_object_origin, mode))
    req.direct_path.destination.CopyFrom(connector.make_location(pt_object_destination, mode))
    req.direct_path.datetime = fallback_extremity.datetime
    req.direct_path.clockwise = fallback_extremity.represents_start
    req.direct_path.streetnetwork_params.origin_mode = connector.handle_car_no_park_modes(mode)
    req.direct_path.streetnetwork_params.destination_mode = connector.handle_car_no_park_modes(mode)
    req.direct_path.streetnetwork_params.walking_speed = request['walking_speed']
    req.direct_path.streetnetwork_params.max_walking_duration_to_pt = request['max_walking_duration_to_pt']
    req.direct_path.streetnetwork_params.bike_speed = request['bike_speed']
    req.direct_path.streetnetwork_params.max_bike_duration_to_pt = request['max_bike_duration_to_pt']
    req.direct_path.streetnetwork_params.bss_speed = request['bss_speed']
    req.direct_path.streetnetwork_params.max_bss_duration_to_pt = request['max_bss_duration_to_pt']
    req.direct_path.streetnetwork_params.car_speed = request['car_speed']
    req.direct_path.streetnetwork_params.max_car_duration_to_pt = request['max_car_duration_to_pt']
    req.direct_path.streetnetwork_params.language = language
    if mode in (
        FallbackModes.ridesharing.name,
        FallbackModes.taxi.name,
        FallbackModes.car_no_park.name,
        FallbackModes.car.name,
    ):
        req.direct_path.streetnetwork_params.car_no_park_speed = request['{}_speed'.format(mode)]
        req.direct_path.streetnetwork_params.max_car_no_park_duration_to_pt = request[
            'max_{}_duration_to_pt'.format(mode)
        ]

    return req


def create_kraken_matrix_request(
    connector, origins, destinations, street_network_mode, max_duration, speed_switcher, request, **kwargs
):
    req = request_pb2.Request()
    req.requested_api = type_pb2.street_network_routing_matrix

    req.sn_routing_matrix.default_projection_radius = app.config['DEFAULT_ASGARD_PROJECTION_RADIUS']

    req.sn_routing_matrix.origins.extend((connector.make_location(o, street_network_mode) for o in origins))
    req.sn_routing_matrix.destinations.extend(
        (connector.make_location(d, street_network_mode) for d in destinations)
    )

    req.sn_routing_matrix.mode = connector.handle_car_no_park_modes(street_network_mode)
    req.sn_routing_matrix.speed = speed_switcher.get(street_network_mode, kwargs.get("walking"))
    req.sn_routing_matrix.max_duration = max_duration

    req.sn_routing_matrix.streetnetwork_params.origin_mode = connector.handle_car_no_park_modes(
        street_network_mode
    )
    req.sn_routing_matrix.streetnetwork_params.walking_speed = speed_switcher.get(
        "walking", kwargs.get("walking")
    )
    req.sn_routing_matrix.streetnetwork_params.bike_speed = speed_switcher.get("bike", kwargs.get("bike"))
    req.sn_routing_matrix.streetnetwork_params.bss_speed = speed_switcher.get("bss", kwargs.get("bss"))
    req.sn_routing_matrix.streetnetwork_params.car_speed = speed_switcher.get("car", kwargs.get("car"))
    req.sn_routing_matrix.streetnetwork_params.car_no_park_speed = speed_switcher.get(
        "car_no_park", kwargs.get("car_no_park")
    )
    return req


def add_cycle_lane_length(response):
    def _is_cycle_lane(path):
        if path.HasField(str("cycle_path_type")):
            return path.cycle_path_type != response_pb2.NoCycleLane

        return False

    # We have multiple journeys and multiple sections in direct path
    for journey in response.journeys:
        for section in journey.sections:
            # do not add cycle_lane_length for bss_rent/bss_return & walking sections
            if section.type == response_pb2.STREET_NETWORK and section.street_network.mode == response_pb2.Bike:
                cycle_lane_length = sum(
                    (s.length for s in section.street_network.street_information if _is_cycle_lane(s))
                )
                # Since path.length are doubles and we want an int32 in the proto
                section.cycle_lane_length = int(cycle_lane_length)

    return response
