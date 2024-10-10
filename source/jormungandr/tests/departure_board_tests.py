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

from six.moves.urllib.parse import quote
from six.moves import zip_longest
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *
import datetime
from jormungandr import app


def is_valid_stop_schedule_datetime(dt_wrapper, tester, only_time):
    dt = dt_wrapper["date_time"]
    if only_time:
        get_valid_time(dt)
    else:
        get_valid_datetime(dt)
    # TODO remove href_mandatory=False after link refactor, they should always be there :)
    check_links(dt_wrapper, tester, href_mandatory=False)
    is_valid_rt_level(get_not_null(dt_wrapper, 'data_freshness'))


def is_valid_stop_schedule(schedules, tester, only_time=False):
    """
    check the structure of a departure board response
    """
    for schedule in schedules:
        is_valid_stop_point(get_not_null(schedule, "stop_point"), depth_check=2)

        route = get_not_null(schedule, "route")
        is_valid_route(route, depth_check=2)

        datetimes = get_not_null(schedule, "date_times")

        assert len(datetimes) != 0, "we have to have date_times"
        for dt_wrapper in datetimes:
            is_valid_stop_schedule_datetime(dt_wrapper, tester, only_time)

        # TODO remove href_mandatory=False after link refactor, they should always be there :)
        check_links(schedule, tester, href_mandatory=False)


def is_valid_route_schedule_header(header):
    get_not_null(header, 'display_informations')

    links = get_not_null(header, 'links')

    # we should have at least:
    # physical mode and vj link
    # and some optional notes
    links_by_type = {l['type'] for l in links}

    mandatory_fields = {'vehicle_journey', 'physical_mode'}
    optional_fields = {'notes'}

    assert mandatory_fields.issubset(links_by_type)
    assert mandatory_fields.union(optional_fields).issuperset(links_by_type)


def is_valid_route_schedule(schedule, only_time=False):
    d = get_not_null(schedule, 'display_informations')

    get_not_null(d, 'direction')
    get_not_null(d, 'label')
    get_not_null(d, 'network')
    get_not_null(d, 'name')

    shape(get_not_null(schedule, 'geojson'))

    table = get_not_null(schedule, 'table')

    headers = get_not_null(table, 'headers')
    for h in headers:
        is_valid_route_schedule_header(h)

    rows = get_not_null(table, 'rows')
    for row in rows:
        for dt_obj in get_not_null(row, 'date_times'):
            assert "additional_informations" in dt_obj
            assert "links" in dt_obj
            assert isinstance(dt_obj["links"], list)
            dt = dt_obj["date_time"]
            if dt == "":
                pass  # hole in the schedule
            elif only_time:
                get_valid_time(dt)
            else:
                get_valid_datetime(dt)

        is_valid_stop_point(get_not_null(row, 'stop_point'), depth_check=1)
    assert not d["links"]


def is_valid_departures(departures):
    for departure in departures:
        is_valid_departure(departure)


def is_valid_departure(departure):
    d = get_not_null(departure, 'display_informations')

    get_not_null(d, 'direction')
    get_not_null(d, 'label')
    get_not_null(d, 'network')
    get_not_null(d, 'physical_mode')
    get_not_null(d, 'headsign')
    get_not_null(d, 'name')

    route = get_not_null(departure, 'route')
    is_valid_route(route)

    stop_point = get_not_null(departure, 'stop_point')
    is_valid_stop_point(stop_point)

    stop_date_time = get_not_null(departure, 'stop_date_time')
    is_valid_stop_date_time(stop_date_time)


def get_real_notes(obj, full_response):
    real_notes = {n['id']: n for n in get_not_null(full_response, 'notes')}
    return [real_notes[n['id']] for n in get_not_null(obj, 'links') if n['type'] == 'notes']


def is_valid_terminus_schedule(terminus_schedule, tester, only_time=False):
    is_valid_stop_point(get_not_null(terminus_schedule, "stop_point"), depth_check=2)
    is_valid_route(get_not_null(terminus_schedule, "route"), depth_check=2)
    if len(terminus_schedule.get('date_times', [])) == 0:
        return
    datetimes = get_not_null(terminus_schedule, "date_times")
    assert len(datetimes) != 0, "we have to have date_times"
    for dt_wrapper in datetimes:
        is_valid_stop_schedule_datetime(dt_wrapper, tester, only_time)

    d = get_not_null(terminus_schedule, 'display_informations')
    get_not_null(d, 'direction')
    get_not_null(d, 'label')
    get_not_null(d, 'network')
    get_not_null(d, 'commercial_mode')
    get_not_null(d, 'name')


def is_valid_terminus_schedules(terminus_schedules, tester, only_time=False):
    for terminus_schedule in terminus_schedules:
        is_valid_terminus_schedule(terminus_schedule, tester, only_time)


