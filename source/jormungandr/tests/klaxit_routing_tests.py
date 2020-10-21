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
from tests.check_utils import get_not_null, get_links_dict
from tests.tests_mechanism import dataset, NewDefaultScenarioAbstractTestFixture

DUMMY_KLAXIT_FEED_PUBLISHER = {'id': '42', 'name': '42', 'license': 'I dunno', 'url': 'http://w.tf'}

MOCKED_INSTANCE_CONF = {
    'scenario': 'new_default',
    'instance_config': {
        'ridesharing': [
            {
                "args": {
                    "service_url": "http://wtf",
                    "api_key": "key",
                    "network": "Super Covoit",
                    "feed_publisher": DUMMY_KLAXIT_FEED_PUBLISHER,
                },
                "class": "jormungandr.scenarios.ridesharing.klaxit.Klaxit",
            }
        ]
    },
}

KLAXIT_RESPONSE = [
    {
        "departureToPickupWalkingDistance": 475,
        "departureToPickupWalkingPolyline": "keliHiyoMqAxCjFvHIRdA~BjE`G",
        "departureToPickupWalkingTime": 174,
        "distance": 18869,
        "driver": {"alias": "Mohamed M", "grade": 5, "picture": "https://driver.png"},
        "driverArrivalLat": 0.00071865,
        "driverArrivalLng": 0.00188646,
        "driverDepartureDate": 1601988149,
        "driverDepartureLat": 0.0000898312,
        "driverDepartureLng": 0.0000898312,
        "dropoffToArrivalWalkingDistance": 1237,
        "dropoffToArrivalWalkingPolyline": "{deiHq~nMh@hBrNaK?u@jKdFrBnAvMw@fEjCXu@[]`CkG",
        "dropoffToArrivalWalkingTime": 76,
        "duration": 1301,
        "id": "fe08fceb-03a2-4dc6-8ba4-b422c1256227",
        "journeyPolyline": "svr_H}fyC{@g@[[o@u@Qc@[sBMm@Qe@[g@e@k@aBuAIKOI?AACCEEAIDAN@DMf@GZ@d@QbBOzAy@xFAJ]r@IPOQm@a@wDoB?C?ECIGAGD?@m@c@uCiCcCgBEKsB}AK?IGs@i@m@y@Wm@oAoDaB_EYk@y@kAyDmG_A}Ao@}AQeAIcAOiBUw@Yc@WWUKiAe@USSQKOACAIEQMOOGO@MHKPGd@CZK^INq@`A}@lAmEhGcIdLcPlUqGdJ[XE?K@KFK\AH?DQl@aFjH{IlMoCdEgBrCkG~JqDtFcLpPyb@pn@aHdKoHjKcAtAQLSLC?G@KFMZAVI`@eCvDoAnBs@rAe@~@{@pBg@tAk@fBcEpO{FpTeL~a@wBfIoAjEy@fBK?KDOPIXCd@F`@UxAY`AQn@o@zCwAxFiAbEMd@cCdJ_BxF[t@GAE?KDILAP@RHLBB]`Bw@fDYbAQn@gBrGqAnFq@rCyIj\qH|XMNe@vAq@dCcFpRaF|QuFbT}CdL{CbLgE`PiCjJcApDYWoB{@gBw@_EiBiAi@[KwBmAiF}CuAm@Q@}A{@sFeDkDqBsIwEkYmOyJ_FgEmBiEeB_EuAkCw@aCm@{AWaCa@aCYcF[yCG_D@iEPaDXeANmBZsCl@wDdA_DhAcEfBiHjDoLzF}h@fWoKbFcAh@]E_Cl@s@L_AJqA?}@IkAOcAUaA[y@e@{@i@SISA_@Fa@T]XYd@Gn@R|@X~@p@dAn@|@h@z@?B?D@HJNB?@?|@~ArE`IPl@t@tAt@hBdAvCzBxFj@tHn@|Iz@zL~@nMf@hGLb@F`@TzCJpB^@VERSN]@a@Ae@|By@bA[dCw@NKpDiAxAc@|@U",
        "price": {"amount": 1.0, "type": "PAYING"},
        "type": "PLANNED",
        "webUrl": "https://klaxitlines.com",
    }
]


def mock_klaxit(_, params):
    return MockResponse(KLAXIT_RESPONSE, 200)


@pytest.fixture(scope="function", autouse=True)
def mock_http_klaxit(monkeypatch):
    monkeypatch.setattr('jormungandr.scenarios.ridesharing.klaxit.Klaxit._call_service', mock_klaxit)


@dataset({'main_routing_test': MOCKED_INSTANCE_CONF})
class TestKlaxit(NewDefaultScenarioAbstractTestFixture):
    """
    Integration test with Klaxit VIA API
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
        assert tickets[0].get('cost').get('value') == '1.0'
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
        assert rs_journeys[0].get('durations').get('walking') == 0  # two crow_fly sections have 0 duration
        assert rs_journeys[0].get('durations').get('ridesharing') == 1301
        assert 'ridesharing' in rs_journeys[0].get('tags')
        rsj_sections = rs_journeys[0].get('sections')
        assert len(rsj_sections) == 3

        assert rsj_sections[0].get('type') == 'crow_fly'
        assert rsj_sections[0].get('mode') == 'walking'
        # crowfly duration in ridesharing is always 0
        assert rsj_sections[0].get('duration') == 0

        assert rsj_sections[1].get('type') == 'ridesharing'
        assert rsj_sections[1].get('geojson').get('coordinates')[0] == [0.0000898312, 0.0000898312]
        assert rsj_sections[1].get('geojson').get('coordinates')[2] == [0.78995, 47.28728]
        # ridesharing duration comes from the offer
        assert rsj_sections[1].get('duration') == 1301
        rsj_info = rsj_sections[1].get('ridesharing_informations')
        assert rsj_info.get('network') == 'Super Covoit'
        assert rsj_info.get('operator') == 'Klaxit VIA API'

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
        # crowfly duration in ridesharing is always 0
        assert rsj_sections[2].get('duration') == 0

        fps = response['feed_publishers']
        assert len(fps) == 2

        def equals_to_dummy_fp(fp):
            return fp == DUMMY_KLAXIT_FEED_PUBLISHER

        assert any(equals_to_dummy_fp(fp) for fp in fps)
