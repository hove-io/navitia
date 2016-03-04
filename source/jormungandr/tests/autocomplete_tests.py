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
import logging

from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


def valid_autocomplete_with_one_stop_area(response):
    """ response contains 1 stop_areas "gare" """
    assert len(response['links']) == 1
    links = get_not_null(response, 'links')
    places = get_not_null(response, 'places')
    assert len(links) == 1
    assert len(places) == 1
    assert places[0]['embedded_type'] == 'stop_area'
    assert places[0]['name'] == 'Gare (Quimper)'

def valid_autocomplete_with_multi_object(response):
    """
    response contains 10 elements
    1 admin, 6 stop_areas and three addresses
    """
    links = get_not_null(response, 'links')
    places = get_not_null(response, 'places')
    assert len(links) == 2
    assert len(places) == 10
    assert places[0]['embedded_type'] == 'administrative_region'
    assert places[1]['embedded_type'] == 'stop_area'
    assert places[1]['name'] == 'Becharles (Quimper)'
    assert places[2]['embedded_type'] == 'stop_area'
    assert places[3]['embedded_type'] == 'stop_area'
    assert places[4]['embedded_type'] == 'stop_area'
    assert places[5]['embedded_type'] == 'stop_area'
    assert places[6]['embedded_type'] == 'stop_area'
    assert places[7]['embedded_type'] == 'address'
    assert places[8]['embedded_type'] == 'address'
    assert places[9]['embedded_type'] == 'address'

def is_response_empty(response):
    """response is empty"""
    assert len(response['links']) == 0
    assert 'places' not in response

def is_valid_autocomplete(response, depth):
    links = get_not_null(response, 'links')
    places = get_not_null(response, 'places')

    for link in links:
        assert 'href' in link
        assert 'rel' in link
        assert 'templated' in link

    for place in places:
        if place['embedded_type'] == 'stop_area':
            is_valid_stop_area(get_not_null(place, "stop_area"), depth-1)


@dataset({"main_autocomplete_test": {}})
class TestAutocomplete(AbstractTestFixture):
    """
    Test the autocomplete responses
    """
    def test_autocomplete_without_test(self):
        """
        Test of empty result
        """
        response = self.query_region("places?q=bob marley", display=False)
        is_response_empty(response)

    def test_autocomplete_with_one_stop_area_test(self):
        """
        Test with one object in the result
        """
        response = self.query_region("places?q=Gare", display=False)
        is_valid_autocomplete(response, 2)
        valid_autocomplete_with_one_stop_area(response)

    def test_autocomplete_with_multi_objects_test(self):
        """
        Test with 10 objects of different types in the result
        """
        response = self.query_region("places?q=quimper", display=False)
        is_valid_autocomplete(response, 2)
        valid_autocomplete_with_multi_object(response)
