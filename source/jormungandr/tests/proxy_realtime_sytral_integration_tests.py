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

import mock

from jormungandr.tests.utils_test import MockRequests
from tests.check_utils import get_not_null, get_departure, get_schedule
from .tests_mechanism import AbstractTestFixture, dataset

MOCKED_PROXY_CONF = [
    {
        "object_id_tag": "Kisio数字",
        "id": "Kisio数字",
        "class": "jormungandr.realtime_schedule.sytral.Sytral",
        "args": {
            "destination_id_tag": "Kisio数字",
            "timezone": "UTC",
            "service_url": "http://XXXX",
            "timeout": 15,
        },
    }
]


@dataset({'multiple_schedules': {'instance_config': {'realtime_proxies': MOCKED_PROXY_CONF}}})
class TestSytralSchedules(AbstractTestFixture):
    """
    integration tests for sytral

    """

    query_template_scs = (
        'stop_points/{sp}/stop_schedules?data_freshness=realtime&_current_datetime=20160102T0800'
    )
    query_template_dep = 'stop_points/{sp}/departures?data_freshness=realtime&_current_datetime=20160102T0800'
    query_template_ter = (
        'stop_points/{sp}/terminus_schedules?data_freshness=realtime&_current_datetime=20160102T0800'
    )

    def test_stop_schedule_with_realtime_only(self):
        mock_requests = MockRequests(
            {
                'http://XXXX?stop_id=syn_stoppoint1': (
                    {
                        "departures": [
                            {
                                "line": "Kisio数字 A",
                                "stop": "syn_stoppoint1",
                                "direction_id": "SP_2",
                                "direction_name": "Piscine Chambéry",
                                "datetime": "2016-01-02T10:17:17+02:00",
                                "type": "E",
                            },
                            {
                                "line": "Kisio数字 A",
                                "stop": "syn_stoppoint1",
                                "direction_id": "SP_2",
                                "direction_name": "Piscine Chambéry",
                                "type": "E",
                                "datetime": "2016-01-02T11:17:17+02:00",
                            },
                        ]
                    },
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template_scs.format(sp='SP_1')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            # 2016-01-02 08:17:17
            assert get_schedule(scs, 'SP_1', 'code A') == [
                {'rt': True, 'dt': '20160102T081717'},
                {'rt': True, 'dt': '20160102T091717'},
            ]

            query = self.query_template_dep.format(sp='SP_1')
            response = self.query_region(query)
            dep = get_not_null(response, 'departures')
            assert len(dep) == 2
            assert get_departure(dep, 'SP_1', 'code A') == [
                {'rt': True, 'dt': '20160102T081717'},
                {'rt': True, 'dt': '20160102T091717'},
            ]

            query = self.query_template_ter.format(sp='SP_1')
            response = self.query_region(query)
            ter = get_not_null(response, 'terminus_schedules')
            assert len(ter) == 1
            # 2016-01-02 08:17:17
            assert get_schedule(scs, 'SP_1', 'code A') == [
                {'rt': True, 'dt': '20160102T081717'},
                {'rt': True, 'dt': '20160102T091717'},
            ]

    def test_stop_schedule_with_theoric_and_realtime(self):
        mock_requests = MockRequests(
            {
                'http://XXXX?stop_id=syn_stoppoint1': (
                    {
                        "departures": [
                            {
                                "line": "Kisio数字 A",
                                "stop": "syn_stoppoint1",
                                "direction": "SA_2",
                                "direction_name": "Piscine Chambéry",
                                "datetime": "2016-01-02T09:17:17+01:00",
                                "type": "E",
                            },
                            {
                                "line": "Kisio数字 A",
                                "stop": "syn_stoppoint1",
                                "direction": "SA_2",
                                "direction_name": "Piscine Chambéry",
                                "datetime": "2016-01-02T10:17:17+01:00",
                                "type": "T",
                            },
                        ]
                    },
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template_scs.format(sp='SP_1')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            # 2016-01-02 08:17:17
            assert get_schedule(scs, 'SP_1', 'code A') == [
                {'rt': True, 'dt': '20160102T081717'},
                {'rt': False, 'dt': '20160102T091717'},
            ]

            query = self.query_template_dep.format(sp='SP_1')
            response = self.query_region(query)
            dep = get_not_null(response, 'departures')
            assert len(dep) == 2
            assert get_departure(dep, 'SP_1', 'code A') == [
                {'rt': True, 'dt': '20160102T081717'},
                {'rt': False, 'dt': '20160102T091717'},
            ]

            query = self.query_template_ter.format(sp='SP_1')
            response = self.query_region(query)
            ter = get_not_null(response, 'terminus_schedules')
            assert len(ter) == 1
            # 2016-01-02 08:17:17
            assert get_schedule(ter, 'SP_1', 'code A') == [
                {'rt': True, 'dt': '20160102T081717'},
                {'rt': False, 'dt': '20160102T091717'},
            ]
