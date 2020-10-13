# -*- coding: utf-8 -*-
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
from tests.check_utils import get_not_null, s_coord, r_coord, journey_basic_query
from tests.tests_mechanism import dataset, NewDefaultScenarioAbstractTestFixture

DUMMY_INSTANT_SYSTEM_FEED_PUBLISHER = {'id': '42', 'name': '42', 'license': 'I dunno', 'url': 'http://w.tf'}

MOCKED_INSTANCE_CONF = {
    'scenario': 'new_default',
    'instance_config': {
        'ridesharing': [
            {
                "args": {
                    "service_url": "http://wtf",
                    "api_key": "key",
                    "network": "Super Covoit",
                    "rating_scale_min": 0,
                    "rating_scale_max": 5,
                    "crowfly_radius": 200,
                    "timeframe_duration": 1800,
                    "feed_publisher": DUMMY_INSTANT_SYSTEM_FEED_PUBLISHER,
                },
                "class": "jormungandr.scenarios.ridesharing.instant_system.InstantSystem",
            }
        ]
    },
}

INSTANT_SYSTEM_RESPONSE = {
    "total": 1,
    "journeys": [
        {
            "id": "4bcd0b9d-2c9d-42a2-8ffb-4508c952f4fb",
            "departureDate": "2017-12-25T08:07:59+01:00",
            "arrivalDate": "2017-12-25T08:25:36+01:00",
            "duration": 55,
            "distance": 224,
            "url": "https://jky8k.app.goo.gl/?efr=1&apn=com.is.android.rennes&ibi=&isi=&utm_campaign=KISIO&link=https%3A%2F%2Fwww.star.fr%2Fsearch%2F%3FfeatureName%3DsearchResultDetail%26networkId%3D33%26journeyId%3D4bcd0b9d-2c9d-42a2-8ffb-4508c952f4fb",
            "paths": [
                {
                    "mode": "RIDESHARINGAD",
                    "from": {"name": "", "lat": 0.0000898312, "lon": 0.0000898312},
                    "to": {"name": "", "lat": 0.00071865, "lon": 0.00188646},
                    "departureDate": "2017-12-25T08:07:59+01:00",
                    "arrivalDate": "2017-12-25T08:25:36+01:00",
                    "shape": "wosdH|ihIRVDTFDzBjPNhADJ\\`C?TJt@Hj@h@tDp@bFR?bAFRBZDR@JCL@~AJl@Df@DfBNv@B~@DjAFh@HXH~@VbEfANDh@PdAl@\\RdAZnBHpADvBDf@@d@Gv@S\\OlAOl@EbAHjAVNDd@Dd@Mt@u@FGrE{EtBaBr@zCp@dDd@~BRtAHj@X`BFXlAjDLd@v@dDXlAh@TVl@hBtIB`ANpAh@nBf@xATf@Xd@JFPD@JHRLBLKDBbCbBbBbBjApA?VHPPBL`@\\^|BrBDHJ`@AP?PDRFL\\TRAJGRD`Al@jBhA~BbBx@VfALl@PHVDHPFNCVNdCnBpHzDdB|AfAjAj@h@^d@jAhBhAvA?^BNFJPHPCFGVNpBhApBt@ZL|B^dCJfDAZFLRHBNEJQZIdUa@b@JJ`@TXTFTAPKNUH]nBGtOb@vDd@`C`ArAp@zAjAnBnBJJh@h@`_@l`@fIvIfMhNl@t@dAzBnAnDx@xDh@jFfBbRdAnMdBnSjB|JbDbIhMj[rN`_@nEfJzCxDrCtDl@pBDtE^Bn@?h@?t@IdAe@XUFIvBaBvBaBf@Wl@OdAEfAJJXJHJBLCbAbAx@j@fBn@p@X`HfDdAd@NB\\CBLJDFCBI?OGILYn@gDb@uAVe@\\_@jEgDlFgARElBa@|G}AxFwA`AWv@YNI~AaArAg@bEw@pA[t@Y`B{@~BmAtAo@fAk@TYBBH?DGBKTEd@U^QlBcA^QvEcCP@Le@Cm@Eo@Ia@AI",
                    "rideSharingAd": {
                        "id": "24bab9de-653c-4cc4-a947-389c59cf0423",
                        "type": "DRIVER",
                        "from": {"name": "9 Allee Rochester, Rennes", "lat": 0.0000998312, "lon": 0.0000998312},
                        "to": {"name": "2 Avenue Alphonse Legault, Bruz", "lat": 0.00081865, "lon": 0.00198646},
                        "user": {
                            "alias": "Jean P.",
                            "gender": "MALE",
                            "imageUrl": "https://dummyimage.com/128x128/C8E6C9/000.png&text=JP",
                            "rating": {"rate": 0, "count": 0},
                        },
                        "price": {"amount": 170.0, "currency": "EUR"},
                        "vehicle": {"availableSeats": 4},
                    },
                }
            ],
        }
    ],
    "url": "https://jky8k.app.goo.gl/?efr=1&apn=com.is.android.rennes&ibi=&isi=&utm_campaign=KISIO&link=https%3A%2F%2Fwww.star.fr%2Fsearch%2F%3FfeatureName%3DsearchResults%26networkId%3D33%26from%3D48.109377%252C-1.682103%26to%3D48.020335%252C-1.743929%26multimodal%3Dfalse%26departureDate%3D2017-12-25T08%253A00%253A00%252B01%253A00",
}


