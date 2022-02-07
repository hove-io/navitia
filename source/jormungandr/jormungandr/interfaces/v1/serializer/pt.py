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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

from jormungandr.interfaces.v1.serializer.base import (
    PbGenericSerializer,
    EnumListField,
    LiteralField,
    SortedGenericSerializer,
    get_proto_attr_or_default,
)
from jormungandr.interfaces.v1.serializer.jsonschema.fields import Field, DateType
from jormungandr.interfaces.v1.serializer.time import (
    TimeField,
    PeriodSerializer,
    DateTimeField,
    PeriodDateSerializer,
    PeriodTimeSerializer,
)
from jormungandr.interfaces.v1.serializer.fields import *
from jormungandr.interfaces.v1.serializer import jsonschema, base
from jormungandr.parking_space_availability.bss import stands
from navitiacommon.type_pb2 import (
    ActiveStatus,
    Channel,
    hasEquipments,
    Properties,
    NavitiaType,
    Severity,
    StopTimeUpdateStatus,
    RTLevel,
    EquipmentDetails,
    CurrentAvailability,
)


LABEL_DESCRIPTION = """
Label of the stop area. The name is directly taken from the data whereas the label is
 something we compute for better traveler information. If you don't know what to display, display the label.
"""


class Equipments(EnumListField):
    """
    hack for equipments as there is a useless level in the proto
    """

    def __init__(self, **kwargs):
        super(Equipments, self).__init__(hasEquipments.Equipment, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        # For enum we need the full object :(
        return lambda x: x.has_equipments


class AdditionalInformation(EnumListField):
    """
    hack for AdditionalInformation as there is a useless level in the proto
    """

    def as_getter(self, serializer_field_name, serializer_cls):
        # For enum we need the full object :(
        return lambda x: x.properties


class ChannelSerializer(PbNestedSerializer):
    content_type = jsonschema.Field(schema_type=str, display_none=True)
    id = jsonschema.Field(schema_type=str, display_none=True)
    name = jsonschema.Field(schema_type=str, display_none=True)
    types = EnumListField(attr='channel_types', pb_type=Channel.ChannelType)  # type: ignore


class MessageSerializer(PbNestedSerializer):
    text = jsonschema.Field(schema_type=str)
    channel = ChannelSerializer()


class SeveritySerializer(PbNestedSerializer):
    name = jsonschema.Field(schema_type=str)
    effect = EnumField(pb_type=Severity.Effect, lower_case=False)  # type: ignore
    color = jsonschema.Field(schema_type=str)
    priority = jsonschema.Field(schema_type=int)


class DisruptionPropertySerializer(PbNestedSerializer):
    key = jsonschema.Field(schema_type=str)
    _type = jsonschema.Field(attr='type', label='type', schema_type=str)
    value = jsonschema.Field(schema_type=str)


class PtObjectSerializer(PbGenericSerializer):
    quality = jsonschema.Field(schema_type=int, required=False, display_none=True, deprecated=True)
    stop_area = jsonschema.MethodField(schema_type=lambda: StopAreaSerializer())
    stop_point = jsonschema.MethodField(schema_type=lambda: StopPointSerializer())
    line = jsonschema.MethodField(schema_type=lambda: LineSerializer())
    network = jsonschema.MethodField(schema_type=lambda: NetworkSerializer())
    route = jsonschema.MethodField(schema_type=lambda: RouteSerializer())
    commercial_mode = jsonschema.MethodField(schema_type=lambda: CommercialModeSerializer())
    trip = jsonschema.MethodField(schema_type=lambda: TripSerializer())
    embedded_type = EnumField(attr='embedded_type', pb_type=NavitiaType, display_none=True)

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

    def get_stop_point(self, obj):
        if obj.HasField(str('stop_point')):
            return StopPointSerializer(obj.stop_point, display_none=False).data
        else:
            return None


class TripSerializer(PbGenericSerializer):
    pass


class ValidityPatternSerializer(PbNestedSerializer):
    beginning_date = Field(schema_type=DateType)
    days = Field(schema_type=str)


class WeekPatternSerializer(PbNestedSerializer):
    monday = BoolField()
    tuesday = BoolField()
    wednesday = BoolField()
    thursday = BoolField()
    friday = BoolField()
    saturday = BoolField()
    sunday = BoolField()


class CalendarPeriodSerializer(PbNestedSerializer):
    begin = Field(schema_type=DateType)
    end = Field(schema_type=DateType)


class CalendarExceptionSerializer(PbNestedSerializer):
    datetime = Field(attr='date', schema_type=DateType)
    type = EnumField()


class CalendarSerializer(PbNestedSerializer):
    id = jsonschema.MethodField(schema_type=str, display_none=False, description='Identifier of the object')

    def get_id(self, obj):
        if obj.HasField(str('uri')) and obj.uri:
            return obj.uri
        else:
            return None

    name = jsonschema.MethodField(schema_type=str, description='Name of the object')

    def get_name(self, obj):
        if obj.HasField(str('name')) and obj.name:
            return obj.name
        else:
            return None

    week_pattern = WeekPatternSerializer()
    validity_pattern = ValidityPatternSerializer(display_none=False)
    exceptions = CalendarExceptionSerializer(many=True)
    active_periods = CalendarPeriodSerializer(many=True)


class ImpactedStopSerializer(PbNestedSerializer):
    stop_point = jsonschema.MethodField(schema_type=lambda: StopPointSerializer())
    base_arrival_time = TimeField(attr='base_stop_time.arrival_time')
    base_departure_time = TimeField(attr='base_stop_time.departure_time')
    amended_arrival_time = TimeField(attr='amended_stop_time.arrival_time')
    amended_departure_time = TimeField(attr='amended_stop_time.departure_time')
    cause = jsonschema.Field(schema_type=str, display_none=True)
    stop_time_effect = EnumField(attr='effect', pb_type=StopTimeUpdateStatus)
    departure_status = EnumField()
    arrival_status = EnumField()
    is_detour = jsonschema.Field(schema_type=bool, display_none=True)

    def get_stop_point(self, obj):
        if obj.HasField(str('stop_point')):
            return StopPointSerializer(obj.stop_point, display_none=False).data
        else:
            return None


class ImpactedSectionSerializer(PbNestedSerializer):
    f = PtObjectSerializer(label='from', attr='from')
    to = PtObjectSerializer()
    routes = jsonschema.MethodField(schema_type=lambda: RouteSerializer(many=True))

    def get_routes(self, obj):
        return RouteSerializer(obj.routes, display_none=False, many=True).data


class ImpactedSerializer(PbNestedSerializer):
    pt_object = PtObjectSerializer(display_none=False)
    impacted_stops = ImpactedStopSerializer(many=True, display_none=False)
    impacted_section = ImpactedSectionSerializer(display_none=False)
    impacted_rail_section = ImpactedSectionSerializer(display_none=False)


class StringListField(Field):
    def __init__(self, **kwargs):
        super(StringListField, self).__init__(schema_type=str, **kwargs)
        self.many = True

    def as_getter(self, serializer_field_name, serializer_cls):
        obj_op = operator.attrgetter(self.attr or serializer_field_name)
        dic_op = operator.itemgetter(self.attr or serializer_field_name)

        def getter(v):
            if isinstance(v, dict):
                return list(dic_op(v))
            return list(obj_op(v))

        return getter


class ApplicationPatternSerializer(PbNestedSerializer):
    week_pattern = WeekPatternSerializer()
    application_period = PeriodDateSerializer()
    time_slots = PeriodTimeSerializer(many=True)


class DisruptionSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str, display_none=True, attr='uri')

    disruption_id = jsonschema.Field(schema_type=str, attr='disruption_uri')
    impact_id = jsonschema.Field(schema_type=str, attr='uri')
    title = (jsonschema.Field(schema_type=str),)
    application_periods = PeriodSerializer(many=True)
    application_patterns = ApplicationPatternSerializer(many=True, display_none=False)
    status = EnumField(attr='status', pb_type=ActiveStatus)
    updated_at = DateTimeField()
    tags = StringListField(display_none=False)
    cause = jsonschema.Field(schema_type=str, display_none=True)
    category = jsonschema.MethodField(schema_type=str, display_none=False)

    def get_category(self, obj):
        if obj.HasField(str("category")) and obj.category:
            return obj.category
        return None

    severity = SeveritySerializer()
    messages = MessageSerializer(many=True)
    impacted_objects = ImpactedSerializer(many=True, display_none=False)
    uri = jsonschema.Field(schema_type=str, attr='uri')
    disruption_uri = jsonschema.Field(schema_type=str)
    contributor = jsonschema.Field(schema_type=str, display_none=True)
    properties = DisruptionPropertySerializer(many=True, display_none=False)


