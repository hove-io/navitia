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

from __future__ import absolute_import, print_function, unicode_literals, division

from datetime import datetime
from jormungandr.interfaces.v1.serializer.base import PbNestedSerializer, EnumField, EnumListField
from jormungandr.interfaces.v1.serializer import pt, jsonschema, base
from jormungandr.interfaces.v1.serializer.fields import LinkSchema, MultiLineStringField
from jormungandr.interfaces.v1.serializer.time import DateTimeField
from jormungandr.interfaces.v1.make_links import create_internal_link
from jormungandr.interfaces.v1.serializer.jsonschema.fields import TimeOrDateTimeType
from jormungandr.utils import timestamp_to_str


def _get_links(obj):
    display_info = obj.pt_display_informations
    uris = display_info.uris
    l = [
        ("line", uris.line),
        ("company", uris.company),
        ("vehicle_journey", uris.vehicle_journey),
        ("route", uris.route),
        ("commercial_mode", uris.commercial_mode),
        ("physical_mode", uris.physical_mode),
        ("network", uris.network),
    ]
    return [{"type": k, "id": v} for k, v in l if v != ""] + base.make_notes(display_info.notes)


class PassageSerializer(PbNestedSerializer):
    route = pt.RouteSerializer()
    stop_point = pt.StopPointSerializer()
    stop_date_time = pt.StopDateTimeSerializer()
    display_informations = pt.PassageDisplayInformationSerializer(attr='pt_display_informations')
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True))

    def get_links(self, obj):
        return _get_links(obj)


class DateTimeTypeSerializer(PbNestedSerializer):
    date_time = jsonschema.MethodField(schema_type=TimeOrDateTimeType, display_none=True)
    base_date_time = DateTimeField()
    additional_informations = pt.AdditionalInformation(attr='additional_informations', display_none=True)
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True), display_none=True)
    data_freshness = EnumField(attr="realtime_level", display_none=True)
    occupancy = jsonschema.MethodField(schema_type=str, display_none=False)

    def get_occupancy(self, obj):
        return obj.occupancy if obj.HasField('occupancy') else None

    def get_links(self, obj):
        disruption_links = [
            create_internal_link(_type="disruption", rel="disruptions", id=uri) for uri in obj.impact_uris
        ]
        properties_links = pt.make_properties_links(obj.properties)
        return properties_links + disruption_links

    def get_date_time(self, obj):
        __date_time_null_value__ = 2 ** 64 - 1
        if obj.time == __date_time_null_value__:
            return ""
        if obj.HasField('date'):
            return timestamp_to_str(obj.date + obj.time)
        return datetime.utcfromtimestamp(obj.time).strftime('%H%M%S')


class StopScheduleSerializer(PbNestedSerializer):
    stop_point = pt.StopPointSerializer()
    route = pt.RouteSerializer()
    additional_informations = EnumField(attr="response_status", display_none=True)
    display_informations = pt.RouteDisplayInformationSerializer(attr='pt_display_informations')
    date_times = DateTimeTypeSerializer(many=True, display_none=True)
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True))
    first_datetime = DateTimeTypeSerializer(display_none=False)
    last_datetime = DateTimeTypeSerializer(display_none=False)

    def get_links(self, obj):
        return _get_links(obj)


class TerminusScheduleSerializer(PbNestedSerializer):
    stop_point = pt.StopPointSerializer()
    route = pt.RouteSerializer()
    additional_informations = EnumField(attr="response_status", display_none=True)
    display_informations = pt.RouteDisplayInformationSerializer(attr='pt_display_informations')
    date_times = DateTimeTypeSerializer(many=True, display_none=True)
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True))
    first_datetime = DateTimeTypeSerializer(display_none=False)
    last_datetime = DateTimeTypeSerializer(display_none=False)

    def get_links(self, obj):
        return _get_links(obj)


class RowSerializer(PbNestedSerializer):
    stop_point = pt.StopPointSerializer()
    date_times = DateTimeTypeSerializer(many=True, display_none=True)


class HeaderSerializer(PbNestedSerializer):
    additional_informations = EnumListField(attr='additional_informations', display_none=True)
    display_informations = pt.VJDisplayInformationSerializer(attr='pt_display_informations')
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True))

    def get_links(self, obj):
        return _get_links(obj)


class TableSerializer(PbNestedSerializer):
    rows = RowSerializer(many=True, display_none=True)
    headers = HeaderSerializer(many=True, display_none=True)


class RouteScheduleSerializer(PbNestedSerializer):
    table = TableSerializer()
    display_informations = pt.RouteDisplayInformationSerializer(attr='pt_display_informations')
    geojson = MultiLineStringField(display_none=False)
    additional_informations = EnumField(attr="response_status", display_none=True)
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True))

    def get_links(self, obj):
        return _get_links(obj)
