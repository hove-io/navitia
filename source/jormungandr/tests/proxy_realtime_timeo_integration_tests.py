# coding=utf-8

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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import is_valid_notes
from mock import MagicMock
from jormungandr.realtime_schedule.timeo import Timeo


class Obj(object):
    pass


class MockTimeo(Timeo):
    def __init__(
        self,
        id,
        service_url,
        service_args,
        timezone,
        object_id_tag=None,
        destination_id_tag=None,
        instance=None,
        timeout=10,
    ):
        Timeo.__init__(
            self, id, service_url, service_args, timezone, object_id_tag, destination_id_tag, instance, timeout
        )

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
                                {"NextStop": "10:00:52", "Destination": "DIRECTION AA"},
                                {"NextStop": "10:13:52", "Destination": "DIRECTION AA"},
                            ],
                        },
                    }
                ],
                "MessageResponse": [{"ResponseCode": 0, "ResponseComment": "success"}],
            }
        if url == 'S42':
            json = {
                "CorrelationID": "AA",
                "StopTimesResponse": [
                    {
                        "StopID": "S42",
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
                                    "Terminus": "Kisio数字_C:S43",
                                },
                                {
                                    "NextStop": "10:13:52",
                                    "Destination": "DIRECTION AA",
                                    "Terminus": "Kisio数字_C:S43",
                                },
                            ],
                        },
                    }
                ],
                "MessageResponse": [{"ResponseCode": 0, "ResponseComment": "success"}],
            }
        if url == 'S39':
            json = {
                "CorrelationID": "AA",
                "StopTimesResponse": [
                    {
                        "StopID": "S39",
                        "StopTimeoCode": "AAAAA",
                        "StopLongName": "Malraux",
                        "StopShortName": "Malraux",
                        "StopVocalName": "Malraux",
                        "ReferenceTime": "08:54:53",
                        "NextStopTimesMessage": {
                            "LineID": "AAA",
                            "Way": "A",
                            "LineMainDirection": "DIRECTION AA",
                            "NextExpectedStopTime": [
                                {
                                    "NextStop": "09:00:52",
                                    "Destination": "DIRECTION AA",
                                    "Terminus": "Kisio数字_C:S43",
                                    "is_realtime": False,
                                },
                                {
                                    "NextStop": "09:13:52",
                                    "Destination": "DIRECTION AA",
                                    "Terminus": "Kisio数字_C:S43",
                                },
                            ],
                        },
                    }
                ],
                "MessageResponse": [{"ResponseCode": 0, "ResponseComment": "success"}],
            }
        if url == 'TS39':
            json = {
                "CorrelationID": "AA",
                "StopTimesResponse": [
                    {
                        "StopID": "TS39",
                        "StopTimeoCode": "AAAAA",
                        "StopLongName": "Malraux",
                        "StopShortName": "Malraux",
                        "StopVocalName": "Malraux",
                        "ReferenceTime": "08:54:53",
                        "NextStopTimesMessage": {
                            "LineID": "AAA",
                            "Way": "A",
                            "LineMainDirection": "DIRECTION AA",
                            "NextExpectedStopTime": [
                                {
                                    "NextStop": "09:00:52",
                                    "Destination": "DIRECTION AA",
                                    "Terminus": "Kisio数字_C:S43",
                                    "is_realtime": False,
                                },
                                {
                                    "NextStop": "09:13:52",
                                    "Destination": "DIRECTION AA",
                                    "Terminus": "Kisio数字_C:S43",
                                    "is_realtime": False,
                                },
                            ],
                        },
                    }
                ],
                "MessageResponse": [{"ResponseCode": 0, "ResponseComment": "success"}],
            }
        if url == 'C:S1':
            json = {"MessageResponse": [{"ResponseCode": 208, "ResponseComment": "no serviceID number found"}]}
        if url == 'S40':
            json = {
                "CorrelationID": "AA",
                "StopTimesResponse": [
                    {
                        "StopID": "S40",
                        "StopTimeoCode": "AAAAA",
                        "StopLongName": "Malraux",
                        "StopShortName": "Malraux",
                        "StopVocalName": "Malraux",
                        "ReferenceTime": "09:54:53",
                        "NextStopTimesMessage": {
                            "LineID": "AAA",
                            "Way": "A",
                            "LineMainDirection": "DIRECTION AA",
                        },
                    }
                ],
                "MessageResponse": [{"ResponseCode": 0, "ResponseComment": "success"}],
            }
        resp.json = MagicMock(return_value=json)
        return resp

    def _make_url(self, route_point, count=None, from_dt=None):
        return route_point.pb_stop_point.uri


MOCKED_PROXY_CONF = [
    {
        "object_id_tag": "Kisio数字",
        "id": "Kisio数字",
        "class": "tests.proxy_realtime_timeo_integration_tests.MockTimeo",
        "args": {
            "destination_id_tag": "Kisio数字",
            "timezone": "Europe/Paris",
            "service_url": "http://XXXX",
            "timeout": 15,
            "service_args": {"serviceID": "X", "EntityID": "XX", "Media": "XXX"},
        },
    }
]