class PoiTypeSerializer(PbGenericSerializer):
    pass


class StandsSerializer(PbNestedSerializer):
    available_places = jsonschema.IntField()
    available_bikes = jsonschema.IntField()
    total_stands = jsonschema.IntField()
    status = jsonschema.StrField(
        schema_metadata={"enum": [v.name for v in stands.StandsStatus], "type": "string"}
    )


class CarParkSerializer(PbNestedSerializer):
    available = jsonschema.IntField()
    occupied = jsonschema.IntField()
    available_PRM = jsonschema.IntField()
    occupied_PRM = jsonschema.IntField()
    total_places = jsonschema.IntField()
    # items for new api star
    available_ridesharing = jsonschema.IntField()
    occupied_ridesharing = jsonschema.IntField()
    available_electric_vehicle = jsonschema.IntField()
    occupied_electric_vehicle = jsonschema.IntField()
    state = jsonschema.Field(schema_type=str)


class AdminSerializer(SortedGenericSerializer, PbGenericSerializer):
    level = jsonschema.Field(schema_type=int)
    zip_code = jsonschema.Field(schema_type=str, display_none=True)
    label = jsonschema.Field(schema_type=str)
    insee = jsonschema.Field(schema_type=str)
    coord = CoordSerializer(required=False)

    def sort_key(self, obj):
        return obj["level"]


