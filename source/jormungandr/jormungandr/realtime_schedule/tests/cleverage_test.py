# coding=utf-8
# Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from jormungandr.realtime_schedule.cleverage import Cleverage
import validators
import datetime
import pytz
import pytest


def make_url_test():
    cleverage = Cleverage(
        id='tata',
        timezone='Europe/Paris',
        service_url='http://bob.com/',
        service_args={'a': 'bobette', 'b': '12'},
    )

    url = cleverage._make_url(MockRoutePoint(line_code='line_toto', stop_id='stop_tutu'))

    # it should be a valid url
    assert validators.url(url)

    assert url == 'http://bob.com/stop_tutu'


def make_url_invalid_code_test():
    """
    test make_url when RoutePoint does not have a mandatory code

    we should not get any url
    """
    cleverage = Cleverage(
        id='tata',
        timezone='Europe/Paris',
        service_url='http://bob.com/',
        service_args={'a': 'bobette', 'b': '12'},
    )

    url = cleverage._make_url(MockRoutePoint(line_code='line_toto', stop_id=None))

    assert url is None


class MockResponse(object):
    def __init__(self, data, status_code, url, *args, **kwargs):
        self.data = data
        self.status_code = status_code
        self.url = url

    def json(self):
        return self.data


class MockRequests(object):
    def __init__(self, responses):
        self.responses = responses

    def get(self, url, *args, **kwargs):
        return MockResponse(self.responses[url][0], self.responses[url][1], url)


@pytest.fixture(scope="module")
def mock_good_response():
    return [
        {
            "name": "Lianes 5",
            "code": "05",
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
                    "departure": "2016-04-11 14:37:15",
                    "departure_commande": "2016-04-11 14:35:47",
                    "departure_theorique": "2016-04-11 14:35:47",
                    "arrival": "2016-04-11 14:37:15",
                    "arrival_commande": "2016-04-11 14:35:47",
                    "arrival_theorique": "2016-04-11 14:35:47",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:10:53",
                    "updated_at": "2016-04-11 14:26:21",
                    "vehicle_id": "2662",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
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
                    "departure": "2016-04-11 14:45:35",
                    "departure_commande": "2016-04-11 14:44:47",
                    "departure_theorique": "2016-04-11 14:44:47",
                    "arrival": "2016-04-11 14:45:35",
                    "arrival_commande": "2016-04-11 14:44:47",
                    "arrival_theorique": "2016-04-11 14:44:47",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:19:13",
                    "updated_at": "2016-04-11 14:25:41",
                    "vehicle_id": "2660",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
                    "origin": "bdsi",
                },
            ],
        },
        {
            "name": "Lianes 4",
            "code": "04",
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
                    "departure": "2016-04-11 14:37:15",
                    "departure_commande": "2016-04-11 14:40:17",
                    "departure_theorique": "2016-04-11 14:40:17",
                    "arrival": "2016-04-11 14:40:17",
                    "arrival_commande": "2016-04-11 14:40:17",
                    "arrival_theorique": "2016-04-11 14:40:17",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:10:53",
                    "updated_at": "2016-04-11 14:26:21",
                    "vehicle_id": "2662",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
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
                    "departure": "2016-04-11 14:49:35",
                    "departure_commande": "2016-04-11 14:49:35",
                    "departure_theorique": "2016-04-11 14:49:35",
                    "arrival": "2016-04-11 14:49:35",
                    "arrival_commande": "2016-04-11 14:49:35",
                    "arrival_theorique": "2016-04-11 14:49:35",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:19:13",
                    "updated_at": "2016-04-11 14:25:41",
                    "vehicle_id": "2660",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
                    "origin": "bdsi",
                },
            ],
        },
    ]


@pytest.fixture(scope="module")
def mock_empty_response():
    return []


