from flask.ext.restful import fields
from copy import deepcopy
from collections import OrderedDict

class PbField(fields.Nested):
    def __init__(self, nested, allow_null=True, **kwargs):
        super(PbField, self).__init__(nested, **kwargs)
        self.display_null = False

    def output(self, key, obj):
        if self.attribute:
            key = self.attribute
        try :
            if obj.HasField(key):
                return super(PbField, self).output(key, obj)
        except ValueError:
            return None


class NonNullNested(fields.Nested):
    def __init__(self, *args, **kwargs):
        super(NonNullNested, self).__init__(*args, **kwargs)
        self.display_null = False


class enum_type(fields.Raw):
    def output(self, key, obj):
        if self.attribute:
            key = self.attribute
        keys = key.split(".")
        if len(keys) > 1:
            try :
                if obj.HasField(keys[0]):
                    return enum_type().output(".".join(keys[1:]), getattr(obj, keys[0]))
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
        properties = getattr(obj, "has_properties")
        enum = properties.DESCRIPTOR.enum_types_by_name["AdditionalInformation"]
        return [str.lower(enum.values_by_number[v].name) for v
                in properties.additional_informations]

class additional_informations_header(fields.Raw):
    def output(self, key, obj):
        addinfo = getattr(obj, "add_info_vehicle_journey")
        enum_t = addinfo.DESCRIPTOR.fields_by_name['vehicle_journey_type'].enum_type.values_by_name
        if addinfo.vehicle_journey_type == enum_t['virtual_with_stop_time'].number :
            return ["virtual_with_stop_time"]
        if addinfo.vehicle_journey_type == enum_t['virtual_without_stop_time'].number:
            return ["virtual_without_stop_time"]
        if addinfo.vehicle_journey_type == enum_t['stop_point_to_stop_point'].number:
            return ["stop_point_to_stop_point"]
        if addinfo.vehicle_journey_type == enum_t['adress_to_stop_point'].number:
            return ["adress_to_stop_point"]
        if addinfo.vehicle_journey_type == enum_t['odt_point_to_point'].number:
            return ["odt_point_to_point"]
        return ["regular"]

class has_vehicle_propertie(fields.Raw):
    def output(self, key, obj):
        properties = getattr(obj, "has_vehicle_properties")
        enum = properties.DESCRIPTOR.enum_types_by_name["VehiclePropertie"]
        return [str.lower(enum.values_by_number[v].name) for v
                in properties.vehicle_properties]

class display_informations(fields.Raw):
    def output(self, key, obj):
        display_information = getattr(obj, "pt_display_informations")
        result = {}
        if display_information.network != '' :
            result["network"] = display_information.network
        if display_information.headsign != '':
            result["headsign"] = display_information.headsign
        if display_information.direction != '':
            result["direction"] = display_information.direction
        if display_information.physical_mode != '':
            result["physical_mode"] = display_information.physical_mode
        if display_information.description != '':
            result["description"] = display_information.description
        if display_information.name != '':
            result["label"] = display_information.name
        if display_information.code != '':
            result["label"] = display_information.code
        if display_information.color != '':
            result["color"] = display_information.color
        if display_information.commercial_mode != '':
            result["commercial_mode"] = display_information.commercial_mode
        #result["equipments"] = has_vehicle_propertie()

        return result

class notes(fields.Raw):
    def output(self, key, obj):
        properties = getattr(obj, "has_properties")
        r = []
        for note_ in properties.notes:
            r.append({"id": note_.uri, "value": note_.note})
        return r

class notes_links(fields.Raw):
    def output(self, key, obj):
        properties = getattr(obj, "has_properties")
        r = []
        for note_ in properties.notes:
            r.append({"id": note_.uri, "type": "notes", "value": note_.note})
        return r

class equipments():
    def output(self, key, obj):
        vehiclejourney = getattr(obj, "vehiclejourney")
        eq = []
        if vehiclejourney.visual_announcement:
            eq.append({"id": "has_visual_announcement"})
        if vehiclejourney.audible_announcement:
            eq.append({"id": "has_audible_announcement"})
        if vehiclejourney.appropriate_escort:
            eq.append({"id": "has_appropriate_escort"})
        if vehiclejourney.appropriate_signage:
            eq.append({"id": "has_appropriate_signage"})
        return eq

