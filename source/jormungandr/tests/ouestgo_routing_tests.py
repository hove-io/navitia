# -*- coding: utf-8 -*-
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
from __future__ import absolute_import, print_function, unicode_literals, division
import pytest

from jormungandr.tests.utils_test import MockResponse
from tests.check_utils import get_not_null, get_links_dict
from tests.tests_mechanism import dataset, NewDefaultScenarioAbstractTestFixture

DUMMY_OUESTGO_FEED_PUBLISHER = {'id': '10', 'name': 'ouestgo', 'license': 'I dunno', 'url': 'http://ouest.tf'}

MOCKED_INSTANCE_CONF = {
    'scenario': 'new_default',
    'instance_config': {
        'ridesharing': [
            {
                "args": {
                    "service_url": "http://ouestgo.fr",
                    "api_key": "key",
                    "network": "Super Covoit",
                    "timeout": 15,
                    "driver_state": 1,
                    "passenger_state": 0,
                    "feed_publisher": DUMMY_OUESTGO_FEED_PUBLISHER,
                },
                "class": "jormungandr.scenarios.ridesharing.ouestgo.Ouestgo",
            }
        ]
    },
}


OUESTGO_RESPONSE = [
    {
        "journeys": {
            "uuid": 910,
            "operator": "Ouestgo",
            "origin": "test.ouestgo.mobicoop.io",
            "url": "https://test.ouestgo.mobicoop.io/covoiturage/rdex/8e975ea22a915a2d7c01",
            "driver": {
                "uuid": 2,
                "alias": "Colin P.",
                "image": None,
                "gender": "male",
                "seats": 3,
                "state": 1
            },
            "passenger": {
                "uuid": 2,
                "alias": "Colin P.",
                "image": None,
                "gender": "male",
                "persons": 0,
                "state": 0
            },
            "from": {
                "address": None,
                "city": "Nancy",
                "postalcode": "54100",
                "country": "France",
                "latitude": "0.0000898312",
                "longitude": "0.0000898312"
            },
            "to": {
                "address": None,
                "city": "Amnéville",
                "postalcode": "57360",
                "country": "France",
                "latitude": "0.00071865",
                "longitude": "0.00188646"
            },
            "distance": 18869,
            "duration": 1301,
            "route": None,
            "number_of_waypoints": 0,
            "waypoints": {},
            "cost": {
                "variable": "1"
            },
            "details": None,
            "vehicle": None,
            "frequency": "punctual",
            "type": "one-way",
            "real_time": None,
            "stopped": None,
            "days": {
                "monday": 0,
                "tuesday": 0,
                "wednesday": 0,
                "thursday": 0,
                "friday": 1,
                "saturday": 0,
                "sunday": 0
            },
            "outward": {
                "mindate": "2012-06-14",
                "maxdate": "2012-06-14",
                "monday": None,
                "tuesday": None,
                "wednesday": None,
                "thursday":  {
                    "mintime": "08:55:00",
                    "maxtime": "09:15:00"
                },
                "friday": None,
                "saturday": None,
                "sunday": None
            },
            "return": None
        }
    }
]


def mock_ouestgo(_, params, headers):
    return MockResponse(OUESTGO_RESPONSE, 200)


