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
from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *
import datetime


def check_departure_board(schedules, tester, only_time=False):
    """
    check the structure of a departure board response
    """
    assert len(schedules) == 1, "there should be only one elt"

    schedule = schedules[0]

    is_valid_stop_point(get_not_null(schedule, "stop_point"), depth_check=2)

    route = get_not_null(schedule, "route")
    is_valid_route(route)

    datetimes = get_not_null(schedule, "date_times")

    assert len(datetimes) != 0, "we have to have date_times"
    #TODO, make that works before merging the time zone
    for dt_wrapper in datetimes:
        dt = dt_wrapper["date_time"]
        if only_time:
            get_valid_time(dt)
        else:
            get_valid_datetime(dt)

    #TODO uncomment after link refactor
    #check_links(schedule, tester)


@dataset(["departure_board_test"])
class TestDepartureBoard(AbstractTestFixture):
    """
    Test the structure of the departure board api
    """

    def test_calendar_association(self):
        """
        Same test as departure_board_test.cpp/test_calendar_week
        but after jormungandr
        """
        response = self.query_region("stop_points/stop1/stop_schedules?"
                                     "calendar=week_cal&from_datetime=20120615T080000")

        assert "stop_schedules" in response
        #all datetime in the response should be only time (no dates since we query if on a period (a calendar))
        check_departure_board(response["stop_schedules"], self.tester, only_time=True)

        #after the structure check, we check some test specific stuff
        assert response["stop_schedules"][0]["stop_point"]["id"] == "stop1"
        assert response["stop_schedules"][0]["route"]["line"]["id"] == "line:A"
        dts = response["stop_schedules"][0]["date_times"]

        assert len(dts) == 3
        assert get_valid_time(dts[0]["date_time"]) == datetime.datetime(1900, 1, 1, 10, 10, 00)
        assert get_valid_time(dts[1]["date_time"]) == datetime.datetime(1900, 1, 1, 11, 10, 00)
        assert get_valid_time(dts[2]["date_time"]) == datetime.datetime(1900, 1, 1, 15, 10, 00)

    def test_no_calendar_error(self):
        """
        bob_the_calendar does not exists, we got an error
        """
        response, error_code = self.query_region("stop_areas/stop1/stop_schedules?"
                                                 "calendar=bob_the_calendar", check=False)

        assert error_code == 404

        error = get_not_null(response, "error")

        assert error["message"] == "stop_schedules : calendar does not exist"
        assert error["id"] == "unknown_object"

    def test_on_datetime(self):
        """
        departure board for a given date
        """
        response = self.query_region("stop_areas/stop1/stop_schedules?"
                                     "from_datetime=20120615T080000")

        assert "stop_schedules" in response
        #all datetime in the response should be only time (no dates since we query if on a period (a calendar))
        check_departure_board(response["stop_schedules"], self.tester, only_time=False)

        #after the structure check, we check some test specific stuff
        assert response["stop_schedules"][0]["stop_point"]["id"] == "stop1"
        assert response["stop_schedules"][0]["route"]["line"]["id"] == "line:A"