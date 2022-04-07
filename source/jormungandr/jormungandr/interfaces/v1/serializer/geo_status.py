# Copyright (c) 2001-2017, Hove and/or its affiliates. All rights reserved.
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
from jormungandr.interfaces.v1.serializer.jsonschema import Field
from jormungandr.interfaces.v1.serializer.pt import StringListField
import serpy


class GeoStatusSerializer(serpy.Serializer):
    nb_addresses = Field(schema_type=int)
    nb_admins = Field(schema_type=int)
    nb_admins_from_cities = Field(schema_type=int)
    nb_pois = Field(schema_type=int)
    nb_ways = Field(schema_type=int)
    poi_sources = StringListField(display_none=True)
    street_network_sources = StringListField(display_none=True)
