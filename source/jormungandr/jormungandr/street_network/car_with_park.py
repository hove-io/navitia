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

from jormungandr.street_network.street_network import (
    AbstractStreetNetworkService,
    StreetNetworkPathKey,
    StreetNetworkPathType,
)
from jormungandr.fallback_modes import FallbackModes
from jormungandr.street_network import utils
from navitiacommon import type_pb2


class CarWithPark(AbstractStreetNetworkService):
    def __init__(self, instance, **kwargs):
        self._instance = instance
        self._walking_service = instance.get_street_network(FallbackModes.walking.name)
        self._car_service = instance.get_street_network(FallbackModes.car.name)

    def _direct(self, object_origin, object_destination, fallback_extremity, request, request_id):

        object_type = type_pb2.POI
        filter = 'poi_type.uri="poi_type:amenity:parking"'
        speed_switcher = utils.make_speed_switcher(request)

        car_parks = self._instance.georef.get_crow_fly(
            object_destination.uri,
            FallbackModes.car.name,
            request.get('max_walking_duration_to_pt', 1800),
            10,
            object_type,
            filter,
            0,
            request_id,
            **speed_switcher
        )

        park_ride_car_parks = utils.pick_up_park_ride_car_park(car_parks)

        routing_matrix = self._walking_service.self.get_street_network_routing_matrix(
            self._instance,
            park_ride_car_parks,
            object_destination,
            FallbackModes.walking.name,
            request.get('max_walking_duration_to_pt', 1800),
            request,
            request_id,
            **speed_switcher
        )

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

    def get_street_network_routing_matrix(self, *args, **kwargs):
        return self._car_service.get_street_network_routing_matrix(*args, **kwargs)
