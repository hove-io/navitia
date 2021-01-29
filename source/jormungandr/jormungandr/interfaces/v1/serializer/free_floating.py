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


class CoordSerializer(serpy.DictSerializer):
    lat = DoubleToStringField(attr='lat')
    lon = DoubleToStringField(attr='lat')


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


class FreeFloatingsSerializer(serpy.DictSerializer):
    free_floatings = jsonschema.MethodField(display_none=True)
    warnings = BetaEndpointsSerializer()

    def get_free_floatings(self, obj):
        res = []
        for free_floating in obj.get('free_floatings', []):
            res.append(FreeFloatingSerializer(free_floating).data)

        return res
