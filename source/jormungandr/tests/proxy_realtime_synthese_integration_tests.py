# coding=utf-8

# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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


MOCKED_PROXY_CONF = [
    {
        "object_id_tag": "Kisio数字",
        "id": "Kisio数字",
        "class": "jormungandr.realtime_schedule.synthese.Synthese",
        "args": {"timezone": "UTC", "service_url": "http://bob.com", "timeout": 15},
    }
]


def _get_schedule(sched, sp_uri, route_uri):
    """ small helper that extract the information from a route point stop schedule """
    return [
        {'rt': r['data_freshness'] == 'realtime', 'dt': r['date_time']}
        for r in next(
            rp_sched['date_times']
            for rp_sched in sched['stop_schedules']
            if rp_sched['stop_point']['id'] == sp_uri and rp_sched['route']['id'] == route_uri
        )
    ]


@dataset({'multiple_schedules': {'instance_config': {'realtime_proxies': MOCKED_PROXY_CONF}}})
class TestSyntheseSchedules(AbstractTestFixture):
    """
    integration tests for synthese

    Note: for readability, in the dataset, base schedule is always on hours (8:00, 9:00),
    and realtime schedule is always at 17min (8:17, 9:17)
    """

    query_template = 'stop_points/{sp}/stop_schedules?data_freshness=realtime&_current_datetime=20160102T0800'

    def test_stop_schedule_good_id(self):
        """
        test for a route point with good codes (but with multiple codes on the route)
        we should be able to find all the synthese passages and aggregate them
        """
        mock_requests = MockRequests(
            {
                'http://bob.com?SERVICE=tdg&roid=syn_stoppoint1&rn=10000&date=2016-01-02 08:00': (
                    """<?xml version="1.0" encoding="UTF-8"?>
                <timeTable>
                  <journey routeId="syn_cute_routeA1" dateTime="2016-Jan-02 09:17:17" blink="0" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint1"/>
                    <line id="syn_lineA">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                <journey routeId="syn_cute_routeA1" dateTime="2016-Jan-02 10:17:17" blink="0" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint1"/>
                    <line id="syn_lineA">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                <!-- another of the route's code-->
                <journey routeId="syn_routeA1" dateTime="2016-Jan-02 11:17:17" blink="0" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint1"/>
                    <line id="syn_lineA">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                <!-- an unkown route's code, should not be considered-->
                <journey routeId="unknown_route" dateTime="2016-Jan-02 12:17:17" blink="0" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint1"/>
                    <line id="syn_lineA">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                </timeTable>
            """,
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template.format(sp='SP_1')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            print(_get_schedule(response, 'SP_1', 'A1'))
            assert _get_schedule(response, 'SP_1', 'A1') == [
                {'rt': True, 'dt': '20160102T091717'},
                {'rt': True, 'dt': '20160102T101717'},
                {'rt': True, 'dt': '20160102T111717'},
            ]

    def test_stop_schedule_bad_id_one_route_good_line(self):
        """
        test for a route point when the route code is not in synthese
        since there is only one route in navitia and 1 response in synthese (with the good line),
        we get some realtime
        """
        mock_requests = MockRequests(
            {
                'http://bob.com?SERVICE=tdg&roid=syn_stoppoint11&rn=10000&date=2016-01-02 08:00': (
                    """<?xml version="1.0" encoding="UTF-8"?>
                <timeTable>
                  <journey routeId="unknown_route_code" dateTime="2016-Jan-02 09:17:17" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint11"/>
                    <line id="syn_lineB">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                  <journey routeId="unknown_route_code" dateTime="2016-Jan-02 10:17:17" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint11"/>
                    <line id="syn_lineB">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                </timeTable>
             """,
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template.format(sp='SP_11')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            assert _get_schedule(response, 'SP_11', 'B1') == [
                {'rt': True, 'dt': '20160102T091717'},
                {'rt': True, 'dt': '20160102T101717'},
            ]

    def test_stop_schedule_bad_id_one_route_bad_line(self):
        """
        test for a route point when the route code is not in synthese
        since there is only one route in navitia and 1 response in synthese but not on the same line
        we don't get some realtime
        """
        mock_requests = MockRequests(
            {
                'http://bob.com?SERVICE=tdg&roid=syn_stoppoint11&rn=10000&date=2016-01-02 08:00': (
                    """<?xml version="1.0" encoding="UTF-8"?>
                <timeTable>
                  <journey routeId="unknown_route_code" dateTime="2016-Jan-02 09:17:17" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint11"/>
                    <line id="another_line_code_but_not_B">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                </timeTable>
             """,
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template.format(sp='SP_11')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            assert _get_schedule(response, 'SP_11', 'B1') == [{'rt': False, 'dt': '20160102T090000'}]

    def test_stop_schedule_bad_id_multiple_response(self):
        """
        test for a route point when the route code is not in synthese
        since there is only one route in navitia but multiple routes in synthese, we get all the synthese
        passage on this route
        """
        mock_requests = MockRequests(
            {
                'http://bob.com?SERVICE=tdg&roid=syn_stoppoint11&rn=10000&date=2016-01-02 08:00': (
                    """<?xml version="1.0" encoding="UTF-8"?>
                <timeTable>
                  <journey routeId="unknown_route_code1" dateTime="2016-Jan-02 09:17:17" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint11"/>
                    <line id="syn_lineB">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>

                  <journey routeId="unknown_route_code2" dateTime="2016-Jan-02 10:17:17" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint11"/>
                    <line id="syn_lineB">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>

                  <journey routeId="unknown_route_code3" dateTime="2016-Jan-02 11:17:17" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint11"/>
                    <line id="another_line_this_will_not_be_taken">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                </timeTable>
             """,
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template.format(sp='SP_11')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 1
            sched = _get_schedule(response, 'SP_11', 'B1')
            assert sched == [{'rt': True, 'dt': '20160102T091717'}, {'rt': True, 'dt': '20160102T101717'}]

    def test_stop_schedule_bad_id_multiple_routes(self):
        """
        test for a route point when the route code is not in synthese
        since there are multiple routes in navitia, we can't sort them out, and we don't get some realtime
        """
        mock_requests = MockRequests(
            {
                'http://bob.com?SERVICE=tdg&roid=syn_stoppoint21&rn=10000&date=2016-01-02 08:00': (
                    """<?xml version="1.0" encoding="UTF-8"?>
                <timeTable>
                  <journey routeId="unknown_route" dateTime="2016-Jan-02 09:17:17" blink="0" realTime="yes"
                  waiting_time="00:07:10">
                    <stop id="syn_stoppoint21"/>
                    <line id="syn_lineC">
                      <network id="syn_network" name="nice_network" image=""/>
                    </line>
                  </journey>
                </timeTable>
            """,
                    200,
                )
            }
        )
        with mock.patch('requests.get', mock_requests.get):
            query = self.query_template.format(sp='SP_21')
            response = self.query_region(query)
            scs = get_not_null(response, 'stop_schedules')
            assert len(scs) == 3
            assert _get_schedule(response, 'SP_21', 'C1') == [{'rt': False, 'dt': '20160102T090000'}]
            assert _get_schedule(response, 'SP_21', 'C2') == [{'rt': False, 'dt': '20160102T100000'}]

    def test_pt_ref(self):
        """there are 50 stopareas in the dataset to test the pagination handling"""
        from jormungandr import i_manager
        from navitiacommon import type_pb2

        i = i_manager.instances['multiple_schedules']

        assert len([r for r in i.ptref.get_objs(type_pb2.STOP_AREA)]) == 50
