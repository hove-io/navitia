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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from enum import Enum


class FallbackModes(Enum):
    walking = 0
    bike = 1
    bss = 2
    car = 3
    ridesharing = 4
    taxi = 5

    @classmethod
    def get_all_modes_str(cls):
        return [e.name for e in cls]

    @classmethod
    def get_all_modes_enum(cls):
        return list(cls)

    @classmethod
    def get_allowed_combinations_enum(cls):
        def sub_combi(first_sections_modes, last_section_modes):
            from itertools import product

            return set(product(first_sections_modes, last_section_modes))

        # this allowed combination is temporary, it does not handle all the use cases at all

        allowed_combinations = (
            sub_combi([cls.walking], [cls.walking, cls.ridesharing])
            | sub_combi([cls.bike], [cls.walking, cls.bike, cls.bss])
            | sub_combi([cls.bss], [cls.bss, cls.ridesharing])
            | sub_combi([cls.car], [cls.walking, cls.bss])
            | sub_combi([cls.ridesharing], [cls.walking, cls.bss])
            | sub_combi([cls.taxi], cls.get_all_modes_enum())
            | sub_combi(cls.get_all_modes_enum(), [cls.taxi])
        )
        return allowed_combinations

    @classmethod
    def get_allowed_combinations_str(cls):
        # python 2/3 portability
        import six

        allowed_combinations_enum = cls.get_allowed_combinations_enum()
        # transform all enum to str
        return set(six.moves.map(lambda modes: (modes[0].name, modes[1].name), allowed_combinations_enum))


all_fallback_modes = FallbackModes.get_all_modes_str()

allowed_combinations = FallbackModes.get_allowed_combinations_str()
