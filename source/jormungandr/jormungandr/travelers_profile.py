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
from jormungandr import app, cache, memory_cache
from jormungandr.utils import can_connect_to_database
from navitiacommon import models
from navitiacommon.default_traveler_profile_params import (
    default_traveler_profile_params,
    acceptable_traveler_types,
)
from six.moves import map
from six import text_type
from typing import Dict, Text, List, Optional
import logging


class TravelerProfile(object):
    def __init__(
        self,
        walking_speed=1.11,  # type: float
        bike_speed=3.33,  # type: float
        bss_speed=3.33,  # type: float
        car_speed=11.11,  # type: float
        max_duration_to_pt=None,  # type: Optional[int]
        first_section_mode=None,  # type: Optional[List[Text]]
        last_section_mode=None,  # type: Optional[List[Text]]
        wheelchair=False,  # type: bool
        first_and_last_section_mode=None,  # type: Optional[List[Text]]
        max_walking_duration_to_pt=15 * 60,  # type: int
        max_bike_duration_to_pt=15 * 60,  # type: int
        max_bss_duration_to_pt=15 * 60,  # type: int
        max_car_duration_to_pt=15 * 60,  # type: int
        traveler_type='',  # type: Text
        is_from_db=False,  # type: bool
    ):

        self.walking_speed = walking_speed
        self.bike_speed = bike_speed
        self.bss_speed = bss_speed
        self.car_speed = car_speed
        # Only used in v1/coverage/region/status, for debug purpose
        self.traveler_type = traveler_type
        # Only used in v1/coverage/region/status, for debug purpose
        self.is_from_db = is_from_db

        if max_duration_to_pt:
            self.max_walking_duration_to_pt = max_duration_to_pt
            self.max_bike_duration_to_pt = max_duration_to_pt
            self.max_bss_duration_to_pt = max_duration_to_pt
            self.max_car_duration_to_pt = max_duration_to_pt
        else:
            self.max_walking_duration_to_pt = max_walking_duration_to_pt
            self.max_bike_duration_to_pt = max_bike_duration_to_pt
            self.max_bss_duration_to_pt = max_bss_duration_to_pt
            self.max_car_duration_to_pt = max_car_duration_to_pt

        if first_and_last_section_mode:
            self.first_section_mode = self.last_section_mode = first_and_last_section_mode
        else:
            self.first_section_mode = first_section_mode or []
            self.last_section_mode = last_section_mode or []

        self.wheelchair = wheelchair

    def __repr__(self):
        return text_type(self.__dict__)

    def override_params(self, args):
        arg_2_profile_attr = (
            ('walking_speed', self.walking_speed),
            ('bike_speed', self.bike_speed),
            ('bss_speed', self.bss_speed),
            ('car_speed', self.car_speed),
            ('max_walking_duration_to_pt', self.max_walking_duration_to_pt),
            ('max_bike_duration_to_pt', self.max_bike_duration_to_pt),
            ('max_bss_duration_to_pt', self.max_bss_duration_to_pt),
            ('max_car_duration_to_pt', self.max_car_duration_to_pt),
            ('origin_mode', self.first_section_mode),
            ('destination_mode', self.last_section_mode),
            ('wheelchair', self.wheelchair),
        )

        def override(pair):
            arg, profile_attr = pair
            # we use profile's attr only when it's not defined in args(aka: the request)
            if args.get(arg) is None:
                args[arg] = profile_attr

        list(map(override, arg_2_profile_attr))

    @classmethod
    @memory_cache.memoize(app.config.get(str('MEMORY_CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_PARAMS'), 30))
    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_PARAMS'), 300))
    def make_traveler_profile(cls, coverage, traveler_type):
        # type: (Text, Text) -> TravelerProfile
        """
        travelers_profile factory method,
        Return a traveler_profile constructed with default params or params retrieved from db
        """
        if app.config[str('DISABLE_DATABASE')] or not can_connect_to_database():
            return default_traveler_profiles[traveler_type]

        # retrieve TravelerProfile from db with coverage and traveler_type
        # if the record doesn't exist, we use pre-defined default traveler profile
        try:
            model = models.TravelerProfile.get_by_coverage_and_type(coverage, traveler_type)
        except Exception as e:
            logging.getLogger(__name__).exception('No access to table traveler_profile (error: {})'.format(e))
            # If the table is not accessible return the default traveler profile for given traveler type
            return default_traveler_profiles[traveler_type]

        if model is None:
            return default_traveler_profiles[traveler_type]

        return cls(
            traveler_type=traveler_type,
            walking_speed=model.walking_speed,
            bike_speed=model.bike_speed,
            car_speed=model.car_speed,
            bss_speed=model.bss_speed,
            wheelchair=model.wheelchair,
            max_walking_duration_to_pt=model.max_walking_duration_to_pt,
            max_bss_duration_to_pt=model.max_bss_duration_to_pt,
            max_bike_duration_to_pt=model.max_bike_duration_to_pt,
            max_car_duration_to_pt=model.max_car_duration_to_pt,
            first_section_mode=model.first_section_mode,
            last_section_mode=model.last_section_mode,
            is_from_db=True,
        )

    @classmethod
    def get_profiles_by_coverage(cls, coverage):
        # type: (Text) -> List[TravelerProfile]
        traveler_profiles = []
        for traveler_type in acceptable_traveler_types:
            profile = cls.make_traveler_profile(coverage, traveler_type)
            traveler_profiles.append(profile)
        return traveler_profiles


default_traveler_profiles = {}  # type: Dict[Text, TravelerProfile]

for (traveler_type_id, params) in default_traveler_profile_params.items():
    default_traveler_profiles[traveler_type_id] = TravelerProfile(is_from_db=False, **params)  # type: ignore