class AddressSerializer(PbGenericSerializer):
    house_number = jsonschema.Field(schema_type=int, display_none=True)
    coord = CoordSerializer(required=False)
    label = jsonschema.Field(schema_type=str, display_none=False)
    administrative_regions = AdminSerializer(many=True, display_none=False)


class PoiSerializer(PbGenericSerializer):
    coord = CoordSerializer(required=False)
    label = jsonschema.Field(schema_type=str)
    administrative_regions = AdminSerializer(many=True, display_none=False)
    poi_type = PoiTypeSerializer(display_none=False)
    properties = jsonschema.MethodField(
        schema_metadata={'type': 'object', 'additionalProperties': {'type': 'string'}}
    )
    address = AddressSerializer()
    stands = LiteralField(None, schema_type=StandsSerializer, display_none=False)
    car_park = LiteralField(None, schema_type=CarParkSerializer, display_none=False)

    def get_properties(self, obj):
        return {p.type: p.value for p in obj.properties}


class PhysicalModeSerializer(PbGenericSerializer):
    pass


class CommercialModeSerializer(PbGenericSerializer):
    pass


class CauseSerializer(PbNestedSerializer):
    label = base.PbStrField(display_none=True, required=False)


class EffectSerializer(PbNestedSerializer):
    label = base.PbStrField(display_none=True, required=False)


class CurrentAvailabilitySerializer(PbNestedSerializer):
    status = EnumField(
        pb_type=CurrentAvailability.EquipmentStatus, display_none=False, required=False  # type: ignore
    )
    periods = PeriodSerializer(many=True, display_none=False, required=False)
    updated_at = base.PbStrField(display_none=False, required=False)
    cause = CauseSerializer(display_none=False, required=False)
    effect = EffectSerializer(display_none=False, required=False)


class EquipmentDetailsSerializer(PbNestedSerializer):
    id = base.PbStrField(display_none=False, required=False)
    name = base.PbStrField(display_none=False, required=False)
    embedded_type = EnumField(
        pb_type=EquipmentDetails.EquipmentType, display_none=False, required=False  # type: ignore
    )
    current_availability = CurrentAvailabilitySerializer(display_none=False, required=False)


