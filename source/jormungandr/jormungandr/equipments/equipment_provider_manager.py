# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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

import logging


class EquipmentProviderManager(object):
    def __init__(self, equipment_providers_configuration):
        self.logger = logging.getLogger(__name__)
        self.providers_config = equipment_providers_configuration
        self.providers = {}

    def get_providers(self, provider_key):
        """
        :param provider_key: provider set in instance configuration
        :return: True if provider key is defined in Jormun configuration
        """
        if not any([provider['key'] == provider_key for provider in self.providers_config]):
            return None

        provider = self.providers.get(provider_key)
        if not provider:
            provider = self.providers[provider_key] = self._create_provider(self.providers_config[provider_key])

        return provider

    def _create_provider(self, config):
        """
        Create the Equipment Provider class
        :param config: Configuration of the equipment provider found in the config file
        :return: provider
        """

        # TODO: creation of the provider instance will be created in another PR
        return None

