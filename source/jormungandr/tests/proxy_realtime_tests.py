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
from collections import namedtuple

from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr.realtime_schedule import realtime_proxy
from jormungandr.schedule import RealTimePassage
import datetime
import pytz
from .check_utils import is_valid_stop_date_time, get_not_null


MOCKED_PROXY_CONF = [{"id": "Kisio数字", "class": "tests.proxy_realtime_tests.MockedTestProxy", "args": {}}]


class MockedTestProxy(realtime_proxy.RealtimeProxy):
    def __init__(self, id, object_id_tag, instance):
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.instance = instance

    @staticmethod
    def _create_next_passages(passages, year=2016, month=1, day=2, tzinfo=pytz.UTC):
        next_passages = []
        for next_expected_st, direction in passages:
            t = datetime.datetime.strptime(next_expected_st, "%H:%M:%S")
            dt = datetime.datetime(
                year=year, month=month, day=day, hour=t.hour, minute=t.minute, second=t.second, tzinfo=tzinfo
            )
            next_passage = RealTimePassage(dt, direction)
            next_passages.append(next_passage)
        return next_passages

    def _get_next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=None
    ):
        if route_point.fetch_stop_id(self.object_id_tag) == "Kisio数字_C:S1":
            return []

        if route_point.fetch_stop_id(self.object_id_tag) == "AnotherSource_C:S1":
            return self._create_next_passages([("10:42:42", "l'infini"), ("11:42:42", "l'au dela")])

        if route_point.fetch_stop_id(self.object_id_tag) == "Kisio数字_C:S0":
            return self._create_next_passages([("11:32:42", "l'infini"), ("11:42:42", "l'au dela")])

        if route_point.pb_stop_point.uri == "S42":
            if route_point.pb_route.name == "J":
                return self._create_next_passages([("10:00:00", None), ("10:03:00", None)])
            if route_point.pb_route.name == "K":
                return self._create_next_passages([("10:01:00", "bob"), ("10:04:00", None)])

        return None

    def status(self):
        return None


DepartureCheck = namedtuple('DepartureCheck', ['route', 'dt', 'data_freshness', 'direction', 'physical_mode'])


