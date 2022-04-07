# Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import

import mock
from mock import PropertyMock

from jormungandr.parking_space_availability import (
    AbstractParkingPlacesProvider,
    Stands,
    StandsStatus,
    get_from_to_pois_of_journeys,
)
from jormungandr.ptref import FeedPublisher
from tests.check_utils import is_valid_poi, get_not_null, journey_basic_query
from tests.tests_mechanism import AbstractTestFixture, dataset, mock_bss_providers, MockBssProvider


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

    def test_pois_without_stands_on_pois(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region('pois', display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {
                'poi:station_1',
                'poi:station_2',
                'poi:parking_1',
                'poi:parking_2',
            }
            for p in pois:
                is_valid_poi(p)
                assert not 'stands' in p

    def test_pois_with_stands_on_first_of_pois(self):
        with mock_bss_providers(pois_supported=['poi:station_1']):
            r = self.query_region('pois', display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {
                'poi:station_1',
                'poi:station_2',
                'poi:parking_1',
                'poi:parking_2',
            }
            for p in pois:
                is_valid_poi(p)
                if p['id'] == 'poi:station_1':
                    assert 'stands' in p
                    stands = p['stands']
                    assert stands['available_places'] == 13
                    assert stands['available_bikes'] == 3
                    assert stands['total_stands'] == 16
                    assert stands['status'] == 'open'
                else:
                    assert not 'stands' in p

    def test_pois_with_stands_on_all_pois(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region('pois', display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {
                'poi:station_1',
                'poi:station_2',
                'poi:parking_1',
                'poi:parking_2',
            }
            for p in pois:
                is_valid_poi(p)
                if p['id'] == 'poi:station_1' or p['id'] == 'poi:station_2':
                    assert 'stands' in p
                    stands = p['stands']
                    if p['id'] == 'poi:station_1':
                        assert stands['available_places'] == 13
                        assert stands['available_bikes'] == 3
                        assert stands['total_stands'] == 16
                        assert stands['status'] == 'open'
                    elif p['id'] == 'poi:station_2':
                        assert stands['available_places'] == 99
                        assert stands['available_bikes'] == 98
                        assert stands['total_stands'] == 197
                        assert stands['status'] == 'open'
                else:
                    assert not 'stands' in p

    ############################
    #####
    ##### API: pois/<poi_id>
    ##### BSS support: Yes
    #####
    ############################

    def test_pois_without_stands_on_poi(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region("pois/poi:station_1", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'poi:station_1'}

            poi = pois[0]
            is_valid_poi(poi)
            assert not 'stands' in poi

    def test_pois_with_stands_on_first_poi(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("pois/poi:station_1", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'poi:station_1'}

            poi = pois[0]
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 13
            assert stands['available_bikes'] == 3
            assert stands['total_stands'] == 16
            assert stands['status'] == 'open'

    def test_pois_with_stands_on_second_poi(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("pois/poi:station_2", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'poi:station_2'}

            poi = pois[0]
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 99
            assert stands['available_bikes'] == 98
            assert stands['total_stands'] == 197
            assert stands['status'] == 'open'

    def test_pois_with_stands_on_first_parking_poi(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("pois/poi:parking_1", display=False)

            pois = get_not_null(r, 'pois')
            assert {p['id'] for p in pois} == {'poi:parking_1'}

            poi = pois[0]
            is_valid_poi(poi)
            assert not 'stands' in poi

    ############################
    #####
    ##### API: places/<place_id>
    ##### BSS support: Yes
    #####
    ############################

    def test_pois_without_stands_on_place(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region("places/poi:station_1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'poi:station_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert not 'stands' in poi

    def test_pois_with_stands_on_first_place(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/poi:station_1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'poi:station_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 13
            assert stands['available_bikes'] == 3
            assert stands['total_stands'] == 16
            assert stands['status'] == 'open'

    def test_pois_with_stands_on_second_place(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/poi:station_2", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'poi:station_2'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert 'stands' in poi
            stands = poi['stands']
            assert stands['available_places'] == 99
            assert stands['available_bikes'] == 98
            assert stands['total_stands'] == 197
            assert stands['status'] == 'open'

    def test_pois_with_stands_on_first_parking_place(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/poi:parking_1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'poi:parking_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert not 'stands' in poi

    ############################
    #####
    ##### API: places/<place_id>/places_nearby
    ##### BSS support: Yes
    #####
    ############################

    def test_pois_without_stands_on_places_nearby(self):
        with mock_bss_providers(pois_supported=['station_unknown']):
            r = self.query_region("places/admin:74435/places_nearby?count=20", display=False)

            places_nearby = get_not_null(r, 'places_nearby')
            assert {p[p['embedded_type']]['id'] for p in places_nearby} == {
                'poi:parking_1',
                'poi:station_1',
                'stop_point:stopB',
                'stopB',
                'poi:station_2',
                'poi:parking_2',
                'stopC',
                'stop_point:stopC',
                'stopA',
                'stop_point:stopA',
                'stop_point:uselessA',
            }
            for p in places_nearby:
                embedded_type = p['embedded_type']
                if embedded_type == 'poi':
                    is_valid_poi(p[embedded_type])
                assert not 'stands' in p[embedded_type]

    def test_pois_with_stands_on_second_places_nearby(self):
        with mock_bss_providers(pois_supported=['poi:station_1']):
            r = self.query_region('places/admin:74435/places_nearby?count=20', display=False)

            places_nearby = get_not_null(r, 'places_nearby')
            assert {p[p['embedded_type']]['id'] for p in places_nearby} == {
                'poi:parking_1',
                'poi:station_1',
                'stop_point:stopB',
                'stopB',
                'poi:station_2',
                'poi:parking_2',
                'stopC',
                'stop_point:stopC',
                'stopA',
                'stop_point:stopA',
                'stop_point:uselessA',
            }
            for p in places_nearby:
                embedded_type = p['embedded_type']
                if embedded_type == 'poi':
                    is_valid_poi(p[embedded_type])
                    if p[embedded_type]['id'] == 'poi:station_1':
                        assert 'stands' in p[embedded_type]
                        stands = p[embedded_type]['stands']
                        assert stands['available_places'] == 13
                        assert stands['available_bikes'] == 3
                        assert stands['total_stands'] == 16
                        assert stands['status'] == 'open'
                    else:
                        assert not 'stands' in p[embedded_type]
                else:
                    assert not 'stands' in p[embedded_type]

    def test_pois_with_stands_on_all_places_nearby(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region('places/admin:74435/places_nearby?count=20', display=False)

            places_nearby = get_not_null(r, 'places_nearby')
            assert {p[p['embedded_type']]['id'] for p in places_nearby} == {
                'poi:parking_1',
                'poi:station_1',
                'stop_point:stopB',
                'stopB',
                'poi:station_2',
                'poi:parking_2',
                'stopC',
                'stop_point:stopC',
                'stopA',
                'stop_point:stopA',
                'stop_point:uselessA',
            }
            for p in places_nearby:
                embedded_type = p['embedded_type']
                if embedded_type == 'poi':
                    is_valid_poi(p[embedded_type])
                    if p[embedded_type]['id'] == 'poi:station_1' or p[embedded_type]['id'] == 'poi:station_2':
                        assert 'stands' in p[embedded_type]
                        stands = p[embedded_type]['stands']
                    if p[embedded_type]['id'] == 'poi:station_1':
                        assert stands['available_places'] == 13
                        assert stands['available_bikes'] == 3
                        assert stands['total_stands'] == 16
                        assert stands['status'] == 'open'
                    elif p[embedded_type]['id'] == 'poi:station_2':
                        assert stands['available_places'] == 99
                        assert stands['available_bikes'] == 98
                        assert stands['total_stands'] == 197
                        assert stands['status'] == 'open'
                    else:
                        assert not 'stands' in p[embedded_type]
                else:
                    assert not 'stands' in p[embedded_type]

    def test_pois_places_nearby_depth_zero(self):
        """
        with depth=0 we don't have to poi_type, so it's not possible to use bss availability
        we don't want jormungandr to crash in this case
        """
        with mock_bss_providers(pois_supported=['station_1']):
            r = self.query_region('places/admin:74435/places_nearby?depth=0&count=20', display=False)

            places_nearby = get_not_null(r, 'places_nearby')
            assert {p[p['embedded_type']]['id'] for p in places_nearby} == {
                'poi:parking_1',
                'poi:station_1',
                'stop_point:stopB',
                'stopB',
                'poi:station_2',
                'poi:parking_2',
                'stopC',
                'stop_point:stopC',
                'stopA',
                'stop_point:stopA',
                'stop_point:uselessA',
            }

    ############################
    #####
    ##### API: places
    ##### BSS support: No
    #####
    ############################

    def test_pois_without_stands_on_places_autocompletion(self):
        with mock_bss_providers(pois_supported=[]):
            r = self.query_region("places/?q=first station&count=1", display=False)

            places = get_not_null(r, 'places')
            assert {p[p['embedded_type']]['id'] for p in places} == {'poi:station_1'}

            poi = places[0]['poi']
            is_valid_poi(poi)
            assert not 'stands' in poi

    ############################
    #####
    ##### API: journeys
    ##### BSS support: Yes
    #####
    ############################

    def test_bss_stands_on_journeys_disabled_by_default(self):
        supported_pois = ['poi:station_1', 'poi:station_2']
        with mock_bss_providers(pois_supported=supported_pois):
            query = journey_basic_query + "&first_section_mode=bss&last_section_mode=bss"
            response = self.query_region(query)
            self.is_valid_journey_response(response, query)
            journeys = get_not_null(response, 'journeys')
            for poi in get_from_to_pois_of_journeys(journeys):
                is_valid_poi(poi)
                assert 'stands' not in poi

            # since no stands have been added, we have not added the bss provider as a feed publisher
            # thus we only have the 'static' data feed publisher
            feeds = get_not_null(response, 'feed_publishers')
            assert len(feeds) == 1
            assert next(f for f in feeds if f['name'] == 'routing api data')

    def test_journey_sections_from_to_poi_with_stands(self):
        supported_pois = ['poi:station_1']
        with mock_bss_providers(pois_supported=supported_pois):
            query = (
                journey_basic_query + "&first_section_mode=bss&last_section_mode=bss&add_poi_infos[]=bss_stands"
            )
            response = self.query_region(query)
            self.is_valid_journey_response(response, query)
            journeys = get_not_null(response, 'journeys')

            for poi in get_from_to_pois_of_journeys(journeys):
                is_valid_poi(poi)
                if poi['id'] in supported_pois:
                    assert 'stands' in poi
                    assert poi['stands']['available_places'] == 13
                    assert poi['stands']['available_bikes'] == 3
                    assert poi['stands']['total_stands'] == 16
                    assert poi['stands']['status'] == 'open'
                else:
                    assert 'stands' not in poi

            # check the feed publishers
            # we have the feed publisher of the 'static' data
            feeds = get_not_null(response, 'feed_publishers')
            tc_data = next(f for f in feeds if f['name'] == 'routing api data')
            assert tc_data == {
                'id': 'builder',
                'name': 'routing api data',
                'license': 'ODBL',
                'url': 'www.canaltp.fr',
            }
            # we check that the feedpublisher of the bss provider has been added
            bss_provider = next(f for f in feeds if f['name'] == 'mock bss provider')
            assert bss_provider == {
                'id': 'mock_bss',
                'name': 'mock bss provider',
                'license': 'the death license',
                'url': 'bob.com',
            }

    def test_journey_sections_with_different_providers(self):
        """
        each poi is handled by a different bss provider, we should find both providers in the feed publishers
        """
        providers = [
            MockBssProvider(pois_supported='poi:station_1', name='provider 1'),
            MockBssProvider(pois_supported='poi:station_2', name='provider 2'),
        ]
        with mock.patch(
            'jormungandr.parking_space_availability.bss.BssProviderManager._get_providers',
            new_callable=PropertyMock,
            return_value=lambda: providers,
        ):
            query = (
                journey_basic_query + "&first_section_mode=bss&last_section_mode=bss&add_poi_infos[]=bss_stands"
            )
            response = self.query_region(query)
            self.is_valid_journey_response(response, query)

            # check the feed publishers
            # we have the feed publisher of the 'static' data
            feeds = get_not_null(response, 'feed_publishers')
            tc_data = next(f for f in feeds if f['name'] == 'routing api data')
            assert tc_data == {
                'id': 'builder',
                'name': 'routing api data',
                'license': 'ODBL',
                'url': 'www.canaltp.fr',
            }
            # we check that the feedpublisher of the bss providers has been added
            bss_provider_1 = next(f for f in feeds if f['name'] == 'provider 1')
            assert bss_provider_1 == {
                'id': 'mock_bss',
                'name': 'provider 1',
                'license': 'the death license',
                'url': 'bob.com',
            }
            bss_provider_2 = next(f for f in feeds if f['name'] == 'provider 2')
            assert bss_provider_2 == {
                'id': 'mock_bss',
                'name': 'provider 2',
                'license': 'the death license',
                'url': 'bob.com',
            }
