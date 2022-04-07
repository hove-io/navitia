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
from tests.check_utils import get_not_null
from .tests_mechanism import AbstractTestFixture, dataset
import requests_mock

MOCKED_PROXY_CONF = [
    {
        "object_id_tag": "Kisio数字",
        "id": "Kisio数字",
        "class": "jormungandr.realtime_schedule.siri_lite.SiriLite",
        "args": {
            "destination_id_tag": "Kisio数字",
            "timezone": "Europe/Paris",
            "service_url": "http://siri.com?apikey=bob",
            "timeout": 15,
        },
    }
]


def _get_formated_schedule(scs, sp, route):
    """ small helper that extract the information from a route point stop schedule """
    return [
        {'rt': r['data_freshness'] == 'realtime', 'dt': r['date_time']}
        for r in next(
            rp_sched['date_times']
            for rp_sched in scs
            if rp_sched['stop_point']['id'] == sp and rp_sched['route']['id'] == route
        )
    ]


@dataset({"multiple_schedules": {'instance_config': {'realtime_proxies': MOCKED_PROXY_CONF}}})
class TestSiriLite(AbstractTestFixture):
    query_template = 'stop_points/{sp}/stop_schedules?_current_datetime=20160102T0800'

    def test_stop_schedule(self):
        """
        test a stop schedule with a sirilite realtime proxy

        we have 3 routes that pass by sp_21

        C1 : SP_20 -> SP_21 -> SP_22
        C2 : SP_20 -> SP_21 -> SP_22
        C-backward : SP_22 -> SP_21 -> SP_20

        we have 5 departures in the realtime sirilite response:
        - 2 that goes to SP_22 (so we attach them to C1 and C2)
        - 1 that goes to SP_20 (so we attach them to C-backward)
        - 1 with an unknown line ref
        - 1 with an unknown destination
        """
        mock_response = {
            "Siri": {
                "ServiceDelivery": {
                    "ProducerRef": "bob",
                    "ResponseMessageIdentifier": "bob_id",
                    "ResponseTimestamp": "2017-07-19T13:01:59Z",
                    "StopMonitoringDelivery": [
                        {
                            "MonitoredStopVisit": [
                                {
                                    "ItemIdentifier": "complex_id_for_syn_stoppoint1",
                                    "MonitoredVehicleJourney": {
                                        "DestinationName": [{"value": "to stop 22"}],
                                        "DestinationRef": {"value": "syn_stoppoint22"},
                                        "LineRef": {"value": "syn_lineC"},
                                        "MonitoredCall": {
                                            "ExpectedDepartureTime": "2016-01-02T10:02:42.000Z",
                                            "StopPointName": [{"value": "the stop"}],
                                            "VehicleAtStop": False,
                                        },
                                    },
                                    "MonitoringRef": {"value": "syn_stoppoint21"},
                                    "RecordedAtTime": "2017-07-19T13:01:54.727Z",
                                },
                                # we add another passage on to stop 22 (route C)
                                {
                                    "ItemIdentifier": "complex_id_for_syn_stoppoint1",
                                    "MonitoredVehicleJourney": {
                                        "DestinationName": [{"value": "to stop 22"}],
                                        "DestinationRef": {"value": "syn_stoppoint22"},
                                        "LineRef": {"value": "syn_lineC"},
                                        "MonitoredCall": {
                                            "ExpectedDepartureTime": "2016-01-02T10:05:42.000Z",
                                            "StopPointName": [{"value": "the stop"}],
                                            "VehicleAtStop": False,
                                        },
                                    },
                                    "MonitoringRef": {"value": "syn_stoppoint21"},
                                    "RecordedAtTime": "2017-07-19T13:01:54.727Z",
                                },
                                # we add a passage to SP_20 (route C-backward)
                                {
                                    "ItemIdentifier": "complex_id_for_syn_stoppoint1",
                                    "MonitoredVehicleJourney": {
                                        "DestinationName": [{"value": "to stop 20"}],
                                        "DestinationRef": {"value": "syn_stoppoint20"},
                                        "LineRef": {"value": "syn_lineC"},
                                        "MonitoredCall": {
                                            "ExpectedDepartureTime": "2016-01-02T10:03:42.000Z",
                                            "StopPointName": [{"value": "the stop"}],
                                            "VehicleAtStop": False,
                                        },
                                    },
                                    "MonitoringRef": {"value": "syn_stoppoint21"},
                                    "RecordedAtTime": "2017-07-19T13:01:54.727Z",
                                },
                                # we add a passage with no departure time (we should not consider it)
                                {
                                    "ItemIdentifier": "complex_id_for_syn_stoppoint1",
                                    "MonitoredVehicleJourney": {
                                        "DestinationName": [{"value": "to stop 20"}],
                                        "DestinationRef": {"value": "syn_stoppoint22"},
                                        "LineRef": {"value": "syn_lineC"},
                                        "MonitoredCall": {
                                            "ExpectedDepartureTime": "",
                                            "StopPointName": [{"value": "the stop"}],
                                            "VehicleAtStop": False,
                                        },
                                    },
                                    "MonitoringRef": {"value": "syn_stoppoint21"},
                                    "RecordedAtTime": "2017-07-19T13:01:54.727Z",
                                },
                                # we add a passage with departure time in the past (we should not consider it)
                                {
                                    "ItemIdentifier": "complex_id_for_syn_stoppoint1",
                                    "MonitoredVehicleJourney": {
                                        "DestinationName": [{"value": "to stop 20"}],
                                        "DestinationRef": {"value": "syn_stoppoint22"},
                                        "LineRef": {"value": "syn_lineC"},
                                        "MonitoredCall": {
                                            "ExpectedDepartureTime": "2016-01-02T02:06:42.000Z",
                                            "StopPointName": [{"value": "the stop"}],
                                            "VehicleAtStop": False,
                                        },
                                    },
                                    "MonitoringRef": {"value": "syn_stoppoint21"},
                                    "RecordedAtTime": "2017-07-19T13:01:54.727Z",
                                },
                                # we add a passage on an unknown line (we should not consider it)
                                {
                                    "ItemIdentifier": "complex_id_for_syn_stoppoint1",
                                    "MonitoredVehicleJourney": {
                                        "DestinationName": [{"value": "to stop 20"}],
                                        "DestinationRef": {"value": "syn_stoppoint20"},
                                        "LineRef": {"value": "an_unkonwn_line"},
                                        "MonitoredCall": {
                                            "ExpectedDepartureTime": "2016-01-02T10:13:42.000Z",
                                            "StopPointName": [{"value": "the stop"}],
                                            "VehicleAtStop": False,
                                        },
                                    },
                                    "MonitoringRef": {"value": "syn_stoppoint21"},
                                    "RecordedAtTime": "2017-07-19T13:01:54.727Z",
                                },
                                # we add a passage on an unknown destination, we should skip it too
                                {
                                    "ItemIdentifier": "complex_id_for_syn_stoppoint1",
                                    "MonitoredVehicleJourney": {
                                        "DestinationName": [{"value": "to stop bob"}],
                                        "DestinationRef": {"value": "an_unknown_stop"},
                                        "LineRef": {"value": "syn_lineC"},
                                        "MonitoredCall": {
                                            "ExpectedDepartureTime": "2016-01-02T10:23:42.000Z",
                                            "StopPointName": [{"value": "the stop"}],
                                            "VehicleAtStop": False,
                                        },
                                    },
                                    "MonitoringRef": {"value": "syn_stoppoint21"},
                                    "RecordedAtTime": "2017-07-19T13:01:54.727Z",
                                },
                            ],
                            "ResponseTimestamp": "2017-07-19T13:01:59Z",
                            "Status": "true",
                            "Version": "2.0",
                        }
                    ],
                }
            }
        }

        with requests_mock.Mocker() as m:
            m.get('http://siri.com?apikey=bob&MonitoringRef=syn_stoppoint21', json=mock_response)
            # first we make a base schedule call for the test to be more readable
            query = self.query_template.format(sp='SP_21') + "&data_freshness=base_schedule"
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 3
            # we have 1 passage per route point
            assert _get_formated_schedule(scs, sp='SP_21', route='C-backward') == [
                {'rt': False, 'dt': '20160102T101000'}
            ]

            assert _get_formated_schedule(scs, sp='SP_21', route='C1') == [
                {'rt': False, 'dt': '20160102T090000'}
            ]

            assert _get_formated_schedule(scs, sp='SP_21', route='C2') == [
                {'rt': False, 'dt': '20160102T100000'}
            ]

            # now we make a realtime call
            query = self.query_template.format(sp='SP_21') + "&data_freshness=realtime"
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 3

            assert _get_formated_schedule(scs, sp='SP_21', route='C-backward') == [
                {'rt': True, 'dt': '20160102T100342'}
            ]

            # c1 and c2 have the same destination, so we cannot know at which route it is from the
            # realtime passage data, so we have the realtime passage on both routes
            # this is not great, but it will do for the moment
            assert (
                _get_formated_schedule(scs, sp='SP_21', route='C1')
                == _get_formated_schedule(scs, sp='SP_21', route='C2')
                == [{'rt': True, 'dt': '20160102T100242'}, {'rt': True, 'dt': '20160102T100542'}]
            )
