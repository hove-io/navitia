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
from jormungandr.interfaces.v1.serializer.base import GenericSerializer, EnumListField
from jormungandr.interfaces.v1.serializer.time import LocalTimeField, PeriodSerializer, DateTimeField
from jormungandr.interfaces.v1.serializer.fields import *


class Equipments(EnumListField):
    """
    hack for equiments their is a useless level in the proto
    """
    def as_getter(self, serializer_field_name, serializer_cls):
        #For enum we need the full object :(
        return lambda x: x.has_equipments


class ChannelSerializer(PbNestedSerializer):
    content_type = serpy.Field()
    id = serpy.Field()
    name = serpy.Field()
    types = EnumListField(attr='channel_types')


class MessageSerializer(PbNestedSerializer):
    text = serpy.Field()
    channel = ChannelSerializer()


class SeveritySerializer(PbNestedSerializer):
    name = serpy.Field()
    effect = serpy.Field()
    color = serpy.Field()
    priority = serpy.Field()


class PtObjectSerializer(GenericSerializer):
    quality = serpy.Field(required=False)
    stop_area = serpy.MethodField(display_none=False)
    line = serpy.MethodField(display_none=False)
    network = serpy.MethodField(display_none=False)
    route = serpy.MethodField(display_none=False)
    commercial_mode = serpy.MethodField(display_none=False)
    trip = serpy.MethodField(display_none=False)
    embedded_type = EnumField(attr='embedded_type')

    def get_trip(self, obj):
        if obj.HasField(str('trip')):
            return TripSerializer(obj.trip, display_none=False).data
        else:
            return None

    def get_commercial_mode(self, obj):
        if obj.HasField(str('commercial_mode')):
            return CommercialModeSerializer(obj.commercial_mode, display_none=False).data
        else:
            return None

    def get_route(self, obj):
        if obj.HasField(str('route')):
            return RouteSerializer(obj.route, display_none=False).data
        else:
            return None

    def get_network(self, obj):
        if obj.HasField(str('network')):
            return NetworkSerializer(obj.network, display_none=False).data
        else:
            return None

    def get_line(self, obj):
        if obj.HasField(str('line')):
            return LineSerializer(obj.line, display_none=False).data
        else:
            return None

    def get_stop_area(self, obj):
        if obj.HasField(str('stop_area')):
            return StopAreaSerializer(obj.stop_area, display_none=False).data
        else:
            return None


class TripSerializer(GenericSerializer):
    pass#@TODO


class ImpactedStopSerializer(PbNestedSerializer):
    stop_point = serpy.MethodField(display_none=False)
    base_arrival_time = LocalTimeField(attr='base_stop_time.arrival_time')
    base_departure_time = LocalTimeField(attr='base_stop_time.departure_time')
    amended_arrival_time = LocalTimeField(attr='amended_stop_time.arrival_time')
    amended_departure_time = LocalTimeField(attr='amended_stop_time.departure_time')
    cause = serpy.Field()
    stop_time_effect = EnumField(attr='effect')

    def get_stop_point(self, obj):
        if obj.HasField(str('stop_point')):
            return StopPointSerializer(obj.stop_point, display_none=False).data
        else:
            return None


class ImpactedSectionSerializer(PbNestedSerializer):
    f = PtObjectSerializer(label='from', attr='from')
    to = PtObjectSerializer()


class ImpactedSerializer(PbNestedSerializer):
    pt_object = PtObjectSerializer(display_none=False)
    impacted_stops = ImpactedStopSerializer(many=True, display_none=False)
    impacted_section = ImpactedSectionSerializer(display_none=False)


class DisruptionSerializer(PbNestedSerializer):
    id = serpy.Field(attr='uri')
    disruption_id = serpy.Field(attr='disruption_uri')
    impact_id = serpy.Field(attr='uri')
    title = serpy.Field(),
    application_periods = PeriodSerializer(many=True)
    status = EnumField(attr='status')
    updated_at = DateTimeField()
    tags = serpy.Serializer(many=True, display_none=False)
    cause = serpy.Field()
    category = serpy.Field(display_none=False)
    severity = SeveritySerializer()
    messages = MessageSerializer(many=True)
    impacted_objects = ImpactedSerializer(many=True, display_none=False)
    uri = serpy.Field(attr='uri')
    disruption_uri = serpy.Field()
    contributor = serpy.Field()


class AdminSerializer(GenericSerializer):
    level = serpy.Field()
    zip_code = serpy.Field()
    label = serpy.Field()
    insee = serpy.Field()
    coord = CoordSerializer(required=False)


class AddressSerializer(GenericSerializer):
    house_number = serpy.Field()
    coord = CoordSerializer(required=False, display_none=False)
    label = serpy.Field()
    administrative_regions = AdminSerializer(many=True, display_none=False)


class PhysicalModeSerializer(GenericSerializer):
    pass


class CommercialModeSerializer(GenericSerializer):
    pass


class StopPointSerializer(GenericSerializer):
    comments = CommentSerializer(many=True, display_none=False)
    comment = FirstCommentField(attr='comments', display_none=False)
    codes = CodeSerializer(many=True)
    label = serpy.Field()
    coord = CoordSerializer(required=False)
    links = LinkSerializer(attr='impact_uris')
    commercial_modes = CommercialModeSerializer(many=True, display_none=False)
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)
    administrative_regions = AdminSerializer(many=True, display_none=False)
    stop_area = serpy.MethodField(display_none=False)
    equipments = Equipments(attr='has_equipments')
    address = AddressSerializer(display_none=False)

    def get_stop_area(self, obj):
        if obj.HasField(str('stop_area')):
            return StopAreaSerializer(obj.stop_area, display_none=False).data
        else:
            return None


class StopAreaSerializer(GenericSerializer):
    comments = CommentSerializer(many=True, display_none=False)
    comment = FirstCommentField(attr='comments', display_none=False)
    codes = CodeSerializer(many=True)
    timezone = serpy.Field()
    label = serpy.Field()
    coord = CoordSerializer(required=False)
    links = LinkSerializer(attr='impact_uris')
    commercial_modes = CommercialModeSerializer(many=True, display_none=False)
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)
    administrative_regions = AdminSerializer(many=True, display_none=False)
    stop_points = StopPointSerializer(many=True, display_none=False)


class PlaceSerializer(GenericSerializer):
    id = serpy.Field(attr='uri')
    name = serpy.Field()
    quality = serpy.Field(required=False)
    stop_area = StopAreaSerializer(display_none=False)
    stop_point = StopPointSerializer(display_none=False)
    administrative_region = AdminSerializer(display_none=False)
    embedded_type = EnumField(attr='embedded_type')
#    @TODO "address": PbField(address),
#    @TODO "poi": PbField(poi),


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
    comments = CommentSerializer(many=True, display_none=False)
    codes = CodeSerializer(many=True, display_none=False)
    direction = PlaceSerializer()
    geojson = MultiLineStringField(display_none=False)
    links = LinkSerializer(attr='impact_uris')
    line = serpy.MethodField(display_none=False)
    stop_points = StopPointSerializer(many=True, display_none=False)

    def get_line(self, obj):
        if obj.HasField(str('line')):
            return LineSerializer(obj.line, display_none=False).data
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
    routes = RouteSerializer(many=True, display_none=False)
    network = NetworkSerializer(display_none=False)
    opening_time = LocalTimeField()
    closing_time = LocalTimeField()
    properties = PropertySerializer(many=True, display_none=False)
    geojson = MultiLineStringField(display_none=False)
    links = LinkSerializer(attr='impact_uris')
    line_groups = LineGroupSerializer(many=True, display_none=False)
