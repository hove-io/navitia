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

from unittest.mock import patch
from jormungandr.travelers_profile import TravelerProfile, default_traveler_profiles
from jormungandr import cache
from navitiacommon.default_traveler_profile_params import acceptable_traveler_types
from six.moves import map


def test_get_traveler_profile_and_override():
    """
    Test traveler profile's factory method make_traveler_profile and override_params

    when overriding args, only non-defined args will be overrided.

    """
    region = 'default'
    traveler_type = 'standard'
    traveler_profile = TravelerProfile.make_traveler_profile(region, traveler_type)

    assert traveler_profile.walking_speed == 1.11
    assert traveler_profile.bike_speed == 3.33
    assert traveler_profile.walking_use_hills == 0.5
    assert traveler_profile.walking_step_penalty == 30.0

    args = {'walking_speed': 42424242, 'bike_speed': 42424242, 'walking_step_penalty': 33.0}
    traveler_profile.override_params(args)

    assert args['walking_speed'] == 42424242
    assert args['bike_speed'] == 42424242
    assert args['walking_use_hills'] == 0.5
    assert args['walking_step_penalty'] == 33.0

    # Attributes modified in args above are absent in arg_vs_profile_attr
    arg_vs_profile_attr = (
        ('bss_speed', 'bss_speed'),
        ('car_speed', 'car_speed'),
        ('max_walking_duration_to_pt', 'max_walking_duration_to_pt'),
        ('max_bike_duration_to_pt', 'max_bike_duration_to_pt'),
        ('max_bss_duration_to_pt', 'max_bss_duration_to_pt'),
        ('max_car_duration_to_pt', 'max_car_duration_to_pt'),
        ('origin_mode', 'first_section_mode'),
        ('destination_mode', 'last_section_mode'),
        ('wheelchair', 'wheelchair'),
        ('walking_use_hills', 'walking_use_hills'),
        ('max_walking_direct_path_duration', 'max_walking_direct_path_duration'),
        ('max_bike_direct_path_duration', 'max_bike_direct_path_duration'),
        ('max_bss_direct_path_duration', 'max_bss_direct_path_duration'),
        ('max_car_direct_path_duration', 'max_car_direct_path_duration'),
        ('max_ridesharing_direct_path_duration', 'max_ridesharing_direct_path_duration'),
    )

    standard_profile = default_traveler_profiles['standard']

    def check(arg_attr):
        (arg, attr) = arg_attr
        assert args[arg] == getattr(standard_profile, attr)

    list(map(check, arg_vs_profile_attr))


def test_make_profile_cache_decorator():
    cache.delete_memoized(TravelerProfile.make_traveler_profile)

    region = 'default'
    traveler_type = 'standard'
    traveler_profile_1 = TravelerProfile.make_traveler_profile(region, traveler_type)
    traveler_profile_2 = TravelerProfile.make_traveler_profile(region, traveler_type)

    assert traveler_profile_1 is traveler_profile_2


def test_get_profiles_by_coverage_none_fallback():
    """
    When make_traveler_profile returns None (e.g. corrupted cache entry),
    get_profiles_by_coverage should fall back to default profiles instead of
    including None in the result list.
    """
    with patch.object(TravelerProfile, 'make_traveler_profile', return_value=None):
        profiles = TravelerProfile.get_profiles_by_coverage('default')

    assert len(profiles) == len(acceptable_traveler_types)

    for profile in profiles:
        assert profile is not None
        assert isinstance(profile, TravelerProfile)
        # Verify we can access attributes without AttributeError
        assert profile.bike_speed is not None
