# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
from flask.ext.restful import fields
from copy import deepcopy
import datetime
import logging
from flask.globals import g
import pytz
from jormungandr.interfaces.v1.make_links import create_internal_link, create_external_link
from jormungandr.utils import timestamp_to_str
from navitiacommon import response_pb2, type_pb2
import ujson


class Lit(fields.Raw):
    def __init__(self, val):
        self.val = val

    def output(self, key, obj):
        return self.val


class ListLit(fields.Raw):
    def __init__(self, l):
        self.l = l

    def output(self, key, obj):
        return [e.output(key, obj) for e in self.l]


class PbField(fields.Nested):

    def __init__(self, nested, allow_null=True, **kwargs):
        super(PbField, self).__init__(nested, **kwargs)
        self.display_null = False

    def output(self, key, obj):
        if not obj:
            return None
        if self.attribute:
            key = self.attribute.split(".")[0]
        try:
            if obj.HasField(key):
                return super(PbField, self).output(key, obj)
        except ValueError:
            return None


class NonNullNested(fields.Nested):

    def __init__(self, *args, **kwargs):
        super(NonNullNested, self).__init__(*args, **kwargs)
        self.display_null = False
        self.allow_null = True


class NonNullProtobufNested(NonNullNested):
    """
    this class is for nested protobuff field
    we don't want to display null or non defined values
    """
    def __init__(self, *args, **kwargs):
        super(NonNullProtobufNested, self).__init__(*args, **kwargs)
        self.display_null = False
        self.allow_null = True

    def output(self, key, obj):
        if not obj or not obj.HasField(key):
            value = None
        else:
            value = fields.get_value(key if self.attribute is None else self.attribute, obj)
        if self.allow_null and value is None:
            return None
        return fields.marshal(value, self.nested, self.display_null)


class DateTime(fields.Raw):
    """
    custom date format from timestamp
    """
    def __init__(self, *args, **kwargs):
        super(DateTime, self).__init__(*args, **kwargs)

    def output(self, key, obj):
        attribute = self.attribute or key
        if not obj.HasField(attribute):
            return self.default

        value = fields.get_value(attribute, obj)

        if value is None:
            return self.default

        return self.format(value)

    def format(self, value):
        return timestamp_to_str(value)


class Links(fields.Raw):
    def __init__(self, *args, **kwargs):
        super(Links, self).__init__(*args, **kwargs)

    def format(self, value):
        # note: some request args can be there several times,
        # but when there is only one elt, flask does not want lists
        args = {}
        for e in value.kwargs:
            if len(e.values) > 1:
                args[e.key] = [v for v in e.values]
            else:
                 args[e.key] = e.values[0]

        return create_external_link('v1.{}'.format(value.ressource_name),
                                    rel=value.rel,
                                    _type=value.type,
                                    templated=value.is_templated,
                                    description=value.description,
                                    **args)


# a time null value is represented by the max value (since 0 is a perfectly valid value)
# WARNING! be careful to change that value if the time type change (from uint64 to uint32 for example)
__date_time_null_value__ = 2**64 - 1


class SplitDateTime(DateTime):
    """
    custom date format from 2 fields:
      - one for the date (in timestamp to midnight)
      - one for the time in seconds from midnight

      the date can be null (for example when the time is for a period like for calendar schedule)

      if the date is not null be convert to local using the default timezone

      if the date is null, we do not convert anything.
            Thus the given date HAS to be previously converted to local
            if that is what is wanted
    """
    def __init__(self, date, time, *args, **kwargs):
        super(DateTime, self).__init__(*args, **kwargs)
        self.date = date
        self.time = time

    def output(self, key, obj):

        date = fields.get_value(self.date, obj) if self.date else None
        time = fields.get_value(self.time, obj)

        if time == __date_time_null_value__:
            return ""

        if not date:
            return self.format_time(time)

        date_time = date + time
        return self.format(date_time)

    @staticmethod
    def format_time(time):
        t = datetime.datetime.utcfromtimestamp(time)
        return t.strftime("%H%M%S")


class enum_type(fields.Raw):

    def output(self, key, obj):
        if self.attribute:
            key = self.attribute
        keys = key.split(".")
        if len(keys) > 1:
            try:
                if obj.HasField(keys[0]):
                    return enum_type().output(".".join(keys[1:]),
                                              getattr(obj, keys[0]))
                return None
            except ValueError:
                return None
        try:
            if not obj.HasField(key):
                return None
        except ValueError:
            return None
        enum = obj.DESCRIPTOR.fields_by_name[key].enum_type.values_by_number
        return enum[getattr(obj, key)].name.lower()


