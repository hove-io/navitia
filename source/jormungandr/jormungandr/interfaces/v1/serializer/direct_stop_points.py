# Copyright (c) 2001-2026, Hove and/or its affiliates. All rights reserved.
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


accessible_stop_point_schema = {
    "type": "object",
    "properties": {
        "id": {"type": "string"},
    },
}

direct_stop_points_result_schema = {
    "type": "object",
    "properties": {
        "from_stop_point_id": {"type": "string"},
        "accessible_stop_points": {
            "type": "array",
            "items": accessible_stop_point_schema,
        },
    },
}


class AccessibleStopPointSerializer(serpy.DictSerializer):
    id = serpy.StrField(display_none=True)


class DirectStopPointsResultSerializer(serpy.DictSerializer):
    from_stop_point_id = serpy.StrField(display_none=True)
    accessible_stop_points = AccessibleStopPointSerializer(many=True, display_none=True)


class DirectStopPointsSerializer(serpy.DictSerializer):
    direct_stop_points = jsonschema.MethodField(
        schema_metadata=direct_stop_points_result_schema,
        display_none=True,
    )

    def get_direct_stop_points(self, obj):
        return DirectStopPointsResultSerializer(obj.get('direct_stop_points', {})).data
