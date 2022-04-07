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
from datetime import datetime
import time
from jormungandr.interfaces.v1.serializer.base import PbField, PbNestedSerializer, NestedPbField
from jormungandr.interfaces.v1.serializer.jsonschema.fields import Field, TimeType, DateTimeType, DateType
from jormungandr.utils import timestamp_to_str, dt_to_str, DATETIME_FORMAT


# a time null value is represented by the max value (since 0 is a perfectly valid value)
# WARNING! be careful to change that value if the time type change (from uint64 to uint32 for example)
__date_time_null_value__ = 2 ** 64 - 1


class TimeField(NestedPbField, TimeType):
    """
    This field convert a number of second from midnight to a string with the format: HH:MM:SS
    No conversion from UTC will be done, we expect the time to already be in desired timezone
    """

    def to_value(self, value):
        if value == __date_time_null_value__:
            return ""
        return datetime.utcfromtimestamp(value).strftime('%H%M%S')


class DateTimeField(PbField, DateTimeType):
    """
    custom date format from timestamp
    """

    def to_value(self, value):
        return timestamp_to_str(value)


class DateTimeDbField(Field, DateType):
    """
    custom date format from datetime db
    """

    def to_value(self, value):
        return dt_to_str(value, _format=DATETIME_FORMAT)


class DateField(PbField, DateType):
    """
    custom date format from timestamp
    """

    def to_value(self, value):
        date = datetime.fromtimestamp(value).date()
        return dt_to_str(date, _format="%Y%m%d")


class DateTimeDictField(Field, DateTimeType):
    """
    custom date format from timestamp
    """

    def to_value(self, value):
        return timestamp_to_str(value) if value else None


class PeriodSerializer(PbNestedSerializer):
    begin = DateTimeField()
    end = DateTimeField()


class PeriodDateSerializer(PbNestedSerializer):
    begin = DateField()
    end = DateField()


class LocalTimeField(NestedPbField, TimeType):
    def to_value(self, value):
        if value == __date_time_null_value__:
            return ""
        return time.strftime("%H%M%S", time.gmtime(value))


class PeriodTimeSerializer(PbNestedSerializer):
    begin = LocalTimeField()
    end = LocalTimeField()