class PbEnum(fields.Raw):
    """
    output a protobuf enum as lower case string
    """

    def __init__(self, pb_enum_type, *args, **kwargs):
        super(PbEnum, self).__init__(*args, **kwargs)
        self.pb_enum_type = pb_enum_type

    def format(self, value):
        return self.pb_enum_type.Name(value).lower()


class NonNullList(fields.List):

    def __init__(self, *args, **kwargs):
        super(NonNullList, self).__init__(*args, **kwargs)
        self.display_empty = False


class NonNullString(fields.Raw):
    """
    Print a string if it is not null
    """
    def __init__(self, *args, **kwargs):
        super(NonNullString, self).__init__(*args, **kwargs)

    def output(self, key, obj):
        k = key if self.attribute is None else self.attribute
        if not obj or not obj.HasField(k):
            return None
        else:
            return fields.get_value(k, obj)


class additional_informations(fields.Raw):

    def output(self, key, obj):
        properties = obj.properties
        descriptor = properties.DESCRIPTOR
        enum = descriptor.enum_types_by_name["AdditionalInformation"]
        return [enum.values_by_number[v].name.lower() for v
                in properties.additional_informations]


class equipments(fields.Raw):
    def output(self, key, obj):
        equipments = obj.has_equipments
        descriptor = equipments.DESCRIPTOR
        enum = descriptor.enum_types_by_name["Equipment"]
        return [enum.values_by_number[v].name.lower() for v
                in equipments.has_equipments]


class disruption_status(fields.Raw):
    def output(self, key, obj):
        status = obj.status
        enum = type_pb2._ACTIVESTATUS
        return enum.values_by_number[status].name.lower()

class channel_types(fields.Raw):
    def output(self, key, obj):
        channel = obj
        descriptor = channel.DESCRIPTOR
        enum = descriptor.enum_types_by_name["ChannelType"]
        return [enum.values_by_number[v].name.lower() for v
                in channel.channel_types]


class notes(fields.Raw):

    def output(self, key, obj):
        properties = obj.has_properties
        r = []
        for note_ in properties.notes:
            r.append({"id": note_.uri, "value": note_.note})
        return r


class stop_time_properties_links(fields.Raw):

    def output(self, key, obj):
        properties = obj.properties
        r = []
        #Note: all those links should be created with crete_{internal|external}_links,
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


class get_label(fields.Raw):

    def output(self, key, obj):
        if obj.code != '':
            return obj.code
        else:
            if obj.name != '':
                return obj.name

class get_key_value(fields.Raw):

    def output(self, key, obj):
        res = {}
        for code in obj.properties:
            res[code.type] = code.value
        return res


class MultiLineString(fields.Raw):
    def __init__(self, **kwargs):
        super(MultiLineString, self).__init__(**kwargs)

    def output(self, key, obj):
        if hasattr(g, 'disable_geojson') and g.disable_geojson:
            return None

        val = fields.get_value(key if self.attribute is None else self.attribute, obj)

        lines = []
        for l in val.lines:
            lines.append([[c.lon, c.lat] for c in l.coordinates])

        response = {
            "type": "MultiLineString",
            "coordinates": lines,
        }
        return response


class FirstComment(fields.Raw):
    """
    for compatibility issue we want to continue to output a 'comment' field
    even if now we have a list of comments, so we take the first one
    """
    def output(self, key, obj):
        for c in obj.comments:
            return c.value
        return None


class SectionGeoJson(fields.Raw):
    """
    format a journey section as geojson
    """
    def __init__(self, **kwargs):
        super(SectionGeoJson, self).__init__(**kwargs)

    def output(self, key, obj):
        coords = []
        if not obj.HasField(str("type")):
            logging.getLogger(__name__).warn("trying to output wrongly formated object as geojson, we skip")
            return

        if obj.type == response_pb2.STREET_NETWORK:
            coords = obj.street_network.coordinates
        elif obj.type == response_pb2.CROW_FLY and len(obj.shape) != 0:
            coords = obj.shape
        elif obj.type == response_pb2.PUBLIC_TRANSPORT:
            coords = obj.shape
        elif obj.type == response_pb2.TRANSFER:
            coords.append(obj.origin.stop_point.coord)
            coords.append(obj.destination.stop_point.coord)
        else:
            return

        response = {
            "type": "LineString",
            "coordinates": [],
            "properties": [{
                "length": 0 if not obj.HasField(str("length")) else obj.length
            }]
        }
        for coord in coords:
            response["coordinates"].append([coord.lon, coord.lat])
        return response


