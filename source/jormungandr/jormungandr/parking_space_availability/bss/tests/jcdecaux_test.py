# encoding: utf-8
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
from jormungandr.parking_space_availability.bss.jcdecaux import JcdecauxProvider
from jormungandr.parking_space_availability.bss.stands import Stands
from mock import MagicMock

poi = {
    'properties': {
        'network': u"Vélib'",
        'operator': 'JCDecaux',
        'ref': '2'
    },
    'poi_type': {
        'name': 'station vls',
        'id': 'poi_type:amenity:bicycle_rental'
    }
}

def parking_space_availability_jcdecaux_support_poi_test():
    """
    Jcdecaux bss provider support
    """
    provider = JcdecauxProvider(u"Vélib'", 'Paris', 'api_key')
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'JCDecaux'
    poi['properties']['network'] = 'Bad_network'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)

def parking_space_availability_jcdecaux_get_informations_test():
    """
    Jcdecaux validate return good stands informations or None if an error occured
    """
    webservice_response = {
        'available_bike_stands': 4,
        'available_bikes': 8
    }
    provider = JcdecauxProvider(u"Vélib'", 'Paris', 'api_key')
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(4, 8)
    provider._call_webservice = MagicMock(return_value=None)
    assert provider.get_informations(poi) is None

def parking_space_availability_jcdecaux_get_informations_unauthorized_test():
    """
    Jcdecaux validate return None if not authorized
    """
    webservice_unauthorized_response = {
        'error': 'Unauthorized'
    }
    provider = JcdecauxProvider(u"Vélib'", 'Paris', 'unauthorized_api_key')
    provider._call_webservice = MagicMock(return_value=webservice_unauthorized_response)
    assert provider.get_informations(poi) is None