@dataset({"basic_schedule_test": {'instance_config': {'realtime_proxies': MOCKED_PROXY_CONF}}})
class TestDepartures(AbstractTestFixture):

    query_template_scs = 'stop_points/{sp}/stop_schedules?from_datetime={dt}{data_freshness}'
    query_template_ter = 'stop_points/{sp}/terminus_schedules?from_datetime={dt}{data_freshness}'

    def test_stop_schedule(self):
        query = self.query_template_scs.format(sp='C:S0', dt='20160102T1100', data_freshness='')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert ["20160102T113242", "20160102T114242"] == next_passage_dts

        for dt in stop_schedules:
            assert dt['data_freshness'] == 'realtime'

        notes = {n['id']: n for n in response.get('notes', [])}

        directions = [
            notes[l['id']]['value'] for dt in stop_schedules for l in dt['links'] if l['rel'] == 'notes'
        ]

        assert directions == ["l'infini", "l'au dela"]

    def test_stop_schedule_base(self):
        query = self.query_template_scs.format(
            sp='C:S0', dt='20160102T1100', data_freshness='&data_freshness=base_schedule'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert "20160102T113000" in next_passage_dts

        for dt in stop_schedules:
            assert dt['data_freshness'] == 'base_schedule'

    def test_empty_stop_schedule(self):
        query = self.query_template_scs.format(sp='C:S1', dt='20160102T1100', data_freshness='')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert len(next_passage_dts) == 0

        for dt in stop_schedules:
            assert dt['data_freshness'] == 'base_schedule'

    def test_stop_schedule_stop_point_has_no_rt(self):
        query = self.query_template_scs.format(sp='S1', dt='20160102T1030', data_freshness='')
        response = self.query_region(query)

        stop_schedule_A = response['stop_schedules'][0]
        assert stop_schedule_A['route']['name'] == 'A:0'
        next_passage_dts = [dt["date_time"] for dt in stop_schedule_A['date_times']]
        next_passage_base_dts = [dt["base_date_time"] for dt in stop_schedule_A['date_times']]
        values = ['20160102T110000', '20160103T090000', '20160103T100000']
        assert values == next_passage_dts
        assert values == next_passage_base_dts

        for dt in stop_schedule_A['date_times']:
            assert dt['data_freshness'] == 'base_schedule'

        stop_schedule_B = response['stop_schedules'][1]
        assert stop_schedule_B['route']['name'] == 'B:1'
        next_passage_dts = [dt["date_time"] for dt in stop_schedule_B['date_times']]
        assert ['20160102T113000'] == next_passage_dts

        for dt in stop_schedule_B['date_times']:
            assert dt['data_freshness'] == 'base_schedule'

    def test_terminus_schedules_realtime(self):
        """
        Here we have two realtime date_times from a realtime proxy
        data_freshness default value is base_schedule for terminus_schedules
        """
        query = self.query_template_ter.format(
            sp='C:S0', dt='20160102T1100', data_freshness='&data_freshness=realtime'
        )
        response = self.query_region(query)
        date_times = response['terminus_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in date_times]
        assert ["20160102T113242", "20160102T114242"] == next_passage_dts

        for dt in date_times:
            assert dt['data_freshness'] == 'realtime'

        notes = {n['id']: n for n in response.get('notes', [])}

        directions = [notes[l['id']]['value'] for dt in date_times for l in dt['links'] if l['rel'] == 'notes']

        assert directions == ["l'infini", "l'au dela"]

    def test_terminus_schedules_base(self):
        query = self.query_template_ter.format(
            sp='C:S0', dt='20160102T1100', data_freshness='&data_freshness=base_schedule'
        )
        response = self.query_region(query)
        date_times = response['terminus_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in date_times]
        assert "20160102T113000" in next_passage_dts

        for dt in date_times:
            assert dt['data_freshness'] == 'base_schedule'

    def test_departures_realtime_informations(self):
        query = 'stop_areas/S42/departures?from_datetime=20160102T1000&count=7&_current_datetime=20160102T1000'
        response = self.query_region(query)

        assert "departures" in response
        assert len(response["departures"]) == 7

        departures = [
            DepartureCheck(
                route=d['route']['name'],
                dt=d['stop_date_time']['departure_date_time'],
                data_freshness=d['stop_date_time']['data_freshness'],
                direction=d['display_informations']['direction'],
                physical_mode=d['display_informations']['physical_mode'],
            )
            for d in response['departures']
        ]
        expected_departures = [
            DepartureCheck(
                route="J",
                dt="20160102T100000",
                data_freshness="realtime",
                direction='',
                physical_mode='name physical_mode:0',
            ),
            # rt we got the given direction:
            DepartureCheck(
                route="K",
                dt="20160102T100100",
                data_freshness="realtime",
                direction='bob',
                physical_mode='name physical_mode:0',
            ),
            DepartureCheck(
                route="L",
                dt="20160102T100200",
                data_freshness="base_schedule",
                direction='Terminus (Quimper)',
                physical_mode='name physical_mode:0',
            ),
            DepartureCheck(
                route="J",
                dt="20160102T100300",
                data_freshness="realtime",
                direction='',
                physical_mode='name physical_mode:0',
            ),
            # rt but no direction:
            DepartureCheck(
                route="K",
                dt="20160102T100400",
                data_freshness="realtime",
                direction='',
                physical_mode='name physical_mode:0',
            ),
            DepartureCheck(
                route="L",
                dt="20160102T100700",
                data_freshness="base_schedule",
                direction='Terminus (Quimper)',
                physical_mode='name physical_mode:0',
            ),
            DepartureCheck(
                route="L",
                dt="20160102T101100",
                data_freshness="base_schedule",
                direction='Terminus (Quimper)',
                physical_mode='name physical_mode:0',
            ),
        ]
        assert departures == expected_departures

        # stop_time with data_freshness = realtime
        d = get_not_null(response["departures"][0], "stop_date_time")
        get_not_null(d, "arrival_date_time")
        get_not_null(d, "departure_date_time")
        assert "base_arrival_date_time" not in d
        assert "base_departure_date_time" not in d

        # For realtime some attributs in display_informations are deleted
        disp_info = get_not_null(response["departures"][0], "display_informations")
        assert len(disp_info["headsign"]) == 0

        # stop_time with data_freshness = base_schedule
        d = get_not_null(response["departures"][2], "stop_date_time")
        # verify that all the attributs are present
        is_valid_stop_date_time(d)

        # For base_schedule verify some attributs in display_informations
        d = get_not_null(response["departures"][2], "display_informations")
        get_not_null(d, "physical_mode")
        get_not_null(d, "headsign")
        get_not_null(d, "name")

    def test_departures_pagination(self):
        """
        We create a set of 1 base_schedule departure and
        2 realtime departures to highlight pagination's counters.
        """

        # 1. Test with realtime and base_schedule
        # expected output, 2 realtime ->
        # "pagination" { "items_on_page": 2, "items_per_page": 100,
        #                "start_page": 0,"total_result": 2}
        query = 'stop_areas/S42/lines/J/departures?from_datetime=20160102T1000&count=100'
        response = self.query_region(query)
        assert "departures" in response
        assert len(response["departures"]) == 2
        for departures in response["departures"]:
            assert departures["stop_date_time"]["data_freshness"] == "realtime"
        assert response["pagination"]["items_on_page"] == 2
        assert response["pagination"]["items_per_page"] == 100
        assert response["pagination"]["start_page"] == 0
        assert response["pagination"]["total_result"] == 2

        # 2. Test with realtime
        # expected output, 2 realtime ->
        # "pagination" { "items_on_page": 2, "items_per_page": 100,
        #                "start_page": 0,"total_result": 2}
        query = (
            'stop_areas/S42/lines/J/departures?'
            'from_datetime=20160102T1000&count=100'
            '&data_freshness=realtime'
        )
        response = self.query_region(query)
        assert "departures" in response
        assert len(response["departures"]) == 2
        for departures in response["departures"]:
            assert departures["stop_date_time"]["data_freshness"] == "realtime"
        assert response["pagination"]["items_on_page"] == 2
        assert response["pagination"]["items_per_page"] == 100
        assert response["pagination"]["start_page"] == 0
        assert response["pagination"]["total_result"] == 2

        # 3. Test with base_schedule
        # expected output, 1 base_schedule ->
        # "pagination" { "items_on_page": 1, "items_per_page": 100,
        #                "start_page": 0,"total_result": 1}
        query = (
            'stop_areas/S42/lines/J/departures?'
            'from_datetime=20160102T1000&count=100'
            '&data_freshness=base_schedule'
        )
        response = self.query_region(query)
        assert "departures" in response
        assert len(response["departures"]) == 1
        for departures in response["departures"]:
            assert departures["stop_date_time"]["data_freshness"] == "base_schedule"
        assert response["pagination"]["items_on_page"] == 1
        assert response["pagination"]["items_per_page"] == 100
        assert response["pagination"]["start_page"] == 0
        assert response["pagination"]["total_result"] == 1

    def test_stop_schedule_limit_per_items(self):
        """
        test the limit of item per stop_schedule with a realtime proxy
        """
        query = (
            self.query_template_scs.format(sp='C:S0', dt='20160102T1100', data_freshness='')
            + '&items_per_schedule=1'
        )
        response = self.query_region(query)

        # no base_date_time with rt proxy
        assert not [
            dt
            for ss in response['stop_schedules']
            for dt in ss['date_times']
            if dt.get('base_date_time') is not None
        ]

        for stop_sched in response['stop_schedules']:
            assert len(stop_sched['date_times']) == 1

        # same with a big limit, we get only 2 items (because there are only 2)
        query = (
            self.query_template_scs.format(sp='C:S0', dt='20160102T1100', data_freshness='')
            + '&items_per_schedule=42'
        )
        response = self.query_region(query)
        for stop_sched in response['stop_schedules']:
            assert len(stop_sched['date_times']) == 2

    def test_departures_count(self):
        """
        test that count parameter is correctly taken into account when using realtime proxies
        We search for departures on C:vj1: C:S0 -> 11:30, C:S1 -> 12:30, C:S2 -> 13:30
        So we got departures for the route points of the line C to the stops C:S0, C:S1, C:S2
        So there are two departures:
        Base_schedule:
          - C:S0 -> 11:30
          - C:S1 -> 12:30

        In realtime C:S1 has no departure, but C:S0 got another one, so still two departures:
          - C:S0 -> 11:32:42
          - C:S0 -> 11:42:42

        When we set count to 1 we want only one departure (the first one)
        """
        query = 'vehicle_journeys/vehicle_journey:C:vj1/departures?from_datetime=20160102T1000&count=100&_current_datetime=20160102T1000'
        response = self.query_region(query, display=True)

        assert "departures" in response
        assert len(response["departures"]) == 2

        query = 'vehicle_journeys/vehicle_journey:C:vj1/departures?from_datetime=20160102T1000&count=1&_current_datetime=20160102T1000'
        response_count = self.query_region(query)

        assert "departures" in response_count
        assert len(response_count["departures"]) == 1
        assert response_count["departures"] == response["departures"][:1]


MOCKED_PROXY_CONF = [
    {
        "id": "Kisio数字",
        "object_id_tag": "AnotherSource",
        "class": "tests.proxy_realtime_tests.MockedTestProxy",
        "args": {},
    }
]


@dataset({"basic_schedule_test": {'instance_config': {'realtime_proxies': MOCKED_PROXY_CONF}}})
class TestDeparturesWithAnotherSource(AbstractTestFixture):

    query_template = 'stop_points/{sp}/stop_schedules?from_datetime={dt}{data_freshness}'

    def test_departure_with_another_source(self):
        query = self.query_template.format(sp='C:S1', dt='20160102T1000', data_freshness='')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert ['20160102T104242', '20160102T114242'] == next_passage_dts

        for dt in stop_schedules:
            assert dt['data_freshness'] == 'realtime'


MOCKED_PROXY_WITH_TIMEZONE_CONF = [
    {"id": "Kisio数字", "class": "tests.proxy_realtime_tests.MockedTestProxyWithTimezone", "args": {}}
]


class MockedTestProxyWithTimezone(MockedTestProxy):
    def __init__(self, id, object_id_tag, instance):
        super(MockedTestProxyWithTimezone, self).__init__(id, object_id_tag, instance)

    def _get_next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=None
    ):

        if route_point.fetch_stop_id(self.object_id_tag) == "Kisio数字_C:S0":
            return self._create_next_passages(
                [("00:03:00", "l'infini"), ("00:04:00", "l'au dela")],
                year=2016,
                month=1,
                day=3,
                tzinfo=pytz.timezone("Europe/Paris"),
            )

        return None


@dataset({"basic_schedule_test": {'instance_config': {'realtime_proxies': MOCKED_PROXY_WITH_TIMEZONE_CONF}}})
class TestDeparturesWithTimeZone(AbstractTestFixture):

    query_template = 'stop_points/{sp}/stop_schedules?from_datetime={dt}{data_freshness}'

    def test_departure_with_timezone(self):
        query = self.query_template.format(
            sp='C:S0', dt='20160102T1100', data_freshness='&data_freshness=realtime'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert next_passage_dts


MOCKED_PROXY_WITH_CUSTOM_MERGING_CONF = [
    {"id": "Kisio数字", "class": "tests.proxy_realtime_tests.MockedTestProxyWithCustomMerging", "args": {}}
]


class MockedTestProxyWithCustomMerging(MockedTestProxy):
    def __init__(self, id, object_id_tag, instance):
        super(MockedTestProxyWithCustomMerging, self).__init__(id, object_id_tag, instance)

    def _get_proxy_time_bounds(self):
        start = datetime.datetime.strptime('20160103T040000', '%Y%m%dT%H%M%S')
        end = datetime.datetime.strptime('20160104T040000', '%Y%m%dT%H%M%S')
        return start, end

    # Make proxy active between 20160103T040000 and 20160104T040000
    # Keep everything outside this time bounds
    def _filter_base_stop_schedule(self, date_time):
        start, end = self._get_proxy_time_bounds()
        return start <= datetime.datetime.fromtimestamp(date_time.date + date_time.time) <= end

    def _get_next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=86400
    ):
        return self._create_next_passages(
            [("06:00:00", "l'infini"), ("10:00:00", "l'au dela")], year=2016, month=1, day=3
        )


@dataset(
    {"basic_schedule_test": {'instance_config': {'realtime_proxies': MOCKED_PROXY_WITH_CUSTOM_MERGING_CONF}}}
)
class TestPassagesWithMerging(AbstractTestFixture):
    def test_stop_schedule_with_merging(self):
        query_template = (
            'stop_points/{sp}/stop_schedules?from_datetime={dt}&duration={d}&data_freshness={data_freshness}'
        )
        # data from the proxy in the middle of the time frame
        query = query_template.format(
            sp='C:S0', dt='20160102T113000', d=3600 * 24 * 3, data_freshness='realtime'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']

        next_passages = [(dt["date_time"], dt["data_freshness"]) for dt in stop_schedules]
        expected_passages = [
            ('20160102T113000', 'base_schedule'),
            ('20160103T060000', 'realtime'),
            ('20160103T100000', 'realtime'),
            ('20160104T113000', 'base_schedule'),
            ('20160105T113000', 'base_schedule'),
        ]

        assert next_passages == expected_passages

        # Outside of proxy scopes, should keep all base_schedule datas
        query = query_template.format(sp='C:S0', dt='20160104T113000', d=3600 * 24, data_freshness='realtime')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']

        next_passages = [(dt["date_time"], dt["data_freshness"]) for dt in stop_schedules]
        expected_passages = [('20160104T113000', 'base_schedule'), ('20160105T113000', 'base_schedule')]

        assert next_passages == expected_passages

        # Time frame ends in the proxy time frame
        query = query_template.format(sp='C:S0', dt='20160102T080000', d=3600 * 24, data_freshness='realtime')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']

        next_passages = [(dt["date_time"], dt["data_freshness"]) for dt in stop_schedules]
        expected_passages = [('20160102T113000', 'base_schedule'), ('20160103T060000', 'realtime')]

        assert next_passages == expected_passages

        # Time frame starts in the proxy time frame
        query = query_template.format(
            sp='C:S0', dt='20160103T080000', d=3600 * 24 * 2, data_freshness='realtime'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']

        next_passages = [(dt["date_time"], dt["data_freshness"]) for dt in stop_schedules]
        expected_passages = [('20160103T100000', 'realtime'), ('20160104T113000', 'base_schedule')]

        assert next_passages == expected_passages

    # The departures filtering has not been overloaded
    def test_departures_without_merging(self):
        query_template = (
            'stop_points/{sp}/departures?from_datetime={dt}&duration={d}&data_freshness={data_freshness}'
        )

        # Check that we have indeed 4 departures
        query = query_template.format(
            sp='C:S0', dt='20160102T113000', d=3600 * 24 * 3, data_freshness='base_schedule'
        )
        response = self.query_region(query)

        assert "departures" in response
        assert len(response["departures"]) == 4

        departures = response['departures']
        next_passages = [
            (d['stop_date_time']['departure_date_time'], d['stop_date_time']['data_freshness'])
            for d in departures
        ]
        expected_passages = [
            ('20160102T113000', 'base_schedule'),
            ('20160103T113000', 'base_schedule'),
            ('20160104T113000', 'base_schedule'),
            ('20160105T113000', 'base_schedule'),
        ]

        assert next_passages == expected_passages

        # data from the proxy in the middle of the time frame
        # since there is no merging we have only realtime datas
        query = query_template.format(
            sp='C:S0', dt='20160102T113000', d=3600 * 24 * 3, data_freshness='realtime'
        )
        response = self.query_region(query)

        assert "departures" in response
        assert len(response["departures"]) == 2

        departures = response['departures']
        next_passages = [
            (d['stop_date_time']['departure_date_time'], d['stop_date_time']['data_freshness'])
            for d in departures
        ]
        expected_passages = [('20160103T060000', 'realtime'), ('20160103T100000', 'realtime')]

        assert next_passages == expected_passages
