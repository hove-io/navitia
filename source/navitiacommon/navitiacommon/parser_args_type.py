# encoding: utf-8

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
from collections import namedtuple

import ujson
import geojson
import flask
from dateutil import parser
from flask_restful.inputs import boolean
import six
import sys


class TypeSchema(object):
    def __init__(self, type=None, metadata=None):
        self.type = type
        self.metadata = metadata


class CustomSchemaType(object):
    def schema(self):
        # by default we look for a _schema variable, but it can be overriden
        return self._schema


class DepthArgument(CustomSchemaType):
    def __call__(self, value, name):
        conv_value = int(value)
        if conv_value > 3:
            raise ValueError("The {} argument has to be <= 3, you gave : {}".format(name, value))
        return conv_value

    def schema(self):
        return TypeSchema(type=int, metadata={'minimum': 0, 'maximum': 3})


class PositiveFloat(CustomSchemaType):
    def __call__(self, value, name):
        conv_value = float(value)
        if conv_value <= 0:
            raise ValueError("The {} argument has to be > 0, you gave : {}".format(name, value))
        return conv_value

    def schema(self):
        return TypeSchema(type=float, metadata={'minimum': 0})


class IntRange(CustomSchemaType):
    def __init__(self, min, max):
        self.min = min
        self.max = max

    def __call__(self, value, name):
        conv_value = int(value)
        if not self.min <= conv_value <= self.max:
            raise ValueError(
                "The {} argument has to be in range [{}, {}], you gave : {}".format(
                    name, self.min, self.max, value
                )
            )
        return conv_value

    def schema(self):
        return TypeSchema(type=float, metadata={'minimum': 0})


class FloatRange(CustomSchemaType):
    def __init__(self, min, max):
        self.min = min
        self.max = max

    def __call__(self, value, name):
        conv_value = float(value)
        if not self.min <= conv_value <= self.max:
            raise ValueError(
                "The {} argument has to be in range [{}, {}], you gave : {}".format(
                    name, self.min, self.max, value
                )
            )
        return conv_value

    def schema(self):
        return TypeSchema(type=float, metadata={'minimum': 0})


class SpeedRange(CustomSchemaType):
    map_range = {
        'bike_speed': (0.01, 15),
        'bss_speed': (0.01, 15),
        'walking_speed': (0.01, 4),
        'car_speed': (0.01, 50),
        'taxi_speed': (0.01, 50),
        'car_no_park_speed': (0.01, 50),
        'ridesharing_speed': (0.01, 50),
        'default': (sys.float_info.min, sys.float_info.max),
    }

    def __call__(self, value, name):
        conv_value = float(value)
        (range_min, range_max) = (
            SpeedRange.map_range[name] if name in SpeedRange.map_range else SpeedRange.map_range['default']
        )
        if not range_min <= conv_value <= range_max:
            raise ValueError(
                "The {} argument has to be in range [{}, {}], you gave : {}".format(
                    name, range_min, range_max, value
                )
            )
        return conv_value

    def schema(self):
        return TypeSchema(type=float, metadata={'minimum': 0})


class BooleanType(CustomSchemaType):
    def __call__(self, value):
        if isinstance(value, bool):
            return value
        return boolean(value)

    def schema(self):
        return TypeSchema(type=bool)


class OptionValue(CustomSchemaType):
    def __init__(self, optional_values):

        self.optional_values = optional_values

    def __call__(self, value, name):
        # if input value is iterable
        if hasattr(value, '__iter__') and not isinstance(value, six.text_type):
            if not all((v in self.optional_values for v in value)):
                error = "The {} argument must be in list {}, you gave {}".format(
                    name, str(self.optional_values), value
                )
                raise ValueError(error)
        elif not (value in self.optional_values):
            error = "The {} argument must be in list {}, you gave {}".format(
                name, str(self.optional_values), value
            )
            raise ValueError(error)
        return value

    def schema(self):
        return TypeSchema(type=str, metadata={'enum': self.optional_values})


class DescribedOptionValue(OptionValue):
    def __init__(self, optional_values):
        self.description = "Possible values:\n"
        self.description += '\n'.join([" * '{}' - {}".format(k, v) for k, v in optional_values.items()])
        super(DescribedOptionValue, self).__init__(optional_values.keys())

    def schema(self):
        ts = super(DescribedOptionValue, self).schema()
        ts.metadata['description'] = self.description
        return ts


