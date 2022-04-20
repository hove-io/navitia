#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from jormungandr.interfaces.v1.serializer import jsonschema
import serpy
import six
import operator
import abc
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
                if self.display_none or obj.HasField(self.attr or serializer_field_name):
                    return op(obj)
                else:
                    return None
            except ValueError:
                # HasField throw an exception if the field is repeated...
                return op(obj)

        return getter


class PbStrField(PbField):
    def __init__(self, *args, **kwargs):
        super(PbStrField, self).__init__(schema_type=str, *args, **kwargs)

    to_value = staticmethod(six.text_type)


class PbIntField(PbField):
    def __init__(self, *args, **kwargs):
        super(PbIntField, self).__init__(schema_type=int, *args, **kwargs)

    to_value = staticmethod(int)


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
    def __init__(self, pb_type=None, lower_case=True, **kwargs):
        schema_type = kwargs.pop('schema_type') if 'schema_type' in kwargs else str
        schema_metadata = kwargs.pop('schema_metadata') if 'schema_metadata' in kwargs else {}
        self.lower_case = lower_case
        if pb_type:
            schema_metadata['enum'] = self._get_all_possible_values(pb_type, self.lower_case)
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
        if self.lower_case:
            return value.lower()
        else:
            return value

    @staticmethod
    def _get_all_possible_values(pb_type, lower_case):
        if lower_case:
            return [v.name.lower() for v in pb_type.DESCRIPTOR.values]
        else:
            return [v.name for v in pb_type.DESCRIPTOR.values]


class NestedEnumField(EnumField):
    """
    handle nested Enum field.

    define attr='street_network.mode'

    it will get the mode of the street_network field
    """

    def as_getter(self, serializer_field_name, serializer_cls):
        def getter(val):
            attr = self.attr or serializer_field_name
            enum_field = attr.split('.')[-1]
            cur_obj = val
            for f in attr.split('.')[:-1]:
                if not cur_obj.HasField(f):
                    return None

                cur_obj = getattr(cur_obj, f)
                if not cur_obj:
                    return None

            if not cur_obj.HasField(enum_field):
                return None
            enum = cur_obj.DESCRIPTOR.fields_by_name[enum_field].enum_type.values_by_number
            ret_value = enum[getattr(cur_obj, enum_field)].name
            return ret_value

        return getter


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


class DictGenericSerializer(serpy.DictSerializer):
    id = serpy.StrField(display_none=True)
    name = serpy.StrField(display_none=True)


class DictCodeSerializer(serpy.DictSerializer):
    type = serpy.StrField(attr='name', display_none=True)
    value = serpy.StrField(display_none=True)


class PbGenericSerializer(PbNestedSerializer):
    id = jsonschema.Field(schema_type=str, display_none=True, attr='uri', description='Identifier of the object')
    name = jsonschema.Field(schema_type=str, display_none=True, description='Name of the object')


class SortedGenericSerializer(serpy.Serializer):
    """
    Sorted generic serializer can be used to sort an iterable container according to a specific key.
    The derived class needs to implement `sort_key(self, o)`, a function used to extract the comparison key (as the `key` argument from https://docs.python.org/3.3/library/functions.html#sorted)
    Implementation is based on 'serpy.Serializer.to_value' - https://serpy.readthedocs.io/en/latest/_modules/serpy/serializer.html#Serializer.to_value
    """

    @abc.abstractmethod
    def sort_key(self):
        raise NotImplementedError

    def to_value(self, obj):
        fields = self._compiled_fields
        serialize = self._serialize

        if self.many == False:
            return serialize(obj, fields)

        serialized_objs = (serialize(o, fields) for o in obj)
        return sorted(serialized_objs, key=self.sort_key)


class AmountSerializer(PbNestedSerializer):
    value = jsonschema.Field(schema_type=float)
    unit = jsonschema.Field(schema_type=str)

    # TODO check that retro compatibility is really useful
    def to_value(self, value):
        if value is None:
            return {'value': 0.0, 'unit': ''}
        return super(AmountSerializer, self).to_value(value)


