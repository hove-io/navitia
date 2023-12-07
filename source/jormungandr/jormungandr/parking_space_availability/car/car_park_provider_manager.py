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
from jormungandr.parking_space_availability.abstract_provider_manager import AbstractProviderManager
from jormungandr.utils import can_connect_to_database
import logging
import datetime

POI_TYPE_ID = 'poi_type:amenity:parking'


class CarParkingProviderManager(AbstractProviderManager):
    def __init__(self, car_park_providers_configurations, providers_getter=None, update_interval=60):
        super(CarParkingProviderManager, self).__init__()
        self.car_park_providers = []
        self._db_car_park_providers = {}
        self._db_car_park_providers_last_update = {}
        self._update_interval = update_interval
        self._db_providers_getter = providers_getter
        self._last_update = datetime.datetime(1970, 1, 1)
        for configuration in car_park_providers_configurations:
            arguments = configuration.get('args', {})
            self.car_park_providers.append(self._init_class(configuration['class'], arguments))

    def update_config(self):
        if (
            self._last_update + datetime.timedelta(seconds=self._update_interval) > datetime.datetime.utcnow()
            or not self._db_providers_getter
        ):
            return

        logger = logging.getLogger(__name__)

        # If database is not accessible we update the value of self._last_update and exit
        if not can_connect_to_database():
            logger.debug('Database is not accessible')
            self._last_update = datetime.datetime.utcnow()
            return

        logger.debug('updating car parking providers from database')
        self._last_update = datetime.datetime.utcnow()

        try:
            # car parking provider list from the database (external_service)
            db_providers = self._db_providers_getter()
        except Exception as e:
            logger.exception('No access to table external_service (error: {})'.format(e))
            # database is not accessible, so let's use the values already present in self.car_park_providers and
            # avoid sending query to the database for another update_interval
            self._last_update = datetime.datetime.utcnow()
            return

        if not db_providers:
            logger.debug('No provider active in the database')
            self._db_car_park_providers = {}
            self._db_car_park_providers_last_update = {}
            return

        if not self._need_update(db_providers):
            return

        for provider in db_providers:
            # it's a new car parking provider or it has been updated, we add it
            if (
                provider.id not in self._db_car_park_providers_last_update
                or provider.last_update() > self._db_car_park_providers_last_update[provider.id]
            ):
                self.update_provider(provider)

        # remove deleted car parking providers in the database
        for to_delete in set(self._db_car_park_providers.keys()) - {p.id for p in db_providers}:
            del self._db_car_park_providers[to_delete]
            del self._db_car_park_providers_last_update[to_delete]
            logger.info('deleting par parking provider %s', to_delete)

    def _need_update(self, providers):
        for provider in providers:
            if (
                provider.id not in self._db_car_park_providers_last_update
                or provider.last_update() > self._db_car_park_providers_last_update[provider.id]
            ):
                return True
        return False

    def update_provider(self, provider):
        logger = logging.getLogger(__name__)
        try:
            self._db_car_park_providers[provider.id] = self._init_class(provider.klass, provider.full_args())
            self._db_car_park_providers_last_update[provider.id] = provider.last_update()
        except Exception:
            logger.exception('impossible to initialize car parking provider from the database')

    def _handle_poi(self, item):
        if 'poi_type' in item and item['poi_type']['id'] == POI_TYPE_ID:
            provider = self._find_provider(item)
            if provider:
                item['car_park'] = provider.get_informations(item)
                return provider
        return None

    def _get_providers(self):
        self.update_config()
        return self.car_park_providers + list(self._db_car_park_providers.values())

    def get_providers(self):
        return self._get_providers()

    def exist_provider(self):
        self.update_config()
        return any(self.get_providers())
