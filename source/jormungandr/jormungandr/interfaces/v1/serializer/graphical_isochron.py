# coding: utf-8

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
from jormungandr.interfaces.v1.serializer.pt import PlaceSerializer
from jormungandr.interfaces.v1.serializer.time import DateTimeField
from jormungandr.interfaces.v1.serializer.jsonschema import JsonStrField
from jormungandr.interfaces.v1.serializer.fields import point_2D_schema, Field

import serpy


class GraphicalIsrochoneSerializer(serpy.Serializer):

    geojson = JsonStrField(
        schema_metadata={
            'type': 'object',
            'properties': {
                'type': {
                    # Must be MultiPolygon
                    'enum': ['MultiPolygon']
                },
                'coordinates': {'type': 'array', 'items': {'type': 'array', 'items': point_2D_schema}},
            },
        }
    )
    max_duration = Field(schema_type=int)
    min_duration = Field(schema_type=int)
    origin = PlaceSerializer(label='from')
    to = PlaceSerializer(attr='destination', label='to')
    requested_date_time = DateTimeField()
    min_date_time = DateTimeField()
    max_date_time = DateTimeField()