class JsonString(fields.Raw):
    def __init__(self, **kwargs):
        super(JsonString, self).__init__(**kwargs)

    def format(self, value):

        json = str(value)
        response = ujson.loads(json)

        return response


class Durations(fields.Raw):
    def output(self, key, obj):
        if not obj.HasField(str("durations")):
            return
        return {
            'total': obj.durations.total,
            'walking': obj.durations.walking
        }


class DisruptionLinks(fields.Raw):
    """
    Add link to disruptions on a pt object
    """
    def output(self, key, obj):
        return [create_internal_link(_type="disruption", rel="disruptions", id=uri)
                for uri in obj.impact_uris]


class FieldDateTime(fields.Raw):
    """
    DateTime in timezone of region
    """
    def output(self, key, region):
        if 'timezone' in region and key in region:
            dt = datetime.datetime.utcfromtimestamp(region[key])
            tz = pytz.timezone(region["timezone"])
            if tz:
                dt = pytz.utc.localize(dt)
                dt = dt.astimezone(tz)
                return dt.strftime("%Y%m%dT%H%M%S")
            else:
                return None
        else:
            return None


class Integer(fields.Raw):
    """
    Integer with round value
    2.3 -> 2
    2.6 -> 3
    """
    def __init__(self, default=0, **kwargs):
        super(Integer, self).__init__(default=default, **kwargs)

    def format(self, value):
        if value is None:
            return self.default

        return int(round(value))

validity_pattern = {
    'beginning_date': fields.String(),
    'days': fields.String(),
}

trip = {
    'id': fields.String(attribute="uri"),
    'name': fields.String(),
}

code = {
    "type": fields.String(),
    "value": fields.String()
}

prop = {
    "name": fields.String(),
    "value": fields.String()
}

period = {
    "begin": DateTime(),
    "end": DateTime(),
}

channel = {
    "content_type": fields.String(),
    "id": fields.String(),
    "name": fields.String(),
    "types": channel_types()
}

disruption_message = {
    "text": fields.String(),
    "channel": NonNullNested(channel)
}

disruption_severity = {
    "name": fields.String(),
    "effect": fields.String(),
    "color": fields.String(),
    "priority": fields.Integer(),
}


display_informations_route = {
    "network": fields.String(attribute="network"),
    "direction": fields.String(attribute="direction"),
    "commercial_mode": fields.String(attribute="commercial_mode"),
    "label": get_label(attribute="display_information"),
    "color": fields.String(attribute="color"),
    "code": fields.String(attribute="code"),
    "links": DisruptionLinks(),
    "text_color": fields.String(attribute="text_color"),
}

display_informations_vj = {
    "description": fields.String(attribute="description"),
    "physical_mode": fields.String(attribute="physical_mode"),
    "commercial_mode": fields.String(attribute="commercial_mode"),
    "network": fields.String(attribute="network"),
    "direction": fields.String(attribute="direction"),
    "label": get_label(attribute="display_information"),
    "color": fields.String(attribute="color"),
    "code": fields.String(attribute="code"),
    "equipments": equipments(attribute="has_equipments"),
    "headsign": fields.String(attribute="headsign"),
    "headsigns": NonNullList(fields.String()),
    "links": DisruptionLinks(),
    "text_color": fields.String(attribute="text_color"),
}


class DoubleToStringField(fields.Raw):
    def format(self, value):
        # we don't want to loose precision while converting a double to string
        return "{:.16g}".format(value)

coord = {
    "lon": DoubleToStringField(),
    "lat": DoubleToStringField()
}

generic_type = {
    "name": fields.String(),
    "id": fields.String(attribute="uri"),
    "coord": NonNullNested(coord, True)
}

comment = {
    'value': fields.String(),
    'type': fields.String()
}

feed_publisher = {
    "id": fields.String(),
    "name": fields.String(),
    "url": fields.String(),
    "license": fields.String()
}

