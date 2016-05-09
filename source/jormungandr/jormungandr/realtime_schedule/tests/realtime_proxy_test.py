# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
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
from datetime import datetime
from nose.tools.trivial import eq_
import pytz
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy
from jormungandr.schedule import RealTimePassage


class CustomProxy(RealtimeProxy):
    """
    mock proxy that return a fixed next passages
    """
    def __init__(self, passages):
        self.hard_coded_passages = passages

    def status(self):
        return None

    def _get_next_passage_for_route_point(self, route_point, items_per_schedule=None, from_dt=None):
        return self.hard_coded_passages


def dt(dt_str):
    t = datetime.strptime(dt_str, "%H:%M")
    return datetime(year=2016, month=1, day=2, hour=t.hour, minute=t.minute, second=t.second, tzinfo=pytz.UTC)


def passage(dt_str, **kwargs):
    return RealTimePassage(dt(dt_str), **kwargs)


def get_dt(p):
    return p.datetime


def filter_items_test_under_the_limit():
    proxy = CustomProxy([passage("10:00"), passage("11:00")])

    r = proxy.next_passage_for_route_point(None, items_per_schedule=3, from_dt=None)

    assert map(get_dt, r) == [dt("10:00"), dt("11:00")]


def filter_items_test():
    proxy = CustomProxy([passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")])

    r = proxy.next_passage_for_route_point(None, items_per_schedule=2, from_dt=None)

    assert map(get_dt, r) == [dt("10:00"), dt("11:00")]


def filter_no_filter():
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, items_per_schedule=None, from_dt=None)
    assert map(get_dt, r) == [dt("10:00"), dt("11:00"), dt("12:00"), dt("13:00")]


def filter_filter_dt():
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, items_per_schedule=None, from_dt=dt("12:00"))
    assert map(get_dt, r) == [dt("12:00"), dt("13:00")]


def filter_filter_dt_over_all():
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, items_per_schedule=None, from_dt=dt("17:00"))
    assert r == []


def filter_filter_dt_and_item():
    passages = [passage("10:00"), passage("11:00"), passage("12:00"), passage("13:00")]
    proxy = CustomProxy(passages)

    r = proxy.next_passage_for_route_point(None, items_per_schedule=1, from_dt=dt("11:59"))
    assert map(get_dt, r) == [dt("12:00")]
