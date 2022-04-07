# coding=utf-8

# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

from __future__ import absolute_import
import serpy
from navitiacommon.parser_args_type import TypeSchema, CustomSchemaType


def _init(self, parent_class, schema_type=None, schema_metadata=None, **kwargs):
    """
    Call the parent constructor with the needed kwargs and
    add the remaining kwargs to the schema metadata
    """
    if 'display_none' not in kwargs:
        kwargs['display_none'] = False
    self.many = kwargs.pop('many', False)
    parent_vars = set(parent_class.__init__.__code__.co_names)
    parent_kwargs = {k: v for k, v in kwargs.items() if k in parent_vars}
    remaining_kwargs = {k: v for k, v in kwargs.items() if k not in parent_vars}
    parent_class.__init__(self, **parent_kwargs)
    self.schema_type = schema_type
    self.schema_metadata = schema_metadata or {}
    # the remaining kwargs are added in the schema metadata to add a bit of syntaxic sugar
    self.schema_metadata.update(**remaining_kwargs)


class Field(serpy.Field):
    """
    A :class:`Field` that hold metadata for schema.
    """

    def __init__(self, schema_type=None, schema_metadata=None, **kwargs):
        _init(self, serpy.Field, schema_type=schema_type, schema_metadata=schema_metadata, **kwargs)


class StrField(serpy.StrField):
    """
    A :class:`StrField` that hold metadata for schema.
    """

    def __init__(self, schema_metadata=None, **kwargs):
        _init(self, serpy.StrField, schema_type=str, schema_metadata=schema_metadata, **kwargs)


class BoolField(serpy.BoolField):
    """
    A :class:`BoolField` that hold metadata for schema.
    """

    def __init__(self, schema_metadata=None, **kwargs):
        _init(self, serpy.BoolField, schema_type=bool, schema_metadata=schema_metadata, **kwargs)


class FloatField(serpy.FloatField):
    """
    A :class:`FloatField` that hold metadata for schema.
    """

    def __init__(self, schema_metadata=None, **kwargs):
        _init(self, serpy.FloatField, schema_type=float, schema_metadata=schema_metadata, **kwargs)


class IntField(serpy.IntField):
    """
    A :class:`IntField` that hold metadata for schema.
    """

    def __init__(self, schema_metadata=None, **kwargs):
        _init(self, serpy.IntField, schema_type=int, schema_metadata=schema_metadata, **kwargs)


class MethodField(serpy.MethodField):
    """
    A :class:`MethodField` that hold metadata for schema.
    """

    def __init__(
        self,
        method=None,
        schema_type=None,
        schema_metadata=None,
        attr=None,
        call=False,
        label=None,
        required=True,
        display_none=False,
        **kwargs
    ):
        super(MethodField, self).__init__(
            method=method, attr=attr, call=call, label=label, required=required, display_none=display_none
        )

        self.schema_type = schema_type
        self.schema_metadata = schema_metadata or {}
        self.many = kwargs.pop('many', False)
        # the remaining kwargs are added in the schema metadata to add a bit of syntaxic sugar
        self.schema_metadata.update(**kwargs)


class DateTimeType(CustomSchemaType):
    # we do not respect the official swagger datetime format :(
    _schema = TypeSchema(
        type=str, metadata={'format': 'navitia-date-time', 'pattern': '\d{4}\d{2}\d{2}T\d{2}\d{2}\d{2}'}
    )


class DateType(CustomSchemaType):
    # we do not respect the official swagger datetime format :(
    _schema = TypeSchema(type=str, metadata={'format': 'navitia-date', 'pattern': '\d{4}\d{2}\d{2}'})


class TimeType(CustomSchemaType):
    _schema = TypeSchema(type=str, metadata={'format': 'navitia-time', 'pattern': '\d{2}\d{2}\d{2}'})


class TimeOrDateTimeType(CustomSchemaType):
    # either a time or a datetime
    _schema = TypeSchema(
        type=str,
        metadata={'format': 'navitia-time-or-date-time', 'pattern': '(\d{4}\d{2}\d{2}T)?\d{2}\d{2}\d{2}'},
    )


class JsonStrField(Field):
    """
    A :class:`JsonStrField` a json parser
    """

    def __init__(self, schema_type=None, schema_metadata=None, **kwargs):
        super(JsonStrField, self).__init__(schema_type=schema_type, schema_metadata=schema_metadata, **kwargs)

    def to_value(self, value):
        import ujson

        return ujson.loads(value)