class AccessPointSerializer(PbGenericSerializer):
    coord = CoordSerializer(required=False)
    is_entrance = jsonschema.MethodField(schema_type=bool, display_none=False)
    is_exit = jsonschema.MethodField(schema_type=bool, display_none=False)
    pathway_mode = jsonschema.MethodField(schema_type=int, display_none=False)
    length = jsonschema.MethodField(schema_type=int, display_none=False)
    traversal_time = jsonschema.MethodField(schema_type=int, display_none=False)
    stair_count = jsonschema.MethodField(schema_type=int, display_none=False)
    max_slope = jsonschema.MethodField(schema_type=int, display_none=False)
    min_width = jsonschema.MethodField(schema_type=int, display_none=False)
    signposted_as = jsonschema.MethodField(schema_type=str, display_none=False)
    reversed_signposted_as = jsonschema.MethodField(schema_type=str, display_none=False)
    stop_code = jsonschema.MethodField(schema_type=str, display_none=False)
    parent_station = jsonschema.MethodField(schema_type=lambda: StopAreaSerializer(), display_none=False)

    def get_is_entrance(self, obj):
        return get_proto_attr_or_default(obj, 'is_entrance')

    def get_is_exit(self, obj):
        return get_proto_attr_or_default(obj, 'is_exit')

    def get_pathway_mode(self, obj):
        return get_proto_attr_or_default(obj, 'pathway_mode')

    def get_length(self, obj):
        return get_proto_attr_or_default(obj, 'length')

    def get_traversal_time(self, obj):
        return get_proto_attr_or_default(obj, 'traversal_time')

    def get_stair_count(self, obj):
        return get_proto_attr_or_default(obj, 'stair_count')

    def get_max_slope(self, obj):
        return get_proto_attr_or_default(obj, 'max_slope')

    def get_min_width(self, obj):
        return get_proto_attr_or_default(obj, 'min_width')

    def get_signposted_as(self, obj):
        return get_proto_attr_or_default(obj, 'signposted_as')

    def get_reversed_signposted_as(self, obj):
        return get_proto_attr_or_default(obj, 'reversed_signposted_as')

    def get_stop_code(self, obj):
        return get_proto_attr_or_default(obj, 'stop_code')

    def get_parent_station(self, obj):
        if obj.HasField(str('parent_station')):
            return StopAreaSerializer(obj.parent_station, display_none=False).data
        else:
            return None


class StopPointSerializer(PbGenericSerializer):
    comments = CommentSerializer(many=True, display_none=False)
    comment = FirstCommentField(attr='comments', display_none=False)
    codes = CodeSerializer(many=True, display_none=False)
    label = jsonschema.Field(schema_type=str)
    coord = CoordSerializer(required=False)
    links = DisruptionLinkSerializer(attr='impact_uris', display_none=True)
    commercial_modes = CommercialModeSerializer(many=True, display_none=False)
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)
    administrative_regions = AdminSerializer(many=True, display_none=False)
    stop_area = jsonschema.MethodField(schema_type=lambda: StopAreaSerializer(), display_none=False)
    equipments = Equipments(attr='has_equipments', display_none=True)
    address = AddressSerializer(display_none=False)
    fare_zone = jsonschema.MethodField(schema_type=lambda: FareZoneSerializer(), display_none=False)
    equipment_details = EquipmentDetailsSerializer(many=True)
    lines = jsonschema.MethodField(schema_type=lambda: LineSerializer(many=True), display_none=False)
    access_points = AccessPointSerializer(many=True, display_none=False)

    def get_fare_zone(self, obj):
        if obj.HasField(str('fare_zone')):
            return FareZoneSerializer(obj.fare_zone, display_none=False).data
        else:
            return None

    def get_stop_area(self, obj):
        if obj.HasField(str('stop_area')):
            return StopAreaSerializer(obj.stop_area, display_none=False).data
        else:
            return None

    def get_lines(self, obj):
        return LineSerializer(obj.lines, many=True, display_none=False).data


