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
from jormungandr.parking_space_availability.bss.jcdecaux import JcdecauxProvider
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from mock import MagicMock
import requests_mock

poi = {
    'properties': {'network': u"Vélib'", 'operator': 'JCDecaux', 'ref': '2'},
    'poi_type': {'name': 'station vls', 'id': 'poi_type:amenity:bicycle_rental'},
}


def parking_space_availability_jcdecaux_support_poi_test():
    """
    Jcdecaux bss provider support
    """
    provider = JcdecauxProvider(u"vélib'", 'Paris', 'api_key', {'JCDecaux'})
    assert provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'JCDecaux'
    poi['properties']['network'] = 'Bad_network'
    assert not provider.support_poi(poi)
    poi['properties']['operator'] = 'Bad_operator'
    assert not provider.support_poi(poi)

    invalid_poi = {}
    assert not provider.support_poi(invalid_poi)
    invalid_poi = {'properties': {}}
    assert not provider.support_poi(invalid_poi)


def parking_space_availability_jcdecaux_get_informations_test():
    """
    The service returns realtime stand information or stand with status='Unavailable' if an error occured
    """
    webservice_response = {'2': {'available_bike_stands': 4, 'available_bikes': 8, 'status': 'OPEN'}}
    provider = JcdecauxProvider(u"vélib'", 'Paris', 'api_key', {'jcdecaux'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(4, 8, StandsStatus.open)
    provider._data = {}
    provider._call_webservice = MagicMock(return_value=None)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.unavailable)
    invalid_poi = {}
    assert provider.get_informations(invalid_poi) == Stands(0, 0, StandsStatus.unavailable)


def parking_space_availability_jcdecaux_get_informations_unauthorized_test():
    """
    The service returns realtime stand information or stand with status='Unavailable' if not authorized
    """
    webservice_unauthorized_response = {'error': 'Unauthorized'}
    provider = JcdecauxProvider(u"vélib'", 'Paris', 'unauthorized_api_key', {'jcdecaux'})
    provider._call_webservice = MagicMock(return_value=webservice_unauthorized_response)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.unavailable)


def parking_space_availability_jcdecaux_get_informations_with_status_closed_test():

    webservice_response = {'2': {'available_bike_stands': 4, 'available_bikes': 8, 'status': 'CLOSED'}}
    provider = JcdecauxProvider(u"vélib'", 'Paris', 'api_key', {'jcdecaux'})
    provider._call_webservice = MagicMock(return_value=webservice_response)
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.closed)
    provider._call_webservice = MagicMock(return_value=None)
    provider._data = {}
    assert provider.get_informations(poi) == Stands(0, 0, StandsStatus.unavailable)
    invalid_poi = {}
    assert provider.get_informations(invalid_poi) == Stands(0, 0, StandsStatus.unavailable)


def test_call_mocked_request():
    webservice_response = [{'number': 2, 'available_bike_stands': 4, 'available_bikes': 8, 'status': 'OPEN'}]
    provider = JcdecauxProvider(u"vélib'", 'Paris', 'api_key', {'jcdecaux'})
    with requests_mock.Mocker() as m:
        m.get('https://api.jcdecaux.com/vls/v1/stations/', json=webservice_response)
        assert provider.get_informations(poi) == Stands(4, 8, StandsStatus.open)
        assert m.called
