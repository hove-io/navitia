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

from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathType
from jormungandr.fallback_modes import FallbackModes
from jormungandr.street_network import utils
from jormungandr.utils import PeriodExtremity
from navitiacommon import type_pb2, response_pb2


class CarWithPark(AbstractStreetNetworkService):
    def __init__(self, instance, walking_service, car_service, **kwargs):
        self._instance = instance
        self._walking_service = walking_service
        self._car_service = car_service

    def _direct(self, object_origin, object_destination, fallback_extremity, request, request_id):

        object_type = type_pb2.POI
        filter = 'poi_type.uri="poi_type:amenity:parking"'
        speed_switcher = utils.make_speed_switcher(request)

        car_parks = self._instance.georef.get_crow_fly(
            object_destination.uri,
            FallbackModes.walking.name,
            request.get('max_walking_duration_to_pt', 1800),
            request['max_nb_crowfly_by_mode'].get(FallbackModes.walking.name),
            object_type,
            filter,
            0,
            request_id,
            **speed_switcher
        )

        park_ride_car_parks = utils.pick_up_park_ride_car_park(car_parks)

        if not park_ride_car_parks:
            return None

        routing_response = (
            self._walking_service.get_street_network_routing_matrix(
                self._instance,
                park_ride_car_parks,
                [object_destination],
                FallbackModes.walking.name,
                request.get('max_walking_duration_to_pt', 1800),
                request,
                request_id,
                **speed_switcher
            )
            .rows[0]
            .routing_response
        )

        if not routing_response or all(
            element.routing_status == response_pb2.unreached for element in routing_response
        ):
            return None

        def key(i, response=routing_response):
            if response[i].routing_status == response_pb2.reached:
                return routing_response[i].duration
            return float('inf')

        best_ind = min(range(len(routing_response)), key=key)
        best_car_park = park_ride_car_parks[best_ind]

        car_direct_path = self._car_service.direct_path_with_fp(
            self._instance,
            FallbackModes.car.name,
            object_origin,
            best_car_park,
            fallback_extremity,
            request,
            StreetNetworkPathType.DIRECT,
            request_id,
        )

        if not car_direct_path.journeys:
            return None

        if fallback_extremity.represents_start:
            walking_extremity = PeriodExtremity(
                datetime=fallback_extremity.datetime
                + car_direct_path.journeys[0].duration
                + request['_car_park_duration'],
                represents_start=True,
            )

        walking_direct_path = self._walking_service.direct_path_with_fp(
            self._instance,
            FallbackModes.walking.name,
            best_car_park,
            object_destination,
            walking_extremity,
            request,
            StreetNetworkPathType.DIRECT,
            request_id,
        )

        if not walking_direct_path.journeys:
            return None
        car_last_section = car_direct_path.journeys[0].sections[-1]
        walking_first_section = walking_direct_path.journeys[0].sections[0]

        car_park_section = car_direct_path.journeys[0].sections.add()
        car_park_section.type = response_pb2.PARK
        car_park_section.origin.CopyFrom(car_last_section.destination)
        car_park_section.destination.CopyFrom(walking_first_section.origin)
        car_park_section.begin_date_time = car_last_section.end_date_time
        car_park_section.end_date_time = car_park_section.begin_date_time + request['_car_park_duration']
        car_park_section.duration = request['_car_park_duration']

        car_direct_path.journeys[0].sections.extend(walking_direct_path.journeys[0].sections)

        car_direct_path.journeys[0].duration += (
            request['_car_park_duration'] + walking_direct_path.journeys[0].duration
        )
        car_direct_path.journeys[0].durations.walking += walking_direct_path.journeys[0].duration

        return car_direct_path

    def status(self):
        pass

    def _direct_path(
        self,
        instance,
        mode,
        object_origin,
        object_destination,
        fallback_extremity,
        request,
        direct_path_type,
        request_id,
    ):
        if direct_path_type == StreetNetworkPathType.DIRECT:
            return self._direct(object_origin, object_destination, fallback_extremity, request, request_id)
        return self._car_service._direct_path(
            instance,
            mode,
            object_origin,
            object_destination,
            fallback_extremity,
            request,
            direct_path_type,
            request_id,
        )

    def get_street_network_routing_matrix(self, *args, **kwargs):
        return self._car_service.get_street_network_routing_matrix(*args, **kwargs)

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        return self._car_service.make_path_key(
            mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity
        )
