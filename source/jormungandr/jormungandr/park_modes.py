# Copyright (c) 2001-2024, Hove and/or its affiliates. All rights reserved.
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
#
#


from enum import Enum
from navitiacommon import response_pb2


class ParkMode(Enum):
    none = response_pb2.NONE
    on_street = response_pb2.OnStreet
    park_and_ride = response_pb2.ParkAndRide

    @classmethod
    def modes_str(cls):

        return {e.name for e in cls}

    @classmethod
    def modes_enum(cls):
        return set(cls)

    @classmethod
    def get_allowed_combinations_enums(cls):
        def _combi(first_sections_modes, last_section_modes):
            from itertools import product

            # cartesian product between two iterables
            return set(product(first_sections_modes, last_section_modes))

        return _combi(cls.modes_enum(), cls.modes_enum())

    @classmethod
    def get_allowed_combinations_str(cls):
        # python 2/3 portability
        import six

        allowed_combinations_enum = cls.get_allowed_combinations_enums()
        # transform all enum to str
        return set(six.moves.map(lambda modes: (modes[0].name, modes[1].name), allowed_combinations_enum))


all_park_modes = ParkMode.modes_str()
