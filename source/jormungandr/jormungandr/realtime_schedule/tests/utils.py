# coding=utf-8

# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from dateutil.parser import parse
import pytz
from jormungandr.utils import date_to_timestamp


class MockRoutePoint(object):
    def __init__(self, *args, **kwars):
        self._hardcoded_line_id = kwars['line_id']
        self._hardcoded_stop_id = kwars['stop_id']
        self._hardcoded_route_id = kwars['route_id']

    def fetch_stop_id(self, rt_proxy_id):
        return self._hardcoded_stop_id

    def fetch_line_id(self, rt_proxy_id):
        return self._hardcoded_line_id

    def fetch_route_id(self, rt_proxy_id):
        return self._hardcoded_route_id

    def fetch_line_uri(self):
        return self._hardcoded_line_id

    def fetch_all_route_id(self, rt_proxy_id):
        return [self._hardcoded_route_id]


def _dt(dt_to_parse="00:00", year=2016, month=2, day=7):
    """
    small helper to ease the reading of the tests
    >>> _dt("8:15")
    datetime.datetime(2016, 2, 7, 8, 15, tzinfo=<UTC>)
    >>> _dt("9:15", day=2)
    datetime.datetime(2016, 2, 2, 9, 15, tzinfo=<UTC>)
    """
    d = parse(dt_to_parse)
    pytz.UTC.localize(d)

    return d.replace(year=year, month=month, day=day, tzinfo=pytz.UTC)


def _timestamp(str, **kwargs):
    return date_to_timestamp(_dt(str, **kwargs))