admin = deepcopy(generic_type)
admin["level"] = fields.Integer
admin["zip_code"] = fields.String
admin["label"] = fields.String()
admin["insee"] = fields.String()

generic_type_admin = deepcopy(generic_type)
admins = NonNullList(NonNullNested(admin))
generic_type_admin["administrative_regions"] = admins


stop_point = deepcopy(generic_type_admin)
stop_point["links"] = DisruptionLinks()
stop_point["comment"] = FirstComment()
# for compatibility issue we keep a 'comment' field where we output the first comment (TODO v2)
stop_point["comments"] = NonNullList(NonNullNested(comment))
stop_point["codes"] = NonNullList(NonNullNested(code))
stop_point["label"] = fields.String()
stop_area = deepcopy(generic_type_admin)
stop_area["links"] = DisruptionLinks()
stop_area["comment"] = FirstComment()
# for compatibility issue we keep a 'comment' field where we output the first comment (TODO v2)
stop_area["comments"] = NonNullList(NonNullNested(comment))
stop_area["codes"] = NonNullList(NonNullNested(code))
stop_area["timezone"] = fields.String()
stop_area["label"] = fields.String()

journey_pattern_point = {
    "id": fields.String(attribute="uri"),
}

journey_pattern = deepcopy(generic_type)
jpps = NonNullList(NonNullNested(journey_pattern_point))
journey_pattern["journey_pattern_points"] = jpps
stop_time = {
    "arrival_time": SplitDateTime(date=None, time='arrival_time'),
    "departure_time": SplitDateTime(date=None, time='departure_time'),
    "headsign": fields.String(attribute="headsign"),
    "journey_pattern_point": NonNullProtobufNested(journey_pattern_point),
    "stop_point": NonNullProtobufNested(stop_point)
}

line = deepcopy(generic_type)
line_group = deepcopy(generic_type)

line["links"] = DisruptionLinks()
line["code"] = fields.String()
line["color"] = fields.String()
line["text_color"] = fields.String()
line["comment"] = FirstComment()
# for compatibility issue we keep a 'comment' field where we output the first comment (TODO v2)
line["comments"] = NonNullList(NonNullNested(comment))
line["codes"] = NonNullList(NonNullNested(code))
line["geojson"] = MultiLineString(attribute="geojson")
line["opening_time"] = SplitDateTime(date=None, time="opening_time")
line["closing_time"] = SplitDateTime(date=None, time="closing_time")
line["properties"] = NonNullList(NonNullNested(prop))
line["line_groups"] = NonNullList(NonNullNested(line_group))

line_group["lines"] = NonNullList(NonNullNested(line))
line_group["main_line"] = PbField(line)
line_group["comments"] = NonNullList(NonNullNested(comment))

route = deepcopy(generic_type)
route["links"] = DisruptionLinks()
route["is_frequence"] = fields.String
route["line"] = PbField(line)
route["direction_type"] = fields.String
route["stop_points"] = NonNullList(NonNullNested(stop_point))
route["codes"] = NonNullList(NonNullNested(code))
route["geojson"] = MultiLineString(attribute="geojson")
route["comments"] = NonNullList(NonNullNested(comment))
line["routes"] = NonNullList(NonNullNested(route))
journey_pattern["route"] = PbField(route)

network = deepcopy(generic_type)
network["links"] = DisruptionLinks()
network["lines"] = NonNullList(NonNullNested(line))
network["codes"] = NonNullList(NonNullNested(code))
line["network"] = PbField(network)

commercial_mode = deepcopy(generic_type)
physical_mode = deepcopy(generic_type)
line["commercial_mode"] = PbField(commercial_mode)
line["physical_modes"] = NonNullList(NonNullNested(physical_mode))
route["physical_modes"] = NonNullList(NonNullNested(physical_mode))
stop_area["commercial_modes"] = NonNullList(NonNullNested(commercial_mode))
stop_area["physical_modes"] = NonNullList(NonNullNested(physical_mode))
stop_point["commercial_modes"] = NonNullList(NonNullNested(commercial_mode))
stop_point["physical_modes"] = NonNullList(NonNullNested(physical_mode))

poi_type = deepcopy(generic_type)
poi = deepcopy(generic_type_admin)
poi["poi_type"] = PbField(poi_type)
poi["properties"] = get_key_value()
poi["label"] = fields.String()

