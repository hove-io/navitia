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
from abc import abstractmethod, ABCMeta
import six


def get_from_to_pois_of_journeys(journeys):
    # utility that returns 'from' and 'to' pois for each section for the given journeys
    from_to_places = (s.get(from_to, {}) for j in journeys for s in j.get('sections', [])
                      for from_to in ('from', 'to'))
    return (place['poi'] for place in from_to_places if 'poi' in place)


class AbstrcatProviderManager(six.with_metaclass(ABCMeta, object)):
    @abstractmethod
    def _handle_poi(self, item):
        pass

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

