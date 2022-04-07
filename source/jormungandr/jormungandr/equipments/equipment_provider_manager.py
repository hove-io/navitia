# Copyright (c) 2001-2019, Hove and/or its affiliates. All rights reserved.
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

from importlib import import_module
from itertools import chain
from navitiacommon import type_pb2
from jormungandr.utils import can_connect_to_database

import logging
import datetime


class EquipmentProviderManager(object):
    def __init__(self, equipment_providers_configuration, providers_getter=None, update_interval=600):
        self.logger = logging.getLogger(__name__)
        self.providers_config = equipment_providers_configuration
        self.providers_keys = []
        self._providers_getter = providers_getter
        self._equipment_providers = {}
        self._equipment_providers_legacy = {}
        self._equipment_providers_last_update = {}
        self._last_update = datetime.datetime(1970, 1, 1)
        self._update_interval = update_interval

    def init_providers(self, providers_keys):
        """
        Create equipment providers only if defined in the Jormungandr instance and not already created
        :param providers_keys: list of providers defined in the instance
        """
        self.providers_keys = providers_keys

        # Init legacy providers from config file
        for provider in self.providers_config:
            key = provider['key']
            if key in self.providers_keys and key not in dict(
                self._equipment_providers, **self._equipment_providers_legacy
            ):
                self._equipment_providers_legacy[key] = self._init_class(provider['class'], provider['args'])
            else:
                self.logger.error('impossible to create provider with key: {}'.format(key))

    def _init_class(self, cls, arguments):
        """
        Create an instance of a provider according to config
        :param cls: provider class in Jormungandr found in config file
        :param arguments: parameters to set in the provider class
        :return: instance of provider
        """
        try:
            if '.' not in cls:
                self.logger.warning('impossible to build, wrongly formated class: {}'.format(cls))

            module_path, name = cls.rsplit('.', 1)
            module = import_module(module_path)
            attr = getattr(module, name)
            return attr(**arguments)
        except ImportError:
            self.logger.warning('impossible to build, cannot find class: {}'.format(cls))

    def _update_provider(self, provider):
        self.logger.info('updating/adding {} equipment provider'.format(provider.id))
        try:
            self._equipment_providers[provider.id] = self._init_class(provider.klass, provider.args)
            self._equipment_providers_last_update[provider.id] = provider.last_update()
        except Exception:
            self.logger.exception('impossible to initialize equipments provider')

        # If the provider added in db is also defined in legacy, delete it.
        self._equipment_providers_legacy.pop(provider.id, None)

    def update_config(self):
        """
        Update list of equipment providers from db
        """
        if (
            self._last_update + datetime.timedelta(seconds=self._update_interval) > datetime.datetime.utcnow()
            or not self._providers_getter
        ):
            return

        # If database is not accessible we update the value of self._last_update and exit
        if not can_connect_to_database():
            self.logger.debug('Database is not accessible')
            self._last_update = datetime.datetime.utcnow()
            return

        self.logger.debug('Updating equipment providers from db')
        self._last_update = datetime.datetime.utcnow()

        try:
            providers = self._providers_getter()
        except Exception as e:
            self.logger.exception('No access to table equipments_provider (error: {})'.format(e))
            # database is not accessible, so let's use the values already present in self._equipment_providers and
            # self._equipment_providers_legacy
            return

        if not providers:
            self.logger.debug('No providers/All providers disabled in db')
            self._equipment_providers = {}
            self._equipment_providers_last_update = {}

            return

        for provider in providers:
            if (
                provider.id not in self._equipment_providers_last_update
                or provider.last_update() > self._equipment_providers_last_update[provider.id]
            ):
                self._update_provider(provider)

    def manage_equipments_for_journeys(self, response):
        """
        Call equipment details provider to update response
        :param response: the pb response received
        :return: response: the pb journeys response updated with equipment details
        """

        def get_from_to_stop_points_of_journeys(journeys):
            """
            :param journeys: the pb journey response
            :return: generator of stop points that can be 'origin' or 'destination'
            """
            places = chain(*[(s.origin, s.destination) for j in journeys for s in j.sections])
            return (
                place.stop_point
                for place in places
                if place.embedded_type == type_pb2.NavitiaType.Value('STOP_POINT')
                and place.HasField('stop_point')
            )

        stop_points = get_from_to_stop_points_of_journeys(response.journeys)

        for provider in self._get_providers().values():
            provider.get_informations_for_journeys(stop_points)

        return response

    def manage_equipments_for_equipment_reports(self, response):
        """
        Call equipment details provider to update response
        :param response: the pb response received
        :return: response: the pb equipment_reports response updated with equipment details
        """
        stop_area_equipments = (sae for er in response.equipment_reports for sae in er.stop_area_equipments)

        for provider in self._get_providers().values():
            provider.get_informations_for_equipment_reports(stop_area_equipments)

        return response

    def _get_providers(self):
        # Make sure we update the providers list from the database before returning them
        self.update_config()

        return dict(self._equipment_providers, **self._equipment_providers_legacy)

    def status(self):
        providers_status = []
        for provider in self.providers_config:
            providers_status.append(
                {
                    'key': provider['key'],
                    'codes_types': provider['args']['codes_types'],
                    'timeout': provider['args']['timeout'],
                    'fail_max': provider['args']['fail_max'],
                }
            )
        return providers_status
