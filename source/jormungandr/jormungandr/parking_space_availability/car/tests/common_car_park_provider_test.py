# encoding: utf-8
# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

import pytest
import copy

from jormungandr.parking_space_availability.car.common_car_park_provider import CommonCarParkProvider


class CommonCarParkProviderTest(CommonCarParkProvider):
    def __init__(self, **kwargs):
        CommonCarParkProvider.__init__(self, **kwargs)

    def process_data(self, data, poi):
        pass


@pytest.fixture
def defaultCarParkParameters():
    return {
        'url': 'http://test.com',
        'operators': ["Tank"],
        'dataset': "data",
        'timeout': 1,
        'feed_publisher': {'id': 'fp1', 'name': 'feedpub'},
    }


@pytest.fixture
def carParkProvider(defaultCarParkParameters):
    return CommonCarParkProviderTest(**defaultCarParkParameters)


def test_create_default_common_car_park_provider_with_no_shape(carParkProvider):
    assert carParkProvider.has_boundary_shape() == False


def test_default_car_park_provider_should_handle_poi(carParkProvider):
    assert carParkProvider.support_poi({'properties': {'operator': 'TANK'}}) == True

    assert carParkProvider.support_poi({}) == False
    assert carParkProvider.support_poi({'properties': {}}) == False
    assert carParkProvider.support_poi({'properties': {'operator': 'other_operator'}}) == False


@pytest.fixture
def carParkProviderWithGeometry(defaultCarParkParameters):
    params = defaultCarParkParameters
    params['geometry'] = {
        'type': 'Polygon',
        'coordinates': [
            [
                [3.97131347656250, 47.27224096817965],
                [5.10486602783203, 47.27224096817966],
                [5.10486602783203, 47.36743170534774],
                [4.97131347656250, 47.36743170534774],
                [4.97131347656250, 47.27224096817966],
            ]
        ],
    }
    return CommonCarParkProviderTest(**params)


def test_car_park_provider_with_geometry_should_have_shape(carParkProviderWithGeometry):
    assert carParkProviderWithGeometry.has_boundary_shape() == True


def test_car_park_provider_with_shape_should_not_support_poi_with_no_coords(carParkProviderWithGeometry):
    assert carParkProviderWithGeometry.support_poi({}) == False
    assert carParkProviderWithGeometry.support_poi({}) == False
    assert carParkProviderWithGeometry.support_poi({'properties': {}}) == False
    assert carParkProviderWithGeometry.support_poi({'properties': {'operator': 'other_operator'}}) == False
    assert carParkProviderWithGeometry.support_poi({'properties': {'operator': 'TANK'}}) == False


def test_car_park_provider_with_shape_should_support_poi_with_coords(carParkProviderWithGeometry):
    poi = {'properties': {'operator': 'TANK'}}

    '''
    The coord is a point INSIDE the car park provider polygon
    '''
    poi_inside = copy.copy(poi)
    poi_inside['coord'] = {'lon': '5.041351318359375', 'lat': '47.32881751198527'}
    assert carParkProviderWithGeometry.support_poi(poi_inside) == True

    '''
    The coord is this time, OUTSIDE the car park provider polygon
    '''
    poi_outside = copy.copy(poi)
    poi_outside['coord'] = {'lon': '1.041351318359375', 'lat': '1.32881751198527'}
    assert carParkProviderWithGeometry.support_poi(poi_outside) == False


def test_car_park_provider_with_wrong_geojson_should_have_no_shape(defaultCarParkParameters):
    defaultCarParkParameters['geometry'] = {'type': 'no_idea', 'coordinates': ['blah', 'blah', 'blah']}

    parkProvider = CommonCarParkProviderTest(**defaultCarParkParameters)
    assert parkProvider.has_boundary_shape() == False


def test_car_park_provider_with_falty_geometry_shape(defaultCarParkParameters):
    params = defaultCarParkParameters
    params['geometry'] = {'type': 'Unknow_type', 'coordinates': [[]]}

    car_park = CommonCarParkProviderTest(**params)
    assert car_park.has_boundary_shape() == False
