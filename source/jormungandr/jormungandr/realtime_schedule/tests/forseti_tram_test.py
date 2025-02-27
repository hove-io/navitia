# coding=utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from __future__ import absolute_import, print_function, division
import mock
from jormungandr.realtime_schedule.forseti_multi_stop import ForsetiMultiStop
from jormungandr.realtime_schedule.realtime_proxy import Direction
from jormungandr.tests.utils_test import MockRequests
import datetime
import pytz
import pytest
from jormungandr import ptref


class MockInstance:
    def __init__(self):
        self.ptref = ptref.PtRef(self)


def verify_attributes_in_connector_test():
    """
    Verify all attributes of connector
    """
    forseti = ForsetiMultiStop(id='my_tram_rt', service_url='http://bob.com/', instance=MockInstance())
    assert forseti.rt_system_id == "my_tram_rt"
    assert forseti.object_id_tag == "source"
    assert forseti.destination_id_tag == "source"
    assert forseti.line_id_tag == "source"
    assert forseti.service_url == "http://bob.com/"
    assert forseti.timeout == 2

    forseti = ForsetiMultiStop(
        id='my_tram_rt',
        service_url='http://bob.com/',
        object_id_tag='netex_monomodal_stopplace',
        instance=MockInstance(),
    )
    assert forseti.object_id_tag == "netex_monomodal_stopplace"
    assert forseti.destination_id_tag == "source"
    assert forseti.line_id_tag == "source"

    forseti = ForsetiMultiStop(
        id='my_tram_rt',
        service_url='http://bob.com/',
        object_id_tag='netex_monomodal_stopplace',
        destination_id_tag='netex_monomodal_stopplace',
        instance=MockInstance(),
    )
    assert forseti.object_id_tag == "netex_monomodal_stopplace"
    assert forseti.destination_id_tag == "netex_monomodal_stopplace"
    assert forseti.line_id_tag == "source"

    forseti = ForsetiMultiStop(
        id='my_tram_rt',
        service_url='http://bob.com/',
        object_id_tag='netex_monomodal_stopplace',
        destination_id_tag='netex_monomodal_stopplace',
        line_id_tag='tag_pdl',
        instance=MockInstance(),
    )
    assert forseti.object_id_tag == "netex_monomodal_stopplace"
    assert forseti.destination_id_tag == "netex_monomodal_stopplace"
    assert forseti.line_id_tag == "tag_pdl"


def make_url_params_with_invalid_code_test():
    """
    test make_url when RoutePoint does not have a mandatory code

    we should not get any url
    """
    forseti = ForsetiMultiStop(id='my_tram_rt', service_url='http://bob.com/', instance=MockInstance())
    params = forseti._make_params(MockRoutePoint(line_code='line_toto', stop_id=[]))
    assert params is None


def make_url_params_test():
    forseti = ForsetiMultiStop(id='my_tram_rt', service_url='http://bob.com/', instance=MockInstance())

    # The return is like [(stop_id, val1), (stop_id, val2), (direction_type, val), ...]
    params = forseti._make_params(MockRoutePoint(line_code='line_1', stop_id='stop_1'))
    assert params == [('stop_id', 'stop_1')]

    params = forseti._make_params(MockRoutePoint(line_code='line_1', stop_id=['stop_1', 'stop_2']))
    assert params == [('stop_id', 'stop_1'), ('stop_id', 'stop_2')]

    params = forseti._make_params(MockRoutePoint(line_code='line_1', stop_id='stop_1', direction_type='forward'))
    assert params == [('stop_id', 'stop_1'), ('direction_type', 'forward')]


class MockRoutePoint(object):
    def __init__(self, *args, **kwargs):
        l = kwargs['line_code']
        if isinstance(l, list):
            self._hardcoded_line_ids = l
        else:
            self._hardcoded_line_ids = [l]

        l = kwargs['stop_id']
        if isinstance(l, list):
            self._hardcoded_stop_ids = l
        else:
            self._hardcoded_stop_ids = [l]

        if 'direction_type' in kwargs:
            self._hardcoded_direction_type = kwargs['direction_type']
        else:
            self._hardcoded_direction_type = None

    def fetch_all_stop_id(self, object_id_tag):
        return self._hardcoded_stop_ids

    def fetch_all_line_id(self, object_id_tag):
        return self._hardcoded_line_ids

    def fetch_direction_type(self):
        return self._hardcoded_direction_type

    def fetch_line_uri(self):
        return "line:PDL:NM:Line:1:LOC"


class MockResponse(object):
    def __init__(self, data, status_code, url, *args, **kwargs):
        self.data = data
        self.status_code = status_code
        self.url = url

    def json(self):
        return self.data


