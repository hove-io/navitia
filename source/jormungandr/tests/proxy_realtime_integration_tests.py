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
from mock import MagicMock
from navitiacommon import type_pb2
from jormungandr.realtime_schedule.timeo import Timeo


class Obj(object):
    pass


class MockTimeo(Timeo):

    def __init__(self, id, service_url, service_args, timezone,
                 object_id_tag=None, destination_id_tag=None, instance=None, timeout=10):
        Timeo.__init__(self, id, service_url, service_args, timezone,
                       object_id_tag, destination_id_tag, instance, timeout)

    def _call_timeo(self, url):
        resp = Obj()
        resp.status_code = 200
        json = {}
        if url == 'S41':
            json = {
                "CorrelationID": "AA",
                "StopTimesResponse": [
                    {
                        "StopID": "S41",
                        "StopTimeoCode": "AAAAA",
                        "StopLongName": "Malraux",
                        "StopShortName": "Malraux",
                        "StopVocalName": "Malraux",
                        "ReferenceTime": "09:54:53",
                        "NextStopTimesMessage": {
                            "LineID": "AAA",
                            "Way": "A",
                            "LineMainDirection": "DIRECTION AA",
                            "NextExpectedStopTime": [
                                {
                                    "NextStop": "10:00:52",
                                    "Destination": "DIRECTION AA"
                                },
                                {
                                    "NextStop": "10:13:52",
                                    "Destination": "DIRECTION AA"
                                }
                            ]
                        }
                    }
                ]
            }
        if url == 'S42':
            json = {
                "CorrelationID": "AA",
                "StopTimesResponse": [
                    {
                        "StopID": "C:S0",
                        "StopTimeoCode": "AAAAA",
                        "StopLongName": "Malraux",
                        "StopShortName": "Malraux",
                        "StopVocalName": "Malraux",
                        "ReferenceTime": "09:54:53",
                        "NextStopTimesMessage": {
                            "LineID": "AAA",
                            "Way": "A",
                            "LineMainDirection": "DIRECTION AA",
                            "NextExpectedStopTime": [
                                {
                                    "NextStop": "10:00:52",
                                    "Destination": "DIRECTION AA",
                                    "Terminus": "KisioDigital_C:S43"

                                },
                                {
                                    "NextStop": "10:13:52",
                                    "Destination": "DIRECTION AA",
                                    "Terminus": "KisioDigital_C:S43"
                                }
                            ]
                        }
                    }
                ]
            }
        resp.json = MagicMock(return_value=json)
        return resp

    def _make_url(self, route_point, count=None, from_dt=None):
        return route_point.pb_stop_point.uri


MOCKED_PROXY_CONF = ('[{ "object_id_tag": "KisioDigital",'
                     '"id": "KisioDigital",'
                     '"class": "tests.proxy_realtime_integration_tests.MockTimeo",'
                     '"args": {'
                     '"destination_id_tag": "KisioDigital",'
                     '"timezone": "Europe/Paris",'
                     '"service_url": "http://XXXX",'
                     '"timeout": 15,'
                     '"service_args": {'
                     '"serviceID": "X",'
                     '"EntityID": "XX",'
                     '"Media": "XXX"}}}]')

@dataset({"basic_schedule_test": {"proxy_conf": MOCKED_PROXY_CONF}})
class TestDepartures(AbstractTestFixture):
    query_template = 'stop_points/{sp}/stop_schedules?from_datetime={dt}&show_codes=true{data_freshness}' \
                     '&_current_datetime={c_dt}'

    def test_stop_schedule_without_rt(self):
        query = self.query_template.format(sp='C:S0', dt='20160102T1100', data_freshness='', c_dt='20160102T1100')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 1
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 1
        stop_time = stop_times[0]
        assert stop_time['data_freshness'] == 'base_schedule'
        assert stop_time['date_time'][8:] == 'T113000'

    def test_stop_schedule_with_rt_and_without_destination(self):
        query = self.query_template.format(sp='S41', dt='20160102T1100', data_freshness='', c_dt='20160102T1100')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 1
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 2
        assert stop_times[0]['data_freshness'] == 'realtime'
        assert stop_times[0]['date_time'][8:] == 'T080052'
        links = stop_times[0]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:7a0967bbb281e0d1548d2d5bc6933a20'

        assert stop_times[1]['data_freshness'] == 'realtime'
        assert stop_times[1]['date_time'][8:] == 'T081352'
        links = stop_times[1]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:7a0967bbb281e0d1548d2d5bc6933a20'

        notes = response['notes']
        assert len(notes) == 1
        assert notes[0]["type"] == "notes"
        assert notes[0]["value"] == "DIRECTION AA"
        assert notes[0]["id"] == "note:7a0967bbb281e0d1548d2d5bc6933a20"

    def test_stop_schedule_with_rt_and_with_destination(self):
        query = self.query_template.format(sp='S42', dt='20160102T1100', data_freshness='', c_dt='20160102T1100')

        response = self.query_region(query)
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 3
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 2
        assert stop_times[0]['data_freshness'] == 'realtime'
        assert stop_times[0]['date_time'][8:] == 'T080052'
        links = stop_times[0]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:b5b328cb593ae7b1d73228345fe634fc'

        assert stop_times[1]['data_freshness'] == 'realtime'
        assert stop_times[1]['date_time'][8:] == 'T081352'
        links = stop_times[1]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:b5b328cb593ae7b1d73228345fe634fc'

        notes = response['notes']
        assert len(notes) == 1
        assert notes[0]["type"] == "notes"
        assert notes[0]["value"] == "Terminus (Quimper)"
        assert notes[0]["id"] == "note:b5b328cb593ae7b1d73228345fe634fc"

    def test_stop_schedule_with_from_datetime_tomorrow(self):
        query = self.query_template.format(sp='S42', dt='20160102T1100', data_freshness='', c_dt='20160101T1100')
        response = self.query_region(query)
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 3
        for stop_schedule in stop_schedules:
            assert len(stop_schedule['date_times']) > 0
            for stop_time in stop_schedule['date_times']:
                assert stop_time['data_freshness'] == 'base_schedule'
