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
import datetime
from dateutil.parser import parse
import mock
import pytz
from jormungandr.realtime_schedule.synthese import Synthese
import validators
from jormungandr.realtime_schedule.tests.utils import MockRoutePoint, _timestamp


def make_url_test():
    synthese = Synthese(id='tata', timezone='Europe/Paris', service_url='http://bob.com/')

    url = synthese._make_url(MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu'))

    # it should be a valid url
    assert validators.url(url)

    assert url.startswith('http://bob.com/')
    assert 'SERVICE=tdg' in url
    assert 'roid=stop_tutu' in url
    assert 'rn=' not in url  # we should not have any limit
    assert 'date=' not in url  # we should not have any custom date


def make_url_date_and_count_test():
    synthese = Synthese(id='tata', timezone='UTC', service_url='http://bob.com/')

    url = synthese._make_url(
        MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu'),
        count=2,
        from_dt=_timestamp("12:00"),
    )

    # it should be a valid url
    assert validators.url(url)

    assert url.startswith('http://bob.com/')
    assert 'SERVICE=tdg' in url
    assert 'roid=stop_tutu' in url
    # we should have the param we provided
    assert 'rn=2' in url
    assert 'date=2016-02-07 12:00' in url


def make_url_invalid_code_test():
    """
    test make_url when RoutePoint does not have a mandatory code

    we should not get any url
    """
    synthese = Synthese(id='tata', timezone='Europe/Paris', service_url='http://bob.com/')

    url = synthese._make_url(MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id=None))

    assert url is None


class MockResponse(object):
    def __init__(self, data, status_code, url, *args, **kwargs):
        self.content = data
        self.text = data
        self.status_code = status_code
        self.url = url


def mock_good_response():
    return """
    <timeTable>
      <journey routeId="route_tata" dateTime="2016-Mar-29 13:37:00" realTime="yes">
        <stop id="stop_tutu"/>
      </journey>
      <journey routeId="route_tata" dateTime="2016-Mar-29 13:47:00" realTime="yes">
        <stop id="stop_tutu"/>
      </journey>
      <journey routeId="route_toto" dateTime="2016-Mar-29 13:48:00" realTime="yes">
        <stop id="stop_tutu"/>
      </journey>
      <journey routeId="route_tata" dateTime="2016-Mar-29 13:57:00" realTime="yes">
        <stop id="stop_tutu"/>
      </journey>
    </timeTable>
    """


class MockRequests(object):
    def __init__(self, responses):
        self.responses = responses

    def get(self, url, *args, **kwargs):
        return MockResponse(self.responses[url][0], self.responses[url][1], url)


def next_passage_for_route_point_test():
    """
    test the whole next_passage_for_route_point
    mock the http call to return a good response, we should get some next_passages
    """
    synthese = Synthese(id='tata', timezone='UTC', service_url='http://bob.com/')

    mock_requests = MockRequests({'http://bob.com/?SERVICE=tdg&roid=stop_tutu': (mock_good_response(), 200)})

    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')

    with mock.patch('requests.get', mock_requests.get):
        passages = synthese.next_passage_for_route_point(route_point)

        assert len(passages) == 3

        assert passages[0].datetime == datetime.datetime(2016, 3, 29, 13, 37, tzinfo=pytz.UTC)
        assert passages[1].datetime == datetime.datetime(2016, 3, 29, 13, 47, tzinfo=pytz.UTC)
        assert passages[2].datetime == datetime.datetime(2016, 3, 29, 13, 57, tzinfo=pytz.UTC)


def next_passage_for_route_point_failure_test():
    """
    test the whole next_passage_for_route_point

    the timeo's response is in error (status = 404), we should get 'None'
    """
    synthese = Synthese(id='tata', timezone='UTC', service_url='http://bob.com/')

    mock_requests = MockRequests({'http://bob.com/?SERVICE=tdg&roid=stop_tutu': (mock_good_response(), 404)})

    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')

    with mock.patch('requests.get', mock_requests.get):
        passages = synthese.next_passage_for_route_point(route_point)

        assert passages is None


def status_test():
    synthese = Synthese(id=u'tata-é$~#@"*!\'`§èû', timezone='UTC', service_url='http://bob.com/')
    status = synthese.status()
    assert status['id'] == u"tata-é$~#@\"*!'`§èû"
