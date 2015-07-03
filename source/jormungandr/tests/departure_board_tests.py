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
    is_valid_route(route, depth_check=2)

    datetimes = get_not_null(schedule, "date_times")

    assert len(datetimes) != 0, "we have to have date_times"
    for dt_wrapper in datetimes:
        dt = dt_wrapper["date_time"]
        if only_time:
            get_valid_time(dt)
        else:
            get_valid_datetime(dt)
        #TODO remove href_mandatory=False after link refactor, they should always be there :)
        check_links(dt_wrapper, tester, href_mandatory=False)

    #TODO remove href_mandatory=False after link refactor, they should always be there :)
    check_links(schedule, tester, href_mandatory=False)


def is_valid_route_schedule(schedules):
    assert len(schedules) == 1, "there should be only one elt"
    schedule = schedules[0]
    d = get_not_null(schedule, 'display_informations')

    get_not_null(d, 'direction')
    get_not_null(d, 'label')
    get_not_null(d, 'network')

    shape(get_not_null(schedule, 'geojson'))

    table = get_not_null(schedule, 'table')

    headers = get_not_null(table, 'headers')
    #TODO check headers

    rows = get_not_null(table, 'rows')
    for row in rows:
        for dt in get_not_null(row, 'date_times'):
            assert "additional_informations" in dt
            assert "links" in dt
            get_valid_datetime(get_not_null(dt, "date_time"))

        is_valid_stop_point(get_not_null(row, 'stop_point'), depth_check=1)


def is_valid_departure(departure):
    d = get_not_null(departure, 'display_informations')

    get_not_null(d, 'direction')
    get_not_null(d, 'label')
    get_not_null(d, 'network')
    get_not_null(d, 'physical_mode')
    get_not_null(d, 'headsign')


    route = get_not_null(departure, 'route')
    is_valid_route(route)

    stop_point = get_not_null(departure, 'stop_point')
    is_valid_stop_point(stop_point)

    stop_date_time = get_not_null(departure, 'stop_date_time')
    is_valid_stop_date_time(stop_date_time)


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

    def test_partial_terminus(self):
        """
        Partial Terminus
        """
        response = self.query_region("stop_areas/Tstop1/stop_schedules?"
                                     "from_datetime=20120615T080000")

        assert "stop_schedules" in response
        check_departure_board(response["stop_schedules"], self.tester, only_time=False)

        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["stop_point"]["id"] == "Tstop1"
        assert response["stop_schedules"][0]["route"]["id"] == "A:1"
        assert len(response["stop_schedules"][0]["date_times"]) == 2
        assert response["stop_schedules"][0]["date_times"][0]["links"][0]["type"] == "notes"
        assert response["stop_schedules"][0]["date_times"][0]["links"][0]["id"] == "destination:16710925402715739122"
        assert len(response["notes"]) == 1
        assert response["notes"][0]["type"] == "notes"
        assert response["notes"][0]["value"] == "Tstop2"
        assert response["stop_schedules"][0]["date_times"][0]["links"][1]["type"] == "vehicle_journey"
        assert response["stop_schedules"][0]["date_times"][0]["links"][1]["value"] == "vj1"
        assert response["stop_schedules"][0]["date_times"][1]["links"][0]["type"] == "vehicle_journey"
        assert response["stop_schedules"][0]["date_times"][1]["links"][0]["value"] == "vj2"

    def test_real_terminus(self):
        """
        Real Terminus
        """
        response = self.query_region("stop_areas/Tstop3/stop_schedules?"
                                     "from_datetime=20120615T080000")

        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["additional_informations"] == "terminus"

    def test_no_departure_this_day(self):
        """
        no departure for this day : 20120620T080000
        """
        response = self.query_region("stop_areas/Tstop1/stop_schedules?"
                                     "from_datetime=20120620T080000")

        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["additional_informations"] == "no_departure_this_day"

    def test_routes_schedule(self):
        """
        departure board for a given date
        """
        response = self.query_region("routes/line:A:0/route_schedules?from_datetime=20120615T080000")

        assert "route_schedules" in response
        is_valid_route_schedule(response["route_schedules"])

    def test_with_wrong_type(self):
        """
        we search for stop_areasS, this type doesn't exist
        """
        response, code = self.query_region("stop_areass/stop1/stop_schedules?"
                                     "from_datetime=20120615T080000", check=False)
        eq_(code, 400)
        eq_(response['message'], 'unknown type: stop_areass')

    def test_departures(self):
        """
        departure board for a given date
        """
        response = self.query_region("stop_points/ODTstop1/departures?from_datetime=20120615T080000")

        assert "departures" in response
        assert len(response["departures"]) == 2

        for departure in response["departures"]:
            is_valid_departure(departure)

        assert len(response["departures"][0]["stop_date_time"]["additional_informations"]) == 1
        assert response["departures"][0]["stop_date_time"]["additional_informations"][0] == "date_time_estimated"

        assert len(response["departures"][1]["stop_date_time"]["additional_informations"]) == 1
        assert response["departures"][1]["stop_date_time"]["additional_informations"][0] == "on_demand_transport"