@pytest.fixture(scope="module")
def mock_missing_line_response():
    return [
        {
            "name": "Lianes 4",
            "code": "04",
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
                    "departure": "2016-04-11 14:37:15",
                    "departure_commande": "2016-04-11 14:40:17",
                    "departure_theorique": "2016-04-11 14:40:17",
                    "arrival": "2016-04-11 14:40:17",
                    "arrival_commande": "2016-04-11 14:40:17",
                    "arrival_theorique": "2016-04-11 14:40:17",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:10:53",
                    "updated_at": "2016-04-11 14:26:21",
                    "vehicle_id": "2662",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
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
                    "departure": "2016-04-11 14:49:35",
                    "departure_commande": "2016-04-11 14:49:35",
                    "departure_theorique": "2016-04-11 14:49:35",
                    "arrival": "2016-04-11 14:49:35",
                    "arrival_commande": "2016-04-11 14:49:35",
                    "arrival_theorique": "2016-04-11 14:49:35",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:19:13",
                    "updated_at": "2016-04-11 14:25:41",
                    "vehicle_id": "2660",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
                    "origin": "bdsi",
                },
            ],
        }
    ]


@pytest.fixture(scope="module")
def mock_theoric_response():
    return [
        {
            "name": "Lianes 5",
            "code": "05",
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
                    "departure": "2016-04-11 14:37:15",
                    "departure_commande": "2016-04-11 14:35:47",
                    "departure_theorique": "2016-04-11 14:35:47",
                    "arrival": "2016-04-11 14:37:15",
                    "arrival_commande": "2016-04-11 14:35:47",
                    "arrival_theorique": "2016-04-11 14:35:47",
                    "comment": "",
                    "realtime": "0",
                    "waittime": "00:10:53",
                    "updated_at": "2016-04-11 14:26:21",
                    "vehicle_id": "2662",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
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
                    "departure": "2016-04-11 14:45:35",
                    "departure_commande": "2016-04-11 14:44:47",
                    "departure_theorique": "2016-04-11 14:44:47",
                    "arrival": "2016-04-11 14:45:35",
                    "arrival_commande": "2016-04-11 14:44:47",
                    "arrival_theorique": "2016-04-11 14:44:47",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:19:13",
                    "updated_at": "2016-04-11 14:25:41",
                    "vehicle_id": "2660",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
                    "origin": "bdsi",
                },
            ],
        },
        {
            "name": "Lianes 4",
            "code": "04",
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
                    "departure": "2016-04-11 14:37:15",
                    "departure_commande": "2016-04-11 14:40:17",
                    "departure_theorique": "2016-04-11 14:40:17",
                    "arrival": "2016-04-11 14:40:17",
                    "arrival_commande": "2016-04-11 14:40:17",
                    "arrival_theorique": "2016-04-11 14:40:17",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:10:53",
                    "updated_at": "2016-04-11 14:26:21",
                    "vehicle_id": "2662",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
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
                    "departure": "2016-04-11 14:49:35",
                    "departure_commande": "2016-04-11 14:49:35",
                    "departure_theorique": "2016-04-11 14:49:35",
                    "arrival": "2016-04-11 14:49:35",
                    "arrival_commande": "2016-04-11 14:49:35",
                    "arrival_theorique": "2016-04-11 14:49:35",
                    "comment": "",
                    "realtime": "1",
                    "waittime": "00:19:13",
                    "updated_at": "2016-04-11 14:25:41",
                    "vehicle_id": "2660",
                    "vehicle_position_updated_at": "2016-04-11 14:26:21",
                    "origin": "bdsi",
                },
            ],
        },
    ]


class MockRoutePoint(object):
    def __init__(self, *args, **kwars):
        self._hardcoded_line_code = kwars['line_code']
        self._hardcoded_stop_id = kwars['stop_id']

    def fetch_stop_id(self, object_id_tag):
        return self._hardcoded_stop_id

    def fetch_line_id(self, object_id_tag):
        return self._hardcoded_line_code


