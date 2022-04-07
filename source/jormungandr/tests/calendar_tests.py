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

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


def check_valid_calendar(cal):
    get_not_null(cal, "id")
    get_not_null(cal, "name")
    pattern = get_not_null(cal, "week_pattern")
    is_valid_bool(get_not_null(pattern, "monday"))  # check one field in pattern

    validity_pattern = get_not_null(cal, "validity_pattern")
    is_valid_validity_pattern(validity_pattern)

    exceptions = get_not_null(cal, "exceptions")
    assert len(exceptions) > 0

    active_periods = get_not_null(cal, "active_periods")
    assert len(active_periods) > 0

    beg = get_not_null(active_periods[0], "begin")
    assert is_valid_date(beg)

    end = get_not_null(active_periods[0], "end")
    assert is_valid_date(end)

    # check links


@dataset({"main_ptref_test": {}})
class TestCalendar(AbstractTestFixture):
    def test_calendars(self):
        json_response = self.query_region("calendars/")

        calendars = get_not_null(json_response, "calendars")

        # we need at least one calendar
        assert calendars

        cal = calendars[0]
        check_valid_calendar(cal)

    def test_calendars_lines(self):
        json_response = self.query_region("calendars/monday/lines")

        assert len(json_response['disruptions']) == 0

        lines = get_not_null(json_response, "lines")

        assert lines

    def test_calendars_lines_with_current_datetime(self):
        json_response = self.query_region("calendars/monday/lines?_current_datetime=20140115T160000")

        lines = get_not_null(json_response, "lines")

        assert lines

        assert len(json_response['disruptions']) == 1

        disruptions = get_not_null(json_response, 'disruptions')

        messages = get_not_null(disruptions[0], 'messages')

        assert (messages[0]['text']) == 'Disruption on Line line:A'

    def test_lines_calendars(self):
        json_response = self.query_region("calendars/monday/lines/line:A/calendars")

        calendars = get_not_null(json_response, "calendars")

        assert calendars
        check_valid_calendar(calendars[0])

    def test_forbidden_uris_on_calendars(self):
        """test forbidden uri for calendars"""
        response, code = self.query_no_assert("v1/coverage/main_ptref_test/calendars/monday")

        calendars = get_not_null(response, 'calendars')
        assert len(calendars) == 1

        assert calendars[0]['id'] == 'monday'

        # there is only one calendar, so when we forbid it's id, we find nothing
        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/calendars/monday" "?forbidden_uris[]=monday"
        )
        assert code == 404

        # for retrocompatibility purpose forbidden_id[] is the same
        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/calendars/monday" "?forbidden_id[]=monday"
        )
        assert code == 404

        # when we forbid another id, we find again our calendar
        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/calendars/monday" "?forbidden_uris[]=tuesday"
        )
        assert code == 200
