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

from __future__ import absolute_import
import serpy
from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.serializer.base import (
    NestedPropertyField,
    BetaEndpointsSerializer,
    DoubleToStringField,
)
from jormungandr.scenarios.utils import free_floatings_type


coord_schema = {
    "type": "object",
    "properties": {"lat": {"type": "string"}, "lon": {"type": "string"}},
    "required": ["lat", "lon"],
}

free_floating_schema = {
    "type": "object",
    "properties": {
        "public_id": {"type": "string"},
        "provider_name": {"type": "string"},
        "id": {"type": "string"},
        "type": {"enum": free_floatings_type, "type": "string"},
        "propulsion": {"type": "string"},
        "battery": {"type": "integer"},
        "distance": {"type": "integer"},
        "deeplink": {"type": "string"},
        "coord": coord_schema,
    },
}

pagination_schema = {
    "type": "object",
    "properties": {
        "items_on_page": {"type": ["integer"]},
        "items_per_page": {"type": ["integer"]},
        "start_page": {"type": ["integer"]},
        "total_result": {"type": ["integer"]},
    },
}

free_floatings_schema = {"type": "array", "items": free_floating_schema}


class CoordSerializer(serpy.DictSerializer):
    lat = DoubleToStringField(attr='lat')
    lon = DoubleToStringField(attr='lon')


class FreeFloatingSerializer(serpy.DictSerializer):
    public_id = NestedPropertyField(attr='public_id')
    provider_name = NestedPropertyField(attr='provider_name')
    id = NestedPropertyField(attr='id')
    type = NestedPropertyField(attr='type')
    attributes = NestedPropertyField(attr='attributes')
    propulsion = NestedPropertyField(attr='propulsion')
    battery = NestedPropertyField(attr='battery')
    deeplink = NestedPropertyField(attr='deeplink')
    coord = CoordSerializer(attr='coord')
    distance = NestedPropertyField(attr='distance')


class PaginationSerializer(serpy.DictSerializer):
    start_page = NestedPropertyField(attr='start_page')
    items_on_page = NestedPropertyField(attr='items_on_page')
    items_per_page = NestedPropertyField(attr='items_per_page')
    total_result = NestedPropertyField(attr='total_result')


class FreeFloatingsSerializer(serpy.DictSerializer):
    free_floatings = jsonschema.MethodField(schema_metadata=free_floatings_schema, display_none=True)
    pagination = jsonschema.MethodField(schema_metadata=pagination_schema, display_none=True)
    warnings = BetaEndpointsSerializer()

    def get_free_floatings(self, obj):
        return [FreeFloatingSerializer(ff).data for ff in obj.get('free_floatings', [])]

    def get_pagination(self, obj):
        return PaginationSerializer(obj.get('pagination')).data
