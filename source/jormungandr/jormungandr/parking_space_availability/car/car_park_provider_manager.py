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
from jormungandr.parking_space_availability.abstract_provider_manager import AbstractProviderManager

POI_TYPE_ID = 'poi_type:amenity:parking'


class CarParkingProviderManager(AbstractProviderManager):
    def __init__(self, car_park_providers_configurations):
        super(CarParkingProviderManager, self).__init__()
        self.car_park_providers = []
        for configuration in car_park_providers_configurations:
            arguments = configuration.get('args', {})
            self.car_park_providers.append(self._init_class(configuration['class'], arguments))

    def _handle_poi(self, item):
        if 'poi_type' in item and item['poi_type']['id'] == POI_TYPE_ID:
            provider = self._find_provider(item)
            if provider:
                item['car_park'] = provider.get_informations(item)
                return provider
        return None

    def _get_providers(self):
        return self.car_park_providers
