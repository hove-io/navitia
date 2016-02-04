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
from collections import namedtuple
import itertools
from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *
import datetime


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
            dt = dt_wrapper["date_time"]
            if only_time:
                get_valid_time(dt)
            else:
                get_valid_datetime(dt)
            #TODO remove href_mandatory=False after link refactor, they should always be there :)
            check_links(dt_wrapper, tester, href_mandatory=False)

        #TODO remove href_mandatory=False after link refactor, they should always be there :)
        check_links(schedule, tester, href_mandatory=False)


def is_valid_route_schedule_header(header):
    get_not_null(header, 'display_informations')

    links = get_not_null(header, 'links')

    #we should have at least:
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
            dt = get_not_null(dt_obj, "date_time")
            if only_time:
                get_valid_time(dt)
            else:
                get_valid_datetime(dt)

        is_valid_stop_point(get_not_null(row, 'stop_point'), depth_check=1)


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

    route = get_not_null(departure, 'route')
    is_valid_route(route)

    stop_point = get_not_null(departure, 'stop_point')
    is_valid_stop_point(stop_point)

    stop_date_time = get_not_null(departure, 'stop_date_time')
    is_valid_stop_date_time(stop_date_time)


def get_real_notes(obj, full_response):
    real_notes = {n['id']: n for n in get_not_null(full_response, 'notes')}
    return [real_notes[n['id']] for n in get_not_null(obj, 'links') if n['type'] == 'notes']


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
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=True)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

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
        # all datetime in the response should be datetime
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=False)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

        # after the structure check, we check some test specific stuff
        assert response["stop_schedules"][0]["stop_point"]["id"] == "stop1"
        assert response["stop_schedules"][0]["route"]["line"]["id"] == "line:A"

    def test_partial_terminus(self):
        """
        Partial Terminus
        """
        response = self.query_region("stop_areas/Tstop1/stop_schedules?"
                                     "from_datetime=20120615T080000")

        assert "stop_schedules" in response
        is_valid_stop_schedule(response["stop_schedules"], self.tester, only_time=False)
        assert len(response["stop_schedules"]) == 1, "there should be only one elt"

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

        headers = get_not_null(get_not_null(schedule, 'table'), 'headers')
        #there is 4 vjs
        assert len(headers) == 4

        # we get them by vj to check the vj that have a note
        headers_by_vj = {h['display_informations']['headsign']: h for h in headers}

        assert 'week' in headers_by_vj
        assert 'week_bis' in headers_by_vj
        assert 'all' in headers_by_vj
        assert 'wednesday' in headers_by_vj

        all_vj = headers_by_vj['all']
        all_vj_links = {(l['type'], l['id']): l for l in get_not_null(all_vj, 'links')}
        assert ('vehicle_journey', 'all') in all_vj_links
        assert ('physical_mode', 'physical_mode:0') in all_vj_links
        all_vj_notes = get_real_notes(all_vj, response)
        assert len(all_vj_notes) == 1
        assert all_vj_notes[0]['value'] == 'vj comment'

    def test_routes_schedule_with_calendar(self):
        """
        departure board for a given calendar
        """
        response = self.query_region("routes/line:A:0/route_schedules?calendar=week_cal")

        schedules = get_not_null(response, 'route_schedules')

        assert len(schedules) == 1, "there should be only one elt"
        schedule = schedules[0]
        # the route_schedule with calendar should not have with date (only time)
        is_valid_route_schedule(schedule, only_time=True)

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

        is_valid_departures(response["departures"])

        assert len(response["departures"][0]["stop_date_time"]["additional_informations"]) == 1
        assert response["departures"][0]["stop_date_time"]["additional_informations"][0] == "date_time_estimated"

        assert len(response["departures"][1]["stop_date_time"]["additional_informations"]) == 1
        assert response["departures"][1]["stop_date_time"]["additional_informations"][0] == "on_demand_transport"

    def test_vj_comment(self):
        """
        test that a comment on a vehicle journey is linked to each stop_times in stop_schedules
        but only in the header on route_schedules
        """
        # Check if the comment is on the stop_time
        response = self.query_region("stop_points/stop1/stop_schedules?from_datetime=20120615T080000")
        assert 'error' not in response
        assert 'stop_schedules' in response
        assert len(response["stop_schedules"]) == 1
        assert len(response["notes"]) == 2
        assert len(response["stop_schedules"][0]["date_times"]) == 4

        for date_time in response["stop_schedules"][0]["date_times"]:
            # Extract links of vehicle_journey and notes type
            # Our comment is on the 'all' vj, it should have one (and only one) note
            vj_link = [l for l in date_time["links"] if l["type"] == "vehicle_journey" and l["id"] == "all"]
            notes_link = [l for l in date_time["links"] if l["type"] == "notes"]
            assert(len(vj_link) == 0 or len(notes_link) == 1)

        # Check that the comment is only on the header and not in the rows
        response = self.query_region("lines/line:A/route_schedules?from_datetime=20120615T080000")
        assert 'error' not in response
        assert 'route_schedules' in response
        assert len(response["route_schedules"]) == 1
        assert len(response["notes"]) == 2
        assert len(response["route_schedules"][0]["table"]["headers"]) == 4

        for header in response["route_schedules"][0]["table"]["headers"]:
            vj_link = [l for l in header["links"] if l["type"] == "vehicle_journey" and l["id"] == "all"]
            notes_link = [l for l in header["links"] if l["type"] == "notes"]
            # Assert that if it's the 'all' vj, there is one and only one note
            assert(len(vj_link) == 0 or len(notes_link) == 1)

        for row in response["route_schedules"][0]["table"]["rows"]:
            for date_time in row["date_times"]:
                vj_link = [l for l in date_time["links"] if l["type"] == "vehicle_journey" and l["id"] == "all"]
                notes_link = [l for l in date_time["links"] if l["type"] == "notes"]
                # assert that if it's the 'all' vj there is no note
                assert(len(vj_link) == 0 or len(notes_link) == 0)