@pytest.fixture(scope="function", autouse=True)
def mock_http_ouestgo(monkeypatch):
    monkeypatch.setattr(
        'jormungandr.scenarios.ridesharing.ouestgo.Ouestgo._call_service', mock_ouestgo
    )


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF})
class TestOuestgo(NewDefaultScenarioAbstractTestFixture):
    """
    Integration test with Ouestgo
    Note: '&forbidden_uris[]=PM' used to avoid line 'PM' and it's vj=vjPB in /journeys
    """

    def test_basic_ride_sharing(self):
        """
        test ridesharing_journeys details
        """
        q = (
            "journeys?from=0.0000898312;0.0000698312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&forbidden_uris[]=PM&_min_ridesharing=0".format(
                first='ridesharing', last='walking'
            )
        )
        response = self.query_region(q)
        self.is_valid_journey_response(response, q, check_journey_links=False)

        # Check links: ridesharing_journeys
        links = get_links_dict(response)
        link = links["ridesharing_journeys"]
        assert link["rel"] == "ridesharing_journeys"
        assert link["type"] == "ridesharing_journeys"
        assert link["href"].startswith("http://localhost/v1/coverage/main_routing_test/")
        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 1
        tickets = response.get('tickets')
        assert len(tickets) == 1
        assert tickets[0].get('cost').get('currency') == 'centime'
        assert tickets[0].get('cost').get('value') == '100.0'
        ticket = tickets[0]

        ridesharing_kraken = journeys[0]
        assert 'ridesharing' in ridesharing_kraken['tags']
        assert 'non_pt' in ridesharing_kraken['tags']
        assert ridesharing_kraken.get('type') == 'best'
        assert ridesharing_kraken.get('durations').get('ridesharing') > 0
        assert ridesharing_kraken.get('durations').get('total') == ridesharing_kraken['durations']['ridesharing']
        assert ridesharing_kraken.get('distances').get('ridesharing') > 0

        rs_sections = ridesharing_kraken.get('sections')
        assert len(rs_sections) == 1
        assert rs_sections[0].get('mode') == 'ridesharing'
        assert rs_sections[0].get('type') == 'street_network'

        sections = ridesharing_kraken.get('sections')

        rs_journeys = sections[0].get('ridesharing_journeys')
        assert len(rs_journeys) == 1
        assert rs_journeys[0].get('distances').get('ridesharing') == 18869
        assert rs_journeys[0].get('durations').get('walking') == 2
        assert rs_journeys[0].get('durations').get('ridesharing') == 1301
        assert 'ridesharing' in rs_journeys[0].get('tags')
        rsj_sections = rs_journeys[0].get('sections')
        assert len(rsj_sections) == 3

        assert rsj_sections[0].get('type') == 'crow_fly'
        assert rsj_sections[0].get('mode') == 'walking'
        assert rsj_sections[0].get('duration') == 2
        assert (rsj_sections[0].get('departure_date_time') == '20120614T085458')
        assert rsj_sections[0].get('arrival_date_time') == '20120614T085500'

        assert rsj_sections[1].get('type') == 'ridesharing'
        assert rsj_sections[1].get('duration') == 1301
        assert rsj_sections[1].get('departure_date_time') == '20120614T085500'
        assert rsj_sections[1].get('arrival_date_time') == '20120614T091641'

        rsj_info = rsj_sections[1].get('ridesharing_informations')
        assert rsj_info.get('network') == 'Super Covoit'
        assert rsj_info.get('operator') == 'ouestgo'
        assert rsj_info.get('seats').get('available') == 3

        assert 'total' not in rsj_info.get('seats')

        rsj_links = rsj_sections[1].get('links')
        assert len(rsj_links) == 2
        assert rsj_links[0].get('rel') == 'ridesharing_ad'
        assert rsj_links[0].get('type') == 'ridesharing_ad'

        assert rsj_links[1].get('rel') == 'tickets'
        assert rsj_links[1].get('type') == 'ticket'
        assert rsj_links[1].get('id') == ticket['id']
        assert ticket['links'][0]['id'] == rsj_sections[1]['id']
        assert rs_journeys[0].get('fare').get('total').get('value') == tickets[0].get('cost').get('value')

        assert rsj_sections[2].get('type') == 'crow_fly'
        assert rsj_sections[2].get('mode') == 'walking'
        assert rsj_sections[2].get('duration') == 0
        assert rsj_sections[2].get('departure_date_time') == '20120614T091641'
        assert rsj_sections[2].get('arrival_date_time') == '20120614T091641'

        fps = response['feed_publishers']
        assert len(fps) == 2

        def equals_to_dummy_fp(fp):
            return fp == DUMMY_OUESTGO_FEED_PUBLISHER

        assert any(equals_to_dummy_fp(fp) for fp in fps)
