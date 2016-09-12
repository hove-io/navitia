#  Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
#
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
import serpy
from jormungandr.interfaces.v1.serializer.base import PbNestedSerializer, GenericSerializer, EnumField
from jormungandr.interfaces.v1.serializer.fields import *


class ChannelSerializer(PbNestedSerializer):
    content_type = serpy.Field()
    id = serpy.Field()
    name = serpy.Field()
    #types = EnumField(attr='channel_types')


class MessageSerializer(PbNestedSerializer):
    text = serpy.Field()
    channel = ChannelSerializer()


class DisruptionSerializer(PbNestedSerializer):
    id = serpy.Field(attr='uri')
    disruption_id = serpy.Field(attr='disruption_uri')
    impact_id = serpy.Field(attr='uri')
    title = serpy.Field(),
    #application_periods": NonNullList(NonNullNested(period))
    status = EnumField(attr='status')
    #updated_at": DateTime()
    #tags = serpy.Serializer(many=True)
    cause = serpy.Field()
    #severity": NonNullNested(disruption_severity)
    messages = MessageSerializer(many=True)
    #impacted_objects": NonNullList(NonNullNested(impacted_object))
    uri = serpy.Field(attr='uri')
    disruption_uri = serpy.Field()
    contributor = serpy.Field()


class AdminSerializer(GenericSerializer):
    level = serpy.Field()
    zip_code = serpy.Field()
    label = serpy.Field()
    insee = serpy.Field()
    coord = CoordSerializer(required=False)


class PhysicalModeSerializer(GenericSerializer):
    commercial_modes = serpy.MethodField()
    def get_commercial_modes(self, obj):
        return CommercialModeSerializer(obj.commercial_modes, many=True, display_none=False).data

class CommercialModeSerializer(GenericSerializer):
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)


class StopAreaSerializer(GenericSerializer):
    comments = CommentSerializer(many=True)
    comment = FirstCommentField(attr='comments', display_none=False)
    codes = CodeSerializer(many=True)
    timezone = serpy.Field()
    label = serpy.Field()
    coord = CoordSerializer(required=False)
    links = LinkSerializer(attr='impact_uris')
    commercial_modes = CommercialModeSerializer(many=True, display_none=False)
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)
    administrative_regions = AdminSerializer(many=True, display_none=False)
    #stop_points


class PlaceSerializer(serpy.Serializer):
    id = serpy.Field(attr='uri')
    name = serpy.Field()
    quality = serpy.Field(required=False)
    stop_area = StopAreaSerializer(display_none=False)
    embedded_type = EnumField(attr='embedded_type')

#place = {
#    "stop_point": PbField(stop_point),
#    "stop_area": PbField(stop_area),
#    "address": PbField(address),
#    "poi": PbField(poi),
#    "administrative_region": PbField(admin),
#    "embedded_type": enum_type(),
#}
class NetworkSerializer(GenericSerializer):
    lines = serpy.MethodField(display_none=False)
    links = LinkSerializer(attr='impact_uris')
    codes = CodeSerializer(many=True, display_none=False)

    def get_lines(self, obj):
        return LineSerializer(obj.lines, many=True, display_none=False).data

class RouteSerializer(GenericSerializer):
    is_frequence = serpy.StrField()
    direction_type = serpy.Field()
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)
    comments = CommentSerializer(many=True)
    codes = CodeSerializer(many=True, display_none=False)
    direction = PlaceSerializer()
    geojson = MultiLineStringField(display_none=False)
    links = LinkSerializer(attr='impact_uris')
    line = serpy.MethodField(display_none=False)
#route["stop_points"] = NonNullList(NonNullNested(stop_point))

    def get_line(self, obj):
        if obj.HasField(str('line')):
            return LineSerializer(obj.line).data
        else:
            return None

class LineGroupSerializer(GenericSerializer):
    lines = serpy.MethodField(display_none=False)
    main_line = serpy.MethodField(display_none=False)
    comments = CommentSerializer(many=True)

    def get_lines(self, obj):
        return LineSerializer(obj.lines, many=True, display_none=False).data

    def get_main_line(self, obj):
        if obj.HasField(str('main_line')):
            return LineSerializer(obj.main_line).data
        else:
            return None

class LineSerializer(GenericSerializer):
    code = serpy.Field()
    color = serpy.Field()
    text_color = serpy.Field()
    comments = CommentSerializer(many=True, display_none=False)
    comment = FirstCommentField(attr='comments', display_none=False)
    codes = CodeSerializer(many=True, required=False)
    physical_modes = PhysicalModeSerializer(many=True)
    commercial_mode = CommercialModeSerializer(display_none=False)
    routes = RouteSerializer(many=True)
    network = NetworkSerializer()
    opening_time = LocalTimeField()
    closing_time = LocalTimeField()
    properties = PropertySerializer(many=True, display_none=False)
    geojson = MultiLineStringField(display_none=False)
    links = LinkSerializer(attr='impact_uris')
    line_groups = LineGroupSerializer(many=True, display_none=False)