class StopAreaSerializer(PbGenericSerializer):
    comments = CommentSerializer(many=True, display_none=False)
    comment = FirstCommentField(attr='comments', display_none=False, deprecated=True)
    codes = CodeSerializer(many=True, display_none=False)
    timezone = jsonschema.Field(schema_type=str)
    label = jsonschema.Field(schema_type=str, description=LABEL_DESCRIPTION)
    coord = CoordSerializer(required=False)
    links = DisruptionLinkSerializer(attr='impact_uris', display_none=True)
    commercial_modes = CommercialModeSerializer(many=True, display_none=False)
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)
    administrative_regions = AdminSerializer(
        many=True,
        display_none=False,
        description='Administrative regions of the stop area ' 'in which is the stop area',
    )
    stop_points = StopPointSerializer(
        many=True, display_none=False, description='Stop points contained in this stop area'
    )
    lines = jsonschema.MethodField(schema_type=lambda: LineSerializer(many=True), display_none=False)

    def get_lines(self, obj):
        return LineSerializer(obj.lines, many=True, display_none=False).data


class PlaceSerializer(PbGenericSerializer):
    """
    Warning: This class share it's interface with PlacesCommonSerializer (for Bragi)
    If you add/modify fields here, please reflect your changes in
    'jormungandr.jormungandr.interfaces.v1.serializer.geocode_json.PlacesCommonSerializer'.
    """

    quality = jsonschema.Field(schema_type=int, display_none=True, required=False, deprecated=True)
    stop_area = StopAreaSerializer(display_none=False)
    stop_point = StopPointSerializer(display_none=False)
    administrative_region = AdminSerializer(display_none=False)
    embedded_type = EnumField(attr='embedded_type', pb_type=NavitiaType, display_none=True)
    address = AddressSerializer(display_none=False)
    poi = PoiSerializer(display_none=False)
    access_point = AccessPointSerializer(display_none=False)

    distance = base.PbStrField(
        required=False, display_none=False, description='Distance to the object in meters'
    )


class PlaceNearbySerializer(PlaceSerializer):
    pass


class NetworkSerializer(PbGenericSerializer):
    links = DisruptionLinkSerializer(attr='impact_uris', display_none=True)
    codes = CodeSerializer(many=True, display_none=False)


class RouteSerializer(PbGenericSerializer):
    is_frequence = StrField(schema_metadata={"enum": ["False"], "type": "string"})
    direction_type = jsonschema.Field(schema_type=str, display_none=True)
    physical_modes = PhysicalModeSerializer(many=True, display_none=False)
    comments = CommentSerializer(many=True, display_none=False)
    codes = CodeSerializer(many=True, display_none=False)
    direction = PlaceSerializer()
    geojson = MultiLineStringField(display_none=False)
    links = DisruptionLinkSerializer(attr='impact_uris', display_none=True)
    line = jsonschema.MethodField(schema_type=lambda: LineSerializer())
    stop_points = StopPointSerializer(many=True, display_none=False)

    def get_line(self, obj):
        if obj.HasField(str('line')):
            return LineSerializer(obj.line, display_none=False).data
        else:
            return None


class LineGroupSerializer(PbGenericSerializer):
    lines = jsonschema.MethodField(schema_type=lambda: LineSerializer(many=True), display_none=False)
    main_line = jsonschema.MethodField(schema_type=lambda: LineSerializer(), display_none=False)
    comments = CommentSerializer(many=True)

    def get_lines(self, obj):
        return LineSerializer(obj.lines, many=True, display_none=False).data

    def get_main_line(self, obj):
        if obj.HasField(str('main_line')):
            return LineSerializer(obj.main_line).data
        else:
            return None