company = deepcopy(generic_type)
company["codes"] = NonNullList(NonNullNested(code))
stop_point["equipments"] = equipments(attribute="has_equipments")
stop_point["stop_area"] = PbField(deepcopy(stop_area))
stop_area["stop_point"] = PbField(deepcopy(stop_point))
journey_pattern_point["stop_point"] = PbField(deepcopy(stop_point))
address = deepcopy(generic_type_admin)
address["house_number"] = fields.Integer()
address["label"] = fields.String()

stop_point["address"] = PbField(address)
poi["address"] = PbField(address)

contributor = {
    "id": fields.String(attribute='uri'),
    "name": fields.String(),
    "website": fields.String(),
    "license": fields.String()
}

dataset = {
    "id": fields.String(attribute='uri'),
    "description": fields.String(attribute='desc'),
    "system": fields.String(),
    "start_validation_date": DateTime(),
    "end_validation_date": DateTime(),
    "realtime_level": enum_type(),
    "contributor": PbField(contributor)
}

connection = {
    "origin": PbField(stop_point),
    "destination": PbField(stop_point),
    "duration": fields.Integer(),
    "display_duration": fields.Integer(),
    "max_duration": fields.Integer(),
}

stop_date_time = {
    "departure_date_time": DateTime(),
    "base_departure_date_time": DateTime(),
    "arrival_date_time": DateTime(),
    "base_arrival_date_time": DateTime(),
    "stop_point": PbField(stop_point),
    "additional_informations": additional_informations,
    "links": stop_time_properties_links,
    "data_freshness": enum_type()
}

place = {
    "stop_point": PbField(stop_point),
    "stop_area": PbField(stop_area),
    "address": PbField(address),
    "poi": PbField(poi),
    "administrative_region": PbField(admin),
    "embedded_type": enum_type(),
    "name": fields.String(),
    "quality": fields.Integer(),
    "id": fields.String(attribute='uri')
}

pt_object = {
    "network": PbField(network),
    "commercial_mode": PbField(commercial_mode),
    "line": PbField(line),
    "route": PbField(route),
    "stop_area": PbField(stop_area),
    "trip": PbField(trip),
    "embedded_type": enum_type(),
    "name": fields.String(),
    "quality": fields.Integer(),
    "id": fields.String(attribute='uri')
}

route["direction"] = PbField(place)

pagination = {
    "total_result": fields.Integer(attribute="totalResult"),
    "start_page": fields.Integer(attribute="startPage"),
    "items_per_page": fields.Integer(attribute="itemsPerPage"),
    "items_on_page": fields.Integer(attribute="itemsOnPage"),
}

error = {
    'id': enum_type(),
    'message': fields.String()
}

beta_endpoint = {
    'id': Lit("beta_endpoint"),
    'message': Lit("This service is under construction. You can help through github.com/CanalTP/navitia"),
}


class UrisToLinks():

    def output(self, key, obj):
        display_info = obj.pt_display_informations
        uris = display_info.uris
        response = []
        if uris.line != '':
            response.append({"type": "line", "id": uris.line})
        if uris.company != '':
            response.append({"type": "company", "id": uris.company})
        if uris.vehicle_journey != '':
            response.append({"type": "vehicle_journey",
                             "id": uris.vehicle_journey})
        if uris.route != '':
            response.append({"type": "route", "id": uris.route})
        if uris.commercial_mode != '':
            response.append({"type": "commercial_mode",
                             "id": uris.commercial_mode})
        if uris.physical_mode != '':
            response.append({"type": "physical_mode",
                             "id": uris.physical_mode})
        if uris.network != '':
            response.append({"type": "network", "id": uris.network})
        if uris.note != '':
            response.append({"type": "note", "id": uris.note})

        for value in display_info.notes:
            response.append({"type": 'notes',
                            # Note: type should be 'note' but for retrocompatibility, we can't change it
                             "rel": 'notes',
                             "id": value.uri,
                             'value': value.note,
                             'internal': True})
        return response


instance_status = {
    "data_version": fields.Integer(),
    "end_production_date": fields.String(),
    "is_connected_to_rabbitmq": fields.Boolean(),
    "last_load_at": fields.String(),
    "last_rt_data_loaded": fields.String(),
    "last_load_status": fields.Boolean(),
    "kraken_version": fields.String(attribute="navitia_version"),
    "nb_threads": fields.Integer(),
    "publication_date": fields.String(),
    "start_production_date": fields.String(),
    "status": fields.String(),
    "is_open_data": fields.Boolean(),
    "is_open_service": fields.Boolean(),
    "is_realtime_loaded": fields.Boolean(),
    "realtime_proxies": fields.Raw(),
    "dataset_created_at": fields.String(),
}

