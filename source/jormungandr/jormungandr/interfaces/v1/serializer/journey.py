
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

from jormungandr.interfaces.v1.serializer import jsonschema, base
from jormungandr.interfaces.v1.serializer.pt import PlaceSerializer, CalendarSerializer, VJDisplayInformationSerializer, \
    StopDateTimeSerializer, StringListField
from jormungandr.interfaces.v1.serializer.time import DateTimeField
from jormungandr.interfaces.v1.serializer.fields import LinkSchema, RoundedField, SectionGeoJsonField, StrField
from jormungandr.interfaces.v1.serializer.base import AmountSerializer, PbNestedSerializer, \
    LambdaField, EnumField, EnumListField, NestedEnumField, PbField, PbStrField
from flask import g
from navitiacommon.type_pb2 import StopDateTime
from navitiacommon.response_pb2 import SectionAdditionalInformationType
from jormungandr.interfaces.v1.make_links import create_internal_link


class CO2Serializer(PbNestedSerializer):
    co2_emission = AmountSerializer(attr='car_co2_emission', display_none=True)


class ContextSerializer(PbNestedSerializer):
    car_direct_path = LambdaField(lambda _, obj: CO2Serializer(obj, display_none=True).data,
                                  schema_type=CO2Serializer())


class CostSerializer(PbNestedSerializer):
    value = PbStrField()
    currency = PbField(schema_type=str)


class FareSerializer(PbNestedSerializer):
    found = jsonschema.BoolField()
    total = CostSerializer()
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True), attr='ticket_id', display_none=True)

    def get_links(self, obj):
        if not hasattr(obj, 'ticket_id'):
            return []

        return [create_internal_link(id=value, rel='tickets', _type='ticket') for value in obj.ticket_id]

    #TODO check that retro compatibility is really useful
    def to_value(self, value):
        if value is None:
            return {
                'found': False,
                'links': [],
                'total': {
                    'currency': '',
                    'value': '0.0'
                }
            }
        return super(FareSerializer, self).to_value(value)


class TicketSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str, display_none=True, description='Identifier of the object')
    name = jsonschema.Field(schema_type=str, display_none=True, description='Name of the object')
    comment = jsonschema.Field(schema_type=str)
    found = jsonschema.BoolField()
    cost = CostSerializer(display_none=True)
    links = jsonschema.MethodField(schema_type=LinkSchema(many=True))

    def get_links(self, obj):
        if not hasattr(obj, 'section_id'):
            return None

        return [create_internal_link(id=value, rel='sections', _type='section') for value in obj.section_id]


class DurationsSerializer(PbNestedSerializer):
    total = jsonschema.Field(schema_type=int, display_none=True,
                             description='Total duration of the journey (seconds)')
    walking = jsonschema.Field(schema_type=int, display_none=True,
                               description='Walking total duration of the journey (seconds)')
    bike = jsonschema.Field(schema_type=int, display_none=True,
                            description='Total duration on bike of the journey (seconds)')
    car = jsonschema.Field(schema_type=int, display_none=True,
                           description='Total car duration of the journey (meters)')


class DistancesSerializer(PbNestedSerializer):
    walking = jsonschema.Field(schema_type=int, display_none=True,
                               description='Total walking distance of the journey (meters)')
    bike = jsonschema.Field(schema_type=int, display_none=True,
                            description='Total bike distance of the journey (meters)')
    car = jsonschema.Field(schema_type=int, display_none=True,
                           description='Total walking distance in the car of the journey (seconds)')


class JourneyDebugSerializer(PbNestedSerializer):
    streetnetwork_duration = jsonschema.Field(schema_type=int, display_none=True, attr='sn_dur',
                                              description='Total duration of streetnetwork use (seconds)')
    transfer_duration = jsonschema.Field(schema_type=int, display_none=True, attr='transfer_dur',
                                         description='Total duration of transfers (seconds)')
    min_waiting_duration = jsonschema.Field(schema_type=int, display_none=True, attr='min_waiting_dur',
                                            description='Minimum on all waiting durations (seconds)')
    nb_vj_extentions = jsonschema.Field(schema_type=int, display_none=True,
                                        description='Number of stay-in')
    nb_sections = jsonschema.Field(schema_type=int, display_none=True,
                                   description='Number of sections')
    internal_id = jsonschema.Field(schema_type=str, display_none=False)


class PathSerializer(PbNestedSerializer):
    length = RoundedField(display_none=True)
    name = jsonschema.Field(schema_type=str, display_none=True)
    duration = RoundedField(display_none=True)
    direction = jsonschema.Field(schema_type=int, display_none=True)


