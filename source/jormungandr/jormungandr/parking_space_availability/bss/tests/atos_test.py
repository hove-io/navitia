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
import pytest
from jormungandr.parking_space_availability.bss.atos import AtosProvider
from jormungandr.parking_space_availability.bss.stands import Stands
from mock import MagicMock
from urllib2 import URLError
from suds import WebFault

poi = {
    'properties': {
        'network': u'Vélitul',
        'operator': 'Keolis',
        'ref': '2'
    },
    'poi_type': {
        'name': 'station vls',
        'id': 'poi_type:amenity:bicycle_rental'
    }
}

def parking_space_availability_atos_support_poi_test():
    """
    Atos bss provider support
    """
    provider = AtosProvider(u'10', u'Vélitul', u'https://webservice.atos.com?wsdl')
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'EFFIA'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'Keolis'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'EFFIA Transport'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = u'Kéolis'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    poi['properties']['network'] = 'Bad_network'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)

def parking_space_availability_atos_get_informations_test():
    """
    Atos validate return good stands informations or None if an error occured
    """
    stands = Stands(5, 9)
    all_stands = {
        '1': Stands(4, 8),
        '2': stands
    }
    provider = AtosProvider(u'10', u'Vélitul', u'https://webservice.atos.com?wsdl')
    provider.get_all_stands = MagicMock(return_value=all_stands)
    assert provider.get_informations(poi) == stands
    provider.get_all_stands = MagicMock(side_effect=WebFault('fake fault', 'mock'))
    assert provider.get_informations(poi) is None

def parking_space_availability_atos_get_all_stands_test():
    """
    Atos validate transformation of webservice result
    """
    stands = lambda: None
    all_stands_list = []
    stands.libelle = '1'
    stands.nbPlacesDispo = 4
    stands.nbVelosDispo = 8
    all_stands_list.append(stands)
    stands2 = lambda: None
    stands2.libelle = '2'
    stands2.nbPlacesDispo = 5
    stands2.nbVelosDispo = 9
    all_stands_list.append(stands2)

    provider = AtosProvider(u'10', u'Vélitul', u'https://webservice.atos.com?wsdl')
    client = lambda: None
    client.service = lambda: None
    client.service.getSummaryInformationTerminals = MagicMock(return_value=all_stands_list)
    provider.get_client = MagicMock(return_value=client)
    all_stands = provider.get_all_stands()
    assert len(all_stands) == 2
    assert isinstance(all_stands.get('2'), Stands)

def parking_space_availability_atos_get_all_stands_urlerror_test():
    """
    Atos webservice error should raise an URLError exception
    """
    provider = AtosProvider(u'10', u'Vélitul', u'https://error.fake.com?wsdl')

    with pytest.raises(URLError):
        provider.get_all_stands()