class LiteralField(jsonschema.Field):
    """
    :return literal value
    """

    def __init__(self, value, *args, **kwargs):
        if 'display_none' not in kwargs:
            kwargs['display_none'] = True
        super(LiteralField, self).__init__(*args, **kwargs)
        self.value = value

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda *args, **kwargs: self.value


class DictCommentSerializer(serpy.DictSerializer):
    # To be compatible, type = 'standard'
    type = LiteralField('standard', display_none=True)
    value = serpy.StrField(attr='name', display_none=True)


def value_by_path(obj, path, default=None):
    """
    >>> value_by_path({'a': {'b' : {'c': 42}}}, 'a.b.c')
    42
    >>> value_by_path({'a': {'b' : {'c': 42}}}, 'a.b.c.d', default=4242)
    4242
    >>> value_by_path({'a': {'b' : {'c': 42}}}, 'a.b.c.d', default=0)
    0
    >>> value_by_path({'a': {'b' : {'c': 0}}}, 'a.b.c', default=None)
    0
    >>> value_by_path({'a': {'b' : {'c': 42}}}, 'a.b.c.d')
    >>> value_by_path({'a': {'b' : {'c': 42}}}, 'a.b.e')
    >>> value_by_path({'a': {'b' : {'c': 42}}}, 'a.b.c.d.e')

    :param obj: Dict obj or has a __getitem__ implemented
    :param path: path to the desired obj splitted by '.'
    :param default: default value if not exist
    :return:
    """
    # python3 compatibility
    import functools

    if not isinstance(obj, dict):
        raise ValueError("Invalid argument")
    splited_path = path.split('.')

    def pred(x, y):
        return x.get(y) if isinstance(x, dict) else None

    res = functools.reduce(pred, splited_path, obj)
    return res if res is not None else default


class NestedPropertyField(jsonschema.Field):
    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda v: value_by_path(v, self.attr)


class IntNestedPropertyField(NestedPropertyField):
    to_value = staticmethod(int)


class StringNestedPropertyField(NestedPropertyField):
    to_value = staticmethod(six.text_type)


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
        return '%.16g' % value


class DescribedField(LambdaField):
    """
    This class does not output anything, it's here only for description purpose
    (for field added outside of serpy, but that we want to describe in swagger)
    """

    def __init__(self, **kwargs):
        # the field returns always None and None are not displayed, so nothing is displayed
        super(DescribedField, self).__init__(method=lambda *args: None, display_none=False, **kwargs)


class BetaEndpointsSerializer(serpy.Serializer):
    def __init__(self, *args, **kwargs):
        super(BetaEndpointsSerializer, self).__init__(many=True, *args, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda _obj: [None]

    id = LiteralField("beta_endpoint", schema_type=str, display_none=True)
    message = LiteralField(
        'This service is under construction. You can help through github.com/hove-io/navitia',
        schema_type=str,
        display_none=True,
    )


def make_notes(notes):
    result = []
    for value in notes:
        note = {
            "type": "notes",
            "rel": "notes",
            "category": "comment",
            "id": value.uri,
            "value": value.note,
            "internal": True,
        }
        if value.comment_type:
            note["comment_type"] = value.comment_type
        result.append(note)
    return result


class NestedDictGenericField(DictGenericSerializer, NestedPropertyField):
    pass


class NestedDictCommentField(DictCommentSerializer, NestedPropertyField):
    pass


class NestedDictCodeField(DictCodeSerializer, NestedPropertyField):
    def to_value(self, value):
        value = super(NestedDictCodeField, self).to_value(value)
        if not value:
            return {}
        for code in value:
            if code.get('type') == 'navitia1':
                code['type'] = 'external_code'
        return value


class NestedPropertiesField(NestedPropertyField):
    def to_value(self, value):
        if not value:
            return {}
        return {p.get('key'): p.get('value') for p in value}


def get_proto_attr_or_default(obj, attr_name, default=None):
    return getattr(obj, attr_name) if obj.HasField(str(attr_name)) else default
