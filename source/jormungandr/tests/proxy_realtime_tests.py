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

from tests_mechanism import AbstractTestFixture, dataset

from jormungandr.realtime_schedule import realtime_proxy, realtime_proxy_manager

MOCKED_PROXY_CONF = (' [{"id": "proxy_id",\n'
                     ' "class": "proxy_realtime_tests.MockedTestProxy",\n'
                     ' "args": { } }]')


class MockedTestProxy(realtime_proxy.RealtimeProxy):
    def __init__(self, id):
        self.id = id

    def next_passage_for_route_point(self, route_point):
        pass


@dataset({"departure_board_test": {"proxy_conf": MOCKED_PROXY_CONF}})
class TestDepartures(AbstractTestFixture):

    def test_departures(self):
        # TODO
        assert 1
