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
from __future__ import absolute_import, print_function, unicode_literals, division
import logging

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


@dataset({"line_sections_test": {}})
class TestLineSections(AbstractTestFixture):

    def default_query(self, q, **kwargs):
        """query navitia with a current date in the publication period of the impacts"""
        return self.query_region('{}?_current_datetime=20170101T100000'.format(q), **kwargs)

    def has_disruption(self, q, object_get, disruption_uri, **kwargs):
        r = self.default_query(q, **kwargs)
        return has_disruption(r, object_get, disruption_uri)

    def has_disruption(self, object_get, disruption_uri):
        """Little helper calling the detail of an object and checking it's disruptions"""
        r = self.default_query('{col}/{uri}'.format(col=object_get.collection, uri=object_get.uri))
        return has_disruption(r, object_get, disruption_uri)

    def test_on_stop_points(self):
        """
        the line section disruption should be linked to the impacted stop_points
        """
        assert not self.has_disruption(ObjGetter('stop_points', 'A_1'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_points', 'B_1'), 'line_section_on_line_1')
        assert self.has_disruption(ObjGetter('stop_points', 'C_1'), 'line_section_on_line_1')
        assert self.has_disruption(ObjGetter('stop_points', 'D_1'), 'line_section_on_line_1')
        assert self.has_disruption(ObjGetter('stop_points', 'E_1'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_points', 'F_1'), 'line_section_on_line_1')

    def test_on_vehicle_journeys(self):
        """
        the line section disruption should be linked to the impacted vehicle journeys
        """
        assert self.has_disruption(ObjGetter('vehicle_journeys', 'vj:1:1'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('vehicle_journeys', 'vj:1:2'), 'line_section_on_line_1')
