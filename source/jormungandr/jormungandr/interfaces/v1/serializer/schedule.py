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

from datetime import datetime
from jormungandr.interfaces.v1.serializer.base import PbNestedSerializer, EnumField, EnumListField
from jormungandr.interfaces.v1.serializer import pt
from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.serializer.fields import LinkSchema, MultiLineStringField
from jormungandr.interfaces.v1.make_links import create_internal_link
from jormungandr.utils import timestamp_to_str

def _get_links(obj):
    display_info = obj.pt_display_informations
    uris = display_info.uris
    l = [("line", uris.line),
         ("company", uris.company),
         ("vehicle_journey", uris.vehicle_journey),
         ("route", uris.route),
         ("commercial_mode", uris.commercial_mode),
         ("physical_mode", uris.physical_mode),
         ("network", uris.network),
         ("note", uris.note)]
    return [{"type": k, "id": v} for k, v in l if v != ""] + \
        [{"type": "notes", "rel": "notes", "id": value.uri, "value": value.note, "internal": True}
         for value in display_info.notes]


class PassageSerializer(PbNestedSerializer):
    route = pt.RouteSerializer()
    stop_point = pt.StopPointSerializer()
    stop_date_time = pt.StopDateTimeSerializer()
    display_informations = pt.DisplayInformationSerializer(attr='pt_display_informations')
    links = jsonschema.MethodField(schema_type=LinkSchema(), many=True)

    def get_links(self, obj):
        return _get_links(obj)


class DateTimeSerializer(PbNestedSerializer):
    date_time = jsonschema.MethodField(schema_type=LinkSchema(), display_none=True)
    additional_informations = pt.AdditionalInformation(attr='additional_informations', display_none=True)
    links = jsonschema.MethodField(schema_type=LinkSchema(), many=True)
    data_freshness = EnumField(attr="realtime_level")

    def get_date_time(self, obj):
        __date_time_null_value__ = 2**64 - 1
        if obj.time == __date_time_null_value__:
            return ""
        if obj.HasField('date'):
            return timestamp_to_str(obj.date + obj.time)
        return datetime.utcfromtimestamp(obj.time).strftime('%H%M%S')

    def get_links(self, obj):
        properties = obj.properties
        r = []
        #Note: all those links should be created with create_{internal|external}_links,
        # but for retrocompatibility purpose we cannot do that :( Change it for the v2!
        for note_ in properties.notes:
            r.append({"id": note_.uri,
                      "type": "notes",  # type should be 'note' but retrocompatibility...
                      "rel": "notes",
                      "value": note_.note,
                      "internal": True})
        for exception in properties.exceptions:
            r.append({"type": "exceptions",  # type should be 'exception' but retrocompatibility...
                      "rel": "exceptions",
                      "id": exception.uri,
                      "date": exception.date,
                      "except_type": exception.type,
                      "internal": True})
        if properties.destination and properties.destination.uri:
            r.append({"type": "notes",
                      "rel": "notes",
                      "id": properties.destination.uri,
                      "value": properties.destination.destination,
                      "internal": True})
        if properties.vehicle_journey_id:
            r.append({"type": "vehicle_journey",
                      "rel": "vehicle_journeys",
                      # the value has nothing to do here (it's the 'id' field), refactor for the v2
                      "value": properties.vehicle_journey_id,
                      "id": properties.vehicle_journey_id})
        return r


class StopScheduleSerializer(PbNestedSerializer):
    stop_point = pt.StopPointSerializer()
    route = pt.RouteSerializer()
    additional_informations = EnumField(attr="response_status")
    display_informations = pt.DisplayInformationSerializer(attr='pt_display_informations')
    date_times = DateTimeSerializer(many=True, display_none=True)
    links = jsonschema.MethodField(schema_type=LinkSchema(), many=True)

    def get_links(self, obj):
        return _get_links(obj)


class RowSerializer(PbNestedSerializer):
    stop_point = pt.StopPointSerializer()
    date_times = DateTimeSerializer(many=True, display_none=True)


class HeaderSerializer(PbNestedSerializer):
    additional_informations = EnumListField(attr='additional_informations', display_none=True)
    display_informations = pt.DisplayInformationSerializer(attr='pt_display_informations')
    links = jsonschema.MethodField(schema_type=LinkSchema(), many=True)

    def get_links(self, obj):
        return _get_links(obj)


class TableSerializer(PbNestedSerializer):
    rows = RowSerializer(many=True, display_none=True)
    headers = HeaderSerializer(many=True, display_none=True)


class RouteScheduleSerializer(PbNestedSerializer):
    table = TableSerializer()
    display_informations = pt.DisplayInformationSerializer(attr='pt_display_informations')
    geojson = MultiLineStringField(display_none=False)
    links = jsonschema.MethodField(schema_type=LinkSchema(), many=True)

    def get_links(self, obj):
        return _get_links(obj)