def next_passage_for_route_point_test(mock_good_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a good response, we should get some next_passages
    """
    cleverage = Cleverage(
        id='tata', timezone='UTC', service_url='http://bob.com/', service_args={'a': 'bobette', 'b': '12'}
    )

    mock_requests = MockRequests({'http://bob.com/stop_tutu': (mock_good_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='stop_tutu')

    with mock.patch('requests.get', mock_requests.get):
        passages = cleverage.next_passage_for_route_point(route_point)

        assert len(passages) == 2

        assert passages[0].datetime == datetime.datetime(2016, 4, 11, 14, 37, 15, tzinfo=pytz.UTC)
        assert passages[0].is_real_time
        assert passages[1].datetime == datetime.datetime(2016, 4, 11, 14, 45, 35, tzinfo=pytz.UTC)
        assert passages[1].is_real_time


def next_passage_for_route_point_local_timezone_test(mock_good_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a good response, we should get some next_passages
    """
    cleverage = Cleverage(
        id='tata',
        timezone='Europe/Paris',
        service_url='http://bob.com/',
        service_args={'a': 'bobette', 'b': '12'},
    )

    mock_requests = MockRequests({'http://bob.com/stop_tutu': (mock_good_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='stop_tutu')

    with mock.patch('requests.get', mock_requests.get):
        passages = cleverage.next_passage_for_route_point(route_point)

        assert len(passages) == 2

        assert passages[0].datetime == datetime.datetime(2016, 4, 11, 12, 37, 15, tzinfo=pytz.UTC)
        assert passages[0].is_real_time
        assert passages[1].datetime == datetime.datetime(2016, 4, 11, 12, 45, 35, tzinfo=pytz.UTC)
        assert passages[1].is_real_time


def next_passage_for_empty_response_test(mock_empty_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a empty response, we should get no departure
    """
    cleverage = Cleverage(
        id='tata', timezone='UTC', service_url='http://bob.com/', service_args={'a': 'bobette', 'b': '12'}
    )

    mock_requests = MockRequests({'http://bob.com/stop_tutu': (mock_empty_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='stop_tutu')

    with mock.patch('requests.get', mock_requests.get):
        passages = cleverage.next_passage_for_route_point(route_point)

        assert passages is None


def next_passage_for_missing_line_response_test(mock_missing_line_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a response without wanted line  we should get no departure
    """
    cleverage = Cleverage(
        id='tata', timezone='UTC', service_url='http://bob.com/', service_args={'a': 'bobette', 'b': '12'}
    )

    mock_requests = MockRequests({'http://bob.com/stop_tutu': (mock_missing_line_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='stop_tutu')

    with mock.patch('requests.get', mock_requests.get):
        passages = cleverage.next_passage_for_route_point(route_point)

        assert passages is None


def next_passage_with_theoric_time_response_test(mock_theoric_response):
    """
    test the whole next_passage_for_route_point
    mock the http call to return a response with a theoric time we should get one departure
    """
    cleverage = Cleverage(
        id='tata', timezone='UTC', service_url='http://bob.com/', service_args={'a': 'bobette', 'b': '12'}
    )

    mock_requests = MockRequests({'http://bob.com/stop_tutu': (mock_theoric_response, 200)})

    route_point = MockRoutePoint(line_code='05', stop_id='stop_tutu')

    with mock.patch('requests.get', mock_requests.get):
        passages = cleverage.next_passage_for_route_point(route_point)

        assert len(passages) == 2

        assert passages[0].datetime == datetime.datetime(2016, 4, 11, 14, 37, 15, tzinfo=pytz.UTC)
        assert not passages[0].is_real_time
        assert passages[1].datetime == datetime.datetime(2016, 4, 11, 14, 45, 35, tzinfo=pytz.UTC)
        assert passages[1].is_real_time


def status_test():
    cleverage = Cleverage(
        id=u'tata-é$~#@"*!\'`§èû',
        timezone='Europe/Paris',
        service_url='http://bob.com/',
        service_args={'a': 'bobette', 'b': '12'},
    )
    status = cleverage.status()
    assert status['id'] == u"tata-é$~#@\"*!'`§èû"
