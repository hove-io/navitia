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
from jormungandr.parking_space_availability.bss.bss_provider_manager import BssProviderManager
from jormungandr import app

CONFIG = ([
    {
        'class': 'jormungandr.parking_space_availability.bss.tests.BssMockProvider'
    }
])

def realtime_place_creation_test():
    """
    simple bss provider creation
    """
    manager = BssProviderManager(CONFIG)
    assert len(manager.bss_providers) == 1

def realtime_place_bad_creation_test():
    """
    simple bss provider bad creation
    """

    with pytest.raises(Exception):
        manager = BssProviderManager((
            {
                'class': 'jormungandr.parking_space_availability.bss.tests.BssMockProvider'
            },
            {
                'class': 'jormungandr.parking_space_availability.bss.BadProvider'
            }
        ))

def realtime_places_handle_test():
    """
    test correct handle places include bss stands
    """
    places = [
        {
            'embedded_type': 'poi',
            'distance': '0',
            'name': 'Cit\u00e9 Universitaire (Laval)',
            'poi': {
                'poi_type': {
                    'name': 'station vls',
                    'id': 'poi_type:amenity:bicycle_rental'
                },
                'id': 'station_1'
            },
            'quality': 0,
            'id': 'poi:n3762373698'
        }
    ]
    manager = BssProviderManager(CONFIG)
    providers = manager.handle_places(places)
    assert 'stands' in places[0]['poi']
    stands = places[0]['poi']['stands']
    assert stands.available_places == 5
    assert stands.available_bikes == 9
    assert stands.total_stands == 14
    assert stands.status == 'open'

    assert providers and len(providers) == 1
    f = providers.pop().feed_publisher()
    assert f and f.name == 'mock provider'


def realtime_pois_handle_test():
    """
    test correct handle pois include bss stands
    """
    pois = [
        {
            'poi_type': {
                'name': 'station vls',
                'id': 'poi_type:amenity:bicycle_rental'
            },
            'id': 'station_1'
        }
    ]
    manager = BssProviderManager(CONFIG)
    manager.handle_places(pois)
    assert 'stands' in pois[0]
    stands = pois[0]['stands']
    assert stands.available_places == 5
    assert stands.available_bikes == 9
    assert stands.total_stands == 14
    assert stands.status == 'open'


def realtime_poi_supported_handle_test():
    """
    test correct handle pois include bss stands
    """
    poi = {
        'poi_type': {
            'name': 'station vls',
            'id': 'poi_type:amenity:bicycle_rental'
        },
        'id': 'station_1'
    }
    manager = BssProviderManager(CONFIG)
    manager._handle_poi(poi)
    assert 'stands' in poi
    stands = poi['stands']
    assert stands.available_places == 5
    assert stands.available_bikes == 9
    assert stands.total_stands == 14
    assert stands.status == 'open'


def realtime_poi_not_supported_handle_test():
    """
    test correct handle pois include bss stands
    """
    poi = {
        'poi_type': {
            'name': 'station vls',
            'id': 'poi_type:amenity:bicycle_rental'
        },
        'id': 'station_2'
    }
    manager = BssProviderManager(CONFIG)
    manager._handle_poi(poi)
    assert 'stands' not in poi


def realtime_place_find_provider_test():
    """
    test manager return provider
    """
    poi = {
        'poi_type': {
            'name': 'station vls',
            'id': 'poi_type:amenity:bicycle_rental'
        },
        'id': 'station_1'
    }
    manager = BssProviderManager(CONFIG)
    provider = manager._find_provider(poi)
    assert provider == manager.bss_providers[0]


def realtime_journey_handle_test():
    """
    test that manager correctly populates "from" and "to" poi of a journey response
    """
    journeys = [
        {
            'sections': [
                {
                    'from': {
                        'embedded_type': 'poi',
                        'poi': {
                            'id': 'station_1',
                            'poi_type': {
                                'id': 'poi_type:amenity:bicycle_rental'
                            }
                        },
                        'id': 'station_1'
                    },
                    'to': {
                        'embedded_type': 'poi',
                        'poi': {
                            'id': 'station_1',
                            'poi_type': {
                                'id': 'poi_type:amenity:bicycle_rental'
                            }
                        },
                        'id': 'station_1'
                    }
                }
            ]
        }
    ]

    manager = BssProviderManager(CONFIG)
    manager.handle_journeys(journeys)
    journey_from = journeys[0]['sections'][0]['from']
    journey_to = journeys[0]['sections'][0]['to']
    assert 'stands' in journey_from['poi']
    assert 'stands' in journey_to['poi']

    assert journey_from['poi']['stands'].available_places == 5
    assert journey_from['poi']['stands'].available_bikes == 9
    assert journey_from['poi']['stands'].total_stands == 14
    assert journey_to['poi']['stands'].available_places == 5
    assert journey_to['poi']['stands'].available_bikes == 9
    assert journey_to['poi']['stands'].total_stands == 14
    assert journey_to['poi']['stands'].status == 'open'
