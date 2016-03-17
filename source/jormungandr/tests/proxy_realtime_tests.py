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

from __future__ import absolute_import, print_function, unicode_literals, division
from collections import namedtuple

from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr.realtime_schedule import realtime_proxy, realtime_proxy_manager
from jormungandr.schedule import RealTimePassage
import datetime
from nose.tools import eq_
import pytz
from .check_utils import is_valid_stop_date_time, get_not_null


MOCKED_PROXY_CONF = (' [{"id": "KisioDigital",\n'
                     ' "class": "tests.proxy_realtime_tests.MockedTestProxy",\n'
                     ' "args": { } }]')


class MockedTestProxy(realtime_proxy.RealtimeProxy):
    def __init__(self, id):
        self.service_id = id

    @staticmethod
    def _create_next_passages(passages):
        next_passages = []
        for next_expected_st, direction in passages:
            t = datetime.datetime.strptime(next_expected_st, "%H:%M:%S")
            dt = datetime.datetime(year=2016, month=1, day=2,
                                   hour=t.hour, minute=t.minute, second=t.second,
                                   tzinfo=pytz.UTC)
            next_passage = RealTimePassage(dt, direction)
            next_passages.append(next_passage)
        return next_passages

    def next_passage_for_route_point(self, route_point):
        if route_point.fetch_stop_id(self.service_id) == "KisioDigital_C:S1":
            return []

        if route_point.fetch_stop_id(self.service_id) == "KisioDigital_C:S0":
            return self._create_next_passages([("11:32:42", "l'infini"), ("11:42:42", "l'au dela")])

        if route_point.pb_stop_point.uri == "S42":
            if route_point.pb_route.name == "J":
                return self._create_next_passages([("10:00:00", None), ("10:03:00", None)])
            if route_point.pb_route.name == "K":
                return self._create_next_passages([("10:01:00", "bob"), ("10:04:00", None)])

        return None


DepartureCheck = namedtuple('DepartureCheck', ['route', 'dt', 'data_freshness', 'direction', 'physical_mode'])


@dataset({"basic_schedule_test": {"proxy_conf": MOCKED_PROXY_CONF}})
class TestDepartures(AbstractTestFixture):

    query_template = 'stop_points/{sp}/stop_schedules?from_datetime={dt}&show_codes=true{data_freshness}'

    def test_stop_schedule(self):
        query = self.query_template.format(sp='C:S0', dt='20160102T1100', data_freshness='')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert ["20160102T113242", "20160102T114242"] == next_passage_dts

        for dt in stop_schedules:
            assert dt['data_freshness'] == 'realtime'

        notes = {n['id']: n for n in response.get('notes', [])}

        directions = [notes[l['id']]['value'] for dt in stop_schedules for l in dt['links'] if l['rel'] ==
                      'notes']

        assert directions == ["l'infini", "l'au dela"]

    def test_stop_schedule_base(self):
        query = self.query_template.format(sp='C:S0', dt='20160102T1100',
                                           data_freshness='&data_freshness=base_schedule')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert "20160102T113000" in next_passage_dts

        for dt in stop_schedules:
            assert dt['data_freshness'] == 'base_schedule'

    def test_empty_stop_schedule(self):
        query = self.query_template.format(sp='C:S1', dt='20160102T1100',
                                           data_freshness='')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules'][0]['date_times']
        next_passage_dts = [dt["date_time"] for dt in stop_schedules]
        assert len(next_passage_dts) == 0

        for dt in stop_schedules:
            assert dt['data_freshness'] == 'base_schedule'

    def test_stop_schedule_stop_point_has_no_rt(self):
        query = self.query_template.format(sp='S1', dt='20160102T1030',
                                           data_freshness='')
        response = self.query_region(query)

        stop_schedule_A = response['stop_schedules'][0]
        assert stop_schedule_A['route']['name'] == 'A'
        next_passage_dts = [dt["date_time"] for dt in stop_schedule_A['date_times']]
        assert ['20160102T110000', '20160103T090000', '20160103T100000'] == next_passage_dts

        for dt in stop_schedule_A['date_times']:
            assert dt['data_freshness'] == 'base_schedule'

        stop_schedule_B = response['stop_schedules'][1]
        assert stop_schedule_B['route']['name'] == 'B'
        next_passage_dts = [dt["date_time"] for dt in stop_schedule_B['date_times']]
        assert ['20160102T113000'] == next_passage_dts

        for dt in stop_schedule_B['date_times']:
            assert dt['data_freshness'] == 'base_schedule'

    def test_departures_realtime_informations(self):
        query = 'stop_areas/S42/departures?from_datetime=20160102T1000&show_codes=true&count=7'
        response = self.query_region(query)

        assert "departures" in response
        assert len(response["departures"]) == 7

        departures = [DepartureCheck(route=d['route']['name'],
                                     dt=d['stop_date_time']['departure_date_time'],
                                     data_freshness=d['stop_date_time']['data_freshness'],
                                     direction=d['display_informations']['direction'],
                                     physical_mode=d['display_informations']['physical_mode'])
                      for d in response['departures']]
        expected_departures = [
            DepartureCheck(route="J", dt="20160102T100000", data_freshness="realtime",
                           direction='', physical_mode='name physical_mode:0'),
            # rt we got the given direction:
            DepartureCheck(route="K", dt="20160102T100100", data_freshness="realtime",
                           direction='bob', physical_mode='name physical_mode:0'),
            DepartureCheck(route="L", dt="20160102T100200", data_freshness="base_schedule",
                           direction='S43', physical_mode='name physical_mode:0'),
            DepartureCheck(route="J", dt="20160102T100300", data_freshness="realtime",
                           direction='', physical_mode='name physical_mode:0'),
            # rt but no direction:
            DepartureCheck(route="K", dt="20160102T100400", data_freshness="realtime",
                           direction='', physical_mode='name physical_mode:0'),
            DepartureCheck(route="L", dt="20160102T100700", data_freshness="base_schedule",
                           direction='S43', physical_mode='name physical_mode:0'),
            DepartureCheck(route="L", dt="20160102T101100", data_freshness="base_schedule",
                           direction='S43', physical_mode='name physical_mode:0'),
        ]
        eq_(departures, expected_departures)

        #stop_time with data_freshness = realtime
        d = get_not_null(response["departures"][0], "stop_date_time")
        get_not_null(d, "arrival_date_time")
        get_not_null(d, "departure_date_time")
        assert "base_arrival_date_time" not in d
        assert "base_departure_date_time" not in d

        #For realtime some attributs in display_informations are deleted
        disp_info = get_not_null(response["departures"][0], "display_informations")
        assert len(disp_info["headsign"]) == 0

        #stop_time with data_freshness = base_schedule
        d = get_not_null(response["departures"][2], "stop_date_time")
        #verify that all the attributs are present
        is_valid_stop_date_time(d)

        #For base_schedule verify some attributs in display_informations
        d = get_not_null(response["departures"][2], "display_informations")
        get_not_null(d, "physical_mode")
        get_not_null(d, "headsign")
