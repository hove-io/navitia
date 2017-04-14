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
from jormungandr.interfaces.v1.serializer.base import EnumField, PbNestedSerializer, DoubleToStringField
import operator
import serpy


class MultiLineStringField(serpy.Field):

    def _jsonschema_type_mapping(self):
        return {
            'type': 'object',
            'properties': {
                'type': {
                    'type': 'string',
                },
                'coordinates': {
                    'type': 'array',
                    'items': {
                        'type': 'array',
                        'minItems': 2,
                        'maxItems': 2,
                        'items': {
                            'type': 'number'
                        }
                    }
                }
            },
            'required': ['type', 'coordinates']
        }

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
    id = serpy.StrField()
    name = serpy.StrField()
    url = serpy.StrField()
    license = serpy.StrField()


class ErrorSerializer(PbNestedSerializer):
    id = EnumField(attr='id')
    message = serpy.StrField()


class CoordSerializer(serpy.Serializer):
    lon = DoubleToStringField()
    lat = DoubleToStringField()


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
    def _jsonschema_pre_type_mapping(self):
        return CommentSerializer(attr=self.attr, label=self.label, required=self.required, display_none=self.display_none)

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


class LinkSerializer(serpy.Field):
    """
    Add link to disruptions on a pt object
    """
    def _jsonschema_type_mapping(self):
        return {
            'type': 'array',
            'items': [
                {
                    'type': 'object',
                    'properties': {
                        'type': {
                            'type': 'string',
                        },
                        'rel': {
                            'type': 'string',
                        },
                        'templated': {
                            'type': 'boolean',
                        },
                        'href': {
                            'type': 'string',
                        },
                        'id': {
                            'type': 'string',
                        },
                        'description': {
                            'type': 'string',
                        },
                        'internal': {
                            'type': 'boolean',
                        },
                    },
                    'required': ['type', 'rel', 'templated']
                }
            ]
        }

    def to_value(self, value):
        return [create_internal_link(_type="disruption", rel="disruptions", id=uri)
                for uri in value]


class PaginationSerializer(serpy.Serializer):
    total_result = serpy.IntField(attr='totalResult')
    start_page = serpy.IntField(attr='startPage')
    items_per_page = serpy.IntField(attr='itemsPerPage')
    items_on_page = serpy.IntField(attr='itemsOnPage')
