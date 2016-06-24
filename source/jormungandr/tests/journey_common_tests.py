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
from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr import i_manager
import mock




@dataset({"main_routing_test": {}})
class TestJourneyCommon(AbstractTestFixture):

        @mock.patch.object(i_manager, 'dispatch')
        def test_max_duration_to_pt(self, mock):
            q = "v1/coverage/main_routing_test/journeys?max_duration_to_pt=0&from=toto&to=tata"
            self.query(q)
            max_walking = i_manager.dispatch.call_args[0][0]["max_walking_duration_to_pt"]
            max_bike = i_manager.dispatch.call_args[0][0]['max_bike_duration_to_pt']
            max_bss = i_manager.dispatch.call_args[0][0]['max_bss_duration_to_pt']
            max_car = i_manager.dispatch.call_args[0][0]['max_car_duration_to_pt']
            assert max_walking == 0
            assert max_bike == 0
            assert max_bss == 0
            assert max_car == 0
