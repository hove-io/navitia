# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
from jormungandr.utils import can_connect_to_database


class RealtimeProxyManager(object):
    """
    class managing real-time proxies
    """

    def __init__(self, proxies_configuration, instance, providers_getter=None, update_interval=600):
        """
        Read the dict configuration to build realtime proxies
        Each entry contains 3 values:
            * id: the id of the system
            * class: the class (the full python path) handling the proxy
            * args: the argument to forward to the class contructor
        """
        self.logger = logging.getLogger(__name__)
        self._proxies_configuration = proxies_configuration if proxies_configuration else []
        self._realtime_proxies_getter = providers_getter
        self._realtime_proxies = {}
        self._realtime_proxies_legacy = {}
        self._realtime_proxies_from_db = {}
        self._realtime_proxies_last_update = {}
        self._last_update = datetime.datetime(1970, 1, 1)
        self._update_interval = update_interval
        self.instance = instance

        self.init_realtime_proxies()

    def init_realtime_proxies(self):

        for configuration in self._proxies_configuration:
            try:
                cls = configuration['class']
                proxy_id = configuration['id']
            except KeyError:
                self.logger.warning(
                    'impossible to build a realtime proxy, missing mandatory field in configuration'
                )
                continue

            object_id_tag = configuration.get('object_id_tag', proxy_id)
            args = configuration.get('args', {})

            rt_proxy = self._init_class(proxy_id, cls, object_id_tag, args)
            if not rt_proxy:
                continue

            self._realtime_proxies_legacy[proxy_id] = rt_proxy

    def _init_class(self, proxy_id, cls, object_id_tag, args):
        try:
            if '.' not in cls:
                self.logger.warning(
                    'impossible to build rt proxy {}, wrongly formated class: {}'.format(proxy_id, cls)
                )
                return None

            module_path, name = cls.rsplit('.', 1)
            module = import_module(module_path)
            attr = getattr(module, name)
        except ImportError:
            self.logger.warning('impossible to build rt proxy {}, cannot find class: {}'.format(proxy_id, cls))
            return None

        try:
            return attr(id=proxy_id, object_id_tag=object_id_tag, instance=self.instance, **args)
        except TypeError as e:
            self.logger.warning('impossible to build rt proxy {}, wrong arguments: {}'.format(proxy_id, e))
            return None

    def update_config(self):
        """
        Update list of realtime proxies services from db
        """
        if (
            self._last_update + datetime.timedelta(seconds=self._update_interval) > datetime.datetime.utcnow()
            or not self._realtime_proxies_getter
        ):
            return

        # If database is not accessible we update the value of self._last_update and exit
        if not can_connect_to_database():
            self.logger.debug('Database is not accessible')
            self._last_update = datetime.datetime.utcnow()
            return

        self.logger.debug('Updating realtime proxies from db')
        self._last_update = datetime.datetime.utcnow()

        try:
            realtime_proxies = self._realtime_proxies_getter()
        except Exception as e:
            self.logger.exception('Failure to retrieve realtime proxies configuration (error: {})'.format(e))
            # database is not accessible, so let's use the values already present in self._realtime_proxies
            return

        if not realtime_proxies:
            self.logger.debug('No realtime proxies available in db')
            self._realtime_proxies = {}
            self._realtime_proxies_last_update = {}

            return

        for rt_proxy in realtime_proxies:
            if (
                rt_proxy.id not in self._realtime_proxies_last_update
                or rt_proxy.last_update() > self._realtime_proxies_last_update[rt_proxy.id]
            ):
                self._update_realtime_proxies(rt_proxy)

    def _update_realtime_proxies(self, rt_proxy):
        self.logger.info('updating/adding {} realtime proxy'.format(rt_proxy.id))
        try:
            object_id_tag = rt_proxy.args.get('object_id_tag', rt_proxy.id)
            if 'object_id_tag' in rt_proxy.args:
                del rt_proxy.args["object_id_tag"]
            self._realtime_proxies[rt_proxy.id] = self._init_class(
                rt_proxy.id, rt_proxy.klass, object_id_tag, rt_proxy.args
            )
            self._realtime_proxies_last_update[rt_proxy.id] = rt_proxy.last_update()
        except Exception:
            self.logger.exception('impossible to update realtime proxy {}'.format(rt_proxy.id))

        # If a legacy realtime proxy exists in db, remove it from self._realtime_proxies_legacy
        realtime_proxies_from_db = list(self._realtime_proxies.values())
        realtime_proxies_legacy = self._realtime_proxies_legacy
        if realtime_proxies_legacy and realtime_proxies_from_db:
            new_realtime_proxies_legacy = dict()
            for rt_proxy_id, rt in realtime_proxies_legacy.items():
                if rt not in realtime_proxies_from_db:
                    new_realtime_proxies_legacy[rt_proxy_id] = rt
            self._realtime_proxies_legacy = new_realtime_proxies_legacy

    def get(self, proxy_name):
        self.update_config()
        return self._realtime_proxies.get(proxy_name, self._realtime_proxies_legacy.get(proxy_name))

    def get_all_realtime_proxies(self):
        return list(self._realtime_proxies.values()) + list(self._realtime_proxies_legacy.values())
