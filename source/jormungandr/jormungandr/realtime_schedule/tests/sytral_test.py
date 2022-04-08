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
from jormungandr.realtime_schedule.sytral import Sytral
from jormungandr.realtime_schedule.realtime_proxy import Direction
from jormungandr.tests.utils_test import MockRequests
import datetime
import pytz
import pytest
from jormungandr import ptref


class MockInstance:
    def __init__(self):
        self.ptref = ptref.PtRef(self)


def make_url_params_test():
    sytral = Sytral(id='tata', service_url='http://bob.com/', instance=MockInstance())

    # The return is like [(stop_id, val1), (stop_id, val2), (direction_type, val), ...]
    params = sytral._make_params(MockRoutePoint(line_code='line_toto', stop_id='stop_tutu'))

    assert params == [('stop_id', 'stop_tutu')]

    params = sytral._make_params(MockRoutePoint(line_code='line_toto', stop_id=['stop_tutu', 'stop_toto']))

    assert params == [('stop_id', 'stop_tutu'), ('stop_id', 'stop_toto')]

    params = sytral._make_params(
        MockRoutePoint(line_code='line_toto', stop_id='stop_tutu', direction_type='forward')
    )

    assert params == [('stop_id', 'stop_tutu'), ('direction_type', 'forward')]


def make_url_params_with_invalid_code_test():
    """
    test make_url when RoutePoint does not have a mandatory code

    we should not get any url
    """
    sytral = Sytral(id='tata', service_url='http://bob.com/', instance=MockInstance())

    # The return is like [(stop_id, val1), (stop_id, val2), (direction_type, val), ...]
    params = sytral._make_params(MockRoutePoint(line_code='line_toto', stop_id=[]))

    assert params == None


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
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:37:15+02:00",
                "type": "E",
                "line": "05A",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:38:15+02:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:45:35+02:00",
                "type": "E",
                "line": "05B",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:49:35+02:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
        ]
    }


@pytest.fixture(scope="module")
def mock_good_response():
    return {
        "departures": [
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:37:15+02:00",
                "type": "E",
                "line": "05",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:38:15+02:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:45:35+02:00",
                "type": "E",
                "line": "05",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:49:35+02:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
        ]
    }


@pytest.fixture(scope="module")
def mock_direction_type_is_forward_response():
    return {
        "departures": [
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "forward",
                "datetime": "2016-04-11T14:37:15+02:00",
                "type": "E",
                "line": "05",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "forward",
                "datetime": "2016-04-11T14:38:15+02:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
        ]
    }


@pytest.fixture(scope="module")
def mock_empty_response():
    return {}


@pytest.fixture(scope="module")
def mock_no_departure_response():
    return {"departures": []}


@pytest.fixture(scope="module")
def mock_missing_line_response():
    return {
        "departures": [
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:38:15+02:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:49:35+02:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
        ]
    }


@pytest.fixture(scope="module")
def mock_theoric_response():
    return {
        "departures": [
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:37:15+01:00",
                "type": "T",
                "line": "05",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:38:15+01:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:45:35+01:00",
                "type": "E",
                "line": "05",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:49:35+01:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
        ]
    }


@pytest.fixture(scope="module")
def mock_multi_stop_point_id_response():
    return {
        "departures": [
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:37:15+01:00",
                "type": "T",
                "line": "05",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:38:15+01:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:45:35+01:00",
                "type": "E",
                "line": "05",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:49:35+01:00",
                "type": "E",
                "line": "04",
                "stop": "42",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:38:15+01:00",
                "type": "T",
                "line": "05",
                "stop": "43",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:38:15+01:00",
                "type": "E",
                "line": "04",
                "stop": "43",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:47:35+01:00",
                "type": "E",
                "line": "05",
                "stop": "43",
            },
            {
                "direction": "3341",
                "direction_name": "Piscine Chambéry",
                "direction_type": "both",
                "datetime": "2016-04-11T14:49:35+01:00",
                "type": "E",
                "line": "04",
                "stop": "43",
            },
        ]
    }


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
        return "AA"