@pytest.fixture(scope="module")
def mock_multiline_response():
    return {
        "departures": [
            {
                "line": "NM:Line:1:LOC",
                "stop": "MOBIITI:StopPlace:18977",
                "type": "E",
                "direction": "MOBIITI:StopPlace:18638",
                "direction_name": "François Mitterrand",
                "datetime": "2025-02-26T15:50:48+01:00",
                "direction_type": "forward",
            },
            {
                "line": "NM:Line:1:LOC",
                "stop": "MOBIITI:StopPlace:18977",
                "type": "E",
                "direction": "MOBIITI:StopPlace:19578",
                "direction_name": "Jamet",
                "datetime": "2025-02-26T15:54:00+01:00",
                "direction_type": "forward",
            },
            {
                "line": "NM:Line:2:LOC",
                "stop": "MOBIITI:StopPlace:18977",
                "type": "E",
                "direction": "MOBIITI:StopPlace:18590",
                "direction_name": "Grand Val",
                "datetime": "2025-02-26T15:54:12+01:00",
                "direction_type": "backward",
            },
            {
                "line": "NM:Line:1:LOC",
                "stop": "MOBIITI:StopPlace:18977",
                "type": "E",
                "direction": "MOBIITI:StopPlace:18920",
                "direction_name": "Ranzay",
                "datetime": "2025-02-26T16:02:06+01:00",
                "direction_type": "backward",
            },
            {
                "line": "NM:Line:2:LOC",
                "stop": "MOBIITI:StopPlace:18977",
                "type": "E",
                "direction": "MOBIITI:StopPlace:18590",
                "direction_name": "Grand Val",
                "datetime": "2025-02-26T16:05:00+01:00",
                "direction_type": "forward",
            },
            {
                "line": "NM:Line:1:LOC",
                "stop": "MOBIITI:StopPlace:18977",
                "type": "E",
                "direction": "MOBIITI:StopPlace:18638",
                "direction_name": "François Mitterrand",
                "datetime": "2025-02-26T16:15:06+01:00",
                "direction_type": "forward",
            },
        ]
    }


def next_passage_for_route_point_test(mock_multiline_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a good response, we should get some next_passages
    we match also direction_type = "forward" in the connector (forward = outbound / backward = inbound)
    The connector keeps only date_times with line = NM:Line:1:LOC and direction_type = forward
    """
    forseti = ForsetiMultiStop(id='my_tram_rt', service_url='http://bob.com/', instance=MockInstance())

    mock_requests = MockRequests(
        {
            'http://bob.com/?direction_type=forward&stop_id=MOBIITI%3AStopPlace%3A18977': (
                mock_multiline_response,
                200,
            )
        }
    )

    route_point = MockRoutePoint(
        line_code='NM:Line:1:LOC', stop_id='MOBIITI:StopPlace:18977', direction_type='forward'
    )

    with mock.patch('requests.get', mock_requests.get):
        with mock.patch(
            'jormungandr.realtime_schedule.forseti_multi_stop.ForsetiMultiStop._get_direction',
            lambda ForsetiMultiStop, **kwargs: Direction(
                "stop_area:PDL:MOBIITI:StopPlace:8730", "François Mitterrand (Saint-Herblain)"
            ),
        ):
            passages = forseti.next_passage_for_route_point(route_point)

            assert len(passages) == 3

            assert passages[0].datetime == datetime.datetime(2025, 2, 26, 14, 50, 48, tzinfo=pytz.UTC)
            assert passages[0].is_real_time is True
            assert passages[0].direction == "François Mitterrand (Saint-Herblain)"
            assert passages[0].direction_uri == "stop_area:PDL:MOBIITI:StopPlace:8730"

            assert passages[1].datetime == datetime.datetime(2025, 2, 26, 14, 54, tzinfo=pytz.UTC)
            assert passages[1].is_real_time is True
            assert passages[1].direction == "François Mitterrand (Saint-Herblain)"
            assert passages[1].direction_uri == "stop_area:PDL:MOBIITI:StopPlace:8730"

            assert passages[2].datetime == datetime.datetime(2025, 2, 26, 15, 15, 6, tzinfo=pytz.UTC)
            assert passages[2].is_real_time is True
            assert passages[2].direction == "François Mitterrand (Saint-Herblain)"
            assert passages[2].direction_uri == "stop_area:PDL:MOBIITI:StopPlace:8730"


def status_test():
    forseti = ForsetiMultiStop(
        id='my_tram_rt',
        service_url='http://bob.com/',
        service_args={'a': 'test', 'b': '12'},
        instance=MockInstance(),
    )
    status = forseti.status()
    assert status['id'] == "my_tram_rt"
