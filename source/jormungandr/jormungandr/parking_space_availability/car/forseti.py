# coding: utf-8

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

from jormungandr.parking_space_availability.car.parking_places import ParkingPlaces
from jormungandr.parking_space_availability.car.common_car_park_provider import CommonCarParkProvider
from jormungandr.street_network.utils import crowfly_distance_between
from jormungandr.utils import Coords
import six

DEFAULT_FORSETI_FEED_PUBLISHER = None


class ForsetiProvider(CommonCarParkProvider):
    """
    class managing calls to Forseti external service providing real-time car parking availability

    """

    def __init__(
        self,
        service_url,
        distance=50,
        timeout=2,
        feed_publisher=DEFAULT_FORSETI_FEED_PUBLISHER,
        **kwargs
    ):
        self.provider_name = "FORSETI"
        self.service_url = service_url
        self.distance = distance
        super(ForsetiProvider, self).__init__(service_url, [], [], timeout, feed_publisher, **kwargs)


    def status(self):
        return {
            'id': six.text_type(self.provider_name),
            'url': self.service_url,
            'class': self.__class__.__name__,
        }

    def support_poi(self, poi):
        return True

    def get_informations(self, poi):
        if not poi.get('properties', {}).get('amenity'):
            return None

        data = self._call_webservice(self.ws_service_template.format(self.dataset))

        if data:
            return self.process_data(data, poi)

    def process_data(self, data, poi):
        poi_coord = Coords(poi.get('coord').get('lat'), poi.get('coord').get('lon'))
        for parking in data.get('parkings', []):
            parking_coord = Coords(parking.get('coord').get('lat'), parking.get('coord').get('lon'))
            distance = crowfly_distance_between(poi_coord, parking_coord)
            if distance  < self.distance:
                return ParkingPlaces(availability=parking.get('availability'))