class IntervalValue(CustomSchemaType):
    def __init__(self, type=int, min_value=None, max_value=None):
        self.type = type
        self.min_value = min_value
        self.max_value = max_value

    def __call__(self, value, name):
        v = self.type(value)
        if self.min_value:
            v = max(v, self.min_value)
        if self.max_value:
            v = min(v, self.max_value)
        return v

    def schema(self):
        metadata = {}
        if self.min_value:
            metadata['minimum'] = self.min_value
        if self.max_value:
            metadata['maximum'] = self.max_value
        return TypeSchema(type=self.type, metadata=metadata)


def geojson_argument(value):
    def is_geometry_valid(geometry):
        geometry_str = ujson.dumps(geometry)
        geometry = geojson.loads(geometry_str)
        return geometry.is_valid

    if value:
        if not isinstance(value, dict):
            raise ValueError('invalid json')

        if not is_geometry_valid(value):
            raise ValueError('invalid geojson')

        geometry = value.get('geometry', {}).get('type')
        if not geometry or geometry.lower() != 'polygon':
            raise ValueError('invalid geometry type')

    return value


class CoordFormat(CustomSchemaType):
    def __init__(self, nullable=False):
        super(CoordFormat, self).__init__()
        self.nullable = nullable

    def __call__(self, coord):
        """
        Validate coordinates format (lon;lat)
        """
        if coord == '' and self.nullable:
            return coord
        lon_lat_splitted = coord.split(";")
        if len(lon_lat_splitted) != 2:
            raise ValueError('Invalid coordinate parameter. It must be lon;lat where lon and lat are floats.')

        lon, lat = lon_lat_splitted
        lat = float(lat)
        if not (-90.0 <= lat <= 90.0):
            raise ValueError("lat should be between -90 and 90")
        lon = float(lon)
        if not (180.0 >= lon >= -180.0):
            raise ValueError("lon should be between -180 and 180")

        return coord

    def schema(self):
        return TypeSchema(type=str, metadata={'pattern': '.*;.*'})


class UnsignedInteger(CustomSchemaType):
    def __call__(self, value):
        try:
            d = int(value)
            if d < 0:
                raise ValueError('invalid unsigned int')

            return d
        except ValueError as e:
            raise ValueError("Unable to evaluate, {}".format(e))

    def schema(self):
        return TypeSchema(type=int, metadata={'minimum': 0})


class PositiveInteger(CustomSchemaType):
    def __call__(self, value):
        try:
            d = int(value)
            if d <= 0:
                raise ValueError('invalid positive int')

            return d
        except ValueError as e:
            raise ValueError("Unable to evaluate, {}".format(e))

    def schema(self):
        return TypeSchema(type=int, metadata={'minimum': 1})


def _parse_input_date(date):
    """
    datetime parse date seems broken, '155' with format '%H%M%S' is not
    rejected but parsed as 1h, 5mn, 5s...
    so use use for the input date parse dateutil even if the 'guess'
    mechanism seems a bit dangerous
    """
    return parser.parse(date, dayfirst=False, yearfirst=True)


class DateTimeFormat(CustomSchemaType):
    def __call__(self, value):
        """
        we want to valid the date format
        """
        try:
            d = _parse_input_date(value)
            if d.year <= 1970:
                raise ValueError('date is too early!')

            return d
        except ValueError as e:
            raise ValueError("Unable to parse datetime, {}".format(e))

    def schema(self):
        return TypeSchema(type=str, metadata={'format': 'date-time'})


class KeyValueType(CustomSchemaType):
    """
    >>> k = KeyValueType()
    >>> k("test,0")
    ('test', 0)
    >>> k("test,-1")
    Traceback (most recent call last):
      File "<input>", line 1, in <module>
      File "<input>", line 21, in __call__
    ValueError: Unable to evaluate invalid value, out of range (0, inf),value must be an integer
    >>> k = KeyValueType(1, 10)
    >>> k("test,5")
    ('test', 5)
    >>> k("test,0")
    Traceback (most recent call last):
      File "<input>", line 1, in <module>
      File "<input>", line 21, in __call__
    ValueError: Unable to evaluate invalid value, out of range (1, 10),value must be an integer

    """

    def __init__(self, min_value=None, max_value=None):
        super(KeyValueType, self).__init__()
        self.min_value = min_value or 0
        self.max_value = max_value or float("inf")

    def __call__(self, value):
        try:
            key, value = value.split(',')
            value = int(value)
            if not (self.min_value <= value <= self.max_value):
                raise ValueError('invalid value, out of range {}'.format((self.min_value, self.max_value)))

            return key, value
        except ValueError as e:
            raise ValueError("Unable to evaluate {},value must be an integer".format(e))

    def schema(self):
        return TypeSchema(type=str, metadata={'minimum': self.min_value, "maximum": self.max_value})
