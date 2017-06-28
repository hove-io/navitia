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

from functools import partial

from jormungandr.interfaces.v1.serializer import jsonschema
import serpy
import operator

from jormungandr.interfaces.v1.serializer.jsonschema.fields import Field


class PbField(Field):
    def __init__(self, *args, **kwargs):
        super(PbField, self).__init__(*args, **kwargs)

    """
    This field handle protobuf, it aim to handle field absent value:
    When a field is not present protobuf return a default object, but we want a None.
    If the object is always initialised it's recommended to use serpy.Field as it will be faster
    """
    def as_getter(self, serializer_field_name, serializer_cls):
        op = operator.attrgetter(self.attr or serializer_field_name)
        def getter(obj):
            if obj is None:
                return None
            try:
                if obj.HasField(self.attr or serializer_field_name):
                    return op(obj)
                else:
                    return None
            except ValueError:
                #HasField throw an exception if the field is repeated...
                return op(obj)
        return getter

class NestedPbField(PbField):
    """
    handle nested pb field.
    
    define attr='base_stop_time.departure_time'
    
    it will get the departure_time field of the base_stop_time field
    """
    def as_getter(self, serializer_field_name, serializer_cls):
        attr = self.attr or serializer_field_name

        def getter(obj):
            cur_obj = obj
            for f in attr.split('.'):
                if not cur_obj.HasField(f):
                    return None
                cur_obj = getattr(cur_obj, f)
            return cur_obj
        return getter


class NullableDictSerializer(serpy.Serializer):
    @classmethod
    def default_getter(cls, attr):
        return lambda d: d.get(attr)


class PbNestedSerializer(serpy.Serializer, PbField):
    def __init__(self, *args, **kwargs):
        if 'display_none' not in kwargs:
            kwargs['display_none'] = False
        super(PbNestedSerializer, self).__init__(*args, **kwargs)


class EnumField(jsonschema.Field):
    def __init__(self, pb_type=None, **kwargs):
        schema_type = kwargs.pop('schema_type') if 'schema_type' in kwargs else str
        schema_metadata = kwargs.pop('schema_metadata') if 'schema_metadata' in kwargs else {}
        if pb_type:
            schema_metadata['enum'] = self._get_all_possible_values(pb_type)
        super(EnumField, self).__init__(schema_type=schema_type, schema_metadata=schema_metadata, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        def getter(val):
            attr = self.attr or serializer_field_name
            if val is None or not val.HasField(attr):
                return None
            enum = val.DESCRIPTOR.fields_by_name[attr].enum_type.values_by_number
            return enum[getattr(val, attr)].name
        return getter

    def to_value(self, value):
        if value is None:
            return None
        return value.lower()

    @staticmethod
    def _get_all_possible_values(pb_type):
        return [v.name for v in pb_type.DESCRIPTOR.values]


class EnumListField(EnumField):

    """WARNING: the enumlist field does not work without a self.attr"""

    def __init__(self, pb_type=None, **kwargs):
        super(EnumListField, self).__init__(pb_type, **kwargs)
        self.many = True

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda x: x

    def to_value(self, obj):
        enum = obj.DESCRIPTOR.fields_by_name[self.attr].enum_type.values_by_number
        return [enum[value].name.lower() for value in getattr(obj, self.attr)]


class GenericSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str, attr='uri', description='Identifier of the object')
    name = jsonschema.Field(schema_type=str, description='Name of the object')


class LiteralField(jsonschema.Field):
    """
    :return literal value
    """
    def __init__(self, value, *args, **kwargs):
        super(LiteralField, self).__init__(*args, **kwargs)
        self.value = value

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda *args, **kwargs: self.value


def flatten(obj):
    if type(obj) != dict:
        raise ValueError("Invalid argument")
    new_obj = {}
    for key, value in obj.items():
        if type(value) == dict:
            tmp = {'.'.join([key, _key]): _value for _key, _value in flatten(value).items()}
            new_obj.update(tmp)
        else:
            new_obj[key] = value
    return new_obj


def value_by_path(obj, path, default=None):
    new_obj = flatten(obj)
    if new_obj:
        return new_obj.get(path, default)
    else:
        return default


class NestedPropertyField(jsonschema.Field):
    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda v: value_by_path(v, self.attr)


class IntNestedPropertyField(NestedPropertyField):
    to_value = staticmethod(int)


class LambdaField(Field):
    getter_takes_serializer = True

    def __init__(self, method, **kwargs):
        super(LambdaField, self).__init__(**kwargs)
        self.method = method

    def as_getter(self, serializer_field_name, serializer_cls):
        return self.method


class DoubleToStringField(Field):

    def __init__(self, **kwargs):
        super(DoubleToStringField, self).__init__(schema_type=str, **kwargs)

    def to_value(self, value):
        # we don't want to loose precision while converting a double to string
        return "{:.16g}".format(value)
