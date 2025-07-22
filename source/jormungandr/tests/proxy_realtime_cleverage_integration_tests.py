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
        "object_id_tag": "Hove数字",
        "id": "Hove数字",
        "class": "jormungandr.realtime_schedule.cleverage.Cleverage",
        "args": {
            "destination_id_tag": "Hove数字",
            "timezone": "UTC",
            "service_url": "http://XXXX",
            "timeout": 15,
            "service_args": {"serviceID": "X", "EntityID": "XX", "Media": "XXX"},
        },
    }
]


@dataset({'multiple_schedules': {'instance_config': {'realtime_proxies': MOCKED_PROXY_CONF}}})
class TestCleverageSchedules(AbstractTestFixture):
    """
    integration tests for cleverage

    """

    query_template_scs = (
        'stop_points/{sp}/stop_schedules?data_freshness=realtime&_current_datetime=20160102T0800'
    )
    query_template_dep = 'stop_points/{sp}/departures?data_freshness=realtime&_current_datetime=20160102T0800'

    def test_stop_schedule_with_realtime_only(self):
        mock_requests = MockRequests(
            {
                'http://XXXX/syn_stoppoint1': (
                    [
                        {
                            "name": "Lianes 5",
                            "code": "Hove数字 A",
                            "type": "Bus",
                            "schedules": [
                                {
                                    "vehicle_lattitude": "44.792112483318",
                                    "vehicle_longitude": "-0.56718390706918",
                                    "waittime_text": "11 minutes",
                                    "trip_id": "268436451",
                                    "schedule_id": "268476273",
                                    "destination_id": "3341",
                                    "destination_name": "Piscine Chambéry",
                                    "departure": "2016-01-02 08:17:17",
                                    "departure_commande": "2016-01-02 08:17:17",
                                    "departure_theorique": "2016-01-02 08:17:17",
                                    "arrival": "2016-01-02 08:16:00",
                                    "arrival_commande": "2016-01-02 08:16:00",
                                    "arrival_theorique": "2016-01-02 08:16:00",
                                    "comment": "",
                                    "realtime": "1",
                                    "waittime": "00:10:53",
                                    "updated_at": "2016-01-02 08:16:00",
                                    "vehicle_id": "2662",
                                    "vehicle_position_updated_at": "2016-01-02 08:16:00",
                                    "origin": "bdsi",
                                },
                                {
                                    "vehicle_lattitude": "44.814043370749",
                                    "vehicle_longitude": "-0.57294492449656",
                                    "waittime_text": "19 minutes",
                                    "trip_id": "268436310",
                                    "schedule_id": "268468351",
                                    "destination_id": "3341",
                                    "destination_name": "Piscine Chambéry",
                                    "departure": "2016-01-02 09:17:17",
                                    "departure_commande": "2016-01-02 09:17:17",
                                    "departure_theorique": "2016-01-02 09:17:17",
                                    "arrival": "2016-01-02 14:45:35",
                                    "arrival_commande": "2016-01-02 09:16:00",
                                    "arrival_theorique": "2016-01-02 09:16:00",
                                    "comment": "",
                                    "realtime": "1",
                                    "waittime": "00:19:13",
                                    "updated_at": "2016-01-02 09:16:00",
                                    "vehicle_id": "2660",
                                    "vehicle_position_updated_at": "2016-01-02 09:16:00",
                                    "origin": "bdsi",
                                },
                            ],
                        }
                    ],
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template_scs.format(sp='SP_1')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            # 2016-01-02 08:17:00
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

    def test_stop_schedule_with_theoric_and_realtime(self):
        mock_requests = MockRequests(
            {
                'http://XXXX/syn_stoppoint1': (
                    [
                        {
                            "name": "Lianes 5",
                            "code": "Hove数字 A",
                            "type": "Bus",
                            "schedules": [
                                {
                                    "vehicle_lattitude": "44.792112483318",
                                    "vehicle_longitude": "-0.56718390706918",
                                    "waittime_text": "11 minutes",
                                    "trip_id": "268436451",
                                    "schedule_id": "268476273",
                                    "destination_id": "3341",
                                    "destination_name": "Piscine Chambéry",
                                    "departure": "2016-01-02 08:17:17",
                                    "departure_commande": "2016-01-02 08:17:17",
                                    "departure_theorique": "2016-01-02 08:17:17",
                                    "arrival": "2016-01-02 08:16:00",
                                    "arrival_commande": "2016-01-02 08:16:00",
                                    "arrival_theorique": "2016-01-02 08:16:00",
                                    "comment": "",
                                    "realtime": "1",
                                    "waittime": "00:10:53",
                                    "updated_at": "2016-01-02 08:16:00",
                                    "vehicle_id": "2662",
                                    "vehicle_position_updated_at": "2016-01-02 08:16:00",
                                    "origin": "bdsi",
                                },
                                {
                                    "vehicle_lattitude": "44.814043370749",
                                    "vehicle_longitude": "-0.57294492449656",
                                    "waittime_text": "19 minutes",
                                    "trip_id": "268436310",
                                    "schedule_id": "268468351",
                                    "destination_id": "3341",
                                    "destination_name": "Piscine Chambéry",
                                    "departure": "2016-01-02 09:17:17",
                                    "departure_commande": "2016-01-02 09:17:17",
                                    "departure_theorique": "2016-01-02 09:17:17",
                                    "arrival": "2016-01-02 14:45:35",
                                    "arrival_commande": "2016-01-02 09:16:00",
                                    "arrival_theorique": "2016-01-02 09:16:00",
                                    "comment": "",
                                    "realtime": "0",
                                    "waittime": "00:19:13",
                                    "updated_at": "2016-01-02 09:16:00",
                                    "vehicle_id": "2660",
                                    "vehicle_position_updated_at": "2016-01-02 09:16:00",
                                    "origin": "bdsi",
                                },
                            ],
                        }
                    ],
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template_scs.format(sp='SP_1')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            # 2016-01-02 08:17:00
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
