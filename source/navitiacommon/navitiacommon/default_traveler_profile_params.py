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


default_traveler_profile_params = {
    'standard': {
        'traveler_type': 'standard',
        'walking_speed': 1.11,
        'bike_speed': 3.33,
        'bss_speed': 3.33,
        'car_speed': 11.11,
        'max_walking_duration_to_pt': 20 * 60,
        'max_bike_duration_to_pt': 20 * 60,
        'max_bss_duration_to_pt': 20 * 60,
        'max_car_duration_to_pt': 20 * 60,
        'first_section_mode': ['walking', 'bss', 'bike'],
        'last_section_mode': ['walking', 'bss', 'bike'],
        'walking_step_penalty': 30.0,
        'walking_use_hills': 0.5,
        'max_walking_direct_path_duration': 24 * 60 * 60,
        'max_bike_direct_path_duration': 24 * 60 * 60,
        'max_bss_direct_path_duration': 24 * 60 * 60,
        'max_car_direct_path_duration': 24 * 60 * 60,
        'max_ridesharing_direct_path_duration': 24 * 60 * 60,
    },
    'slow_walker': {
        'traveler_type': 'slow_walker',
        'walking_speed': 0.83,
        'bike_speed': 2.77,
        'bss_speed': 2.77,
        'car_speed': 11.11,
        'max_walking_duration_to_pt': 20 * 60,
        'max_bike_duration_to_pt': 20 * 60,
        'max_bss_duration_to_pt': 20 * 60,
        'max_car_duration_to_pt': 20 * 60,
        'first_section_mode': ['walking', 'bss', 'bike'],
        'last_section_mode': ['walking', 'bss', 'bike'],
        'walking_step_penalty': 30.0,
        'walking_use_hills': 0.5,
        'max_walking_direct_path_duration': 24 * 60 * 60,
        'max_bike_direct_path_duration': 24 * 60 * 60,
        'max_bss_direct_path_duration': 24 * 60 * 60,
        'max_car_direct_path_duration': 24 * 60 * 60,
        'max_ridesharing_direct_path_duration': 24 * 60 * 60,
    },
    'fast_walker': {
        'traveler_type': 'fast_walker',
        'walking_speed': 1.67,
        'bike_speed': 4.16,
        'bss_speed': 4.16,
        'car_speed': 11.11,
        'max_walking_duration_to_pt': 20 * 60,
        'max_bike_duration_to_pt': 20 * 60,
        'max_bss_duration_to_pt': 20 * 60,
        'max_car_duration_to_pt': 20 * 60,
        'first_section_mode': ['walking', 'bss', 'bike'],
        'last_section_mode': ['walking', 'bss', 'bike'],
        'walking_step_penalty': 30.0,
        'walking_use_hills': 0.5,
        'max_walking_direct_path_duration': 24 * 60 * 60,
        'max_bike_direct_path_duration': 24 * 60 * 60,
        'max_bss_direct_path_duration': 24 * 60 * 60,
        'max_car_direct_path_duration': 24 * 60 * 60,
        'max_ridesharing_direct_path_duration': 24 * 60 * 60,
    },
    'luggage': {
        'traveler_type': 'luggage',
        'walking_speed': 1.11,
        'bike_speed': 3.33,
        'bss_speed': 3.33,
        'car_speed': 11.11,
        'max_walking_duration_to_pt': 20 * 60,
        'max_bike_duration_to_pt': 20 * 60,
        'max_bss_duration_to_pt': 20 * 60,
        'max_car_duration_to_pt': 20 * 60,
        'first_section_mode': ['walking'],
        'last_section_mode': ['walking'],
        'wheelchair': True,
        'walking_step_penalty': 3600.0,
        'walking_use_hills': 0,
        'max_walking_direct_path_duration': 24 * 60 * 60,
        'max_bike_direct_path_duration': 24 * 60 * 60,
        'max_bss_direct_path_duration': 24 * 60 * 60,
        'max_car_direct_path_duration': 24 * 60 * 60,
        'max_ridesharing_direct_path_duration': 24 * 60 * 60,
    },
    'wheelchair': {
        'traveler_type': 'wheelchair',
        'walking_speed': 0.83,
        'bike_speed': 3.33,
        'bss_speed': 3.33,
        'car_speed': 11.11,
        'max_walking_duration_to_pt': 12 * 60,
        'max_bike_duration_to_pt': 12 * 60,
        'max_bss_duration_to_pt': 12 * 60,
        'max_car_duration_to_pt': 12 * 60,
        'first_section_mode': ['walking'],
        'last_section_mode': ['walking'],
        'wheelchair': True,
        'walking_step_penalty': 3600.0,
        'walking_use_hills': 0,
        'max_walking_direct_path_duration': 24 * 60 * 60,
        'max_bike_direct_path_duration': 24 * 60 * 60,
        'max_bss_direct_path_duration': 24 * 60 * 60,
        'max_car_direct_path_duration': 24 * 60 * 60,
        'max_ridesharing_direct_path_duration': 24 * 60 * 60,
    },
    # Temporary Profile
    'cyclist': {
        'traveler_type': 'cyclist',
        'walking_speed': 1.11,
        'bike_speed': 3.33,
        'bss_speed': 3.33,
        'car_speed': 11.11,
        'max_walking_duration_to_pt': 20 * 60,
        'max_bike_duration_to_pt': 20 * 60,
        'max_bss_duration_to_pt': 20 * 60,
        'max_car_duration_to_pt': 20 * 60,
        'first_section_mode': ['walking', 'bss', 'bike'],
        'last_section_mode': ['walking', 'bss'],
        'walking_step_penalty': 30.0,
        'walking_use_hills': 0.5,
        'max_walking_direct_path_duration': 24 * 60 * 60,
        'max_bike_direct_path_duration': 24 * 60 * 60,
        'max_bss_direct_path_duration': 24 * 60 * 60,
        'max_car_direct_path_duration': 24 * 60 * 60,
        'max_ridesharing_direct_path_duration': 24 * 60 * 60,
    },
    # Temporary Profile
    'motorist': {
        'traveler_type': 'motorist',
        'walking_speed': 1.11,
        'bike_speed': 3.33,
        'bss_speed': 3.33,
        'car_speed': 11.11,
        'max_walking_duration_to_pt': 15 * 60,
        'max_bike_duration_to_pt': 20 * 60,
        'max_bss_duration_to_pt': 20 * 60,
        'max_car_duration_to_pt': 35 * 60,
        'first_section_mode': ['walking', 'car'],
        'last_section_mode': ['walking'],
        'walking_step_penalty': 30.0,
        'walking_use_hills': 0.5,
        'max_walking_direct_path_duration': 24 * 60 * 60,
        'max_bike_direct_path_duration': 24 * 60 * 60,
        'max_bss_direct_path_duration': 24 * 60 * 60,
        'max_car_direct_path_duration': 24 * 60 * 60,
        'max_ridesharing_direct_path_duration': 24 * 60 * 60,
    },
}

acceptable_traveler_types = default_traveler_profile_params.keys()
