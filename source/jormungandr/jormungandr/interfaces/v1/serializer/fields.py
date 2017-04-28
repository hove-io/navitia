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

from __future__ import absolute_import, print_function, unicode_literals, division
from flask import g
from jormungandr.interfaces.v1.make_links import create_internal_link
from jormungandr.interfaces.v1.serializer.base import EnumField, PbNestedSerializer
import operator
import serpy


class MultiLineStringField(serpy.Field):

    def to_value(self, value):
        if getattr(g, 'disable_geojson', False):
            return None

        lines = []
        for l in value.lines:
            lines.append([[c.lon, c.lat] for c in l.coordinates])

        return {
            "type": "MultiLineString",
            "coordinates": lines,
        }


class PropertySerializer(serpy.Serializer):
    name = serpy.Field()
    value = serpy.Field()


class FeedPublisherSerializer(PbNestedSerializer):
    id = serpy.Field()
    name = serpy.Field()
    url = serpy.Field()
    license = serpy.Field()


class ErrorSerializer(PbNestedSerializer):
    id = EnumField(attr='id')
    message = serpy.Field()


class CoordSerializer(serpy.Serializer):
    lon = serpy.StrField()
    lat = serpy.StrField()


class CodeSerializer(serpy.Serializer):
    type = serpy.Field()
    value = serpy.Field()


class CommentSerializer(serpy.Serializer):
    value = serpy.Field()
    type = serpy.Field()


class FirstCommentField(serpy.Field):
    """
    for compatibility issue we want to continue to output a 'comment' field
    even if now we have a list of comments, so we take the first one
    """
    def as_getter(self, serializer_field_name, serializer_cls):
        op = operator.attrgetter(self.attr or serializer_field_name)
        return lambda v: next(iter(op(v)), None)

    def to_value(self, item):
        if item:
            return item.value
        else:
            return None


class LinkSerializer(serpy.Field):
    """
    Add link to disruptions on a pt object
    """
    def to_value(self, value):
        return [create_internal_link(_type="disruption", rel="disruptions", id=uri)
                for uri in value]


class PaginationSerializer(serpy.Serializer):
    total_result = serpy.Field(attr='totalResult')
    start_page = serpy.Field(attr='startPage')
    items_per_page = serpy.Field(attr='itemsPerPage')
    items_on_page = serpy.Field(attr='itemsOnPage')