@dataset({"departure_board_test": {}})
class TestDepartureBoard(AbstractTestFixture):
    """
    Test the structure of the departure board api
    """

    def test_calendar_association(self):
        """
        Same test as departure_board_test.cpp/test_calendar_week
        but after jormungandr
        """
        response = self.query_region(
            "stop_points/stop1/stop_schedules?" "calendar=week_cal&from_datetime=20120615T080000"
        )

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        # all datetime in the response should be only time (no dates since we query if on a period (a calendar))
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=True)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        # after the structure check, we check some test specific stuff
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
        response, error_code = self.query_region(
            "stop_areas/stop1/stop_schedules?" "calendar=bob_the_calendar", check=False
        )

        assert error_code == 404

        error = get_not_null(response, "error")

        assert error["message"] == "stop_schedules : calendar does not exist"
        assert error["id"] == "unknown_object"

    def test_datetime_error(self):
        """
        datetime invalid, we got an error
        """

        def sched(dt):
            return self.query_region(
                "stop_points/stop1/stop_schedules?from_datetime={dd}".format(dd=dt), check=False
            )

        # invalid month
        resp, code = sched("20121501T100000")
        assert code == 400
        assert 'month must be in 1..12' in resp['message']

        # too much digit
        _, code = sched("201215001T100000")
        assert code == 400

    def test_datetime_withtz(self):
        """
        datetime invalid, we got an error
        """

        def sched(dt):
            return self.query_region("stop_points/stop1/stop_schedules?from_datetime={dt}".format(dt=dt))

        # those should not raise an error
        sched("20120615T080000Z")
        sched("2012-06-15T08:00:00.222Z")

        # it should work also with another timezone (and for fun another format)
        sched(quote("2012-06-15 08-00-00+02"))

    def test_on_datetime(self):
        """
        departure board for a given date
        """
        response = self.query_region("stop_areas/stop1/stop_schedules?" "from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        # all datetime in the response should be datetime
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=False)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        # after the structure check, we check some test specific stuff
        assert response["stop_schedules"][0]["stop_point"]["id"] == "stop1"
        assert response["stop_schedules"][0]["route"]["line"]["id"] == "line:A"

        shape(get_not_null(response["stop_schedules"][0]["route"]["line"], 'geojson'))
        shape(get_not_null(response["stop_schedules"][0]["route"], 'geojson'))

    def test_no_geojson(self):
        """
        stop_schedule without geojson
        """
        response = self.query_region(
            "stop_areas/stop1/stop_schedules?" "from_datetime=20120615T080000&disable_geojson=true"
        )

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        # all datetime in the response should be datetime
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=False)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        # after the structure check, we check some test specific stuff
        assert response["stop_schedules"][0]["stop_point"]["id"] == "stop1"
        assert response["stop_schedules"][0]["route"]["line"]["id"] == "line:A"

        assert 'geojson' not in response["stop_schedules"][0]["route"]["line"]
        assert 'geojson' not in response["stop_schedules"][0]["route"]

    def test_partial_terminus(self):
        """
        Partial Terminus
        """
        # Stop_schedules in TStop1: Destination in TStop1 for VJ1
        response = self.query_region("stop_areas/Tstop1/stop_schedules?" "from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=False)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["stop_point"]["id"] == "Tstop1"
        assert response["stop_schedules"][0]["route"]["id"] == "A:1"
        assert len(response["stop_schedules"][0]["date_times"]) == 2
        assert response["stop_schedules"][0]["date_times"][0]["links"][0]["type"] == "notes"
        assert (
            response["stop_schedules"][0]["date_times"][0]["links"][0]["id"]
            == "destination:16710925402715739122"
        )
        assert len(response["notes"]) == 1
        assert response["notes"][0]["type"] == "notes"
        assert response["notes"][0]["value"] == "Tstop2"
        assert response["notes"][0]["category"] == "terminus"
        assert response["stop_schedules"][0]["date_times"][0]["links"][1]["type"] == "vehicle_journey"
        assert response["stop_schedules"][0]["date_times"][0]["links"][1]["value"] == "vehicle_journey:vj1"
        assert response["stop_schedules"][0]["date_times"][1]["links"][0]["type"] == "vehicle_journey"
        assert response["stop_schedules"][0]["date_times"][1]["links"][0]["value"] == "vehicle_journey:vj2"

        # Stop_schedules in TStop2: Only VJ2 in response
        response = self.query_region("stop_areas/Tstop2/stop_schedules?" "from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=False)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["stop_point"]["id"] == "Tstop2"
        assert response["stop_schedules"][0]["route"]["id"] == "A:1"
        assert len(response["stop_schedules"][0]["date_times"]) == 1
        assert response["stop_schedules"][0]["date_times"][0]["links"][0]["type"] == "vehicle_journey"
        assert response["stop_schedules"][0]["date_times"][0]["links"][0]["value"] == "vehicle_journey:vj2"

        # vj1 not in response, Tstop2 is partial terminus
        response = self.query_region("stop_areas/Tstop2/stop_schedules?" "from_datetime=20120615T100000")

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=False)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["stop_point"]["id"] == "Tstop2"
        assert response["stop_schedules"][0]["route"]["id"] == "A:1"
        assert len(response["stop_schedules"][0]["date_times"]) == 1
        assert response["stop_schedules"][0]["date_times"][0]["date_time"] == "20120615T110000"
        assert response["stop_schedules"][0]["date_times"][0]["links"][0]["type"] == "vehicle_journey"
        assert response["stop_schedules"][0]["date_times"][0]["links"][0]["value"] == "vehicle_journey:vj2"

        # Stop_schedules in TStop2: Partial Terminus
        response = self.query_region(
            "stop_areas/Tstop1/stop_schedules?" "from_datetime=20120615T080000&calendar=cal_partial_terminus"
        )

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"
        assert response["stop_schedules"][0]["route"]["id"] == "A:1"
        assert len(response["stop_schedules"][0]["date_times"]) == 1
        assert response["stop_schedules"][0]["date_times"][0]["date_time"] == "100000"
        assert response["stop_schedules"][0]["stop_point"]["id"] == "Tstop1"

        response = self.query_region(
            "stop_areas/Tstop2/stop_schedules?" "from_datetime=20120615T080000&calendar=cal_partial_terminus"
        )

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["additional_informations"] == "partial_terminus"
        assert response["stop_schedules"][0]["route"]["id"] == "A:1"
        assert len(response["stop_schedules"][0]["date_times"]) == 0
        assert response["stop_schedules"][0]["stop_point"]["id"] == "Tstop2"

        response = self.query_region("stop_areas/StopR3/stop_schedules?from_datetime=20120615T080000")
        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"
        assert response["stop_schedules"][0]["route"]["id"] == "R:5"
        assert len(response["stop_schedules"][0]["date_times"]) == 1
        assert response["stop_schedules"][0]["date_times"][0]["date_time"] == "20120615T110000"
        assert response["stop_schedules"][0]["stop_point"]["id"] == "StopR3"
        assert response["notes"][0]["value"] == "StopR4"

        response = self.query_region("stop_areas/StopR2/stop_schedules?from_datetime=20120615T080000")
        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"
        assert response["stop_schedules"][0]["route"]["id"] == "R:5"
        assert len(response["stop_schedules"][0]["date_times"]) == 2
        assert response["stop_schedules"][0]["date_times"][0]["date_time"] == "20120615T103000"
        assert response["stop_schedules"][0]["date_times"][1]["date_time"] == "20120615T103000"
        assert response["stop_schedules"][0]["stop_point"]["id"] == "StopR2"

        # terminus_schedules on partial_terminus with calendar
        # There is neither terminus nor partial_terminus in terminus_schedules
        # Here the disruption could be injected by chaos or kirin

        response = self.query_region(
            "stop_areas/Tstop2/terminus_schedules?"
            "from_datetime=20120615T080000&calendar=cal_partial_terminus&data_freshness=realtime"
        )

        is_valid_notes(response["notes"])
        assert "terminus_schedules" in response
        assert len(response["terminus_schedules"]) == 1
        is_valid_terminus_schedules(response["terminus_schedules"], self.tester, only_time=False)
        assert response["terminus_schedules"][0]["additional_informations"] == "active_disruption"
        assert response["terminus_schedules"][0]["route"]["id"] == "A:1"
        assert len(response["terminus_schedules"][0]["date_times"]) == 0
        assert response["terminus_schedules"][0]["stop_point"]["id"] == "Tstop2"

    def test_real_terminus(self):
        """
        Real Terminus
        """
        response = self.query_region("stop_areas/Tstop3/stop_schedules?" "from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["additional_informations"] == "terminus"

    def test_terminus_schedules_on_terminus(self):
        """
        No terminus_schedules on terminus 'Tstop3' as there is no route/vj of return.
        """
        response = self.query_region("stop_areas/Tstop3/terminus_schedules?" "from_datetime=20120615T080000")
        is_valid_terminus_schedules(response["terminus_schedules"], self.tester, only_time=False)
        is_valid_notes(response["notes"])
        assert "terminus_schedules" in response
        assert len(response["terminus_schedules"]) == 0

    def test_no_departure_this_day(self):
        """
        no departure for this day : 20120620T080000
        """
        response = self.query_region("stop_areas/Tstop1/stop_schedules?" "from_datetime=20120620T080000")

        is_valid_notes(response["notes"])
        assert "stop_schedules" in response
        assert len(response["stop_schedules"]) == 1
        assert response["stop_schedules"][0]["additional_informations"] == "no_departure_this_day"
        assert response["stop_schedules"][0]["display_informations"]["direction"] == "Tstop3 (admin_name)"

    def test_terminus_schedules_on_no_departure_this_day(self):
        """
        no departure for this day : 20120620T080000
        same as stop_schedules
        """
        response = self.query_region("stop_areas/Tstop1/terminus_schedules?" "from_datetime=20120620T080000")
        is_valid_terminus_schedules(response["terminus_schedules"], self.tester, only_time=False)
        is_valid_notes(response["notes"])
        assert "terminus_schedules" in response
        assert len(response["terminus_schedules"]) == 1
        assert response["terminus_schedules"][0]["additional_informations"] == "no_departure_this_day"
        assert response["terminus_schedules"][0]["display_informations"]["direction"] == "Tstop3 (admin_name)"
        assert len(response["terminus_schedules"][0]["date_times"]) == 0

    def test_routes_schedule(self):
        """
        departure board for a given date
        """
        response = self.query_region("routes/line:A:0/route_schedules?from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        schedules = get_not_null(response, 'route_schedules')

        assert len(schedules) == 1, "there should be only one elt"
        schedule = schedules[0]
        is_valid_route_schedule(schedule)

        # check the links
        schedule_links = {(l['type'], l['id']): l for l in get_not_null(schedule, 'links')}
        assert ('line', 'line:A') in schedule_links
        assert ('route', 'line:A:0') in schedule_links
        assert ('network', 'base_network') in schedule_links

        schedule_notes = get_real_notes(schedule, response)
        assert len(schedule_notes) == 1
        assert schedule_notes[0]['value'] == 'walk the line'
        assert schedule_notes[0]['comment_type'] == 'information'

        headers = get_not_null(get_not_null(schedule, 'table'), 'headers')
        # there is 4 vjs
        assert len(headers) == 4

        # we get them by vj to check the vj that have a note
        headers_by_vj = {h['display_informations']['headsign']: h for h in headers}

        assert 'week' in headers_by_vj
        assert 'week_bis' in headers_by_vj
        assert 'all' in headers_by_vj
        assert 'wednesday' in headers_by_vj

        all_vj = headers_by_vj['all']
        all_vj_links = {(l['type'], l['id']): l for l in get_not_null(all_vj, 'links')}
        assert ('vehicle_journey', 'vehicle_journey:all') in all_vj_links
        assert ('physical_mode', 'physical_mode:0') in all_vj_links
        all_vj_notes = get_real_notes(all_vj, response)
        assert len(all_vj_notes) == 1
        assert all_vj_notes[0]['value'] == 'vj comment'
        assert all_vj_notes[0]['comment_type'] == 'on_demand_transport'

    def test_routes_schedule_with_calendar(self):
        """
        departure board for a given calendar
        """
        response = self.query_region("routes/line:A:0/route_schedules?calendar=week_cal")

        is_valid_notes(response["notes"])
        schedules = get_not_null(response, 'route_schedules')

        assert len(schedules) == 1, "there should be only one elt"
        schedule = schedules[0]
        # the route_schedule with calendar should not have with date (only time)
        is_valid_route_schedule(schedule, only_time=True)

    def test_route_schedules_with_holes(self):
        """
        route_schedule on the line R, which is a Y line with holes in the route schedule
        """
        response = self.query_region("routes/R:5/route_schedules?from_datetime=20120615T000000")
        is_valid_notes(response["notes"])
        schedules = get_not_null(response, 'route_schedules')
        assert len(schedules) == 1, "there should be only one elt"
        schedule = schedules[0]
        is_valid_route_schedule(schedule)
        assert schedule["table"]["rows"][2]["date_times"][0]["date_time"] == ""

    def test_route_schedules_two_sa(self):
        """
        route_schedule from 2 stop areas
        """
        response = self.query_region(
            "stop_areas/stop1/stop_areas/stop2/route_schedules?from_datetime=20120615T000000"
        )
        schedules = get_not_null(response, 'route_schedules')
        assert len(schedules) == 1

    def test_with_wrong_type(self):
        """
        we search for stop_areasS, this type doesn't exist
        """
        response, code = self.query_region(
            "stop_areass/stop1/stop_schedules?" "from_datetime=20120615T080000", check=False
        )
        assert code == 400
        assert response['message'] == 'unknown type: stop_areass'

    def test_departures(self):
        """
        departure board for a given date
        """
        response = self.query_region("stop_points/ODTstop1/departures?from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        assert "departures" in response
        assert len(response["departures"]) == 2

        is_valid_departures(response["departures"])

        assert len(response["departures"][0]["stop_date_time"]["additional_informations"]) == 1
        assert response["departures"][0]["stop_date_time"]["additional_informations"][0] == "date_time_estimated"
        assert response["departures"][0]["stop_date_time"]["data_freshness"] == "base_schedule"

        assert len(response["departures"][1]["stop_date_time"]["additional_informations"]) == 1
        assert response["departures"][1]["stop_date_time"]["additional_informations"][0] == "on_demand_transport"
        assert response["departures"][1]["stop_date_time"]["data_freshness"] == "base_schedule"

        assert "terminus" not in response
        assert not response["departures"][0]["display_informations"]["links"]
        assert not response["departures"][1]["display_informations"]["links"]

    def test_departures_arrivals_without_filters(self):
        """
        departure, arrivals api without filters
        """
        apis = ['arrivals', 'departures']
        for api in apis:
            response, code = self.query_region(api, False)
            assert code == 400
            assert 'error' in response
            assert response['message'] == 'Invalid arguments filter'

    def test_vj_comment(self):
        """
        test that a comment on a vehicle journey is linked to each stop_times in stop_schedules
        but only in the header on route_schedules
        """
        # Check if the comment is on the stop_time
        response = self.query_region("stop_points/stop1/stop_schedules?from_datetime=20120615T080000")
        is_valid_notes(response["notes"])
        assert 'error' not in response
        assert 'stop_schedules' in response
        assert len(response["stop_schedules"]) == 1
        assert len(response["notes"]) == 2
        assert len(response["stop_schedules"][0]["date_times"]) == 4

        for date_time in response["stop_schedules"][0]["date_times"]:
            # Extract links of vehicle_journey and notes type
            # Our comment is on the 'all' vj, it should have one (and only one) note
            vj_link = [
                l
                for l in date_time["links"]
                if l["type"] == "vehicle_journey" and l["id"] == "vehicle_journey:all"
            ]
            notes_link = [l for l in date_time["links"] if l["type"] == "notes"]
            assert len(vj_link) == 0 or len(notes_link) == 1

        # Check that the comment is only on the header and not in the rows
        response = self.query_region("lines/line:A/route_schedules?from_datetime=20120615T080000")
        is_valid_notes(response["notes"])
        assert 'error' not in response
        assert 'route_schedules' in response
        assert len(response["route_schedules"]) == 1
        assert len(response["notes"]) == 2
        assert len(response["route_schedules"][0]["table"]["headers"]) == 4

        for header in response["route_schedules"][0]["table"]["headers"]:
            vj_link = [
                l for l in header["links"] if l["type"] == "vehicle_journey" and l["id"] == "vehicle_journey:all"
            ]
            notes_link = [l for l in header["links"] if l["type"] == "notes"]
            # Assert that if it's the 'all' vj, there is one and only one note
            assert len(vj_link) == 0 or len(notes_link) == 1

        for row in response["route_schedules"][0]["table"]["rows"]:
            for date_time in row["date_times"]:
                vj_link = [
                    l
                    for l in date_time["links"]
                    if l["type"] == "vehicle_journey" and l["id"] == "vehicle_journey:all"
                ]
                notes_link = [l for l in date_time["links"] if l["type"] == "notes"]
                # assert that if it's the 'all' vj there is no note
                assert len(vj_link) == 0 or len(notes_link) == 0

    def test_line_closed(self):
        """
        Check the ResponseStatus when the line is closed
        """
        response_1 = self.query_region("lines/C/stop_schedules?from_datetime=20120614T0830&duration=600")
        response_2 = self.query_region("lines/D/stop_schedules?from_datetime=20120614T0700&duration=600")

        # Check that when the line is closed the additional_information is no_active_circulation_this_day for all
        ss1_c = next(ss for ss in response_1['stop_schedules'] if ss['stop_point']['id'] == 'stop1_C')
        ss2_c = next(ss for ss in response_1['stop_schedules'] if ss['stop_point']['id'] == 'stop2_C')
        ss1_d = next(ss for ss in response_2['stop_schedules'] if (ss['stop_point']['id'] == 'stop1_D'))
        ss2_d = next(ss for ss in response_2['stop_schedules'] if (ss['stop_point']['id'] == 'stop2_D'))
        ss3_d = next(ss for ss in response_2['stop_schedules'] if ss['stop_point']['id'] == 'stop3_D')
        assert ss1_c['additional_informations'] == 'no_active_circulation_this_day'
        assert ss2_c['additional_informations'] == 'no_active_circulation_this_day'
        assert ss1_d['additional_informations'] == 'no_active_circulation_this_day'
        assert ss2_d['additional_informations'] == 'no_active_circulation_this_day'
        assert ss3_d['additional_informations'] == 'no_active_circulation_this_day'

    def test_display_informations_in_routes_schedule(self):
        """
        verify some attributs in display_informations of a route_schedule
        """
        response = self.query_region("routes/line:A:0/route_schedules?from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        schedules = get_not_null(response, 'route_schedules')

        assert len(schedules) == 1, "there should be only one elt"
        schedule = schedules[0]
        is_valid_route_schedule(schedule)

        display_information_route = get_not_null(schedule, 'display_informations')
        assert display_information_route['direction'] == 'stop2'
        assert display_information_route['label'] == 'A'
        assert display_information_route['color'] == '289728'
        assert display_information_route['text_color'] == 'FFD700'
        assert display_information_route['name'] == 'line:A'
        assert display_information_route['code'] == 'A'

    def test_terminus_schedules(self):
        """
        terminus_schedules for a given date
        """
        response = self.query_region("stop_points/ODTstop1/terminus_schedules?from_datetime=20120615T080000")

        is_valid_notes(response["notes"])
        assert "terminus_schedules" in response
        assert len(response["terminus_schedules"]) == 1
        is_valid_terminus_schedules(response["terminus_schedules"], self.tester, only_time=False)
        assert len(response["terminus_schedules"][0]["date_times"]) == 2
        assert response["terminus_schedules"][0]["stop_point"]["name"] == "ODTstop1"
        assert response["terminus_schedules"][0]["route"]["name"] == "B"
        assert response["terminus_schedules"][0]["display_informations"]["direction"] == "ODTstop2"
        assert response["terminus_schedules"][0]["display_informations"]["name"] == "B"
        assert response["terminus_schedules"][0]["display_informations"]["commercial_mode"] == "Bus"
        assert response["terminus_schedules"][0]["display_informations"]["headsign"] == "date_time_estimated"
        assert (
            response["terminus_schedules"][0]["display_informations"]["trip_short_name"] == "date_time_estimated"
        )

    # Test on an on_demand_transport with start stop_datetime as on_demand_transport
    def test_journey_with_odt_in_start_stop_date_time(self):
        """
        journeys with odt -> start or end stop_datetime contains "on_demand_transport" in "additional_informations[]"
        network with code.type = "app_code" and code.value = "ilevia"
        There should be a deeplink of app_code ilevia dans section.links[]
        """
        response = self.query_region("journeys?from=ODTstop1&to=ODTstop2&datetime=20120615T103500")
        assert len(response['journeys']) == 1
        journey = response['journeys'][0]
        assert len(journey['sections']) == 1
        section = journey['sections'][0]
        assert section['type'] == "on_demand_transport"
        assert "odt_with_stop_time" in section['additional_informations']
        assert len(section['stop_date_times']) == 2
        assert "on_demand_transport" in section['stop_date_times'][0]['additional_informations']
        assert len(section['stop_date_times'][1]['additional_informations']) == 0

        # verify network in links
        assert len(section['links']) == 7
        assert "base_network" in [link['id'] for link in section['links'] if link['type'] == "network"]
        assert "B" in [link['id'] for link in section['links'] if link['type'] == "line"]

        # verify app deep link in links
        deep_link = section['links'][-1]
        assert "ilevia://home?" in deep_link['href']
        assert "departure_latitude=0" in deep_link['href']
        assert "departure_longitude=0" in deep_link['href']
        assert "destination_latitude=0" in deep_link['href']
        assert "destination_longitude=0" in deep_link['href']
        assert "requested_departure_time=2012-06-15T11%3A00%3A00%2B00%3A00" in deep_link['href']
        assert "territory=territory%3AB" in deep_link['href']
        assert "departure_display_name=ODTstop1" in deep_link['href']
        assert "arrival_display_name=ODTstop2" in deep_link['href']
        assert deep_link['type'] == "tad_dynamic_link"
        assert deep_link['rel'] == "tad_dynamic_link"
        assert deep_link['templated'] is False

    def test_journey_with_estimated_datetime_in_start_stop_date_time(self):
        """
        journeys with public_transport even if start stop_datetime is "date_time_estimated"
        No deeplink
        """
        response = self.query_region("journeys?from=ODTstop1&to=ODTstop2&datetime=20120615T080000")

        assert len(response['journeys']) == 1
        section = response['journeys'][0]['sections'][0]
        assert section['type'] == "public_transport"
        assert "has_datetime_estimated" in section['additional_informations']
        assert len(section['stop_date_times']) == 2
        assert "date_time_estimated" in section['stop_date_times'][0]['additional_informations']
        assert len(section['stop_date_times'][1]['additional_informations']) == 0

    def test_journey_with_skipped_stop_as_intermediate_stop(self):
        """
        Verify stop_date_times in a journey, an intermediate skipped_stop is not displayed.
        But geojson contains coordinates of three stops including skipped_stop
        """
        response = self.query_region("journeys?from=stop1_D&to=stop3_D&datetime=20120615T080000")

        assert len(response['journeys']) == 1
        stop_date_times = response['journeys'][0]['sections'][0]['stop_date_times']
        assert len(stop_date_times) == 2
        assert len(stop_date_times[0]['additional_informations']) == 0
        assert stop_date_times[0]['departure_date_time'] == "20120616T001000"
        assert stop_date_times[0]['arrival_date_time'] == "20120616T001000"
        assert len(stop_date_times[0]['additional_informations']) == 0
        assert stop_date_times[1]['departure_date_time'] == "20120616T025000"
        assert stop_date_times[1]['arrival_date_time'] == "20120616T025000"
        geojson = response['journeys'][0]['sections'][0]['geojson']
        assert len(geojson['coordinates']) == 3

    def test_route_schedules_with_skipped_stop(self):
        """
        Verify stop date_times in a route_schedules, with and without skipped_stop
        Node date_time with skipped_stop will have:
        1. date_time empty
        2. additional_informations contains skipped_stop
        3. data_freshness none
        4. links size = 2 as other normal nodes
        """
        response = self.query_region("routes/D:4/route_schedules?from_datetime=20120615T080000")
        route_schedule = response['route_schedules']
        assert len(route_schedule[0]['table']['rows']) == 3
        nodes = route_schedule[0]['table']['rows']
        assert len(nodes) == 3
        assert len(nodes[0]['date_times']) == 1
        date_time = nodes[0]['date_times'][0]
        assert date_time['base_date_time'] == "20120616T001000"
        assert date_time['date_time'] == "20120616T001000"
        assert len(date_time['additional_informations']) == 0
        assert date_time['data_freshness'] == "base_schedule"
        assert len(date_time['links']) == 2

        # Node with skipped_stop
        assert len(nodes[1]['date_times']) == 1
        date_time = nodes[1]['date_times'][0]
        assert 'base_date_time' not in date_time
        assert date_time['date_time'] == ""
        assert len(date_time['additional_informations']) == 1
        assert date_time['additional_informations'][0] == "skipped_stop"
        assert date_time['data_freshness'] is None
        assert len(date_time['links']) == 2

        assert len(nodes[2]['date_times']) == 1
        date_time = nodes[2]['date_times'][0]
        assert date_time['base_date_time'] == "20120616T025000"
        assert date_time['date_time'] == "20120616T025000"
        assert len(date_time['additional_informations']) == 0
        assert date_time['data_freshness'] == "base_schedule"
        assert len(date_time['links']) == 2


StopSchedule = namedtuple('StopSchedule', ['sp', 'route', 'date_times'])
SchedDT = namedtuple('SchedDT', ['dt', 'vj'])

Departure = namedtuple('Departure', ['sp', 'route', 'dt'])


def check_stop_schedule(response, reference):
    """
    check the values in a stopschedule
    check the id of the stoppoint, the id of the routes, the datetimes and the vj used

    Note: the stop_schedule are not sorted, we just must find all reference StopSchedule
    """
    assert len(response) == len(reference)
    for resp in response:
        ref = next(
            r
            for r in reference
            if r.sp == get_not_null(get_not_null(resp, 'stop_point'), 'id')
            and r.route == get_not_null(get_not_null(resp, 'route'), 'id')
        )

        for (resp_dt, ref_st) in zip_longest(resp['date_times'], ref.date_times):
            assert get_not_null(resp_dt, 'date_time') == ref_st.dt
            assert get_not_null(resp_dt, 'links')[0]['id'].find(ref_st.vj) == 0


def check_departures(response, reference):
    """
    check the values in a departures
    """
    for (resp, ref) in zip_longest(response, reference):
        assert get_not_null(get_not_null(resp, 'stop_point'), 'id') == ref.sp
        assert get_not_null(get_not_null(resp, 'route'), 'id') == ref.route
        assert get_not_null(get_not_null(resp, 'stop_date_time'), 'departure_date_time') == ref.dt


@dataset({"basic_schedule_test": {}})
class TestSchedules(AbstractTestFixture):
    def test_classic_stop_schedule(self):
        """default is Realtime stopschedule"""
        response = self.query_region("stop_points/S1/stop_schedules?from_datetime=20160101T080000")

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

    def test_stop_schedule_base_schedule(self):
        response = self.query_region(
            "stop_points/S1/stop_schedules?from_datetime=20160101T080000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        check_stop_schedule(
            stop_sched,
            [
                StopSchedule(
                    sp='S1',
                    route='A:0',
                    date_times=[
                        SchedDT(dt='20160101T090000', vj='vehicle_journey:A:vj1'),
                        SchedDT(dt='20160101T100000', vj='vehicle_journey:A:vj2'),
                        SchedDT(dt='20160101T110000', vj='vehicle_journey:A:vj3'),
                    ],
                ),
                StopSchedule(
                    sp='S1', route='B:1', date_times=[SchedDT(dt='20160101T113000', vj='vehicle_journey:B:vj1')]
                ),
            ],
        )

    def test_forbidden_uris_on_schedules(self):
        """test forbidden uri for schedules"""
        response, code = self.query_no_assert(
            "v1/coverage/basic_schedule_test/stop_points/S1/stop_schedules"
            "?from_datetime=20160101T080000&data_freshness=base_schedule"
        )
        assert code == 200
        schedules = get_not_null(response, 'stop_schedules')
        assert len(schedules) == 2

        # filtering stop_schedules on line A
        response, code = self.query_no_assert(
            "v1/coverage/basic_schedule_test/stop_points/S1/stop_schedules"
            "?from_datetime=20160101T080000&data_freshness=base_schedule"
            "&forbidden_uris[]=A"
        )
        assert code == 200
        schedules = get_not_null(response, 'stop_schedules')
        assert len(schedules) == 1

        # for retrocompatibility purpose forbidden_id[] is the same
        response, code = self.query_no_assert(
            "v1/coverage/basic_schedule_test/stop_points/S1/stop_schedules"
            "?from_datetime=20160101T080000&data_freshness=base_schedule"
            "&forbidden_id[]=A"
        )
        assert code == 200
        schedules = get_not_null(response, 'stop_schedules')
        assert len(schedules) == 1

        # for retrocompatibility purpose forbidden_id[] is the same
        response, code = self.query_no_assert(
            "v1/coverage/basic_schedule_test/stop_points/S1/stop_schedules"
            "?from_datetime=20160101T080000&data_freshness=base_schedule"
            "&forbidden_id[]=bloubliblou"
        )
        assert code == 200
        schedules = get_not_null(response, 'stop_schedules')
        assert len(schedules) == 2

    @staticmethod
    def check_stop_schedule_rt_sol(stop_sched):
        check_stop_schedule(
            stop_sched,
            [
                StopSchedule(
                    sp='S1',
                    route='A:0',
                    date_times=[
                        SchedDT(dt='20160101T090700', vj='vehicle_journey:A:vj1:RealTime:'),
                        SchedDT(dt='20160101T100700', vj='vehicle_journey:A:vj2:RealTime:'),
                        SchedDT(dt='20160101T110700', vj='vehicle_journey:A:vj3:RealTime:'),
                    ],
                ),
                StopSchedule(
                    sp='S1', route='B:1', date_times=[SchedDT(dt='20160101T113000', vj='vehicle_journey:B:vj1')]
                ),
            ],
        )

    def test_stop_schedule_realtime(self):
        """
        we give a from_datetime and a data_freshness, we should get the schedule
        from this datetime and with realtime data
        """
        response = self.query_region(
            "stop_points/S1/stop_schedules?from_datetime=20160101T080000" "&data_freshness=realtime"
        )

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

        # default data_freshness=realtime, so it should be the same response with only from_datetime
        response = self.query_region("stop_points/S1/stop_schedules?from_datetime=20160101T080000")

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

        # default data_freshness=realtime, so it should be the same response without from_datetime
        response = self.query_region("stop_points/S1/stop_schedules?_current_datetime=20160101T080000")

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

    def test_route_schedule_freshness(self):
        # default freshness is base_schedule
        r = self.query_region("stop_points/S1/route_schedules?_current_datetime=20160101T080000")
        assert r['route_schedules'][0]['table']['rows'][0]['date_times'][0]['data_freshness'] == 'base_schedule'

        # default freshness is base_schedule with from_datetime too
        r = self.query_region("stop_points/S1/route_schedules?from_datetime=20160101T080000")
        assert r['route_schedules'][0]['table']['rows'][0]['date_times'][0]['data_freshness'] == 'base_schedule'

        # respect data_freshness param
        r = self.query_region(
            "stop_points/S1/route_schedules?_current_datetime=20160101T080000&data_freshness=realtime"
        )
        assert r['route_schedules'][0]['table']['rows'][0]['date_times'][0]['data_freshness'] == 'realtime'
        r = self.query_region(
            "stop_points/S1/route_schedules?_current_datetime=20160101T080000&data_freshness=base_schedule"
        )
        assert r['route_schedules'][0]['table']['rows'][0]['date_times'][0]['data_freshness'] == 'base_schedule'

    def test_stop_schedule_realtime_limit_per_schedule(self):
        """
        same as test_stop_schedule_realtime, but we limit the number of item per schedule
        """
        response = self.query_region(
            "stop_points/S1/stop_schedules?from_datetime=20160101T080000"
            "&data_freshness=realtime&items_per_schedule=1"
        )

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        check_stop_schedule(
            stop_sched,
            [
                StopSchedule(
                    sp='S1',
                    route='A:0',
                    date_times=[SchedDT(dt='20160101T090700', vj='vehicle_journey:A:vj1:RealTime:')],
                ),
                StopSchedule(
                    sp='S1', route='B:1', date_times=[SchedDT(dt='20160101T113000', vj='vehicle_journey:B:vj1')]
                ),
            ],
        )

        # and test with a limit at 0
        response = self.query_region(
            "stop_points/S1/stop_schedules?from_datetime=20160101T080000"
            "&data_freshness=realtime&items_per_schedule=0"
        )

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]

        check_stop_schedule(
            stop_sched,
            [
                StopSchedule(sp='S1', route='A:0', date_times=[]),
                StopSchedule(sp='S1', route='B:1', date_times=[]),
            ],
        )

        # test retrocompat'
        response = self.query_region(
            "stop_points/S1/stop_schedules?from_datetime=20160101T080000"
            "&data_freshness=realtime&max_date_times=0"
        )

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]

        check_stop_schedule(
            stop_sched,
            [
                StopSchedule(sp='S1', route='A:0', date_times=[]),
                StopSchedule(sp='S1', route='B:1', date_times=[]),
            ],
        )

    def test_stop_schedule_no_dt(self):
        """
        test a stop schedule without a from_datetime parameter

        we should have the current datetime used and the realtime activated

        Note: for test purpose we override the current_datetime
        """
        response = self.query_region("stop_points/S1/stop_schedules?_current_datetime=20160101T080000")

        is_valid_notes(response["notes"])
        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

    @staticmethod
    def check_departure_base_schedule_sol(departures):
        check_departures(
            departures,
            [
                Departure(sp='S1', route='A:0', dt='20160101T090000'),
                Departure(sp='S1', route='A:0', dt='20160101T100000'),
                Departure(sp='S1', route='A:0', dt='20160101T110000'),
                Departure(sp='S1', route='B:1', dt='20160101T113000'),
            ],
        )

    @staticmethod
    def check_departure_rt_sol(departures):
        # RT activated: all A:0 vj are 7 mn late
        check_departures(
            departures,
            [
                Departure(sp='S1', route='A:0', dt='20160101T090700'),
                Departure(sp='S1', route='A:0', dt='20160101T100700'),
                Departure(sp='S1', route='A:0', dt='20160101T110700'),
                Departure(sp='S1', route='B:1', dt='20160101T113000'),
            ],
        )

    def test_departure(self):
        """
        test a departure without nothing

        we should have the current datetime used and the realtime activated
        """
        response = self.query_region("stop_points/S1/departures?_current_datetime=20160101T080000")

        is_valid_notes(response["notes"])
        departures = response["departures"]
        is_valid_departures(departures)
        # Since data_freshness is not specified, it's set to realtime by default,
        # and there shouldn't be (prev, next) links for api  navigation
        assert not [l for l in response.get('links') if l.get('rel') == 'prev']
        assert not [l for l in response.get('links') if l.get('rel') == 'next']
        self.check_departure_rt_sol(departures)

        # Same with only from_datetime specified
        response = self.query_region("stop_points/S1/departures?from_datetime=20160101T080000")

        is_valid_notes(response["notes"])
        departures = response["departures"]
        is_valid_departures(departures)
        assert not [l for l in response.get('links') if l.get('rel') == 'prev']
        assert not [l for l in response.get('links') if l.get('rel') == 'next']
        self.check_departure_rt_sol(departures)

    def test_departures_with_direction_type(self):
        """
        Test direction type parameter for filtering departures response
        data contains :
           - route A (direction type = forward)
                * 3 departures on stop uri : S1
           - route B (direction type = backward)
                * 1 departure on stop uri : S1
        """

        # Without direction type parameter
        response = self.query_region("stop_points/S1/departures?_current_datetime=20160101T080000")
        assert "departures" in response
        assert len(response["departures"]) == 4

        # With direction type = all (same as without direction type parameter)
        response = self.query_region(
            "stop_points/S1/departures?direction_type=all&_current_datetime=20160101T080000"
        )
        assert "departures" in response
        assert len(response["departures"]) == 4

        # With direction type = forward
        response = self.query_region(
            "stop_points/S1/departures?direction_type=forward&_current_datetime=20160101T080000"
        )
        assert "departures" in response
        assert len(response["departures"]) == 3

        # With direction type = backward
        response = self.query_region(
            "stop_points/S1/departures?direction_type=backward&_current_datetime=20160101T080000"
        )
        assert "departures" in response
        assert len(response["departures"]) == 1

    def test_departure_base_sched(self):
        """
        test a departure, no date time and base schedule, we should get base schedule
        """
        response = self.query_region(
            "stop_points/S1/departures?_current_datetime=20160101T080000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        departures = response["departures"]
        is_valid_departures(departures)
        # test if there are 'prev' and 'next' links for api navigation
        assert next(l for l in response.get('links') if l.get('rel') == 'prev')
        assert next(l for l in response.get('links') if l.get('rel') == 'next')
        self.check_departure_base_schedule_sol(departures)

    def test_departure_dt(self):
        """
        test a departure, with a date time

        we should have the wanted datetime used and the realtime activated
        """
        response = self.query_region("stop_points/S1/departures?from_datetime=20160101T080000")

        is_valid_notes(response["notes"])
        departures = response["departures"]
        is_valid_departures(departures)
        self.check_departure_rt_sol(departures)

    def test_departure_dt_base_schedule(self):
        """
        test a departure, with a date time

        we should have the wanted datetime used and the realtime activated
        """
        response = self.query_region(
            "stop_points/S1/departures?from_datetime=20160101T080000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        departures = response["departures"]
        is_valid_departures(departures)
        self.check_departure_base_schedule_sol(departures)

    def test_departure_schedule_departure_date_time_not_eq_arrival_date_time(self):
        """
        test a departure api

        base_departure_date_time != base_arrival_date_time on S11
        """
        response = self.query_region(
            "stop_points/S11/departures?from_datetime=20160101T080000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        departures = response["departures"]

        is_valid_departures(departures)
        assert len(departures) == 1
        assert departures[0]["route"]["id"] == 'M:6'
        assert departures[0]["stop_point"]["id"] == 'S11'
        assert departures[0]["stop_date_time"]["arrival_date_time"] == '20160101T113000'
        assert departures[0]["stop_date_time"]["base_arrival_date_time"] == '20160101T113000'
        assert departures[0]["stop_date_time"]["departure_date_time"] == '20160101T113500'
        assert departures[0]["stop_date_time"]["base_departure_date_time"] == '20160101T113500'

    def test_arrival_schedule_departure_date_time_not_eq_arrival_date_time(self):
        """
        test a arrival api

        base_departure_date_time != base_arrival_date_time on S11 and without realtime
        """
        response = self.query_region(
            "stop_points/S11/arrivals?from_datetime=20160101T080000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        arrivals = response["arrivals"]

        is_valid_departures(arrivals)
        assert len(arrivals) == 1
        assert arrivals[0]["route"]["id"] == 'M:6'
        assert arrivals[0]["stop_point"]["id"] == 'S11'
        assert arrivals[0]["stop_date_time"]["arrival_date_time"] == '20160101T113000'
        assert arrivals[0]["stop_date_time"]["departure_date_time"] == '20160101T113500'

    def test_arrival_schedule_arrivals_date_time_not_eq_arrival_date_time_realtime(self):
        """
        test a arrival api

        base_departure_date_time != base_arrival_date_time on stopP2 and the realtime activated
        """
        response = self.query_region(
            "stop_points/stopP2/arrivals?from_datetime=20160103T100000" "&data_freshness=realtime"
        )

        is_valid_notes(response["notes"])
        arrivals = response["arrivals"]

        is_valid_departures(arrivals)
        assert len(arrivals) == 1
        assert arrivals[0]["route"]["id"] == 'P:7'
        assert arrivals[0]["stop_point"]["id"] == 'stopP2'
        assert arrivals[0]["stop_date_time"]["arrival_date_time"] == '20160104T000800'
        assert arrivals[0]["stop_date_time"]["base_arrival_date_time"] == '20160104T000400'
        assert arrivals[0]["stop_date_time"]["departure_date_time"] == '20160104T001000'
        assert arrivals[0]["stop_date_time"]["base_departure_date_time"] == '20160104T000600'
        assert arrivals[0]["stop_date_time"]["data_freshness"] == 'realtime'

    def test_departure_schedule_departures_date_time_not_eq_arrival_date_time_realtime(self):
        """
        test a departure api

        base_departure_date_time != base_arrival_date_time on stopP2 and the realtime activated
        """
        response = self.query_region(
            "stop_points/stopP2/departures?from_datetime=20160103T100000" "&data_freshness=realtime"
        )

        is_valid_notes(response["notes"])
        departures = response["departures"]

        is_valid_departures(departures)
        assert len(departures) == 1
        assert departures[0]["route"]["id"] == 'P:7'
        assert departures[0]["stop_point"]["id"] == 'stopP2'
        assert departures[0]["stop_date_time"]["arrival_date_time"] == '20160104T000800'
        assert departures[0]["stop_date_time"]["base_arrival_date_time"] == '20160104T000400'
        assert departures[0]["stop_date_time"]["departure_date_time"] == '20160104T001000'
        assert departures[0]["stop_date_time"]["base_departure_date_time"] == '20160104T000600'
        assert departures[0]["stop_date_time"]["data_freshness"] == 'realtime'

    def test_arrival_schedule_arrivals_date_time_midnight_realtime(self):
        """
        test a arrival api

        base_departure_date_time != base_arrival_date_time on stopQ2 and the realtime activated
        """
        response = self.query_region(
            "stop_points/stopQ2/arrivals?from_datetime=20160103T230000" "&data_freshness=realtime"
        )

        is_valid_notes(response["notes"])
        arrivals = response["arrivals"]

        is_valid_departures(arrivals)
        assert len(arrivals) == 1
        assert arrivals[0]["route"]["id"] == 'Q:8'
        assert arrivals[0]["stop_point"]["id"] == 'stopQ2'
        assert arrivals[0]["stop_date_time"]["arrival_date_time"] == '20160104T000500'
        assert arrivals[0]["stop_date_time"]["base_arrival_date_time"] == '20160103T234400'
        assert arrivals[0]["stop_date_time"]["departure_date_time"] == '20160104T000600'
        assert arrivals[0]["stop_date_time"]["base_departure_date_time"] == '20160103T234600'
        assert arrivals[0]["stop_date_time"]["data_freshness"] == 'realtime'

    def test_departure_schedule_departures_date_time_midnight_realtime(self):
        """
        test a departure api

        base_departure_date_time != base_arrival_date_time on stopQ2 and the realtime activated
        """
        response = self.query_region(
            "stop_points/stopQ2/departures?from_datetime=20160103T100000" "&data_freshness=realtime"
        )

        is_valid_notes(response["notes"])
        departures = response["departures"]

        is_valid_departures(departures)
        assert len(departures) == 1
        assert departures[0]["route"]["id"] == 'Q:8'
        assert departures[0]["stop_point"]["id"] == 'stopQ2'
        assert departures[0]["stop_date_time"]["arrival_date_time"] == '20160104T000500'
        assert departures[0]["stop_date_time"]["base_arrival_date_time"] == '20160103T234400'
        assert departures[0]["stop_date_time"]["departure_date_time"] == '20160104T000600'
        assert departures[0]["stop_date_time"]["base_departure_date_time"] == '20160103T234600'
        assert departures[0]["stop_date_time"]["data_freshness"] == 'realtime'

    def test_arrival_schedule_arrivals_date_time_midnight_base_schedule(self):
        """
        test a arrival api

        base_departure_date_time != base_arrival_date_time on stopQ2 and without realtime
        """
        response = self.query_region(
            "stop_points/stopQ2/arrivals?from_datetime=20160103T230000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        arrivals = response["arrivals"]

        is_valid_departures(arrivals)
        assert len(arrivals) == 1
        assert arrivals[0]["route"]["id"] == 'Q:8'
        assert arrivals[0]["stop_point"]["id"] == 'stopQ2'
        assert arrivals[0]["stop_date_time"]["arrival_date_time"] == '20160103T234400'
        assert arrivals[0]["stop_date_time"]["base_arrival_date_time"] == '20160103T234400'
        assert arrivals[0]["stop_date_time"]["departure_date_time"] == '20160103T234600'
        assert arrivals[0]["stop_date_time"]["base_departure_date_time"] == '20160103T234600'
        assert arrivals[0]["stop_date_time"]["data_freshness"] == 'base_schedule'

    def test_departure_schedule_departures_date_time_midnight_base_schedule(self):
        """
        test a departure api

        base_departure_date_time != base_arrival_date_time on stopQ2 and without realtime
        """
        response = self.query_region(
            "stop_points/stopQ2/departures?from_datetime=20160103T100000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        departures = response["departures"]

        is_valid_departures(departures)
        assert len(departures) == 1
        assert departures[0]["route"]["id"] == 'Q:8'
        assert departures[0]["stop_point"]["id"] == 'stopQ2'
        assert departures[0]["stop_date_time"]["arrival_date_time"] == '20160103T234400'
        assert departures[0]["stop_date_time"]["base_arrival_date_time"] == '20160103T234400'
        assert departures[0]["stop_date_time"]["departure_date_time"] == '20160103T234600'
        assert departures[0]["stop_date_time"]["base_departure_date_time"] == '20160103T234600'
        assert departures[0]["stop_date_time"]["data_freshness"] == 'base_schedule'

    def test_arrival_schedule_arrivals_date_time_frequency_base_schedule(self):
        """
        test a arrival api with frequency line and without realtime
        """
        response = self.query_region(
            "stop_points/stopf2/arrivals?from_datetime=20160103T100000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        arrivals = response["arrivals"]

        is_valid_departures(arrivals)
        assert len(arrivals) == 3
        assert arrivals[0]["route"]["id"] == 'l:freq:9'
        assert arrivals[0]["stop_point"]["id"] == 'stopf2'
        assert arrivals[0]["stop_date_time"]["arrival_date_time"] == '20160103T180500'
        assert arrivals[0]["stop_date_time"]["base_arrival_date_time"] == '20160103T180500'
        assert arrivals[0]["stop_date_time"]["departure_date_time"] == '20160103T181000'
        assert arrivals[0]["stop_date_time"]["base_departure_date_time"] == '20160103T181000'
        assert arrivals[0]["stop_date_time"]["data_freshness"] == 'base_schedule'

        assert arrivals[1]["route"]["id"] == 'l:freq:9'
        assert arrivals[1]["stop_point"]["id"] == 'stopf2'
        assert arrivals[1]["stop_date_time"]["arrival_date_time"] == '20160103T183500'
        assert arrivals[1]["stop_date_time"]["base_arrival_date_time"] == '20160103T183500'
        assert arrivals[1]["stop_date_time"]["departure_date_time"] == '20160103T184000'
        assert arrivals[1]["stop_date_time"]["base_departure_date_time"] == '20160103T184000'
        assert arrivals[1]["stop_date_time"]["data_freshness"] == 'base_schedule'

        assert arrivals[2]["route"]["id"] == 'l:freq:9'
        assert arrivals[2]["stop_point"]["id"] == 'stopf2'
        assert arrivals[2]["stop_date_time"]["arrival_date_time"] == '20160103T190500'
        assert arrivals[2]["stop_date_time"]["base_arrival_date_time"] == '20160103T190500'
        assert arrivals[2]["stop_date_time"]["departure_date_time"] == '20160103T191000'
        assert arrivals[2]["stop_date_time"]["base_departure_date_time"] == '20160103T191000'
        assert arrivals[2]["stop_date_time"]["data_freshness"] == 'base_schedule'

    def test_departure_schedule_departures_date_time_frequency_base_schedule(self):
        """
        test a departure api with frequency line and without realtime
        """
        response = self.query_region(
            "stop_points/stopf2/departures?from_datetime=20160103T100000" "&data_freshness=base_schedule"
        )

        is_valid_notes(response["notes"])
        departures = response["departures"]

        is_valid_departures(departures)
        assert len(departures) == 3
        assert departures[0]["route"]["id"] == 'l:freq:9'
        assert departures[0]["stop_point"]["id"] == 'stopf2'
        assert departures[0]["stop_date_time"]["arrival_date_time"] == '20160103T180500'
        assert departures[0]["stop_date_time"]["base_arrival_date_time"] == '20160103T180500'
        assert departures[0]["stop_date_time"]["departure_date_time"] == '20160103T181000'
        assert departures[0]["stop_date_time"]["base_departure_date_time"] == '20160103T181000'
        assert departures[0]["stop_date_time"]["data_freshness"] == 'base_schedule'

        assert departures[1]["route"]["id"] == 'l:freq:9'
        assert departures[1]["stop_point"]["id"] == 'stopf2'
        assert departures[1]["stop_date_time"]["arrival_date_time"] == '20160103T183500'
        assert departures[1]["stop_date_time"]["base_arrival_date_time"] == '20160103T183500'
        assert departures[1]["stop_date_time"]["departure_date_time"] == '20160103T184000'
        assert departures[1]["stop_date_time"]["base_departure_date_time"] == '20160103T184000'
        assert departures[1]["stop_date_time"]["data_freshness"] == 'base_schedule'

        assert departures[2]["route"]["id"] == 'l:freq:9'
        assert departures[2]["stop_point"]["id"] == 'stopf2'
        assert departures[2]["stop_date_time"]["arrival_date_time"] == '20160103T190500'
        assert departures[2]["stop_date_time"]["base_arrival_date_time"] == '20160103T190500'
        assert departures[2]["stop_date_time"]["departure_date_time"] == '20160103T191000'
        assert departures[2]["stop_date_time"]["base_departure_date_time"] == '20160103T191000'
        assert departures[2]["stop_date_time"]["data_freshness"] == 'base_schedule'

    def test_routes_schedule_with_invalid_forbidden_uri(self):
        """
        there is no reason to have forbidden_uri in route_schedules, but it shouldn't crash...
        """
        _, code = self.query_region(
            "routes/line:A:0/route_schedules?" "from_datetime=20120615T080000&forbidden_uris[]=toto", check=False
        )
        assert code == 404


@dataset({"timezone_cape_verde_test": {}})
class TestFirstLastDatetimeWithNegativeTimezone(AbstractTestFixture):

    """
    test first/last datetime with a timezone whose UTC offset is **negative**

    * The opening time should be the earliest departure of all vjs of the line
    * The closing time should be the latest arrival of all vjs of the line

    The closing time of line X is 02:45(local time) and the opening time is 8:00(local time)

    Datetimes shown in the figure are all **LOCAL** datetimes

    [2017T0101, 2017T0103] and [2017T0106, 2017T0108]
    X_s1 ------------ X_S2 ------------ X_S3 ------------ X_S4
    07:00             15:00             23:30             24:40  X:vj1    route_1
    08:00             16:00             24:30             24:40  X:vj2    route_1
    09:00             17:00             25:30             25:40  X:vj3    route_1

    [2017T0104, 2017T0105]
    X_s1 ------------ X_S2 ------------ X_S3 ------------ X_S4
    07:05             15:05             23:45             23:55  X:vj4    route_1
    08:05             16:05             24:45             24:55  X:vj5    route_1
    09:05             17:05             25:45             25:55  X:vj6    route_1



    [2017T0101, 2017T0108]
    X_S4 ------------ X_S3 ------------ X_S2 ------------ X_S1
    07:55             08:05             16:05             24:45    X:vj7    route_2
    08:55             09:05             17:05             25:45    X:vj8    route_2

    """

    def test_first_last_datetime_same_day(self):
        """
                        20170102
                from_datetime 20170102T1630
                            |
                            v
        (Open)                        (Close)
        07:00                          25:55
         X_s1 ------------------------ X_S4

        """
        from_datetime = "20170102T163000"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2

        assert stop_schedules[0]['first_datetime']['date_time'] == "20170102T150000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170102T170000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170102T160500"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170102T170500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T070000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T090000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

        """              20170102
                       from_datetime 20170103T0001
                                  |
                                  v
        (Open)                        (Close)
        07:00                          25:55
         X_s1 ------------------------ X_S4

        """
        from_datetime = "20170103T000100"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T150000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T170000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170103T160500"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170103T170500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T070000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T090000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

        """              20170102
                       from_datetime 20170102T2301
                                  |
                                  v
        (Open)                        (Close)
        07:00                          25:55
         X_s1 ------------------------ X_S4

        """
        from_datetime = "20170102T2301"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T150000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T170000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170103T160500"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170103T170500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T070000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T090000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

    def test_first_last_datetime_diff_validity(self):
        """
                    20170102                                           20170103
                                      from_datetime 20170103T0300
                                                  |
                                                  v
        (Open)                         (Close)           (Open)                       (Close)
        07:00                          25:55     03:00   07:00                        25:55
         X_s1 ------------------------ X_S4              X_s1 ----------------------- X_S4

        """
        from_datetime = "20170103T030000"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T150000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T170000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170103T160500"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170103T170500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T070000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T090000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

        """
                        20170103            (switch of vp)             20170104
                                         from_datetime 20170104T0300
                                                    |
                                                    v
        (Open)                         (Close)           (Open)                       (Close)
        07:00                          25:55     03:00   07:00                        25:55
         X_s1 ------------------------ X_S4              X_s1 ----------------------- X_S4

        """
        from_datetime = "20170104T030000"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170104T150500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170104T170500"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170104T160500"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170104T170500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170104T070500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170104T090500"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

        """
                         20170105             (switch of vp)            20170106
                  from_datetime 20170105T2200
                                 |
                                 v
         (Open)                 22:00  (Close)           (Open)                       (Close)
         07:00                          25:55             07:00                        25:55
          X_s1 ------------------------ X_S4              X_s1 ----------------------- X_S4

         """
        from_datetime = "20170105T2200"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170106T150000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170106T170000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170106T160500"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170106T170500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170106T070000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170106T090000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

    def test_first_last_datetime_another_validity(self):
        """
                    20170104
                           from_datetime 20170105T0030
                                      |
                                      v
        (Open)                        (Close)
        07:00                          25:55
         X_s1 ------------------------ X_S4

        """
        from_datetime = "20170105T003000"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170105T150500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170105T170500"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170105T160500"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170105T170500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170105T070500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170105T090500"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

    def test_first_last_datetime_just_after_midnight(self):
        """
                    20170101
                           from_datetime 20170102T002959
                                      |
                                      v
        (Open)                        (Close)
        07:00                          25:55
         X_s1 ------------------------ X_S4

        """
        from_datetime = "20170102T002959"
        response = self.query_region(
            "stop_points/X_S3/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170101T233000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170102T013000"

    def test_first_last_datetime_with_very_small_duration(self):
        """
                        20170101
                           from_datetime 20170102T002959
                                      |
                                      v
        (Open)                        (Close)
        07:00                          25:55
         X_s1 ------------------------ X_S4

        """
        from_datetime = "20170102T002959"
        response = self.query_region(
            "stop_points/X_S3/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule&duration=1".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert len(stop_schedules[0]['date_times']) == 1
        assert stop_schedules[0]['route']['name'] == 'route_1'
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170101T233000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170102T013000"

        """              20170102
        from_datetime 20170102T065959
                  |
                  v
                    (Open)                        (Close)
                    07:00                          25:55
                     X_s1 ------------------------ X_S4

        """
        from_datetime = "20170102T065959"
        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule&duration=1".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert len(stop_schedules[0]['date_times']) == 1
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170102T070000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170102T090000"
        # since the duration is very small we can't find anything for this route point
        assert len(stop_schedules[1]['date_times']) == 0
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]


@dataset({"timezone_hong_kong_test": {}})
class TestFirstLastDatetimeWithPositiveTimezone(AbstractTestFixture):
    """
    test first/last datetime with a timezone whose UTC offset is **positive**
    (opening_time - utc_offset < 0)

    The closing time of line X is 07:25(local time) and the opening time is 6:00(local time)

    Datetimes shown in the figure are all **UTC** datetimes

    [2017T0101, 2017T0103] and [2017T0106, 2017T0108]
    X_s1 ------------ X_S2 ------------ X_S3 ------------ X_S4
    06:00             06:30             23:50             24:30    X:vj1    route_1
    06:10             06:40             24:00             24:40    X:vj2    route_1
    06:20             06:50             24:10             24:50    X:vj3    route_1

    [2017T0104, 2017T0105]
    X_s1 ------------ X_S2 ------------ X_S3 ------------ X_S4
    06:05             06:35             23:55             24:35    X:vj4    route_1
    06:05             06:45             24:05             24:45    X:vj5    route_1
    06:05             06:55             24:15             24:55    X:vj6    route_1


    [2017T0101, 2017T0108]
    X_s4 ------------ X_S3 ------------ X_S2 ------------ X_S1
    06:00             06:40             23:40             24:20    X:vj7    route_2
    06:05             06:45             23:45             24:25    X:vj8    route_2
    """

    def test_first_last_datetime_same_day(self):
        """
                        20170102
        from_datetime 20170102T0631
                 |
                 v
        (Open)                        (Close)
        06:00                         00:55
         X_S1 ------------------------ X_S4

        """
        from_datetime = "20170102T063100"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170102T063000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170102T065000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170102T234000"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170102T234500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T060000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T062000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

        """              20170102
                         from_datetime 20170103T000100
                                    |
                                    v
        (Open)                        (Close)
        06:00                         00:55
         X_S1 ------------------------ X_S4

        """
        from_datetime = "20170103T000100"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T063000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T065000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170103T234000"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170103T234500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T060000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T062000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

    def test_first_last_datetime_diff_validity(self):
        """
                    20170102                                            20170103
                                      from_datetime 20170103T0300
                                                 |
                                                 v
        (Open)                        (Close)            (Open)                        (Close)
        06:00                         00:55     03:00    06:00                         00:55
        X_S1 ------------------------ X_S4                X_S1 ------------------------ X_S4

        """
        from_datetime = "20170103T030000"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T063000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T065000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170103T234000"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170103T234500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170103T060000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T062000"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

        """
                        20170104                                                  20170105
                                      from_datetime 20170105T0300
                                                |
                                                v
        (Open)                        (Close)            (Open)                        (Close)
        06:00                         00:55    03:00     06:00                         00:55
        X_S1 ------------------------ X_S4                X_S1 ------------------------ X_S4

        """
        from_datetime = "20170105T030000"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170105T063500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170105T065500"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170105T234000"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170105T234500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170105T060500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170105T062500"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

    def test_first_last_datetime_another_validity(self):
        """
                        20170105
        from_datetime 20170105T0730
                   |
                   v
         (Open)                        (Close)
         06:00                         00:55
          X_S1 ------------------------ X_S4

        """
        from_datetime = "20170105T073000"
        response = self.query_region(
            "stop_points/X_S2/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170106T063500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170106T065500"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170105T234000"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170105T234500"

        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170106T060500"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170106T062500"
        # since it's the last stop_point, we cannot get on the bus....
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

    def test_first_last_datetime_just_after_midnight(self):
        """
                        20170101 (the beginning of production)
                           from_datetime 20170102T000001
                                      |
                                      v
        (Open)                        (Close)
        06:00                         00:55
         X_S1 ------------------------ X_S4

        """
        from_datetime = "20170102T000001"
        response = self.query_region(
            "stop_points/X_S3/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170102T235000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T001000"
        assert stop_schedules[1]['first_datetime']['date_time'] == "20170102T064000"
        assert stop_schedules[1]['last_datetime']['date_time'] == "20170102T064500"

        """              20170102
                           from_datetime 20170103T000001
                                      |
                                      v
        (Open)                        (Close)
        06:00                         00:55
         X_S1 ------------------------ X_S4

        """
        from_datetime = "20170103T000001"
        response = self.query_region(
            "stop_points/X_S3/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170102T235000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T001000"

    def test_first_last_datetime_with_very_small_duration(self):
        """
                    20170102
                            from_datetime 20170102T235959
                                      |
                                      v
        (Open)                        (Close)
        06:00                         00:55
         X_S1 ------------------------ X_S4

        """
        from_datetime = "20170102T235959"
        response = self.query_region(
            "stop_points/X_S3/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule&duration=1".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert len(stop_schedules[0]['date_times']) == 1
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170102T235000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170103T001000"
        # since the duration is very small we can't find anything for this route point
        assert len(stop_schedules[1]['date_times']) == 0
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

        """              20170102
        from_datetime 20170102T055959
                  |
                  v
                    (Open)                        (Close)
                    06:00                         00:55
                    X_S1 ------------------------ X_S4

        """
        from_datetime = "20170102T055959"
        response = self.query_region(
            "stop_points/X_S1/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule&duration=1".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert stop_schedules
        assert len(stop_schedules[0]['date_times']) == 1
        assert stop_schedules[0]['first_datetime']['date_time'] == "20170102T060000"
        assert stop_schedules[0]['last_datetime']['date_time'] == "20170102T062000"
        # since the duration is very small we can't find anything for this route point
        assert len(stop_schedules[1]['date_times']) == 0
        assert 'first_datetime' not in stop_schedules[1]
        assert 'last_datetime' not in stop_schedules[1]

    def test_duration_default_value(self):
        # Query with duration=86400
        # Here we have 4 date_times with the last one at 20170102T235000 + 86400 (24 hours)
        from_datetime = "20170102T235000"
        response = self.query_region(
            "stop_points/X_S3/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule&duration=86400".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert len(stop_schedules[0]['date_times']) == 4
        assert stop_schedules[0]['date_times'][0]['date_time'] == "20170102T235000"
        assert stop_schedules[0]['date_times'][1]['date_time'] == "20170103T000000"
        assert stop_schedules[0]['date_times'][2]['date_time'] == "20170103T001000"
        assert stop_schedules[0]['date_times'][3]['date_time'] == "20170103T235000"

        # Query without parameter duration (default value = 86399)
        # Here we have exclude the last date_time at 20170102T235000 + 86400 (24 hours)
        response = self.query_region(
            "stop_points/X_S3/stop_schedules?from_datetime={}"
            "&data_freshness=base_schedule".format(from_datetime)
        )

        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 2
        assert len(stop_schedules[0]['date_times']) == 3
        assert stop_schedules[0]['date_times'][0]['date_time'] == "20170102T235000"
        assert stop_schedules[0]['date_times'][1]['date_time'] == "20170103T000000"
        assert stop_schedules[0]['date_times'][2]['date_time'] == "20170103T001000"
