# coding=utf-8
# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
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


@dataset({"main_routing_test": {}})
class TestFreeRadius(AbstractTestFixture):

    def test_free_radius_distributed(self):

        # Query journeys without free radius
        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000&override_scenario=experimental&')

        assert(r['journeys'][0]['sections'][0]['type'] == 'street_network')
        assert (r['journeys'][0]['sections'][0]['duration'] != 0)

        # Query journeys with free radius
        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000&free_radius_from=200&_override_scenario=experimental&')

        assert(r['journeys'][0]['sections'][0]['type'] == 'crow_fly')
        assert (r['journeys'][0]['sections'][0]['duration'] == 0)