StopSchedule = namedtuple('StopSchedule', ['sp', 'route', 'date_times'])
SchedDT = namedtuple('SchedDT', ['dt', 'vj'])

Departure = namedtuple('Departure', ['sp', 'route', 'dt'])


def check_stop_schedule(response, reference):
    """
    check the values in a stopschedule
    check the id of the stoppoint, the id of the routes, the datetimes and the vj used

    Note: the stop_schedule are not sorted, we just must find all reference StopSchedule
    """
    eq_(len(response), len(reference))
    for resp in response:
        ref = next(r for r in reference
                   if r.sp == get_not_null(get_not_null(resp, 'stop_point'), 'id')
                   and r.route == get_not_null(get_not_null(resp, 'route'), 'id'))

        for (resp_dt, ref_st) in itertools.izip_longest(get_not_null(resp, 'date_times'), ref.date_times):
            eq_(get_not_null(resp_dt, 'date_time'), ref_st.dt)
            eq_(get_not_null(resp_dt, 'links')[0]['id'], ref_st.vj)


def check_departures(response, reference):
    """
    check the values in a departures
    """
    for (resp, ref) in itertools.izip_longest(response, reference):
        eq_(get_not_null(get_not_null(resp, 'stop_point'), 'id'), ref.sp)
        eq_(get_not_null(get_not_null(resp, 'route'), 'id'), ref.route)
        eq_(get_not_null(get_not_null(resp, 'stop_date_time'), 'departure_date_time'), ref.dt)


