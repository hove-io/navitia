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
from abc import abstractmethod, ABCMeta
import six
from importlib import import_module
import logging


def get_from_to_pois_of_journeys(journeys):
    # utility that returns 'from' and 'to' pois for each section for the given journeys
    from_to_places = (
        s.get(from_to, {}) for j in journeys for s in j.get('sections', []) for from_to in ('from', 'to')
    )
    return (place['poi'] for place in from_to_places if 'poi' in place)


class AbstractProviderManager(six.with_metaclass(ABCMeta, object)):
    def __init__(self):
        self.log = logging.getLogger(__name__)

    @abstractmethod
    def _handle_poi(self, item):
        pass

    @abstractmethod
    def _get_providers(self):
        pass

    def handle(self, response, attribute):
        if attribute == 'journeys':
            return self.handle_journeys(response[attribute])
        elif attribute in ('places', 'places_nearby', 'pois'):
            return self.handle_places(response[attribute])
        return None

    def handle_places(self, places):
        providers = set()
        for place in places or []:
            provider = None
            if 'poi_type' in place:
                provider = self._handle_poi(place)
            elif 'embedded_type' in place and place['embedded_type'] == 'poi':
                provider = self._handle_poi(place['poi'])
            if provider:
                providers.add(provider)
        return providers

    def handle_journeys(self, journeys):
        providers = set()
        for poi in get_from_to_pois_of_journeys(journeys):
            provider = self._handle_poi(poi)
            if provider:
                providers.add(provider)
        return providers

    def _find_provider(self, poi):
        for provider in self._get_providers():
            if provider.handle_poi(poi):
                return provider
        return None

    def _init_class(self, cls, arguments):
        try:
            if '.' not in cls:
                self.log.warning('impossible to build, wrongly formated class: {}'.format(cls))

            module_path, name = cls.rsplit('.', 1)
            module = import_module(module_path)
            attr = getattr(module, name)
            return attr(**arguments)
        except ImportError:
            self.log.warning('impossible to build, cannot find class: {}'.format(cls))
