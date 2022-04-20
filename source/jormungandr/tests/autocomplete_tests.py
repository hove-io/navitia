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
import logging

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


def valid_autocomplete_with_one_stop_area(response):
    """response contains 1 stop_areas "gare" """
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
    assert places[1]['name'] == 'IUT (Quimper)'
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


@dataset({"main_autocomplete_test": {}})
class TestAutocomplete(AbstractTestFixture):
    """
    Test the autocomplete responses
    """

    def test_autocomplete_without(self):
        """
        Test of empty result
        """
        response = self.query_region("places?q=bob marley", display=False)
        is_response_empty(response)

    def test_autocomplete_with_one_stop_area(self):
        """
        Test with one object in the result
        """
        response = self.query_region("places?q=Gare", display=False)
        is_valid_autocomplete(response, 2)
        valid_autocomplete_with_one_stop_area(response)

    def test_autocomplete_with_multi_objects(self):
        """
        Test with 10 objects of different types in the result
        """
        response = self.query_region("places?q=quimper", display=False)
        is_valid_autocomplete(response, 2)
        valid_autocomplete_with_multi_object(response)

    def test_places_coords(self):
        coords = '{lon};{lat}'.format(lon=2, lat=3)
        response = self.query(
            'v1/coverage/{coords}/places?q={q}&type[]=stop_point'.format(coords=coords, q='Becharles')
        )
        places = get_not_null(response, 'places')
        assert len(places) == 1
        assert places[0]['id'] == 'stop_point:Becharles'

    def test_place_uri_coords(self):
        coords = '{lon};{lat}'.format(lon=2, lat=3)
        response = self.query(
            'v1/coverage/{coords}/places/{id}'.format(coords=coords, id='stop_point:Becharles')
        )
        places = get_not_null(response, 'places')
        assert len(places) == 1
        assert places[0]['id'] == 'stop_point:Becharles'

    def test_place_ok_shape(self):
        shape = '{"type":"Feature","geometry":{"type":"Polygon","coordinates":\
        [[[2.283,48.896],[2.280,48.818],[2.417,48.818],[2.416,48.897],[2.283,48.896]]]}}'
        _ = self.query_region("places?q=Gare&shape={}".format(shape))

    def test_place_ko_shape_with_empty_json_object(self):
        _, status = self.query_no_assert('/v1/coverage/main_autocomplete_test/places?q=Gare&shape={}')
        assert status == 400

    def test_place_ko_shape_not_json(self):
        _, status = self.query_no_assert('/v1/coverage/main_autocomplete_test/places?q=Gare&shape=toto')
        assert status == 400

    def test_place_ko_shape_multipolygon_not_yet_accepted(self):
        multipolygon = '{"type":"Feature","geometry":{"type":"MultiPolygo","coordiates":\
        [[[[2.4,48.6],[2.8,48.6],[2.7,48.9],[2.4,48.6]]],\
        [[[2.1,48.9],[2.2,48.6],[2.4,48.9],[2.1,48.9]]]]}}'
        _, status = self.query_no_assert(
            '/v1/coverage/main_autocomplete_test/places?q=Gare&shape={}'.format(multipolygon)
        )
        assert status == 400

    def test_visibility(self):
        """
        Test if visible parameters (way struct) is taken into account

        data :
            quai NEUF (Quimper) with visible=true
            input/output quai NEUF (Quimper) with visible=false
        """
        response = self.query_region("places?q=quai NEUF", display=False)
        is_valid_autocomplete(response, 2)
        places = get_not_null(response, 'places')
        assert len(places) == 1
        assert places[0]['name'] == 'quai NEUF (Quimper)'

    def test_place_with_shape_scope_invald(self):
        _, status = self.query_no_assert('/v1/coverage/main_autocomplete_test/places?q=Gare&shape_scope[]=bob')
        assert status == 400

    def test_place_with_one_shape_scope_accepted(self):
        _, status = self.query_no_assert('/v1/coverage/main_autocomplete_test/places?q=Gare&shape_scope[]=poi')
        assert status == 200

    def test_place_with_many_shape_scope_accepted(self):
        _, status = self.query_no_assert(
            '/v1/coverage/main_autocomplete_test/places?q=Gare'
            '&shape_scope[]=poi&shape_scope[]=street&shape_scope[]=addr'
        )
        assert status == 200