class get_label():
    def output(self, key, obj):
        line = getattr(obj, "line")
        if line.code != '':
            return line.code
        else :
            if line.name != '':
                return line.name
            else:
                route = getattr(obj, "route")
                return route.name


coord = {
    "lon" : fields.Float(),
    "lat" : fields.Float()
}

generic_type = {
    "name" : fields.String(),
    "id" : fields.String(attribute="uri"),
    "coord" : NonNullNested(coord, True)
}

admin = deepcopy(generic_type)
admin["level"] = fields.Integer
admin["zip_code"] = fields.String

generic_type_admin = deepcopy(generic_type)
generic_type_admin["administrative_regions"] = NonNullList(NonNullNested(admin))

stop_point = deepcopy(generic_type_admin)
stop_area = deepcopy(generic_type_admin)
journey_pattern_point = deepcopy(generic_type_admin)
line = deepcopy(generic_type)
line["code"] = fields.String()
line["color"] = fields.String()

route = deepcopy(generic_type)
route["is_frequence"] = fields.String
route["line"] = PbField(line)
line["routes"] = NonNullList(NonNullNested(route))

network = deepcopy(generic_type)
network["lines"] = NonNullList(NonNullNested(line))
line["network"] = PbField(network)

commercial_mode = deepcopy(generic_type)
physical_mode = deepcopy(generic_type)
commercial_mode["physical_modes"] = NonNullList(NonNullNested(commercial_mode))
physical_mode["commercial_modes"] = NonNullList(NonNullNested(physical_mode))
line["commercial_mode"] = PbField(commercial_mode)

poi_type = deepcopy(generic_type)
poi = deepcopy(generic_type)
poi["poi_type"] = PbField(poi_type)

company = deepcopy(generic_type)

stop_point["stop_area"] = PbField(deepcopy(stop_area))
stop_area["stop_point"] = PbField(deepcopy(stop_point))
journey_pattern_point["stop_point"] = PbField(deepcopy(stop_point))
address = deepcopy(generic_type_admin)
address["house_number"] = fields.Integer()


stop_date_time = {
    "departure_date_time" : fields.String(),
    "arrival_date_time" : fields.String(),
    "stop_point" : PbField(stop_point),
    "additional_informations" : additional_informations
}


place = {
    "stop_point" : PbField(stop_point),
    "stop_area" : PbField(stop_area),
    "address" : PbField(address),
    "poi" : PbField(poi),
    "embedded_type" : enum_type(),
    "name" : fields.String(),
    "id" : fields.String(attribute='uri')
}

pagination = {
    "total_result": fields.Integer(attribute="totalResult"),
    "start_page" : fields.Integer(attribute="startPage"),
    "items_per_page" : fields.Integer(attribute="itemsPerPage"),
    "items_on_page" : fields.Integer(attribute="itemsOnPage"),
}

class StopScheduleLinks():
    def output(self, key, obj):
        route = getattr(obj, "route")
        response = []
        response.append({"type": "route", "id": route.uri})
        response.append({"type": "line", "id": route.line.uri})
        response.append({"type": "commercial_mode", "id": route.line.commercial_mode.uri})
        response.append({"type": "network", "id": route.line.network.uri})
        return response


class UrisToLinks():
    def output(self, key, obj):
        display_info = getattr(obj, "pt_display_informations")
        uris = getattr(display_info, "uris")
        response = []
        if uris.line != '' :
            response.append({"type": "line", "id": uris.line})
        if uris.company != '' :
            response.append({"type": "company", "id": uris.company})
        if uris.vehicle_journey != '' :
            response.append({"type": "vehicle_journey", "id": uris.vehicle_journey})
        if uris.route != '' :
            response.append({"type": "route", "id": uris.route})
        if uris.commercial_mode != '' :
            response.append({"type": "commercial_mode", "id": uris.commercial_mode})
        if uris.physical_mode != '' :
            response.append({"type": "physical_mode", "id": uris.physical_mode})
        if uris.network != '' :
            response.append({"type": "network", "id": uris.network})
        if uris.note != '' :
            response.append({"type": "note", "id": uris.note})
        return response