def next_passage_for_route_point_test(mock_good_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a good response, we should get some next_passages
    """
    sytral = Sytral(id='tata', service_url='http://bob.com/', instance=MockInstance())

    mock_requests = MockRequests({'http://bob.com/?stop_id=42': (mock_good_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='42')

    with mock.patch('requests.get', mock_requests.get):
        with mock.patch(
            'jormungandr.realtime_schedule.sytral.Sytral._get_direction',
            lambda Sytral, **kwargs: Direction("3341", "Piscine Chambéry"),
        ):
            passages = sytral.next_passage_for_route_point(route_point)

            assert len(passages) == 2

            assert passages[0].datetime == datetime.datetime(2016, 4, 11, 12, 37, 15, tzinfo=pytz.UTC)
            assert passages[0].is_real_time
            assert passages[1].datetime == datetime.datetime(2016, 4, 11, 12, 45, 35, tzinfo=pytz.UTC)
            assert passages[1].is_real_time


def next_passage_for_route_point_with_direction_type_test(mock_direction_type_is_forward_response):
    """
    test the whole next_passage_for_route_point with direction_type parameter
    mock the http call to return a good response, we should get some next_passages
    """
    sytral = Sytral(id='tata', service_url='http://bob.com/')

    mock_requests = MockRequests(
        {'http://bob.com/?direction_type=forward&stop_id=42': (mock_direction_type_is_forward_response, 200)}
    )

    route_point = MockRoutePoint(line_code='05', stop_id='42', direction_type='forward')

    with mock.patch('requests.get', mock_requests.get):
        with mock.patch(
            'jormungandr.realtime_schedule.sytral.Sytral._get_direction',
            lambda Sytral, **kwargs: Direction("3341", "Piscine Chambéry"),
        ):
            passages = sytral.next_passage_for_route_point(route_point)

            assert len(passages) == 1

            assert passages[0].datetime == datetime.datetime(2016, 4, 11, 12, 37, 15, tzinfo=pytz.UTC)
            assert passages[0].is_real_time


def next_passage_for_empty_response_test(mock_empty_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a empty response, we should get None
    """
    sytral = Sytral(id='tata', service_url='http://bob.com/', instance=MockInstance())

    mock_requests = MockRequests({'http://bob.com/?stop_id=42': (mock_empty_response, 500)})

    route_point = MockRoutePoint(line_code='05', stop_id='42')

    with mock.patch('requests.get', mock_requests.get):
        passages = sytral.next_passage_for_route_point(route_point)

        assert passages is None


def next_passage_for_no_departures_response_test(mock_no_departure_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a response without any departures, we should get no departure
    """
    sytral = Sytral(id='tata', service_url='http://bob.com/', instance=MockInstance())

    mock_requests = MockRequests({'http://bob.com/?stop_id=42': (mock_no_departure_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='42')

    with mock.patch('requests.get', mock_requests.get):
        passages = sytral.next_passage_for_route_point(route_point)

        assert passages == []


def next_passage_for_missing_line_response_test(mock_missing_line_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a response without wanted line  we should get no departure
    """
    sytral = Sytral(
        id='tata',
        service_url='http://bob.com/',
        service_args={'a': 'bobette', 'b': '12'},
        instance=MockInstance(),
    )

    mock_requests = MockRequests({'http://bob.com/?stop_id=42': (mock_missing_line_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='42')

    with mock.patch('requests.get', mock_requests.get):
        passages = sytral.next_passage_for_route_point(route_point)

        assert passages == []


def next_passage_with_theoric_time_response_test(mock_theoric_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a response with a theoric time we should get one departure
    """
    sytral = Sytral(
        id='tata',
        service_url='http://bob.com/',
        service_args={'a': 'bobette', 'b': '12'},
        instance=MockInstance(),
    )

    mock_requests = MockRequests({'http://bob.com/?stop_id=42': (mock_theoric_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='42')

    with mock.patch('requests.get', mock_requests.get):
        with mock.patch(
            'jormungandr.realtime_schedule.sytral.Sytral._get_direction',
            lambda Sytral, **kwargs: Direction("3341", "Piscine Chambéry"),
        ):
            passages = sytral.next_passage_for_route_point(route_point)

            assert len(passages) == 2

            assert passages[0].datetime == datetime.datetime(2016, 4, 11, 13, 37, 15, tzinfo=pytz.UTC)
            assert not passages[0].is_real_time
            assert passages[1].datetime == datetime.datetime(2016, 4, 11, 13, 45, 35, tzinfo=pytz.UTC)
            assert passages[1].is_real_time


def status_test():
    sytral = Sytral(
        id=u'tata-é$~#@"*!\'`§èû',
        service_url='http://bob.com/',
        service_args={'a': 'bobette', 'b': '12'},
        instance=MockInstance(),
    )
    status = sytral.status()
    assert status['id'] == u"tata-é$~#@\"*!'`§èû"


def next_passage_for_route_point_multiline_test(mock_multiline_response):
    """
    test the whole next_passage_for_route_point with a routepoint having multiple SAE lines
    """
    sytral = Sytral(id='tata', service_url='http://bob.com/', instance=MockInstance())

    mock_requests = MockRequests({'http://bob.com/?stop_id=42': (mock_multiline_response, 200)})

    route_point = MockRoutePoint(line_code=['05A', '05B'], stop_id='42')

    with mock.patch('requests.get', mock_requests.get):
        with mock.patch(
            'jormungandr.realtime_schedule.sytral.Sytral._get_direction',
            lambda Sytral, **kwargs: Direction("3341", "Piscine Chambéry"),
        ):
            passages = sytral.next_passage_for_route_point(route_point)

            assert len(passages) == 2

            assert passages[0].datetime == datetime.datetime(2016, 4, 11, 12, 37, 15, tzinfo=pytz.UTC)
            assert passages[0].is_real_time
            assert passages[1].datetime == datetime.datetime(2016, 4, 11, 12, 45, 35, tzinfo=pytz.UTC)
            assert passages[1].is_real_time


def next_passage_for_route_point_multi_stop_point_id_test(mock_multi_stop_point_id_response):
    """
    test next_passage for route point with multi stop point ID
    """
    sytral = Sytral(id='tata', service_url='http://bob.com/', instance=MockInstance())

    mock_requests = MockRequests(
        {'http://bob.com/?stop_id=42&stop_id=43': (mock_multi_stop_point_id_response, 200)}
    )

    route_point = MockRoutePoint(line_code='05', stop_id=['42', '43'])

    with mock.patch('requests.get', mock_requests.get):
        with mock.patch(
            'jormungandr.realtime_schedule.sytral.Sytral._get_direction',
            lambda Sytral, **kwargs: Direction("3341", "Piscine Chambéry"),
        ):
            passages = sytral.next_passage_for_route_point(route_point)

            assert len(passages) == 4

            # Stop id 42
            assert passages[0].datetime == datetime.datetime(2016, 4, 11, 13, 37, 15, tzinfo=pytz.UTC)
            assert not passages[0].is_real_time
            assert passages[1].datetime == datetime.datetime(2016, 4, 11, 13, 45, 35, tzinfo=pytz.UTC)
            assert passages[1].is_real_time
            # Stop id 43
            assert passages[2].datetime == datetime.datetime(2016, 4, 11, 13, 38, 15, tzinfo=pytz.UTC)
            assert not passages[2].is_real_time
            assert passages[3].datetime == datetime.datetime(2016, 4, 11, 13, 47, 35, tzinfo=pytz.UTC)
            assert passages[3].is_real_time
