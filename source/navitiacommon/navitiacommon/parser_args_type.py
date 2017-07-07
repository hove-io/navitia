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
from collections import namedtuple

import ujson
import geojson

def depth_argument(value, name):
    conv_value = int(value)
    if conv_value > 3:
        raise ValueError("The {} argument has to be <= 3, you gave : {}"
                         .format(name, value))
    return conv_value


def float_gt_0(value, name):
    conv_value = float(value)
    if conv_value <= 0:
        raise ValueError("The {} argument has to be > 0, you gave : {}"
                         .format(name, value))
    return conv_value


def true_false(value, name):
    if isinstance(value, bool):
        return value
    if value.lower() == "true":
        return True
    elif value.lower() == "false":
        return False
    else:
        raise ValueError("The {} argument must be true or false, you gave : {}"
                         .format(name, value))


class ParameterDescription(object):
    def __init__(self, type=None, metadata=None):
        self.type = type
        self.metadata = metadata


class option_value(object):

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

    def description(self):
        return ParameterDescription(type=str, metadata={'enum': self.optional_values})


def _make_interval_argument(max_value, min_value):
    def to_return(value):
        v = int(value)
        return min(max(v, min_value), max_value)
    return to_return


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


class coord_format(object):
    def __call__(self, coord):
        """
        Validate coordinates format (lon;lat)
        """
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

    def description(self):
        return ParameterDescription(type=str, metadata={'pattern': '.*;.*'})


default_count_arg_type = _make_interval_argument(max_value=1000, min_value=0)
