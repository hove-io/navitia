# coding=utf-8
# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from __future__ import absolute_import, print_function, division
import datetime
import mock
from time import sleep
from jormungandr.realtime_schedule.timeo import Timeo
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxyError
import validators
from jormungandr.realtime_schedule.tests.utils import MockRoutePoint, _timestamp, _dt
from jormungandr.tests.utils_test import MockRequests
from pytest import raises
from six.moves import range


def make_url_test():
    timeo = Timeo(id='tata', timezone='Europe/Paris', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    url = timeo._make_url(MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu'))

    # it should be a valid url
    assert validators.url(url)

    assert url.startswith('http://bob.com')
    assert 'a=bobette' in url
    assert 'b=12' in url
    assert 'StopTimeoCode=stop_tutu' in url
    assert 'LineTimeoCode=line_toto' in url
    assert 'Way=route_tata' in url
    # we did not provide a datetime, we should not have it in the url
    assert 'NextStopReferenceTime' not in url
    # we did not provide a count, we should have the default value
    assert 'NextStopTimeNumber=5' in url


def make_url_count_and_dt_test():
    """
    same as make_url_test but with a count and a from_dt
    """
    timeo = Timeo(id='tata', timezone='UTC', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'},
                  from_datetime_step=30)

    url = timeo._make_url(MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu'),
                          count=2, from_dt=_timestamp("12:00"))

    # it should be a valid url
    assert validators.url(url)

    assert url.startswith('http://bob.com')
    assert 'a=bobette' in url
    assert 'b=12' in url
    assert 'StopTimeoCode=stop_tutu' in url
    assert 'LineTimeoCode=line_toto' in url
    assert 'Way=route_tata' in url
    # we should have the param we provided
    assert 'NextStopTimeNumber=2' in url
    assert 'NextStopReferenceTime=2016-02-07T12:00:00' in url

    #same as before we only update the seconds of dt
    url = timeo._make_url(MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu'),
            count=2, from_dt=_timestamp("12:00:04"))
    assert 'NextStopReferenceTime=2016-02-07T12:00:00' in url

    url = timeo._make_url(MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu'),
            count=2, from_dt=_timestamp("12:00:59"))
    assert 'NextStopReferenceTime=2016-02-07T12:00:30' in url


def make_url_invalid_code_test():
    """
    test make_url when RoutePoint does not have a mandatory code

    we should not get any url
    """
    timeo = Timeo(id='tata', timezone='Europe/Paris', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    url = timeo._make_url(MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id=None))

    assert url is None


def mock_good_timeo_response():
    mock_response = {
        "CorrelationID": "GetNextStopTimesResponse-16022016 15:30",
        "MessageResponse": [{"ResponseCode": "0", "ResponseComment": "success"}],
        "StopTimesResponse": [
            {
                "StopID": "StopPoint_OLS01070201",
                "StopTimeoCode": "3331",
                "StopLongName": "André Malraux",
                "StopShortName": "André Malraux",
                "StopVocalName": "André Malraux",
                "ReferenceTime": "15:30:06",
                "NextStopTimesMessage": {
                    "LineID": "line_id",
                    "LineTimeoCode": "line_toto",
                    "Way": "route_tata",
                    "LineMainDirection": "Bicharderies",
                    "NextExpectedStopTime": [
                        {
                            "NextStop": "15:40:04",
                            "Destination": "A direction",
                            "Terminus": "StopPoint_Bob"
                        },
                        {
                            "NextStop": "15:55:04",
                            "Destination": "A direction",
                            "Terminus": "StopPoint_Bob"
                        },
                        {
                            "NextStop": "16:10:04",
                            "Destination": "A direction",
                            "Terminus": "StopPoint_Bob"
                        }
                    ]
                }
            }
        ]
    }
    return mock_response


def get_passages_test():
    """
    test the next departures get from timeo's response

    the timezone is UTC for convenience
    """
    timeo = Timeo(id='tata', timezone='UTC', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    mock_response = mock_good_timeo_response()

    # we need to mock the datetime.now() because for timeo we don't have a choice but to combine
    # the current day with the timeo's response
    with mock.patch('jormungandr.realtime_schedule.timeo.Timeo._get_direction_name', lambda timeo, **kwargs: None):
        passages = timeo._get_passages(mock_response, current_dt=_dt("02:02"))

        assert len(passages) == 3

        assert passages[0].datetime == _dt('15:40:04')
        assert passages[1].datetime == _dt('15:55:04')
        assert passages[2].datetime == _dt('16:10:04')


def get_passages_no_passages_test():
    """
    test that if timeo returns 0 response, we return an empty list
    """
    timeo = Timeo(id='tata', timezone='UTC', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    mock_response = {
        "CorrelationID": "GetNextStopTimesResponse-16022016 15:30",
        "MessageResponse": [{"ResponseCode": "0", "ResponseComment": "success"}],
        "StopTimesResponse": [
            {
                "StopID": "StopPoint_OLS01070201",
                "StopTimeoCode": "3331",
                "StopLongName": "André Malraux",
                "StopShortName": "André Malraux",
                "StopVocalName": "André Malraux",
                "ReferenceTime": "15:30:06",
                "NextStopTimesMessage": {
                    "LineID": "line_id",
                    "LineTimeoCode": "line_toto",
                    "Way": "route_tata",
                    "LineMainDirection": "Bicharderies",
                    "NextExpectedStopTime": []  # emtpy list
                }
            }
        ]
    }

    # we need to mock the datetime.now() because for timeo we don't have a choice but to combine
    # the current day with the timeo's response
    with mock.patch('jormungandr.realtime_schedule.timeo.Timeo._get_direction_name', lambda timeo, **kwargs: None):
        passages = timeo._get_passages(mock_response, current_dt=_dt("02:02"))

        assert len(passages) == 0


def get_passages_wrong_response_test():
    """
    test that if timeo returns a not valid response, we get None (and not an empty list)
    """
    timeo = Timeo(id='tata', timezone='UTC', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    mock_response = {
        "CorrelationID": "GetNextStopTimesResponse-16022016 15:30",
        "MessageResponse": [{"ResponseCode": "0", "ResponseComment": "success"}]
    }

    # we need to mock the datetime.now() because for timeo we don't have a choice but to combine
    # the current day with the timeo's response
    with mock.patch('jormungandr.realtime_schedule.timeo.Timeo._get_direction_name', lambda timeo, **kwargs: None),\
            raises(RealtimeProxyError):
        passages = timeo._get_passages(mock_response, current_dt=_dt("02:02"))


def next_passage_for_route_point_test():
    """
    test the whole next_passage_for_route_point
    mock the http call to return a good timeo response, we should get some next_passages
    """
    timeo = Timeo(id='tata', timezone='UTC', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    mock_requests = MockRequests({
        'http://bob.com/tata?a=bobette&b=12&StopDescription=?StopTimeType=TR&LineTimeoCode'
        '=line_toto&Way=route_tata&NextStopTimeNumber=5&StopTimeoCode=stop_tutu;':
        (mock_good_timeo_response(), 200)
    })

    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')
    # we mock the http call to return the hard coded mock_response
    with mock.patch('requests.get', mock_requests.get):
        with mock.patch('jormungandr.realtime_schedule.timeo.Timeo._get_direction_name', lambda timeo, **kwargs: None):
            passages = timeo.next_passage_for_route_point(route_point, current_dt=_dt("02:02"))

            assert len(passages) == 3

            assert passages[0].datetime == _dt('15:40:04')
            assert passages[1].datetime == _dt('15:55:04')
            assert passages[2].datetime == _dt('16:10:04')


def next_passage_for_route_point_timeo_failure_test():
    """
    test the whole next_passage_for_route_point

    the timeo's response is in error (status = 404), we should get 'None'
    """
    timeo = Timeo(id='tata', timezone='UTC', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    mock_requests = MockRequests({
        'http://bob.com/tata?a=bobette&b=12&StopDescription=?StopTimeoCode=stop_tutu&LineTimeoCode'
        '=line_toto&Way=route_tata&NextStopTimeNumber=5&StopTimeType=TR;':
        (mock_good_timeo_response(), 404)
    })

    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')
    with mock.patch('requests.get', mock_requests.get):
        passages = timeo.next_passage_for_route_point(route_point, current_dt=_dt("02:02"))

        assert passages is None


def timeo_circuit_breaker_test():
    """
    Test the circuit breaker around Timeo

    We test that after 2 fail by timeo, no more calls are done
    (we don't test the timeout, it's the 'circuit_breaker' library responsability)

    time fail on the first response, gives a good response afterward, then fail on all call

    There should be only 4 timeo calls
    """
    timeo = Timeo(id='tata', timezone='UTC', service_url='http://bob.com/tata',
                  service_args={'a': 'bobette', 'b': '12'})

    timeo.breaker.fail_max = 2
    timeo.breaker.reset_timeout = 99999

    good_response = {'all_is_ok'}, 200

    class Mocker:
        timeo_call = 0
        def get(self, *args, **kwargs):
            self.timeo_call += 1
            if self.timeo_call == 2:
                return good_response
            raise Exception('test error')

    m = Mocker()
    with mock.patch('requests.get', m.get):
        with raises(RealtimeProxyError):
            timeo._call_timeo('http://bob.com')
        assert good_response == timeo._call_timeo('http://bob.com')
        for _ in range(4):
            with raises(RealtimeProxyError):
                timeo._call_timeo('http://bob.com')


        # we should have called timeo only 4 times
        assert m.timeo_call == 4

def status_test():
    timeo = Timeo(id=u"tata-é$~#@\"*!'`§èû", timezone='UTC', service_url='http://bob.com/', service_args={'a': 'bobette', 'b': '12'})
    status = timeo.status()
    assert status['id'] == u'tata-é$~#@"*!\'`§èû'
