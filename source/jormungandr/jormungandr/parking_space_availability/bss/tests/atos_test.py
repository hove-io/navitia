# encoding: utf-8
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
import pytest
from contextlib import contextmanager
from jormungandr.parking_space_availability.bss.atos import AtosProvider
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from mock import MagicMock

poi = {
    'properties': {'network': u'Vélitul', 'operator': 'Keolis', 'ref': '2'},
    'poi_type': {'name': 'station vls', 'id': 'poi_type:amenity:bicycle_rental'},
}


def parking_space_availability_atos_support_poi_test():
    """
    Atos bss provider support
    """
    provider = AtosProvider(
        u'10', u'vélitul', u'https://webservice.atos.com?wsdl', {'KEOLIS', 'effia', 'effia transport', u'kéolis'}
    )
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'EFFIA'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'Keolis'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'EFFIA Transport'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = u'KÉOLIS'
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    poi['properties']['network'] = 'Bad_network'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)

    invalid_poi = {}
    assert not provider.support_poi(invalid_poi)
    invalid_poi = {'properties': {}}
    assert not provider.support_poi(invalid_poi)


def parking_space_availability_atos_get_informations_test():
    """
    The service returns realtime stand information or stand with status='unavailable' if an error occured
    """
    stand_2 = Stands(5, 9, StandsStatus.open)
    all_stands = {'1': Stands(4, 8, StandsStatus.open), '2': stand_2}
    provider = AtosProvider(u'10', u'vélitul', u'https://webservice.atos.com?wsdl', {'keolis'})
    provider._get_all_stands = MagicMock(return_value=all_stands)
    assert provider.get_informations(poi) == stand_2
    invalid_poi = {}
    assert provider.get_informations(invalid_poi) == Stands(0, 0, StandsStatus.unavailable)

    poi_blur_ref = {'properties': {'ref': '02'}}
    assert provider.get_informations(poi_blur_ref) == stand_2

    provider._get_all_stands = MagicMock(side_effect=Exception('cannot access service'))
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.unavailable)


def parking_space_availability_atos_get_informations_with_closed_status_test():
    """
    The service returns realtime stand information or stand with status='unavailable' if an error occured
    """

    stand_2 = Stands(5, 9, StandsStatus.closed)
    all_stands = {'1': Stands(4, 8, StandsStatus.open), '2': stand_2}
    provider = AtosProvider(u'10', u'vélitul', u'https://webservice.atos.com?wsdl', {'keolis'})
    provider._get_all_stands = MagicMock(return_value=all_stands)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.closed)
    invalid_poi = {}
    assert provider.get_informations(invalid_poi) == Stands(0, 0, StandsStatus.unavailable)


def parking_space_availability_atos_get_all_stands_test():
    """
    Atos validate transformation of webservice result
    """
    stands = lambda: None
    all_stands_list = []
    stands.libelle = '1'
    stands.nbPlacesDispo = 4
    stands.nbVelosDispo = 8
    stands.etatConnexion = 'CONNECTEE'
    all_stands_list.append(stands)
    stands2 = lambda: None
    stands2.libelle = '2'
    stands2.nbPlacesDispo = 5
    stands2.nbVelosDispo = 9
    stands2.etatConnexion = 'CONNECTEE'
    all_stands_list.append(stands2)
    stands3 = lambda: None
    stands3.libelle = '3'
    stands3.nbPlacesDispo = 10
    stands3.nbVelosDispo = 20
    stands3.etatConnexion = 'DECONNECTEE'
    all_stands_list.append(stands3)

    provider = AtosProvider(u'10', u'vélitul', u'https://webservice.atos.com?wsdl', {'keolis'})
    client = lambda: None
    client.service = lambda: None
    client.service.getSummaryInformationTerminals = MagicMock(return_value=all_stands_list)

    @contextmanager
    def mock_get_client():
        yield client

    provider._get_client = mock_get_client

    all_stands = provider._get_all_stands()
    assert len(all_stands) == 3
    assert isinstance(all_stands.get('2'), Stands)

    # The status of stand=3 is converted to status navitia='unavailable' from 'DECONNECTEE'
    # and other attributs are initialized to 0.
    stand = provider.get_informations('3')
    assert stand == Stands(0, 0, StandsStatus.unavailable)


def parking_space_availability_atos_get_all_stands_urlerror_test():
    """
    Atos webservice error should raise an URLError exception
    """
    provider = AtosProvider(u'10', u'vélitul', u'https://error.fake.com?wsdl', {'keolis'})

    with pytest.raises(Exception):
        provider._get_all_stands()
