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

import calendar
from datetime import datetime
from jormungandr import i_manager
import pytz


def str_to_time_stamp(str):
    """
    convert a string to a posix timestamp
    the string must be in the YYYYMMDDTHHMMSS format
    like 20170534T124500
    """
    date = datetime.strptime(str, "%Y%m%dT%H%M%S")

    return date_to_timestamp(date)


def date_to_timestamp(date):
    """
    convert a datatime objet to a posix timestamp (number of seconds from 1070/1/1)
    """
    return int(calendar.timegm(date.timetuple()))

class ResourceUtc:
    def __init__(self):
        self._tz = None

    def tz(self):
        if not self._tz:
            instance = i_manager.instances[self.region]

            tz_name = instance.timezone  # TODO store directly the tz?

            if not tz_name:
                logging.Logger(__name__).warn("unkown timezone for region {}"
                                              .format(self.region))
                return original_datetime
            self._tz = (pytz.timezone(tz_name),)
        return self._tz[0]

    def convert_to_utc(self, original_datetime):
        """
        convert the original_datetime in the args to UTC

        for that we need to 'guess' the timezone wanted by the user

        For the moment We only use the default instance timezone.

        It won't obviously work for multi timezone instances, we'll have to do
        something smarter.

        We'll have to consider either the departure or the arrival of the journey
        (depending on the `clockwise` param)
        and fetch the tz of this point.
        we'll have to store the tz for stop area and the coord for admin, poi, ...
        """

        if self.tz() is None:
            return original_datetime

        utctime = self.tz().normalize(self.tz().localize(original_datetime)).astimezone(pytz.utc)

        return utctime