class SectionTypeEnum(EnumField):
    def as_getter(self, serializer_field_name, serializer_cls):
        def getter(value):
            # TODO, this should be done in kraken
            def if_on_demand_stop_time(stop):
                properties = stop.properties
                descriptor = properties.DESCRIPTOR
                enum = descriptor.enum_types_by_name["AdditionalInformation"]
                return any(enum.values_by_number[v].name == 'on_demand_transport'
                           for v in properties.additional_informations)

            try:
                if value.stop_date_times:
                    first_stop = value.stop_date_times[0]
                    last_stop = value.stop_date_times[-1]
                    if if_on_demand_stop_time(first_stop):
                        return 'on_demand_transport'
                    elif if_on_demand_stop_time(last_stop):
                        return 'on_demand_transport'
                    return 'public_transport'
            except ValueError:
                pass
            attr = self.attr or serializer_field_name
            if value is None or not value.HasField(attr):
                return None
            enum = value.DESCRIPTOR.fields_by_name[attr].enum_type.values_by_number
            return enum[getattr(value, attr)].name
        return getter


class SectionSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str, display_none=True)
    duration = jsonschema.Field(schema_type=int, display_none=True,
                                description='Duration of the section (seconds)')
    co2_emission = AmountSerializer(display_none=True)
    transfer_type = EnumField()
    departure_date_time = DateTimeField(attr='begin_date_time',
                                        description='Departure date and time of the section')
    arrival_date_time = DateTimeField(attr='end_date_time',
                                      description='Arrival date and time of the section')
    base_departure_date_time = DateTimeField(attr='base_begin_date_time',
                                             description='Base-schedule departure date and time of the section')
    base_arrival_date_time = DateTimeField(attr='base_end_date_time',
                                           description='Base-schedule arrival date and time of the section')
    data_freshness = EnumField(attr="realtime_level", display_none=False)
    to = jsonschema.MethodField(schema_type=PlaceSerializer(), attr='destination')

    def get_to(self, obj):
        if obj.HasField(str('type')):
            enum = obj.DESCRIPTOR.fields_by_name['type'].enum_type.values_by_number
            ret_value = enum[getattr(obj, 'type')].name
            if ret_value == 'WAITING':
                return None
        return PlaceSerializer(obj.destination).data

    _from = jsonschema.MethodField(schema_type=PlaceSerializer(), attr='origin', label='from')
    def get__from(self, obj):
        if obj.HasField(str('type')):
            enum = obj.DESCRIPTOR.fields_by_name['type'].enum_type.values_by_number
            ret_value = enum[getattr(obj, 'type')].name
            if ret_value == 'WAITING':
                return None
        return PlaceSerializer(obj.origin).data

    additional_informations = EnumListField(attr='additional_informations', pb_type=SectionAdditionalInformationType)
    geojson = SectionGeoJsonField(display_none=False, description='GeoJSON of the shape of the section')
    mode = NestedEnumField(attr='street_network.mode')
    type = SectionTypeEnum()

    display_informations = VJDisplayInformationSerializer(attr='pt_display_informations', display_none=False)

    links = jsonschema.MethodField(display_none=True, schema_type=LinkSchema(many=True))
    def get_links(self, obj):
        response = []
        if obj.HasField(str("uris")):
            for type_, value in obj.uris.ListFields():
                response.append({"type": type_.name, "id": value})
        if obj.HasField(str('pt_display_informations')):
            response.extend(base.make_notes(obj.pt_display_informations.notes))
        return response

    stop_date_times = StopDateTimeSerializer(many=True)
    path = PathSerializer(attr="street_network.path_items", many=True, display_none=False)


class JourneySerializer(PbNestedSerializer):
    duration = jsonschema.Field(schema_type=int, display_none=True,
                                description='Duration of the journey (seconds)')
    nb_transfers = jsonschema.Field(schema_type=int, display_none=True,
                                    description='Number of transfers along the journey')
    departure_date_time = DateTimeField(description='Departure date and time of the journey')
    arrival_date_time = DateTimeField(description='Arrival date and time of the journey')
    requested_date_time = DateTimeField(deprecated=True)
    to = PlaceSerializer(deprecated=True, attr='destination')
    _from = PlaceSerializer(deprecated=True, attr='origin', label='from')
    type = jsonschema.Field(schema_type=str, display_none=True,
                            description='Used to qualify the journey (can be "best", "comfort", "non_pt_walk", ...')
    status = jsonschema.Field(schema_type=str, attr="most_serious_disruption_effect", display_none=True,
                              description='Status from the whole journey taking into account the most '
                                          'disturbing information retrieved on every object used '
                                          '(can be "NO_SERVICE", "SIGNIFICANT_DELAYS", ...')
    tags = StringListField(display_none=True)
    co2_emission = AmountSerializer(display_none=True)
    durations = DurationsSerializer()
    distances = DistancesSerializer()
    fare = FareSerializer(display_none=True)
    calendars = CalendarSerializer(many=True, display_none=False)
    sections = SectionSerializer(many=True, display_none=False)
    debug = jsonschema.MethodField(schema_type=JourneyDebugSerializer(), display_none=False)

    def get_debug(self, obj):
        if not hasattr(g, 'debug') or not g.debug:
            return None
        return JourneyDebugSerializer(obj, display_none=False).data
