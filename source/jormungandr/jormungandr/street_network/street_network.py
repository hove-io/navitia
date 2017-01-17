# coding=utf-8

# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
import abc

# Using abc.ABCMeta in a way it is compatible both with Python 2.7 and Python 3.x
# http://stackoverflow.com/a/38668373/1614576
ABC = abc.ABCMeta(str("ABC"), (object,), {})


class IStreetNetwrokService(ABC):
    @abc.abstractmethod
    def get_street_network_routing_matrix(self, origins, destinations, street_network_mode, max_duration, request, **kwargs):
        pass

    @abc.abstractmethod
    def direct_path(self, mode, pt_object_origin, pt_object_destination, datetime, clockwise, request):
        pass


class StreetNetwork(object):

    @staticmethod
    def get_street_network_services(instance, street_network_configurations):
        log = logging.getLogger(__name__)
        street_network_services = {}
        for config in street_network_configurations:
            try:
                cls = config['class']
                modes = config['modes']
            except KeyError, TypeError:
                log.critical('impossible to build a routing, missing mandatory field in configuration')
                raise

            args = config.get('args', {})
            service_url = args.get('service_url', None)

            if '.' not in cls:
                log.critical('impossible to build StreetNetwork, wrongly formated class: {}'.format(cls))
                raise ValueError('impossible to build StreetNetwork, wrongly formated class: {}'.format(cls))

            try:
                module_path, name = cls.rsplit('.', 1)
                module = import_module(module_path)
                attr = getattr(module, name)
            except AttributeError:
                log.critical('impossible to build StreetNetwork, cannot find class: {}'.format(cls))
                raise

            service = attr(instance=instance, url=service_url, **args)
            for mode in modes:
                street_network_services[mode] = service
                log.info('** StreetNetwork {} used for direct_path with mode: {} **'.format(name, mode))
        return street_network_services
