# encoding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
import jmespath
from collections import namedtuple

from jormungandr.parking_space_availability.car.common_car_park_provider import CommonCarParkProvider
from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces

DEFAULT_DIVIA_FEED_PUBLISHER = None

SearchPattern = namedtuple('SearchPattern', ['id_park', 'available', 'total'])


def divia_maker(search_patterns):
    class _DiviaProvider(CommonCarParkProvider):
        # search patterns are different depending on divia's dataset
        id_park = None
        available = None
        total = None

        def __init__(
            self, url, operators, dataset, timeout=1, feed_publisher=DEFAULT_DIVIA_FEED_PUBLISHER, **kwargs
        ):
            self.provider_name = 'DIVIA'

            super(_DiviaProvider, self).__init__(url, operators, dataset, timeout, feed_publisher, **kwargs)

        def process_data(self, data, poi):
            park = jmespath.search(
                'records[?to_number(fields.{})==`{}`]|[0]'.format(self.id_park, poi['properties']['ref']), data
            )
            if park:
                available = jmespath.search('fields.{}'.format(self.available), park)
                nb_places = jmespath.search('fields.{}'.format(self.total), park)
                if available is not None and nb_places is not None and nb_places >= available:
                    occupied = nb_places - available
                else:
                    occupied = None
                return ParkingPlaces(available, occupied, None, None)

    _DiviaProvider.id_park = search_patterns.id_park
    _DiviaProvider.available = search_patterns.available
    _DiviaProvider.total = search_patterns.total

    return _DiviaProvider


DiviaProvider = divia_maker(
    SearchPattern(id_park='numero_parking', available='nombre_places_libres', total='nombre_places')
)


DiviaPRParkProvider = divia_maker(
    SearchPattern(id_park='numero_parc', available='nb_places_libres', total='nombre_places')
)
