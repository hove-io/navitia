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

from flask.ext.restful import fields
from copy import deepcopy
from collections import OrderedDict
import datetime
import logging
import pytz
from jormungandr.timezone import get_timezone


class PbField(fields.Nested):

    def __init__(self, nested, allow_null=True, **kwargs):
        super(PbField, self).__init__(nested, **kwargs)
        self.display_null = False

    def output(self, key, obj):
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


class DateTime(fields.Raw):
    """
    custom date format from timestamp
    """
    def __init__(self, *args, **kwargs):
        super(DateTime, self).__init__(*args, **kwargs)

    def output(self, key, obj):
        tz = get_timezone()

        value = fields.get_value(key if self.attribute is None else self.attribute, obj)

        if value is None:
            return self.default

        return self.format(value, tz)

    def format(self, value, timezone):
        dt = datetime.datetime.utcfromtimestamp(value)

        if timezone:
            dt = pytz.utc.localize(dt)
            dt = dt.astimezone(timezone)
            return dt.strftime("%Y%m%dT%H%M%S")
        return None  # for the moment I prefer not to display anything instead of something wrong


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

        date = fields.get_value(self.date, obj)
        time = fields.get_value(self.time, obj)

        if not date:
            return self.format_time(time)

        date_time = date + time
        tz = get_timezone()
        return self.format(date_time, timezone=tz)

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
        return str.lower(enum[getattr(obj, key)].name)


class NonNullList(fields.List):

    def __init__(self, *args, **kwargs):
        super(NonNullList, self).__init__(*args, **kwargs)
        self.display_empty = False


class additional_informations(fields.Raw):

    def output(self, key, obj):
        properties = obj.properties
        descriptor = properties.DESCRIPTOR
        enum = descriptor.enum_types_by_name["AdditionalInformation"]
        return [str.lower(enum.values_by_number[v].name) for v
                in properties.additional_informations]


class additional_informations_vj(fields.Raw):

    def output(self, key, obj):
        addinfo = obj.add_info_vehicle_journey
        result = []
        if (addinfo.has_date_time_estimated):
            result.append("has_date_time_estimated")

        descriptor = addinfo.DESCRIPTOR
        enum_t = descriptor.fields_by_name['vehicle_journey_type'].enum_type
        values = enum_t.values_by_name
        vj_type = addinfo.vehicle_journey_type
        if vj_type == values['virtual_with_stop_time'].number:
            result.append("odt_with_stop_time")
        else:
            if vj_type == values['virtual_without_stop_time'].number or \
               vj_type == values['stop_point_to_stop_point'].number or \
               vj_type == values['address_to_stop_point'].number or \
               vj_type == values['odt_point_to_point'].number:
                result.append("odt_with_zone")
            else:
                result.append("regular")
        return result


class equipments(fields.Raw):
    def output(self, key, obj):
        equipments = obj.has_equipments
        descriptor = equipments.DESCRIPTOR
        enum = descriptor.enum_types_by_name["Equipment"]
        return [str.lower(enum.values_by_number[v].name) for v
                in equipments.has_equipments]

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
        for note_ in properties.notes:
            r.append({"id": note_.uri, "type": "notes", "value": note_.note})
        for exception in properties.exceptions:
            r.append({"type": "exceptions", "id": exception.uri, "date": exception.date,
                      "except_type": exception.type})
        if properties.destination and properties.destination.uri:
            r.append({"type": "notes", "id": properties.destination.uri, "value": properties.destination.destination})
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



code = {
    "type": fields.String(),
    "value": fields.String()
}

generic_message = {
    "level": enum_type(attribute="message_status"),
    "value": fields.String(attribute="message"),
    "start_application_date": fields.String(),
    "end_application_date": fields.String(),
    "start_application_daily_hour": fields.String(),
    "end_application_daily_hour": fields.String(),
}

display_informations_route = {
    "network": fields.String(attribute="network"),
    "direction": fields.String(attribute="direction"),
    "commercial_mode": fields.String(attribute="commercial_mode"),
    "label": get_label(attribute="display_information"),
    "color": fields.String(attribute="color"),
    "code": fields.String(attribute="code"),
    "messages": NonNullList(NonNullNested(generic_message))
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
    "headsign" : fields.String(attribute="headsign"),
    "messages": NonNullList(NonNullNested(generic_message))
}

