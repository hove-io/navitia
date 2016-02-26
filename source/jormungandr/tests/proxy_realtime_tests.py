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
from jormungandr.realtime_schedule import realtime_proxy, realtime_proxy_manager
from jormungandr.schedule import RealTimePassage
import datetime
import pytz

MOCKED_PROXY_CONF = (' [{"id": "KisioDigital",\n'
                     ' "class": "tests.proxy_realtime_tests.MockedTestProxy",\n'
                     ' "args": { } }]')


class MockedTestProxy(realtime_proxy.RealtimeProxy):
    def __init__(self, id):
        self.service_id = id

    def next_passage_for_route_point(self, route_point):
        next_passages = []
        if route_point.fetch_stop_id(self.service_id) == "KisioDigital_C:S1":
            return next_passages

        if route_point.fetch_stop_id(self.service_id) == "KisioDigital_C:S0":
            for next_expected_st in ["11:32:42", "11:42:42"]:
                t = datetime.datetime.strptime(next_expected_st, "%H:%M:%S")
                dt = datetime.datetime(year=2016, month=1, day=2,
                                       hour=t.hour, minute=t.minute, second=t.second,
                                       tzinfo=pytz.UTC)
                next_passage = RealTimePassage(dt)
                next_passages.append(next_passage)
            return next_passages
        return None

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

    def test_stop_schedule_stop_ponit_has_no_rt(self):
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

