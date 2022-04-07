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
from __future__ import absolute_import
from datetime import datetime
import pytz
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy
from jormungandr.schedule import RealTimePassage, RoutePoint
from jormungandr.utils import date_to_timestamp as d2t
from six.moves import map
from navitiacommon.type_pb2 import Route


class CustomProxy(RealtimeProxy):
    """
    mock proxy that return a fixed next passages
    """

    def __init__(self, passages):
        self.hard_coded_passages = passages
        self.rt_system_id = 'test'

    def status(self):
        return None

    def _get_next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=None
    ):
        return self.hard_coded_passages


def dt(dt_str):
    t = datetime.strptime(dt_str, "%H:%M")
    return datetime(year=2016, month=1, day=2, hour=t.hour, minute=t.minute, second=t.second, tzinfo=pytz.UTC)


def passage(dt_str, **kwargs):
    return RealTimePassage(dt(dt_str), **kwargs)


def get_dt(p):
    return p.datetime


def test_filter_items_under_the_limit():
    proxy = CustomProxy([passage("10:00"), passage("11:00")])

    r = proxy.next_passage_for_route_point(None, count=3)

    assert list(map(get_dt, r)) == [dt("10:00"), dt("11:00")]


def filter_items_test():
    proxy = CustomProxy([passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")])

    r = proxy.next_passage_for_route_point(None, count=2)

    assert list(map(get_dt, r)) == [dt("10:00"), dt("11:00")]


def filter_no_filter_test():
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None)
    assert list(map(get_dt, r)) == [dt("10:00"), dt("11:00"), dt("12:00"), dt("13:00")]


def filter_filter_dt_test(mocker):
    mocker.patch('jormungandr.utils.get_timezone', return_value=pytz.timezone('UTC'))
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, from_dt=d2t(dt("12:00")))
    assert list(map(get_dt, r)) == [dt("12:00"), dt("13:00")]


def filter_filter_dt_over_all_test(mocker):
    mocker.patch('jormungandr.utils.get_timezone', return_value=pytz.timezone('UTC'))
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, from_dt=d2t(dt("17:00")))
    assert r is None


def filter_filter_dt_and_item_test(mocker):
    mocker.patch('jormungandr.utils.get_timezone', return_value=pytz.timezone('UTC'))
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, count=1, from_dt=d2t(dt("11:59")))
    assert list(map(get_dt, r)) == [dt("12:00")]


def filter_filter_dt_all_test(mocker):
    mocker.patch('jormungandr.utils.get_timezone', return_value=pytz.timezone('UTC'))
    """the filter will filter all, so we should not get an empty list but None"""
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, count=1, from_dt=d2t(dt("15:00")))
    assert r is None


def filter_filter_dt_duration_test(mocker):
    mocker.patch('jormungandr.utils.get_timezone', return_value=pytz.timezone('UTC'))
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, from_dt=d2t(dt("10:00")), duration=3600)
    assert list(map(get_dt, r)) == [dt("10:00"), dt("11:00")]


def test_route_point_get_code():
    r = Route()
    c = r.codes.add()
    c.value = "foo"
    c.type = "source"

    c = r.codes.add()
    c.value = "bar"
    c.type = "extcode"

    assert RoutePoint._get_all_codes(r, "source") == ["foo"]
    assert RoutePoint._get_all_codes(r, "extcode") == ["bar"]
    # add a duplicate, this happens in real life...
    c = r.codes.add()
    c.value = "foo"
    c.type = "source"
    assert RoutePoint._get_all_codes(r, "source") == ["foo"]

    # source has two different values(think fusion)
    c = r.codes.add()
    c.value = "foo3"
    c.type = "source"
    assert set(RoutePoint._get_all_codes(r, "source")) == set(["foo", "foo3"])
