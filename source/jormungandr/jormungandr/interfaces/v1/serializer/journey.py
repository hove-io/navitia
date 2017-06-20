
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

from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.serializer.pt import PlaceSerializer, CalendarSerializer
from jormungandr.interfaces.v1.serializer.time import DateTimeField
from jormungandr.interfaces.v1.serializer.fields import LinkSchema
from jormungandr.interfaces.v1.serializer.base import AmountSerializer, PbNestedSerializer, \
        LambdaField, EnumField
from flask import g
from navitiacommon.type_pb2 import StopDateTime


class CO2Serializer(PbNestedSerializer):
    co2_emission = AmountSerializer(attr='car_co2_emission')


class ContextSerializer(PbNestedSerializer):
    car_direct_path = LambdaField(lambda _,obj: CO2Serializer(obj, display_none=False).data,
                                  schema_type=lambda: CO2Serializer())


class CostSerializer(PbNestedSerializer):
    value = jsonschema.Field(schema_type=str)
    currency = jsonschema.Field(schema_type=str)


class FareSerializer(PbNestedSerializer):
    found = jsonschema.BoolField()
    total = CostSerializer()
    links = jsonschema.MethodField(schema_type=lambda: LinkSchema(), many=True, attr='ticket_id')

    def get_links(self, obj):
        ticket_ids = []
        try:
            for s_id in obj.ticket_id:
                ticket_ids.append(s_id)
        except ValueError:
            return None
        response = []
        for value in ticket_ids:
            response.append({"type": "ticket", "rel": "tickets",
                             "internal": True, "templated": False,
                             "id": value})
        return response


class TicketSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str, description='Identifier of the object')
    name = jsonschema.Field(schema_type=str, description='Name of the object')
    comment = jsonschema.Field(schema_type=str)
    found = jsonschema.BoolField()
    cost = CostSerializer()
    links = jsonschema.MethodField(schema_type=lambda: LinkSchema(), many=True)

    def get_links(self, obj):
        section_ids = []
        try:
            for s_id in obj.section_id:
                section_ids.append(s_id)
        except ValueError:
            return None
        response = []
        for value in section_ids:
            response.append({"type": "section", "rel": "sections",
                             "internal": True, "templated": False,
                             "id": value})
        return response


class DurationsSerializer(PbNestedSerializer):
    total = jsonschema.Field(schema_type=int, display_none=True,
                             description='Total duration of the journey (seconds)')
    walking = jsonschema.Field(schema_type=int, display_none=True,
                               description='Walking total duration of the journey (seconds)')

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


class NoteSerializer(PbNestedSerializer):
    #TODO
    pass


class ExceptionSerializer(PbNestedSerializer):
    #TODO
    pass


class SectionSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str)
    duration = jsonschema.Field(schema_type=int, display_none=True,
                                description='Duration of the section (seconds)')
    co2_emission = AmountSerializer()
    transfer_type = EnumField()
    departure_date_time = DateTimeField(attr='begin_date_time',
                                        description='Departure date and time of the section')
    arrival_date_time = DateTimeField(attr='end_date_time',
                                      description='Arrival date and time of the section')
    base_departure_date_time = DateTimeField(attr='base_begin_date_time',
                                             description='Base-schedule departure date and time of the section')
    base_arrival_date_time = DateTimeField(attr='base_end_date_time',
                                           description='Base-schedule arrival date and time of the section')
    to = PlaceSerializer(deprecated=True, attr='destination')
    _from = PlaceSerializer(deprecated=True, attr='origin', label='from')
    additional_informations = EnumField(many=True)
    geojson = jsonschema.Field(schema_type=str, display_none=True,
                               description='GeoJSON of the shape of the section')

    mode = jsonschema.MethodField(schema_type=lambda: EnumField())
    def get_mode(self, obj):
        if obj.HasField(str('street_network')) and obj.street_network.HasField(str('mode')):
            return EnumField(obj.street_network.mode, display_none=False)
        else:
            return None

    # TODO:
    # "type": section_type(),
    # "links": SectionLinks(attribute="uris"),
    # "display_informations": PbField(display_informations_vj,
    #                                 attribute='pt_display_informations'),
    # "path": NonNullList(NonNullNested({"length": Integer(),
    #                                    "name": fields.String(),
    #                                    "duration": Integer(),
    #                                    "direction": fields.Integer()}),
    #                     attribute="street_network.path_items"),
    # "stop_date_times": NonNullList(NonNullNested(stop_date_time)),


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
    type = jsonschema.Field(schema_type=str, description='Used to qualify the journey '
                                                         '(can be "best", "comfort", "non_pt_walk", ...')
    status = jsonschema.Field(schema_type=str, attr="most_serious_disruption_effect", display_none=True,
                              description='Status from the whole journey taking into account the most '
                                          'disturbing information retrieved on every object used '
                                          '(can be "NO_SERVICE", "SIGNIFICANT_DELAYS", ...')
    tags = jsonschema.Field(schema_type=str, many=True,
                            description='List of tags on the journey. The tags add additional information '
                                        'on the journey beside the journey type '
                                        '(can be "walking", "bike", ...)') # TODO check que ca marche avec les sections
    co2_emission = AmountSerializer()
    durations = DurationsSerializer()
    fare = FareSerializer()
    calendars = CalendarSerializer(many=True)
    sections = SectionSerializer(many=True, display_none=False)
    debug = jsonschema.MethodField(schema_type=lambda: JourneyDebugSerializer(), display_none=False)

    def get_debug(self, obj):
        if not hasattr(g, 'debug') or not g.debug:
            return None
        return JourneyDebugSerializer(obj, display_none=False).data
