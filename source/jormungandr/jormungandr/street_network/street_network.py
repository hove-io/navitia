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
from __future__ import absolute_import, print_function, unicode_literals, division
from importlib import import_module
import logging


class StreetNetwork(object):
    @staticmethod
    def get_street_network(instance, street_network_configuration):
        log = logging.getLogger(__name__)
        try:
            cls = street_network_configuration['class']
        except KeyError, TypeError:
            log.critical('impossible to build a routing, missing mandatory field in configuration')
            raise

        args = street_network_configuration.get('args', {})
        service_args = args.get('service_args', None)
        if service_args:
            directions_options = service_args.get('directions_options', None)
        else:
            directions_options = None
        service_url = args.get('service_url', None)
        costing_options = args.get('costing_options', None)
        api_key = args.get('api_key', None)
        for to_del in ['service_args', 'service_url', 'costing_options', 'api_key']:
            if to_del in args:
                del args[to_del]

        try:
            if '.' not in cls:
                log.critical('impossible to build StreetNetwork, wrongly formated class: {}'.format(cls))
                raise

            module_path, name = cls.rsplit('.', 1)
            module = import_module(module_path)
            attr = getattr(module, name)
        except ImportError:
            log.critical('impossible to build StreetNetwork, cannot find class: {}'.format(cls))
            raise

        try:
            log.info('** StreetNetwork {} used for direct_path **'.format(name))
            return attr(instance=instance,
                        service_url=service_url,
                        directions_options=directions_options,
                        costing_options=costing_options,
                        api_key=api_key,
                        **args)
        except TypeError as e:
            log.critical('impossible to build StreetNetwork {}, wrong arguments: {}'.format(name, e.message))
        raise

