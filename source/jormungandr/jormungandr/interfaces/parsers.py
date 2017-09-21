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
from __future__ import absolute_import
from dateutil import parser
from jormungandr.interfaces.v1.serializer.jsonschema.fields import DateTimeType
from jormungandr import app
from navitiacommon import parser_args_type

# TODO: to be moved completely into navitiacommon
from navitiacommon.parser_args_type import TypeSchema, CustomSchemaType

depth_argument = parser_args_type.DepthArgument()

float_gt_0 = parser_args_type.PositiveFloat()

parser_min_count = app.config['PARSER_MIN_COUNT']
parser_max_count = app.config['PARSER_MAX_COUNT']

default_count_arg_type = parser_args_type.IntervalValue(min_value=parser_min_count, max_value=parser_max_count)


def parse_input_date(date):
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
            d = parse_input_date(value)
            if d.year < 1970:
                raise ValueError('date is too early!')

            return d
        except ValueError as e:
            raise ValueError("Unable to parse datetime, {}".format(e))

    def schema(self):
        return TypeSchema(type=str, metadata={'format': 'date-time'})


class UnsignedInteger(CustomSchemaType):
    def __call__(self, value):
        try:
            d = int(value)
            if d < 0:
                raise ValueError('invalid positive int')

            return d
        except ValueError as e:
            raise ValueError("Unable to evaluate, {}".format(e))

    def schema(self):
        return TypeSchema(type=int, metadata={'minimum': 0})
