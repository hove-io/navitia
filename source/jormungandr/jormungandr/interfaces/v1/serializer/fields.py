# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
import operator
import serpy
from flask import g
from jormungandr.interfaces.v1.make_links import create_internal_link
from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.serializer.base import EnumField, PbNestedSerializer, DoubleToStringField
from jormungandr.interfaces.v1.serializer.jsonschema import IntField
from jormungandr.interfaces.v1.serializer.jsonschema.fields import StrField, BoolField, Field, DateTimeType
from jormungandr.utils import get_pt_object_coord
from jormungandr.exceptions import UnableToParse, InvalidArguments
from navitiacommon import response_pb2

point_2D_schema = {'type': 'array', 'items': {'type': 'array', 'items': {'type': 'number', 'format': 'float'}}}

properties_schema = {
    'type': 'array',
    'items': {'properties': {'length': {'type': 'number', 'format': 'integer'}}},
}


class MultiLineStringField(jsonschema.Field):
    class MultiLineStringSchema(serpy.Serializer):
        """used not as a serializer, but only for the schema"""

        type = StrField()
        coordinates = jsonschema.Field(schema_metadata={'type': 'array', 'items': point_2D_schema})

    def __init__(self, **kwargs):
        super(MultiLineStringField, self).__init__(
            schema_type=MultiLineStringField.MultiLineStringSchema, **kwargs
        )

    def to_value(self, value):
        if getattr(g, 'disable_geojson', False):
            return None

        lines = []
        for l in value.lines:
            lines.append([[c.lon, c.lat] for c in l.coordinates])

        return {"type": "MultiLineString", "coordinates": lines}


class PropertySerializer(serpy.Serializer):
    name = jsonschema.Field(schema_type=str)
    value = jsonschema.Field(schema_type=str)


class FeedPublisherSerializer(PbNestedSerializer):
    id = StrField(display_none=True)
    name = StrField()
    url = StrField()
    license = StrField()


class ErrorSerializer(PbNestedSerializer):
    id = EnumField(attr='id', display_none=True)
    message = StrField()


class CoordSerializer(serpy.Serializer):
    lon = DoubleToStringField(display_none=True, required=True)
    lat = DoubleToStringField(display_none=True, required=True)


class CodeSerializer(serpy.Serializer):
    type = jsonschema.Field(schema_type=str)
    value = jsonschema.Field(schema_type=str)


class CommentSerializer(serpy.Serializer):
    value = jsonschema.Field(schema_type=str)
    type = jsonschema.Field(schema_type=str)


class FareZoneSerializer(serpy.Serializer):
    name = jsonschema.Field(schema_type=str)


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


class RoundedField(IntField):
    def to_value(self, value):
        if value is None:
            return None
        return int(round(value))


class LinkSchema(serpy.Serializer):
    """This Class is not used as a serializer, but here only to get the schema of a link"""

    id = StrField()
    title = StrField()
    rel = StrField()
    templated = BoolField()
    internal = BoolField()
    type = StrField()
    href = StrField()
    value = StrField()
    category = StrField()
    comment_type = StrField()


class DisruptionLinkSerializer(jsonschema.Field):
    """
    Add link to disruptions on a pt object
    """

    def __init__(self, **kwargs):
        super(DisruptionLinkSerializer, self).__init__(schema_type=LinkSchema(many=True), **kwargs)

    def to_value(self, value):
        return [create_internal_link(_type="disruption", rel="disruptions", id=uri) for uri in value]


class PaginationSerializer(serpy.Serializer):
    total_result = IntField(attr='totalResult', display_none=True)
    start_page = IntField(attr='startPage', display_none=True)
    items_per_page = IntField(attr='itemsPerPage', display_none=True)
    items_on_page = IntField(attr='itemsOnPage', display_none=True)


class SectionGeoJsonField(jsonschema.Field):
    class SectionGeoJsonSchema(serpy.Serializer):
        """used not as a serializer, but only for the schema"""

        type = StrField()
        properties = jsonschema.Field(schema_metadata=properties_schema)
        coordinates = jsonschema.Field(schema_metadata=point_2D_schema)

    def __init__(self, **kwargs):
        super(SectionGeoJsonField, self).__init__(schema_type=SectionGeoJsonField.SectionGeoJsonSchema, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        def getter(v):
            return v

        return getter

    def to_value(self, value):
        if not hasattr(value, 'type'):
            return None

        coords = []

        try:
            if value.type == response_pb2.STREET_NETWORK:
                coords = value.street_network.coordinates
            elif value.type == response_pb2.CROW_FLY:
                if len(value.shape) != 0:
                    coords = value.shape
                else:
                    coords.append(get_pt_object_coord(value.origin))
                    coords.append(get_pt_object_coord(value.destination))
            elif value.type == response_pb2.RIDESHARING and len(value.shape) != 0:
                coords = value.shape
            elif value.type == response_pb2.PUBLIC_TRANSPORT:
                coords = value.shape
            elif value.type == response_pb2.TRANSFER:
                if value.street_network and value.street_network.coordinates:
                    coords = value.street_network.coordinates
                else:
                    coords.append(value.origin.stop_point.coord)
                    coords.append(value.destination.stop_point.coord)
            else:
                return
        except UnableToParse:
            return
        except InvalidArguments:
            return

        response = {
            "type": "LineString",
            "coordinates": [],
            "properties": [{"length": 0 if not value.HasField(str("length")) else value.length}],
        }
        for coord in coords:
            response["coordinates"].append([coord.lon, coord.lat])
        return response


class NoteSerializer(serpy.Serializer):
    type = jsonschema.Field(schema_type=str)
    id = jsonschema.Field(schema_type=str, display_none=True)
    value = jsonschema.Field(schema_type=str)
    category = jsonschema.Field(schema_type=str, schema_metadata={'enum': ['comment', 'terminus']})
    comment_type = jsonschema.Field(schema_type=str, display_none=False)


class ExceptionSerializer(serpy.Serializer):
    type = jsonschema.Field(schema_type=str)
    id = jsonschema.Field(schema_type=str, display_none=True)
    date = Field(attr='date', schema_type=DateTimeType)
