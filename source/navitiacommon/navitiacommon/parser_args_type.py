# encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
from collections import namedtuple

import ujson
import geojson
import flask

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
            raise ValueError("The {} argument has to be <= 3, you gave : {}"
                             .format(name, value))
        return conv_value

    def schema(self):
        return TypeSchema(type=int, metadata={'minimum': 0, 'maximum': 3})

class PositiveFloat(CustomSchemaType):
    def __call__(self, value, name):
        conv_value = float(value)
        if conv_value <= 0:
            raise ValueError("The {} argument has to be > 0, you gave : {}"
                             .format(name, value))
        return conv_value

    def schema(self):
        return TypeSchema(type=float, metadata={'minimum': 0})


class BooleanType(CustomSchemaType):
    def __call__(self, value):
        if isinstance(value, bool):
             return value
        return flask.ext.restful.inputs.boolean(value)

    def schema(self):
        return TypeSchema(type=bool)


class OptionValue(CustomSchemaType):

    def __init__(self, optional_values):
        self.optional_values = optional_values

    def __call__(self, value, name):
        # if input value is iterable
        if hasattr(value, '__iter__'):
            if not all((v in self.optional_values for v in value)):
                error = "The {} argument must be in list {}, you gave {}".\
                    format(name, str(self.optional_values), value)
                raise ValueError(error)
        elif not (value in self.optional_values):
            error = "The {} argument must be in list {}, you gave {}".\
                format(name, str(self.optional_values), value)
            raise ValueError(error)
        return value

    def schema(self):
        return TypeSchema(type=str, metadata={'enum': self.optional_values})


class DescribedOptionValue(OptionValue):
    def __init__(self, optional_values):
        self.description = "Possible values:\n"
        self.description += '\n'.join([" * '{}' - {}".format(k, v) for k,v in optional_values.iteritems()])
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
        valid = geojson.is_valid(geojson.loads(geometry_str))
        return 'valid' in valid and (valid['valid'] == 'yes' or valid['valid'] == '')

    if value:
        if not isinstance(value, dict):
            raise ValueError('invalid json')

        if not is_geometry_valid(value) :
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