instance_parameters = {
    'scenario': fields.Raw(attribute='_scenario_name'),
    'journey_order': fields.Raw,
    'max_walking_duration_to_pt': fields.Raw,
    'max_bike_duration_to_pt': fields.Raw,
    'max_bss_duration_to_pt': fields.Raw,
    'max_car_duration_to_pt': fields.Raw,
    'max_nb_transfers': fields.Raw,
    'walking_speed': fields.Raw,
    'bike_speed': fields.Raw,
    'bss_speed': fields.Raw,
    'car_speed': fields.Raw,
    'min_bike': fields.Raw,
    'min_car': fields.Raw,
    'min_bss': fields.Raw,
    'min_tc_with_bike': fields.Raw,
    'min_tc_with_car': fields.Raw,
    'min_tc_with_bss': fields.Raw,
    'factor_too_long_journey': fields.Raw,
    'min_duration_too_long_journey': fields.Raw,
    'max_duration_criteria': fields.Raw,
    'max_duration_fallback_mode': fields.Raw,
    'max_duration': fields.Raw,
    'walking_transfer_penalty': fields.Raw,
    'night_bus_filter_max_factor': fields.Raw,
    'night_bus_filter_base_factor': fields.Raw,
    'priority': fields.Raw,
    'bss_provider': fields.Boolean,
    'successive_physical_mode_to_limit_id': fields.Raw,
    'max_additional_connections': fields.Raw
}

instance_status_with_parameters = deepcopy(instance_status)
instance_status_with_parameters['parameters'] = fields.Nested(instance_parameters, allow_null=True)
instance_status_with_parameters['realtime_contributors'] = fields.List(fields.String(), attribute='rt_contributors')

instance_traveler_types = {
    'traveler_type': fields.String,
    'walking_speed': fields.Raw,
    'bike_speed': fields.Raw,
    'bss_speed': fields.Raw,
    'car_speed': fields.Raw,
    'wheelchair': fields.Boolean(),
    'max_walking_duration_to_pt': fields.Raw,
    'max_bike_duration_to_pt': fields.Raw,
    'max_bss_duration_to_pt': fields.Raw,
    'max_car_duration_to_pt': fields.Raw,
    'first_section_mode': fields.List(fields.String),
    'last_section_mode': fields.List(fields.String),
    'is_from_db': fields.Boolean(),
}

instance_status_with_parameters['traveler_profiles'] = fields.List(fields.Nested(instance_traveler_types,
                                                                                 allow_null=True))

impacted_section = {
    'from': NonNullNested(pt_object),
    'to': NonNullNested(pt_object),
}

impacted_stop = {
    "stop_point": NonNullNested(stop_point),
    "base_arrival_time": SplitDateTime(date=None, time='base_stop_time.arrival_time'),
    "base_departure_time": SplitDateTime(date=None, time='base_stop_time.departure_time'),
    "amended_arrival_time": SplitDateTime(date=None, time='amended_stop_time.arrival_time'),
    "amended_departure_time": SplitDateTime(date=None, time='amended_stop_time.departure_time'),
    "cause": fields.String(),
    "stop_time_effect": enum_type(attribute='effect')
}

impacted_object = {
    'pt_object': NonNullNested(pt_object),
    'impacted_stops': NonNullList(NonNullNested(impacted_stop)),
    'impacted_section': NonNullProtobufNested(impacted_section)
}

disruption_marshaller = {
    "id": fields.String(attribute="uri"),
    "disruption_id": fields.String(attribute="disruption_uri"),
    "impact_id": fields.String(attribute="uri"),
    "title": fields.String(),
    "application_periods": NonNullList(NonNullNested(period)),
    "status": disruption_status,
    "updated_at": DateTime(),
    "tags": NonNullList(fields.String()),
    "cause": fields.String(),
    "category": NonNullString(),
    "severity": NonNullNested(disruption_severity),
    "messages": NonNullList(NonNullNested(disruption_message)),
    "impacted_objects": NonNullList(NonNullNested(impacted_object)),
    "uri": fields.String(),
    "disruption_uri": fields.String(),
    "contributor": fields.String()
}