@dataset({"basic_schedule_test": {'instance_config': {'realtime_proxies': MOCKED_PROXY_CONF}}})
class TestDepartures(AbstractTestFixture):
    query_template_scs = (
        'stop_points/{sp}/stop_schedules?from_datetime={dt}&show_codes=true{data_freshness}'
        '&_current_datetime={c_dt}'
    )
    query_template_ter = (
        'stop_points/{sp}/terminus_schedules?from_datetime={dt}&show_codes=true{data_freshness}'
        '&_current_datetime={c_dt}'
    )

    def test_stop_schedule_without_rt(self):
        query = self.query_template_scs.format(
            sp='C:S0', dt='20160102T0900', data_freshness='', c_dt='20160102T0900'
        )
        response = self.query_region(query)
        is_valid_notes(response["notes"])
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 1
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 1
        stop_time = stop_times[0]
        assert stop_time['data_freshness'] == 'base_schedule'
        assert stop_time['date_time'] == '20160102T113000'

    def test_stop_schedule_with_rt_in_error(self):
        """
        When timeo service responds a error code >= 100 (into MessageResponse.ResponseCode field),
        we return base schedule results
        """
        query = self.query_template_scs.format(
            sp='C:S1', dt='20160102T0900', data_freshness='', c_dt='20160102T0900'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 1
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 1
        stop_time = stop_times[0]
        assert stop_time['data_freshness'] == 'base_schedule'
        assert stop_time['date_time'] == '20160102T123000'

    def test_stop_schedule_with_rt_empty_list(self):
        """
        When timeo service responds a empty list, we return the empty rt list
        """
        query = self.query_template_scs.format(
            sp='S40', dt='20160102T0900', data_freshness='', c_dt='20160102T0900'
        )
        response = self.query_region(query)
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 1
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 0

    def test_stop_schedule_with_rt_and_without_destination(self):
        query = self.query_template_scs.format(
            sp='S41', dt='20160102T0900', data_freshness='', c_dt='20160102T0900'
        )
        response = self.query_region(query)
        is_valid_notes(response["notes"])
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 1
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 2
        assert stop_times[0]['data_freshness'] == 'realtime'
        assert stop_times[0]['date_time'] == '20160102T090052'
        links = stop_times[0]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:7a0967bbb281e0d1548d2d5bc6933a20'

        assert stop_times[1]['data_freshness'] == 'realtime'
        assert stop_times[1]['date_time'] == '20160102T091352'
        links = stop_times[1]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:7a0967bbb281e0d1548d2d5bc6933a20'

        notes = response['notes']
        assert len(notes) == 1
        assert notes[0]["type"] == "notes"
        assert notes[0]["value"] == "DIRECTION AA"
        assert notes[0]["id"] == "note:7a0967bbb281e0d1548d2d5bc6933a20"

    def test_stop_schedule_with_rt_and_with_destination(self):
        query = self.query_template_scs.format(
            sp='S42', dt='20160102T0900', data_freshness='', c_dt='20160102T0900'
        )

        response = self.query_region(query)
        is_valid_notes(response["notes"])
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 3
        stop_times = stop_schedules[0]['date_times']
        assert len(stop_times) == 2
        assert stop_times[0]['data_freshness'] == 'realtime'
        assert stop_times[0]['date_time'] == '20160102T090052'
        links = stop_times[0]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:b5b328cb593ae7b1d73228345fe634fc'

        assert stop_times[1]['data_freshness'] == 'realtime'
        assert stop_times[1]['date_time'] == '20160102T091352'
        links = stop_times[1]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:b5b328cb593ae7b1d73228345fe634fc'

        notes = response['notes']
        assert len(notes) == 1
        assert notes[0]["type"] == "notes"
        assert notes[0]["value"] == "Terminus (Quimper)"
        assert notes[0]["id"] == "note:b5b328cb593ae7b1d73228345fe634fc"

    def test_stop_schedule_with_from_datetime_tomorrow(self):
        query = self.query_template_scs.format(
            sp='S42', dt='20160102T0900', data_freshness='', c_dt='20160101T0900'
        )
        response = self.query_region(query)
        is_valid_notes(response["notes"])
        stop_schedules = response['stop_schedules']
        assert len(stop_schedules) == 3
        for stop_schedule in stop_schedules:
            assert len(stop_schedule['date_times']) > 0
            for stop_time in stop_schedule['date_times']:
                assert stop_time['data_freshness'] == 'base_schedule'

    def test_terminus_schedule_with_realtime(self):
        """
        Here we have realtime stop_times from proxy_realtime_timeo
        data_freshness default value is base_schedule for terminus_schedules
        """
        query = self.query_template_ter.format(
            sp='S42', dt='20160102T0900', data_freshness='&data_freshness=realtime', c_dt='20160102T0900'
        )

        response = self.query_region(query)
        is_valid_notes(response["notes"])
        terminus_schedules = response['terminus_schedules']
        assert len(terminus_schedules) == 3
        date_times = terminus_schedules[0]['date_times']
        assert len(date_times) == 2
        assert date_times[0]['data_freshness'] == 'realtime'
        assert date_times[0]['date_time'] == '20160102T090052'
        links = date_times[0]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:b5b328cb593ae7b1d73228345fe634fc'

        assert date_times[1]['data_freshness'] == 'realtime'
        assert date_times[1]['date_time'] == '20160102T091352'
        links = date_times[1]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:b5b328cb593ae7b1d73228345fe634fc'

    def test_terminus_schedule_with_realtime_and_is_realtime_field(self):

        query = self.query_template_ter.format(
            sp='S39', dt='20160102T0800', data_freshness='', c_dt='20160102T0800'
        )

        response = self.query_region(query)
        is_valid_notes(response["notes"])
        terminus_schedules = response['terminus_schedules']
        assert len(terminus_schedules) == 1
        date_times = terminus_schedules[0]['date_times']
        assert len(date_times) == 3
        assert date_times[0]['data_freshness'] == 'base_schedule'
        assert date_times[0]['date_time'] == '20160102T090200'

        assert date_times[1]['data_freshness'] == 'base_schedule'
        assert date_times[1]['date_time'] == '20160102T090700'

        assert date_times[2]['data_freshness'] == 'base_schedule'
        assert date_times[2]['date_time'] == '20160102T091100'

        query = self.query_template_ter.format(
            sp='S39', dt='20160102T0800', data_freshness='&data_freshness=realtime', c_dt='20160102T0800'
        )
        response = self.query_region(query)
        is_valid_notes(response["notes"])
        terminus_schedules = response['terminus_schedules']
        assert len(terminus_schedules) == 1
        date_times = terminus_schedules[0]['date_times']
        assert len(date_times) == 1
        assert date_times[0]['data_freshness'] == 'realtime'
        assert date_times[0]['date_time'] == '20160102T081352'
        links = date_times[0]["links"]
        assert len(links) == 1
        assert links[0]['id'] == 'note:b5b328cb593ae7b1d73228345fe634fc'

    def test_terminus_schedule_with_realtime_and_is_realtime_field_all_false(self):
        for data_freshness in ['&data_freshness=base_schedule', '', '&data_freshness=realtime']:
            query = self.query_template_ter.format(
                sp='TS39', dt='20160102T0800', data_freshness=data_freshness, c_dt='20160102T0800'
            )

            response = self.query_region(query)
            is_valid_notes(response["notes"])
            terminus_schedules = response['terminus_schedules']
            assert len(terminus_schedules) == 1
            date_times = terminus_schedules[0]['date_times']
            assert len(date_times) == 3
            assert date_times[0]['data_freshness'] == 'base_schedule'
            assert date_times[0]['date_time'] == '20160102T090200'

            assert date_times[1]['data_freshness'] == 'base_schedule'
            assert date_times[1]['date_time'] == '20160102T090700'

            assert date_times[2]['data_freshness'] == 'base_schedule'
            assert date_times[2]['date_time'] == '20160102T091100'

    def test_stop_schedule_with_realtime_and_is_realtime_field_all_false(self):
        for data_freshness in ['&data_freshness=base_schedule', '', '&data_freshness=realtime']:
            query = self.query_template_scs.format(
                sp='TS39', dt='20160102T0800', data_freshness=data_freshness, c_dt='20160102T0800'
            )

            response = self.query_region(query)
            is_valid_notes(response["notes"])
            terminus_schedules = response['stop_schedules']
            assert len(terminus_schedules) == 1
            date_times = terminus_schedules[0]['date_times']
            assert len(date_times) == 3
            assert date_times[0]['data_freshness'] == 'base_schedule'
            assert date_times[0]['date_time'] == '20160102T090200'

            assert date_times[1]['data_freshness'] == 'base_schedule'
            assert date_times[1]['date_time'] == '20160102T090700'

            assert date_times[2]['data_freshness'] == 'base_schedule'
            assert date_times[2]['date_time'] == '20160102T091100'

    def test_terminus_schedule_without_realtime(self):
        """
        Here we have theoretical stop_times only as C:S0 is absent in proxy_realtime_timeo
        """
        query = self.query_template_ter.format(
            sp='C:S0', dt='20160102T0900', data_freshness='&data_freshness=realtime', c_dt='20160102T0900'
        )
        response = self.query_region(query)
        is_valid_notes(response["notes"])
        terminus_schedules = response['terminus_schedules']
        assert len(terminus_schedules) == 1
        date_times = terminus_schedules[0]['date_times']
        assert len(date_times) == 1
        stop_time = date_times[0]
        assert stop_time['data_freshness'] == 'base_schedule'
        assert stop_time['date_time'] == '20160102T113000'
