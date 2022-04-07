# Copyright (c) 2001-2019, Hove and/or its affiliates. All rights reserved.
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

from enum import Enum
from navitiacommon import response_pb2


class FallbackModes(Enum):
    walking = response_pb2.Walking
    bike = response_pb2.Bike
    bss = response_pb2.Bss
    car = response_pb2.Car
    car_no_park = response_pb2.CarNoPark
    ridesharing = response_pb2.Ridesharing
    taxi = response_pb2.Taxi

    @classmethod
    def modes_str(cls):
        return {e.name for e in cls}

    @classmethod
    def modes_enum(cls):
        return set(cls)

    @classmethod
    def get_allowed_combinations_enum(cls):
        def _combi(first_sections_modes, last_section_modes):
            from itertools import product

            # cartesian product between two iterables
            return set(product(first_sections_modes, last_section_modes))

        # this allowed combination is temporary, it does not handle all the use cases at all

        # non personal modes are modes that don't require any additional equipments
        # In principle, non personal modes are walking, bss, taxi and ridesharing, since we want to minimize the number
        # of ridesharing, we handle combinations with ridesharing manually
        # personal modes are bike and car, we don't allow to use personal modes in ending fallbacks, except bike bike.
        non_personal_modes = {cls.walking, cls.bss, cls.taxi}
        return (
            _combi(cls.modes_enum() - {cls.ridesharing}, non_personal_modes)
            # we remove bss walking and  walking bss, since they are redundant for kraken (bss includes walking)
            - {(cls.walking, cls.bss), (cls.bss, cls.walking)}
            # handle ridesharing manually, we allow only combinations between bss/walking/taxi with ridesharing
            # but we don't allow ridesharing ridesharing
            | _combi({cls.ridesharing}, {cls.bss, cls.walking, cls.taxi})
            | _combi({cls.bss, cls.walking, cls.taxi}, {cls.ridesharing})
            # we don't want use bike at the ending fallback except when the beginning fallback is also bike
            # this is because, when it's bike bike, we consider that the traveler want to get on the public transport
            # with his bike and kraken will take this into consideration
            | {(cls.bike, cls.bike)}
        )

    @classmethod
    def get_allowed_combinations_str(cls):
        # python 2/3 portability
        import six

        allowed_combinations_enum = cls.get_allowed_combinations_enum()
        # transform all enum to str
        return set(six.moves.map(lambda modes: (modes[0].name, modes[1].name), allowed_combinations_enum))


all_fallback_modes = FallbackModes.modes_str()

allowed_combinations = FallbackModes.get_allowed_combinations_str()
