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

route = deepcopy(generic_type)
route["is_frequence"] = fields.String
route["line"] = PbField(line)

network = deepcopy(generic_type)
network["lines"] = fields.List(fields.Nested(line))

commercial_mode = deepcopy(generic_type)
physical_mode = deepcopy(generic_type)
commercial_mode["physical_modes"] = NonNullList(NonNullNested(commercial_mode))
physical_mode["commercial_modes"] = NonNullList(NonNullNested(physical_mode))

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
