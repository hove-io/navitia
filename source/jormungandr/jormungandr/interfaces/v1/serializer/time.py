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
import serpy
from datetime import datetime
from jormungandr.interfaces.v1.serializer.base import PbField, PbNestedSerializer
from jormungandr.utils import timestamp_to_str


# a time null value is represented by the max value (since 0 is a perfectly valid value)
# WARNING! be careful to change that value if the time type change (from uint64 to uint32 for example)
__date_time_null_value__ = 2**64 - 1


class LocalTimeField(serpy.Field):
    """
    This field convert a number of second from midnight to a string with the format: HH:MM:SS
    No conversion from UTC will be done, we expect the time to already be in desired timezone
    """
    def to_value(self, value):
        if value == __date_time_null_value__:
            return ""
        return datetime.utcfromtimestamp(value).strftime('%H%M%S')


class DateTimeField(PbField):
    """
    custom date format from timestamp
    """
    def to_value(self, value):
        return timestamp_to_str(value)


class PeriodSerializer(PbNestedSerializer):
    begin = DateTimeField()
    end = DateTimeField()