class LineSerializer(PbGenericSerializer):
    code = jsonschema.Field(schema_type=str, display_none=True)
    color = jsonschema.Field(schema_type=str)
    text_color = jsonschema.Field(schema_type=str)
    comments = CommentSerializer(many=True, display_none=False)
    comment = FirstCommentField(attr='comments', display_none=False)
    codes = CodeSerializer(many=True, required=False)
    physical_modes = PhysicalModeSerializer(many=True)
    commercial_mode = CommercialModeSerializer(display_none=False)
    routes = RouteSerializer(many=True, display_none=False)
    network = NetworkSerializer(display_none=False)
    opening_time = TimeField()
    closing_time = TimeField()
    properties = PropertySerializer(many=True, display_none=False)
    geojson = MultiLineStringField(display_none=False)
    links = DisruptionLinkSerializer(attr='impact_uris', display_none=True)
    line_groups = LineGroupSerializer(many=True, display_none=False)


class StopAreaEquipmentsSerializer(PbNestedSerializer):
    stop_area = StopAreaSerializer(display_none=False, required=False)
    equipment_details = EquipmentDetailsSerializer(many=True, display_none=False)


class JourneyPatternPointSerializer(PbNestedSerializer):
    id = jsonschema.Field(attr='uri', display_none=True, schema_type=str)
    stop_point = StopPointSerializer(display_none=False)


class JourneyPatternSerializer(PbGenericSerializer):
    journey_pattern_points = JourneyPatternPointSerializer(many=True)
    route = RouteSerializer(display_none=False)


class StopTimeSerializer(PbNestedSerializer):
    arrival_time = TimeField()
    utc_arrival_time = TimeField()
    departure_time = TimeField()
    utc_departure_time = TimeField()
    headsign = jsonschema.Field(schema_type=str)
    journey_pattern_point = JourneyPatternPointSerializer()
    stop_point = StopPointSerializer()
    pickup_allowed = BoolField()
    drop_off_allowed = BoolField()
    skipped_stop = BoolField()


class VehicleJourneySerializer(PbGenericSerializer):
    journey_pattern = JourneyPatternSerializer(display_none=False)
    stop_times = StopTimeSerializer(many=True)
    comments = CommentSerializer(many=True, display_none=False)
    comment = FirstCommentField(attr='comments', display_none=False)
    codes = CodeSerializer(many=True, required=False)
    validity_pattern = ValidityPatternSerializer()
    calendars = CalendarSerializer(many=True)
    trip = TripSerializer()
    disruptions = DisruptionLinkSerializer(attr='impact_uris', display_none=True)
    start_time = TimeField(display_none=False)
    end_time = TimeField(display_none=False)
    headway_secs = jsonschema.MethodField(schema_type=int, display_none=False)
    headsign = jsonschema.Field(schema_type=str, display_none=False)

    def get_headway_secs(self, obj):
        if obj.HasField(str('headway_secs')):
            return obj.headway_secs
        else:
            return None


class VehicleJourneyPositionsSerializer(PbNestedSerializer):
    vehicle_journey = VehicleJourneySerializer(display_none=False, required=False)
    coord = jsonschema.MethodField(schema_type=lambda: CoordSerializer(), display_none=False)
    bearing = jsonschema.MethodField(schema_type=int, display_none=False)
    speed = jsonschema.MethodField(schema_type=int, display_none=False)
    data_freshness = EnumField(attr="data_freshness", display_none=False)
    occupancy = jsonschema.MethodField(schema_type=str, display_none=False)
    feed_created_at = DateTimeField()

    def get_bearing(self, obj):
        return obj.bearing if obj.HasField(str('bearing')) else None

    def get_speed(self, obj):
        return obj.speed if obj.HasField(str('speed')) else None

    def get_coord(self, obj):
        return CoordSerializer(obj.coord).data if obj.HasField(str('coord')) else None

    def get_occupancy(self, obj):
        return get_proto_attr_or_default(obj, 'occupancy')


class ConnectionSerializer(PbNestedSerializer):
    origin = StopPointSerializer()
    destination = StopPointSerializer()
    duration = jsonschema.Field(
        schema_type=int,
        display_none=True,
        description='Duration of connection (seconds). '
        'The duration really used to compute connection with a margin',
    )
    display_duration = jsonschema.Field(
        schema_type=int,
        display_none=True,
        description='The duration (seconds) of the connection as it should be '
        'displayed to traveler, without margin',
    )
    max_duration = jsonschema.Field(
        schema_type=int,
        display_none=True,
        deprecated=True,
        description='Parameter used to specify the maximum length allowed '
        'for a traveler to stay at a connection (seconds)',
    )