coord = {
    "lon": fields.Float(),
    "lat": fields.Float()
}

generic_type = {
    "name": fields.String(),
    "id": fields.String(attribute="uri"),
    "coord": NonNullNested(coord, True)
}

admin = deepcopy(generic_type)
admin["level"] = fields.Integer
admin["zip_code"] = fields.String

generic_type_admin = deepcopy(generic_type)
admins = NonNullList(NonNullNested(admin))
generic_type_admin["administrative_regions"] = admins


stop_point = deepcopy(generic_type_admin)
stop_point["messages"] = NonNullList(NonNullNested(generic_message))
stop_point["comment"] = fields.String()
stop_point["codes"] = NonNullList(NonNullNested(code))
stop_area = deepcopy(generic_type_admin)
stop_area["messages"] = NonNullList(NonNullNested(generic_message))
stop_area["comment"] = fields.String()
stop_area["codes"] = NonNullList(NonNullNested(code))
stop_area["timezone"] = fields.String()
journey_pattern_point = deepcopy(generic_type)
journey_pattern = deepcopy(generic_type)
jpps = NonNullList(NonNullNested(journey_pattern_point))
journey_pattern["journey_pattern_points"] = jpps
stop_time = {
    "arrival_time" : fields.String(),
    "departure_time" : fields.String(),
    "journey_pattern_point" : NonNullNested(journey_pattern_point)
}
vehicle_journey = deepcopy(generic_type)
vehicle_journey["messages"] = NonNullList(NonNullNested(generic_message))
vehicle_journey["journey_pattern"] = PbField(journey_pattern)
vehicle_journey["stop_times"] = NonNullList(NonNullNested(stop_time))
vehicle_journey["comment"] = fields.String()
vehicle_journey["codes"] = NonNullList(NonNullNested(code))
line = deepcopy(generic_type)
line["messages"] = NonNullList(NonNullNested(generic_message))
line["code"] = fields.String()
line["color"] = fields.String()
line["comment"] = fields.String()
line["codes"] = NonNullList(NonNullNested(code))

route = deepcopy(generic_type)
route["messages"] = NonNullList(NonNullNested(generic_message))
route["is_frequence"] = fields.String
route["line"] = PbField(line)
route["thermometer"] = PbField({"stop_points" : NonNullList(NonNullNested(stop_point))})
route["codes"] = NonNullList(NonNullNested(code))
line["routes"] = NonNullList(NonNullNested(route))
journey_pattern["route"] = PbField(route)

network = deepcopy(generic_type)
network["messages"] = NonNullList(NonNullNested(generic_message))
network["lines"] = NonNullList(NonNullNested(line))
network["codes"] = NonNullList(NonNullNested(code))
line["network"] = PbField(network)

commercial_mode = deepcopy(generic_type)
physical_mode = deepcopy(generic_type)
commercial_mode["physical_modes"] = NonNullList(NonNullNested(commercial_mode))
physical_mode["commercial_modes"] = NonNullList(NonNullNested(physical_mode))
line["commercial_mode"] = PbField(commercial_mode)

poi_type = deepcopy(generic_type)
poi = deepcopy(generic_type_admin)
poi["poi_type"] = PbField(poi_type)
poi["properties"] = get_key_value()

company = deepcopy(generic_type)
company["codes"] = NonNullList(NonNullNested(code))
stop_point["equipments"] = equipments(attribute="has_equipments")
stop_point["stop_area"] = PbField(deepcopy(stop_area))
stop_area["stop_point"] = PbField(deepcopy(stop_point))
journey_pattern_point["stop_point"] = PbField(deepcopy(stop_point))
address = deepcopy(generic_type_admin)
address["house_number"] = fields.Integer()

stop_point["address"] = PbField(address)
poi["address"] = PbField(address)

connection = {
    "origin": PbField(stop_point),
    "destination": PbField(stop_point),
    "duration": fields.Integer(attribute="seconds")
}

stop_date_time = {
    "departure_date_time": DateTime(),
    "arrival_date_time": DateTime(),
    "stop_point": PbField(stop_point),
    "additional_informations": additional_informations,
    "links": stop_time_properties_links
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
        return response
