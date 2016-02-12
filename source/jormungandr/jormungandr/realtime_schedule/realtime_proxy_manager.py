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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from importlib import import_module
import logging


class RealtimeProxyManager(object):
    """
    class managing real-time proxies
    """

    def __init__(self, proxies_configuration):
        """
        Read the dict configuration to build realtime proxies
        Each entry contains 3 values:
            * id: the id of the system
            * class: the class (the full python path) handling the proxy
            * args: the argument to forward to the class contructor
        """
        self.realtime_proxies = {}
        log = logging.getLogger(__name__)
        for configuration in proxies_configuration:
            try:
                cls = configuration['class']
                proxy_id = configuration['id']
            except ValueError:
                log.warn('impossible to build a realtime proxy, missing mandatory field in configuration')
                continue
            args = configuration.get('args', {})

            try:
                module_path, name = cls.rsplit('.', 1)
                module = import_module(module_path)
                attr = getattr(module, name)
            except ImportError:
                log.warn('impossible to build rt proxy {}, cannot find class: {}'.format(proxy_id, cls))
                continue
            rt_proxy = attr(**args)
            rt_proxy.id = proxy_id  # all services must have an ID
            self.realtime_proxies[proxy_id] = rt_proxy

    def get(self, proxy_name):
        return self.realtime_proxies[proxy_name]