class CompanieSerializer(PbGenericSerializer):
    codes = CodeSerializer(many=True)


class ContributorSerializer(PbGenericSerializer):
    website = jsonschema.Field(schema_type=str)
    license = jsonschema.Field(schema_type=str)


class DatasetSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str, display_none=True, attr='uri', description='Identifier of the object')
    description = jsonschema.Field(schema_type=str, attr='desc')
    start_validation_date = DateTimeField(description='Start of the validity period for the dataset')
    end_validation_date = DateTimeField(description='End of the validity period for the dataset')
    system = jsonschema.Field(schema_type=str, description='Type of dataset provided (GTFS, Chouette, ...)')
    realtime_level = EnumField()
    contributor = ContributorSerializer(description='Contributor providing the dataset')


class RouteDisplayInformationSerializer(PbNestedSerializer):
    commercial_mode = jsonschema.Field(schema_type=str)
    network = jsonschema.Field(schema_type=str)
    direction = jsonschema.Field(schema_type=str, display_none=True)
    label = jsonschema.MethodField(schema_type=str)

    def get_label(self, obj):
        if obj.HasField(str('code')) and obj.code != '':
            return obj.code
        elif obj.HasField(str('name')):
            return obj.name
        else:
            return None

    color = jsonschema.Field(schema_type=str)
    code = jsonschema.Field(schema_type=str)
    name = jsonschema.Field(schema_type=str)
    links = DisruptionLinkSerializer(attr='impact_uris', display_none=True)
    text_color = jsonschema.Field(schema_type=str)


class VJDisplayInformationSerializer(RouteDisplayInformationSerializer):
    description = jsonschema.Field(schema_type=str)
    physical_mode = jsonschema.Field(schema_type=str)
    equipments = Equipments(attr='has_equipments', display_none=True)
    headsign = jsonschema.Field(schema_type=str, display_none=True)
    headsigns = StringListField(display_none=False)
    trip_short_name = jsonschema.Field(schema_type=str, display_none=False)


def make_properties_links(properties):
    if properties is None:
        return []

    response = base.make_notes(properties.notes)

    response.extend(
        [
            {
                "type": "exceptions",  # type should be 'exception' but retrocompatibility...
                "rel": "exceptions",
                "id": exception.uri,
                "date": exception.date,
                "except_type": exception.type,
                "internal": True,
            }
            for exception in properties.exceptions
        ]
    )

    if properties.HasField(str("destination")) and properties.destination.HasField(str("uri")):
        response.append(
            {
                "type": "notes",
                "rel": "notes",
                "category": "terminus",
                "id": properties.destination.uri,
                "value": properties.destination.destination,
                "internal": True,
            }
        )

    if properties.HasField(str("vehicle_journey_id")):
        response.append(
            {
                "type": "vehicle_journey",
                "rel": "vehicle_journeys",
                "value": properties.vehicle_journey_id,  # to remove for the v2
                "id": properties.vehicle_journey_id,
            }
        )

    return response


class PropertiesLinksSerializer(PbNestedSerializer):
    def __init__(self, **kwargs):
        super(PropertiesLinksSerializer, self).__init__(
            schema_type=LinkSchema(many=True), display_none=True, **kwargs
        )

    def to_value(self, properties):
        return make_properties_links(properties)


class StopDateTimeSerializer(PbNestedSerializer):
    departure_date_time = DateTimeField()
    base_departure_date_time = DateTimeField()
    arrival_date_time = DateTimeField()
    base_arrival_date_time = DateTimeField()
    stop_point = StopPointSerializer()
    # additional_informations is a nullable field, add nullable=True when we migrate to swagger 3
    additional_informations = AdditionalInformation(
        attr='additional_informations',
        display_none=True,
        pb_type=Properties.AdditionalInformation,  # type: ignore
    )
    links = PropertiesLinksSerializer(attr="properties")
    data_freshness = EnumField(pb_type=RTLevel)