def mock_instant_system(_, params, headers):
    return MockResponse(INSTANT_SYSTEM_RESPONSE, 200)


@pytest.fixture(scope="function", autouse=True)
def mock_http_instant_system(monkeypatch):
    monkeypatch.setattr(
        'jormungandr.scenarios.ridesharing.instant_system.InstantSystem._call_service', mock_instant_system
    )


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF})
class TestInstantSystem(NewDefaultScenarioAbstractTestFixture):
    """
    Integration test with Instant System
    Note: '&forbidden_uris[]=PM' used to avoid line 'PM' and it's vj=vjPB in /journeys
    """

    def test_basic_ride_sharing(self):
        """
        test ridesharing_jouneys details
        """
        q = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&forbidden_uris[]=PM".format(
                first='ridesharing', last='walking'
            )
        )
        response = self.query_region(q)
        self.is_valid_journey_response(response, q, check_journey_links=False)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 1
        tickets = response.get('tickets')
        assert len(tickets) == 1
        assert tickets[0].get('cost').get('currency') == 'centime'
        assert tickets[0].get('cost').get('value') == '170.0'
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
        assert rs_journeys[0].get('distances').get('ridesharing') == 224
        assert rs_journeys[0].get('durations').get('walking') == 0  # two crow_fly sections have 0 duration
        assert rs_journeys[0].get('durations').get('ridesharing') == 1057
        assert 'ridesharing' in rs_journeys[0].get('tags')
        rsj_sections = rs_journeys[0].get('sections')
        assert len(rsj_sections) == 3

        assert rsj_sections[0].get('type') == 'crow_fly'
        assert rsj_sections[0].get('mode') == 'walking'

        assert rsj_sections[1].get('type') == 'ridesharing'
        assert rsj_sections[1].get('geojson').get('coordinates')[0] == [0.0000898312, 0.0000898312]
        assert rsj_sections[1].get('geojson').get('coordinates')[2] == [-1.68635, 48.1101]
        rsj_info = rsj_sections[1].get('ridesharing_informations')
        assert rsj_info.get('driver').get('alias') == 'Jean P.'
        assert rsj_info.get('driver').get('gender') == 'male'
        assert rsj_info.get('driver').get('image') == 'https://dummyimage.com/128x128/C8E6C9/000.png&text=JP'
        assert rsj_info.get('driver').get('rating').get('scale_min') == 0.0
        assert rsj_info.get('driver').get('rating').get('scale_max') == 5.0
        assert rsj_info.get('network') == 'Super Covoit'
        assert rsj_info.get('operator') == 'Instant System'
        assert rsj_info.get('seats').get('available') == 4

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

        fps = response['feed_publishers']
        assert len(fps) == 2

        def equals_to_dummy_fp(fp):
            return fp == DUMMY_INSTANT_SYSTEM_FEED_PUBLISHER

        assert any(equals_to_dummy_fp(fp) for fp in fps)

    def test_start_ridesharing_with_pt(self):
        """
        test ridesharing_jouneys details
        """
        q = (
            journey_basic_query
            + "&first_section_mode[]=ridesharing"
            + "&last_section_mode[]=walking"
            + "&ridesharing_speed=2.5"
        )
        response = self.query_region(q)
        self.is_valid_journey_response(response, q, check_journey_links=False)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        # The first journey is direct ridesharing
        assert 'ridesharing' in journeys[0].get('tags')
        assert 'non_pt' in journeys[0].get('tags')
        assert journeys[0].get('type') == 'best'
        sections = journeys[0].get('sections')
        assert len(sections) == 1
        assert sections[0].get('mode') == 'ridesharing'
        assert journeys[0].get('durations').get('ridesharing') == 121
        assert journeys[0].get('durations').get('total') == 121

        # The second one is of combination of ridesharing + public_transport + walking
        assert 'ridesharing' in journeys[1].get('tags')
        assert 'non_pt' not in journeys[1].get('tags')
        assert journeys[1].get('type') == 'fastest'
        sections = journeys[1].get('sections')
        assert len(sections) == 3
        assert journeys[1].get('durations').get('ridesharing') == 7
        assert journeys[1].get('durations').get('walking') == 80
        assert journeys[1].get('durations').get('total') == 89

        # first section is of ridesharing
        rs_section = sections[0]
        assert rs_section.get('mode') == 'ridesharing'
        assert rs_section.get('type') == 'street_network'
        assert rs_section.get('from').get('id') == '8.98312e-05;8.98312e-05'
        assert rs_section.get('to').get('id') == 'stop_point:stopB'
        assert rs_section.get('duration') == 7

        # second section is of public transport
        pt_section = sections[1]
        assert pt_section.get('type') == 'public_transport'
        assert pt_section.get('from').get('id') == 'stop_point:stopB'
        assert pt_section.get('to').get('id') == 'stop_point:stopA'
        assert pt_section.get('duration') == 2

        # third section is of walking
        walking_section = sections[2]
        assert walking_section.get('mode') == 'walking'
        assert walking_section.get('type') == 'street_network'
        assert walking_section.get('from').get('id') == 'stop_point:stopA'
        assert walking_section.get('to').get('id') == '0.00188646;0.00071865'
        assert walking_section.get('duration') == 80

        # with the use of &max_ridesharing_duration_to_pt=0 we have only direct ridesharing
        q = (
            journey_basic_query
            + "&first_section_mode[]=ridesharing"
            + "&last_section_mode[]=walking"
            + "&ridesharing_speed=2.5"
            + "&max_ridesharing_duration_to_pt=0"
        )
        response = self.query_region(q)
        self.is_valid_journey_response(response, q, check_journey_links=False)
        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 1
        assert 'ridesharing' in journeys[0].get('tags')
        assert journeys[0].get('durations').get('ridesharing') == 121

        q = journey_basic_query + (
            "&first_section_mode[]=ridesharing"
            "&first_section_mode[]=car"
            "&last_section_mode[]=walking"
            "&ridesharing_speed=2.5"
            "&car_speed=2.5"
        )
        response = self.query_region(q)
        self.is_valid_journey_response(response, q, check_journey_links=False)
        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 4

        # the first journey is a direct path by ridesharing
        assert 'ridesharing' in journeys[0].get('tags')
        assert 'non_pt' in journeys[0].get('tags')

        assert len(journeys[0]['sections']) == 1
        rs_section = journeys[0]['sections'][0]
        assert rs_section.get('mode') == 'ridesharing'
        assert rs_section.get('type') == 'street_network'

        # the second journey is combined by ridesharing and PT
        assert 'ridesharing' in journeys[1].get('tags')
        assert 'non_pt' not in journeys[1].get('tags')

        assert len(journeys[1]['sections']) == 3
        rs_section = journeys[1]['sections'][0]
        assert rs_section.get('mode') == 'ridesharing'
        assert rs_section.get('type') == 'street_network'

        # the third journey is combined by car and PT
        assert 'car' in journeys[2].get('tags')
        assert 'non_pt' not in journeys[2].get('tags')

        assert len(journeys[2]['sections']) == 5
        rs_section = journeys[2]['sections'][0]
        assert rs_section.get('mode') == 'car'
        assert rs_section.get('type') == 'street_network'

        # the fourth journey is a direct path by car
        assert 'car' in journeys[3].get('tags')
        assert 'non_pt' in journeys[3].get('tags')

        assert len(journeys[3]['sections']) == 3
        rs_section = journeys[3]['sections'][0]
        assert rs_section.get('mode') == 'car'
        assert rs_section.get('type') == 'street_network'

        # test the ridesharing durations in all journeys
        assert journeys[0].get('durations').get('ridesharing') == 121
        assert journeys[1].get('durations').get('ridesharing') == 7
        assert journeys[2].get('durations').get('ridesharing') == 0
        assert journeys[3].get('durations').get('ridesharing') == 0

    def test_end_ridesharing_with_pt(self):
        """
        test that we get a ridesharing_jouney when requesting with no ridesharing in first_section_mode[],
        only in last_section_mode[]

        Nota: response provided by InstantSystem mock won't be consistent (same as the other way),
        but the goal is to check it's correctly called, not that InstantSystem returns a consistent response.
        """
        q = (
            "journeys?from=0.00188646;0.00071865&to=0.0000898312;0.0000898312&datetime=20120614T080000&"
            "ridesharing_speed=2.5&first_section_mode[]={first}&last_section_mode[]={last}".format(
                first='walking', last='ridesharing'
            )
        )
        response = self.query_region(q)
        self.is_valid_journey_response(response, q, check_journey_links=False)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        # The first journey is direct walking
        assert 'walking' in journeys[0].get('tags')
        assert 'ridesharing' not in journeys[0].get('tags')
        assert 'non_pt' in journeys[0].get('tags')
        assert journeys[0].get('type') == 'best'
        sections = journeys[0].get('sections')
        assert len(sections) == 1
        assert sections[0].get('mode') == 'walking'
        assert journeys[0].get('durations').get('walking') == 276
        assert journeys[0].get('durations').get('total') == 276

        # The second one is of combination of walking + public_transport + ridesharing
        assert 'ridesharing' in journeys[1].get('tags')
        assert 'non_pt' not in journeys[1].get('tags')
        assert journeys[1].get('type') == 'rapid'
        sections = journeys[1].get('sections')
        assert len(sections) == 5  # we get also boarding and alighting sections (doesn't matter)
        assert journeys[1].get('durations').get('ridesharing') == 7
        assert journeys[1].get('durations').get('walking') == 80
        assert journeys[1].get('durations').get('total') == 3867

        # first section is of walking
        walking_section = sections[0]
        assert walking_section.get('mode') == 'walking'
        assert walking_section.get('type') == 'street_network'
        assert walking_section.get('from').get('id') == '0.00188646;0.00071865'
        assert walking_section.get('to').get('id') == 'stop_point:stopA'
        assert walking_section.get('duration') == 80

        # third section is of public transport
        pt_section = sections[2]
        assert pt_section.get('type') == 'public_transport'
        assert pt_section.get('from').get('id') == 'stop_point:stopA'
        assert pt_section.get('to').get('id') == 'stop_point:stopB'
        assert pt_section.get('duration') == 180

        # last section is of ridesharing
        rs_section = sections[4]
        assert rs_section.get('mode') == 'ridesharing'
        assert rs_section.get('type') == 'street_network'
        assert rs_section.get('from').get('id') == 'stop_point:stopB'
        assert rs_section.get('to').get('id') == '8.98312e-05;8.98312e-05'
        assert rs_section.get('duration') == 7