@dataset(["basic_schedule_test"])
class TestSchedules(AbstractTestFixture):

    def test_classic_stop_schedule(self):
        """default is Realtime stopschedule"""
        response = self.query_region("stop_points/S1/stop_schedules?from_datetime=20160101T080000")

        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

    def test_stop_schedule_base_schedule(self):
        response = self.query_region("stop_points/S1/stop_schedules?from_datetime=20160101T080000"
                                     "&data_freshness=base_schedule")

        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        check_stop_schedule(stop_sched,
                            [StopSchedule(sp='S1', route='A:0',
                                          date_times=[SchedDT(dt='20160101T090000', vj='A:vj1'),
                                                      SchedDT(dt='20160101T100000', vj='A:vj2'),
                                                      SchedDT(dt='20160101T110000', vj='A:vj3')]),
                             StopSchedule(sp='S1', route='B:1',
                                          date_times=[SchedDT(dt='20160101T113000', vj='B:vj1')])])

    @staticmethod
    def check_stop_schedule_rt_sol(stop_sched):
        check_stop_schedule(stop_sched,
                            [StopSchedule(sp='S1', route='A:0',
                                          date_times=[SchedDT(dt='20160101T090700',
                                                              vj='A:vj1:modified:0:delay_vj1'),
                                                      SchedDT(dt='20160101T100700',
                                                              vj='A:vj2:modified:0:delay_vj2'),
                                                      SchedDT(dt='20160101T110700',
                                                              vj='A:vj3:modified:0:delay_vj3')]),
                             StopSchedule(sp='S1', route='B:1',
                                          date_times=[SchedDT(dt='20160101T113000', vj='B:vj1')])])

    def test_stop_schedule_realtime(self):
        """
        test a stop schedule without a from_datetime parameter

        we should have the current datetime used and the realtime activated

        Note: for test purpose we override the current_datetime
        """
        response = self.query_region("stop_points/S1/stop_schedules?from_datetime=20160101T080000"
                                     "&data_freshness=realtime")

        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

    def test_stop_schedule_no_dt(self):
        """
        test a stop schedule without a from_datetime parameter

        we should have the current datetime used and the realtime activated

        Note: for test purpose we override the current_datetime
        """
        response = self.query_region("stop_points/S1/stop_schedules?_current_datetime=20160101T080000")

        stop_sched = response["stop_schedules"]
        is_valid_stop_schedule(stop_sched, self.tester)

        self.check_stop_schedule_rt_sol(stop_sched)

    @staticmethod
    def check_departure_base_schedule_sol(departures):
        check_departures(departures, [Departure(sp='S1', route='A:0', dt='20160101T090000'),
                                      Departure(sp='S1', route='A:0', dt='20160101T100000'),
                                      Departure(sp='S1', route='A:0', dt='20160101T110000'),
                                      Departure(sp='S1', route='B:1', dt='20160101T113000'),
                                      ])

    @staticmethod
    def check_departure_rt_sol(departures):
        # RT activated: all A:0 vj are 7 mn late
        check_departures(departures, [Departure(sp='S1', route='A:0', dt='20160101T090700'),
                                      Departure(sp='S1', route='A:0', dt='20160101T100700'),
                                      Departure(sp='S1', route='A:0', dt='20160101T110700'),
                                      Departure(sp='S1', route='B:1', dt='20160101T113000'),
                                      ])

    def test_departure(self):
        """
        test a departure without nothing

        we should have the current datetime used and the realtime activated
        """
        response = self.query_region("stop_points/S1/departures?_current_datetime=20160101T080000")

        departures = response["departures"]
        is_valid_departures(departures)
        self.check_departure_rt_sol(departures)

    def test_departure_base_sched(self):
        """
        test a departure, no date time and base schedule, we should get base schedule
        """
        response = self.query_region("stop_points/S1/departures?_current_datetime=20160101T080000"
                                     "&data_freshness=base_schedule")

        departures = response["departures"]
        is_valid_departures(departures)
        self.check_departure_base_schedule_sol(departures)

    def test_departure_dt(self):
        """
        test a departure, with a date time

        we should have the wanted datetime used and the realtime activated
        """
        response = self.query_region("stop_points/S1/departures?from_datetime=20160101T080000")

        departures = response["departures"]
        is_valid_departures(departures)
        self.check_departure_rt_sol(departures)

    def test_departure_dt_base_schedule(self):
        """
        test a departure, with a date time

        we should have the wanted datetime used and the realtime activated
        """
        response = self.query_region("stop_points/S1/departures?from_datetime=20160101T080000"
                                     "&data_freshness=base_schedule")

        departures = response["departures"]
        is_valid_departures(departures)
        self.check_departure_base_schedule_sol(departures)
