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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from importlib import import_module
import logging
import datetime


class FreeFloatingProviderManager(object):
    def __init__(self, provider_configuration, providers_getter=None, update_interval=600):
        self.logger = logging.getLogger(__name__)
        self.providers_config = provider_configuration
        self.providers_keys = []
        self._providers_getter = providers_getter
        self._free_floating_providers = {}
        self._free_floating_providers_legacy = {}
        self._free_floating_providers_last_update = {}
        self._last_update = datetime.datetime(1970, 1, 1)
        self._update_interval = update_interval

    def init_providers(self, providers_keys):
        """
        Create free_floating providers only if defined in the Jormungandr instance and not already created
        :param providers_keys: list of providers defined in the instance
        """
        self.providers_keys = providers_keys

        # Init legacy providers from config file
        for provider in self.providers_config:
            key = provider['id']
            self._free_floating_providers_legacy[key] = self._init_class(provider['class'], provider['args'])

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
        self.logger.info('updating/adding {} free-floating provider'.format(provider.id))
        try:
            self._free_floating_providers[provider.id] = self._init_class(provider.klass, provider.args)
            self._free_floating_providers_last_update[provider.id] = provider.last_update()
        except Exception:
            self.logger.exception('impossible to initialize free-floating provider')

        # If the provider added in db is also defined in legacy, delete it.
        self._free_floating_providers_legacy.pop(provider.id, None)

    def update_config(self):
        """
        Update list of free_floating providers from db
        """
        if (
            self._last_update + datetime.timedelta(seconds=self._update_interval) > datetime.datetime.utcnow()
            or not self._providers_getter
        ):
            return

        self.logger.debug('Updating free_floating providers from db')
        self._last_update = datetime.datetime.utcnow()

        providers = []
        try:
            providers = self._providers_getter()
        except Exception:
            self.logger.exception('Failure to retrieve free_floating providers configuration')
        if not providers:
            self.logger.debug('No providers/All providers disabled in db')
            self._free_floating_providers = {}
            self._free_floating_providers_last_update = {}

            return

        for provider in providers:
            if (
                provider.id not in self._free_floating_providers_last_update
                or provider.last_update() > self._free_floating_providers_last_update[provider.id]
            ):
                self._update_provider(provider)

    # Here comes the function to call forseti/free_floating
    def manage_free_floatins(self, arguments):
        """
        Call free-floating provider to call external service (Forseti)
        :param request: the request query received
        :return: response: free_floatings json ?
        """
        for provider in self._get_providers().values():
            resp = provider.get_free_floatings(arguments)
            if resp:
                return resp

    def _get_providers(self):
        # Make sure we update the providers list from the database before returning them
        self.update_config()

        return dict(self._free_floating_providers, **self._free_floating_providers_legacy)

    def status(self):
        providers_status = []
        for provider in self.providers_config:
            providers_status.append(
                {
                    'id': provider['id'],
                    'timeout': provider['args']['timeout'],
                    'fail_max': provider['args']['fail_max'],
                }
            )
        return providers_status
