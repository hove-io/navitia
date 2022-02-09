# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import, print_function, unicode_literals, division
import pytest

from jormungandr.tests.utils_test import MockResponse
from tests.check_utils import get_not_null, s_coord, r_coord
from tests.tests_mechanism import dataset, NewDefaultScenarioAbstractTestFixture


def sortFeeds(elem):
    return elem.get('id')


MOCKED_INSTANCE_CONF = {
    'scenario': 'distributed',
    'instance_config': {
        'street_network': [
            {
                "args": {
                    "apiKey": "bob_id",
                    "service_base_url": "router.hereapi.com/v8",
                    "language": "french",
                    "timeout": 20,
                },
                "modes": ["car", "car_no_park"],
                "class": "jormungandr.street_network.here.Here",
            },
            {"modes": ["walking", "bike", "bss"], "class": "jormungandr.street_network.kraken.Kraken"},
        ]
    },
}

QUERY_DATETIME_STR = "20120614T070000"


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF})
class TestHere(NewDefaultScenarioAbstractTestFixture):
    """
    Integration test with HERE
    """

    def test_feed_publishers(self):

        # With direct path
        q = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&first_section_mode[]=car_no_park"
            "&last_section_mode[]=car_no_park&debug=true&min_nb_journeys=0".format(
                from_coord=s_coord, to_coord=r_coord, datetime=QUERY_DATETIME_STR
            )
        )
        response = self.query_region(q)

        feeds = get_not_null(response, 'feed_publishers')
        assert len(feeds) == 2
        feeds.sort(key=sortFeeds)

        assert feeds[0].get('id') == 'builder'
        assert feeds[0].get('name') == 'routing api data'
        assert feeds[0].get('license') == 'ODBL'
        assert feeds[0].get('url') == 'www.canaltp.fr'

        assert feeds[1].get('id') == 'here'
        assert feeds[1].get('name') == 'here'
        assert feeds[1].get('license') == 'Private'
        assert feeds[1].get('url') == 'https://developer.here.com/terms-and-conditions'

        # Without direct path
        q = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&first_section_mode[]=car_no_park"
            "&last_section_mode[]=car_no_park&debug=true&min_nb_journeys=0&direct_path=none".format(
                from_coord=s_coord, to_coord=r_coord, datetime=QUERY_DATETIME_STR
            )
        )
        response = self.query_region(q)

        feeds = get_not_null(response, 'feed_publishers')
        assert len(feeds) == 2
        feeds.sort(key=sortFeeds)

        assert feeds[0].get('id') == 'builder'
        assert feeds[0].get('name') == 'routing api data'
        assert feeds[0].get('license') == 'ODBL'
        assert feeds[0].get('url') == 'www.canaltp.fr'

        assert feeds[1].get('id') == 'here'
        assert feeds[1].get('name') == 'here'
        assert feeds[1].get('license') == 'Private'
        assert feeds[1].get('url') == 'https://developer.here.com/terms-and-conditions'
