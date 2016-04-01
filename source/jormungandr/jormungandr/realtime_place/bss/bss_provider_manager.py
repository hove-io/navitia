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
from jormungandr import app
from importlib import import_module
import logging


class BssProviderManager(object):

    def __init__(self):
        self.bss_providers = []
        self.log = logging.getLogger(__name__)
        for configuration in app.config['BSS_PROVIDER']:
            arguments = configuration.get('args', {})
            self.bss_providers.append(self.init_class(configuration['class'], arguments))

    def handle_places(self, places):
        for place in places or []:
            if place['embedded_type'] == 'poi' and place['poi']['poi_type']['id'] == 'poi_type:amenity:bicycle_rental':
                provider = self.find_provider(place['poi'])
                if provider:
                    place['poi']['stands'] = provider.get_informations(place['poi'])
        return places

    def find_provider(self, poi):
        for instance in self.bss_providers:
            if instance.support_poi(poi):
                return instance

    def init_class(self, cls, arguments):
        try:
            if '.' not in cls:
                self.log.warn('impossible to build, wrongly formated class: {}'.format(cls))

            module_path, name = cls.rsplit('.', 1)
            module = import_module(module_path)
            attr = getattr(module, name)
            return attr(**arguments)
        except ImportError:
            self.log.warn('impossible to build, cannot find class: {}'.format(cls))
