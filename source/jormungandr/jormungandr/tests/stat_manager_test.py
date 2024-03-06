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

from __future__ import absolute_import, unicode_literals

from jormungandr.stat_manager import StatManager
from navitiacommon import stat_pb2
from jormungandr import app
import unittest


class TestStatManager(unittest.TestCase):
    def setUp(self):
        self.old_save_stat = app.config['SAVE_STAT']
        app.config['SAVE_STAT'] = False

    def tearDown(self):
        app.config['SAVE_STAT'] = self.old_save_stat

    def test_fill_tags_empty_resp_journey(self):
        resp_journey = {}
        stat_manager = StatManager(auto_delete=True)
        stat_request = stat_pb2.StatRequest()
        stat_journey = stat_request.journeys.add()
        stat_manager.fill_tags(stat_journey, resp_journey)
        assert not stat_journey.tags

    def test_fill_tags_empty_tags(self):
        resp_journey = {"tags": []}
        stat_manager = StatManager(auto_delete=True)
        stat_request = stat_pb2.StatRequest()
        stat_journey = stat_request.journeys.add()
        stat_manager.fill_tags(stat_journey, resp_journey)
        assert not stat_journey.tags

    def test_fill_tags_one_tag(self):
        resp_journey = {"tags": ["olympics"]}
        stat_manager = StatManager(auto_delete=True)
        stat_request = stat_pb2.StatRequest()
        stat_journey = stat_request.journeys.add()
        stat_manager.fill_tags(stat_journey, resp_journey)
        assert len(stat_journey.tags) == 1
        assert stat_journey.tags[0] == stat_pb2.JOURNEY_TAG_OLYMPICS

    def test_fill_tags_many_tags(self):
        resp_journey = {"tags": ["olympics", "best_olympics", "ecologic"]}
        stat_manager = StatManager(auto_delete=True)
        stat_request = stat_pb2.StatRequest()
        stat_journey = stat_request.journeys.add()
        stat_manager.fill_tags(stat_journey, resp_journey)
        assert len(stat_journey.tags) == 3

    def test_fill_tags_unknown_tag(self):
        resp_journey = {"tags": ["toto"]}
        stat_manager = StatManager(auto_delete=True)
        stat_request = stat_pb2.StatRequest()
        stat_journey = stat_request.journeys.add()
        stat_manager.fill_tags(stat_journey, resp_journey)
        assert len(stat_journey.tags) == 1
        assert stat_journey.tags[0] == stat_pb2.JOURNEY_TAG_UNKNOWN

    def test_fill_tags_many_unknown_tag(self):
        resp_journey = {"tags": ["toto", "tata", "titi"]}
        stat_manager = StatManager(auto_delete=True)
        stat_request = stat_pb2.StatRequest()
        stat_journey = stat_request.journeys.add()
        stat_manager.fill_tags(stat_journey, resp_journey)
        assert len(stat_journey.tags) == 1
        assert stat_journey.tags[0] == stat_pb2.JOURNEY_TAG_UNKNOWN
