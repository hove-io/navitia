# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
import mock
from mock import PropertyMock
from jormungandr.parking_space_availability.bss.bss_provider import BssProvider
from jormungandr.parking_space_availability.bss.stands import Stands
from tests.check_utils import is_valid_poi, get_not_null
from tests.tests_mechanism import AbstractTestFixture, dataset


class MockBssProvider(BssProvider):

    def __init__(self, pois_supported):
        self.pois_supported = pois_supported

    def support_poi(self, poi):
        return not self.pois_supported or poi['id'] in self.pois_supported

    def get_informations(self, poi):
        available_places = 13 if poi['id'] == 'station_1' else 99
        available_bikes = 3 if poi['id'] == 'station_1' else 98
        return Stands(available_places=available_places, available_bikes=available_bikes)


def mock_bss_providers(pois_supported):
    providers = [MockBssProvider(pois_supported=pois_supported)]
    return mock.patch('jormungandr.parking_space_availability.bss.BssProviderManager._get_providers',
                      new_callable=PropertyMock, return_value=lambda: providers)

@dataset({"main_routing_test": {}})
class TestBssProvider(AbstractTestFixture):

    """
    BSS tests on following API:
    - supported:
        - pois
        - pois/<poi_id>
        - places/<place_id>
        - places/<place_id>/places_nearby
    - not supported:
        - places (reason: due to performance aim in autocompletion)
    """


    ############################
    #####
    ##### API: pois
    ##### BSS support: Yes
    #####
    ############################


    def pois_without_stands_on_pois_test(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region('pois', display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'station_1', 'station_2', 'parking_1', 'parking_2'}
            for p in pois:
                is_valid_poi(p)
                assert not 'stands' in p


    def pois_with_stands_on_first_of_pois_test(self):
        with mock_bss_providers(pois_supported=['station_1']):
            r = self.query_region('pois', display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'station_1', 'station_2', 'parking_1', 'parking_2'}
            for p in pois:
                is_valid_poi(p)
                if p['id'] == 'station_1':
                    assert 'stands' in p
                    stands = p['stands']
                    assert stands['available_places'] == 13
                    assert stands['available_bikes'] == 3
                    assert stands['total_stands'] == 16
                else:
                    assert not 'stands' in p


    def pois_with_stands_on_all_pois_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region('pois', display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'station_1', 'station_2', 'parking_1', 'parking_2'}
            for p in pois:
                is_valid_poi(p)
                if p['id'] == 'station_1' or p['id'] == 'station_2':
                    assert 'stands' in p
                    stands = p['stands']
                    if p['id'] == 'station_1':
                        assert stands['available_places'] == 13
                        assert stands['available_bikes'] == 3
                        assert stands['total_stands'] == 16
                    elif p['id'] == 'station_2':
                        assert stands['available_places'] == 99
                        assert stands['available_bikes'] == 98
                        assert stands['total_stands'] == 197
                else:
                    assert not 'stands' in p


    ############################
    #####
    ##### API: pois/<poi_id>
    ##### BSS support: Yes
    #####
    ############################


    def pois_without_stands_on_poi_test(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region("pois/station_1", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'station_1'}

            poi = pois[0]
            is_valid_poi(poi)
            assert not 'stands' in poi


    def pois_with_stands_on_first_poi_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("pois/station_1", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'station_1'}

            poi = pois[0]
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 13
            assert stands['available_bikes'] == 3
            assert stands['total_stands'] == 16


    def pois_with_stands_on_second_poi_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("pois/station_2", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'station_2'}

            poi = pois[0]
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 99
            assert stands['available_bikes'] == 98
            assert stands['total_stands'] == 197


    def pois_with_stands_on_first_parking_poi_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("pois/parking_1", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'parking_1'}

            poi = pois[0]
            is_valid_poi(poi)
            assert not 'stands' in poi


    ############################
    #####
    ##### API: places/<place_id>
    ##### BSS support: Yes
    #####
    ############################

    def pois_without_stands_on_place_test(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region("places/station_1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'station_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert not 'stands' in poi


    def pois_with_stands_on_first_place_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/station_1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'station_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 13
            assert stands['available_bikes'] == 3
            assert stands['total_stands'] == 16


    def pois_with_stands_on_second_place_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/station_2", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'station_2'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 99
            assert stands['available_bikes'] == 98
            assert stands['total_stands'] == 197


    def pois_with_stands_on_first_parking_place_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/parking_1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'parking_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert not 'stands' in poi


    ############################
    #####
    ##### API: places/<place_id>/places_nearby
    ##### BSS support: Yes
    #####
    ############################


    def pois_without_stands_on_places_nearby_test(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region("places/stop_area:stop1/places_nearby", display=False)

            places_nearby = get_not_null(r, 'places_nearby')
            assert {p[p['embedded_type']]['id'] for p in places_nearby} == {'parking_1', 'station_1',
                                                                            'stop_point:stopB', 'stopB', 'station_2',
                                                                            'parking_2', 'stopC', 'stop_point:stopC',
                                                                            'stopA', 'stop_point:stopA'}
            for p in places_nearby:
                embedded_type = p['embedded_type']
                if embedded_type == 'poi':
                    is_valid_poi(p[embedded_type])
                assert not 'stands' in p[embedded_type]


    def pois_with_stands_on_second_places_nearby_test(self):
        with mock_bss_providers(pois_supported=['station_1']):
            r = self.query_region('places/stop_area:stop1/places_nearby', display=False)

            places_nearby = get_not_null(r, 'places_nearby')
            assert {p[p['embedded_type']]['id'] for p in places_nearby} == {'parking_1', 'station_1',
                                                                            'stop_point:stopB', 'stopB', 'station_2',
                                                                            'parking_2', 'stopC', 'stop_point:stopC',
                                                                            'stopA', 'stop_point:stopA'}
            for p in places_nearby:
                embedded_type = p['embedded_type']
                if embedded_type == 'poi':
                    is_valid_poi(p[embedded_type])
                    if p[embedded_type]['id'] == 'station_1':
                        assert 'stands' in p[embedded_type]
                        stands = p[embedded_type]['stands']
                        assert stands['available_places'] == 13
                        assert stands['available_bikes'] == 3
                        assert stands['total_stands'] == 16
                    else:
                        assert not 'stands' in p[embedded_type]
                else:
                    assert not 'stands' in p[embedded_type]


    def pois_with_stands_on_all_places_nearby_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region('places/stop_area:stop1/places_nearby', display=False)

            places_nearby = get_not_null(r, 'places_nearby')
            assert {p[p['embedded_type']]['id'] for p in places_nearby} == {'parking_1', 'station_1',
                                                                            'stop_point:stopB', 'stopB', 'station_2',
                                                                            'parking_2', 'stopC', 'stop_point:stopC',
                                                                            'stopA', 'stop_point:stopA'}
            for p in places_nearby:
                embedded_type = p['embedded_type']
                if embedded_type == 'poi':
                    is_valid_poi(p[embedded_type])
                    if p[embedded_type]['id'] == 'station_1' or p[embedded_type]['id'] == 'station_2':
                        assert 'stands' in p[embedded_type]
                        stands = p[embedded_type]['stands']
                    if p[embedded_type]['id'] == 'station_1':
                        assert stands['available_places'] == 13
                        assert stands['available_bikes'] == 3
                        assert stands['total_stands'] == 16
                    elif p[embedded_type]['id'] == 'station_2':
                        assert stands['available_places'] == 99
                        assert stands['available_bikes'] == 98
                        assert stands['total_stands'] == 197
                    else:
                        assert not 'stands' in p[embedded_type]
                else:
                    assert not 'stands' in p[embedded_type]


    ############################
    #####
    ##### API: places
    ##### BSS support: No
    #####
    ############################


    def pois_without_stands_on_places_autocompletion_test(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/?q=first station&count=1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'station_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert not 'stands' in poi