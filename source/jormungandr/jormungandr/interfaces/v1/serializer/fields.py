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
import operator
import serpy
from flask import g
from jormungandr.interfaces.v1.make_links import create_internal_link
from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.serializer.base import EnumField, PbNestedSerializer, DoubleToStringField
from jormungandr.interfaces.v1.serializer.jsonschema import IntField
from jormungandr.interfaces.v1.serializer.jsonschema.fields import StrField, BoolField


class MultiLineStringField(jsonschema.Field):
    class MultiLineStringSchema(serpy.Serializer):
        """used not as a serializer, but only for the schema"""
        class Point2D(serpy.Serializer):
            pass  # TODO make this work :D
        type = StrField()
        coordinates = Point2D(many=True)

    def __init__(self, **kwargs):
        super(MultiLineStringField, self).__init__(schema_type=MultiLineStringField.MultiLineStringSchema,
                                                   **kwargs)

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
    name = jsonschema.Field(schema_type=str)
    value = jsonschema.Field(schema_type=str)


class FeedPublisherSerializer(PbNestedSerializer):
    id = StrField()
    name = StrField()
    url = StrField()
    license = StrField()


class ErrorSerializer(PbNestedSerializer):
    id = EnumField(attr='id')
    message = StrField()


class CoordSerializer(serpy.Serializer):
    lon = DoubleToStringField()
    lat = DoubleToStringField()


class CodeSerializer(serpy.Serializer):
    type = jsonschema.Field(schema_type=str)
    value = jsonschema.Field(schema_type=str)


class CommentSerializer(serpy.Serializer):
    value = jsonschema.Field(schema_type=str)
    type = jsonschema.Field(schema_type=str)


class FirstCommentField(jsonschema.Field):
    def __init__(self, **kwargs):
        super(FirstCommentField, self).__init__(schema_type=str, **kwargs)
    """
    for compatibility issue we want to continue to output a 'comment' field
    even if now we have a list of comments, so we take the first one
    """
    def as_getter(self, serializer_field_name, serializer_cls):
        op = operator.attrgetter(self.attr or serializer_field_name)
        def getter(v):
            return next(iter(op(v)), None)
        return getter

    def to_value(self, item):
        if item:
            return item.value
        else:
            return None


class LinkSchema(serpy.Serializer):
    """This Class is not used as a serializer, but here only to get the schema of a link"""
    id = StrField()
    title = StrField()
    rel = StrField()
    templated = BoolField()
    internal = BoolField()
    type = StrField()


class LinkSerializer(jsonschema.Field):
    """
    Add link to disruptions on a pt object
    """
    def __init__(self, **kwargs):
        super(LinkSerializer, self).__init__(schema_type=LinkSchema(), **kwargs)

    def to_value(self, value):
        return [create_internal_link(_type="disruption", rel="disruptions", id=uri)
                for uri in value]


class PaginationSerializer(serpy.Serializer):
    total_result = IntField(attr='totalResult')
    start_page = IntField(attr='startPage')
    items_per_page = IntField(attr='itemsPerPage')
    items_on_page = IntField(attr='itemsOnPage')
