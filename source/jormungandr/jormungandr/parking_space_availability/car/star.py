# encoding: utf-8
# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
import jmespath

from jormungandr.parking_space_availability.car.common_car_park_provider import CommonCarParkProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces

DEFAULT_STAR_FEED_PUBLISHER = None


class StarProvider(CommonCarParkProvider):
    def __init__(self, url, operators, dataset, timeout=1, feed_publisher=DEFAULT_STAR_FEED_PUBLISHER, **kwargs):
        self.provider_name = 'STAR'

        super(StarProvider, self).__init__(url, operators, dataset, timeout, feed_publisher, **kwargs)

    def process_data(self, data, poi):
        park = jmespath.search('records[?fields.idparc==\'{}\']|[0]'.format(poi['properties']['ref']), data)
        if not park:
            return None

        item = jmespath.search('fields.jrdinfosoliste', park)
        if item is None:
            return self.get_old_items(park)
        else:
            return self.get_new_items(park)

    def get_old_items(self, park):
        available = jmespath.search('fields.nombreplacesdisponibles', park)
        occupied = jmespath.search('fields.nombreplacesoccupees', park)
        # Person with reduced mobility
        available_PRM = jmespath.search('fields.nombreplacesdisponiblespmr', park)
        occupied_PRM = jmespath.search('fields.nombreplacesoccupeespmr', park)
        return ParkingPlaces(available, occupied, available_PRM, occupied_PRM)

    def get_new_items(self, park):
        occupied = 0
        occupied_PRM = 0
        occupied_ridesharing = 0
        occupied_electric_vehicle = 0

        available = jmespath.search('fields.jrdinfosoliste', park)
        total_places = jmespath.search('fields.capacitesoliste', park)
        if any(n is not None for n in [available, total_places]):
            occupied = total_places - (available or 0)
        # Person with reduced mobility
        available_PRM = jmespath.search('fields.jrdinfopmr', park)
        total_places_PRM = jmespath.search('fields.capacitepmr', park)
        if any(n is not None for n in [available_PRM, total_places_PRM]):
            occupied_PRM = total_places_PRM - (available_PRM or 0)
        available_ridesharing = jmespath.search('fields.jrdinfocovoiturage', park)
        total_places_ridesharing = jmespath.search('fields.capacitecovoiturage', park)
        if any(n is not None for n in [available_ridesharing, total_places_ridesharing]):
            occupied_ridesharing = total_places_ridesharing - (available_ridesharing or 0)
        available_electric_vehicle = jmespath.search('fields.jrdinfoelectrique', park)
        total_places_electric_vehicle = jmespath.search('fields.capaciteve', park)
        if any(n is not None for n in [available_electric_vehicle, total_places_electric_vehicle]):
            occupied_electric_vehicle = total_places_electric_vehicle - (available_electric_vehicle or 0)
        state = jmespath.search('fields.etatouverture', park)
        return ParkingPlaces(
            available,
            occupied,
            available_PRM,
            occupied_PRM,
            total_places,
            available_ridesharing,
            occupied_ridesharing,
            available_electric_vehicle,
            occupied_electric_vehicle,
            state,
        )
