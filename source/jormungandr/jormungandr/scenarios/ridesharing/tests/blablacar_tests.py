# -*- coding: utf-8 -*-
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io


from __future__ import absolute_import, print_function, unicode_literals, division

from jormungandr.scenarios.ridesharing.blablacar import Blablacar, DEFAULT_BLABLACAR_FEED_PUBLISHER
from jormungandr.scenarios.ridesharing.ridesharing_service_manager import RidesharingServiceManager
from jormungandr.scenarios.ridesharing.ridesharing_service import RsFeedPublisher, RidesharingServiceError

import mock
from jormungandr.tests import utils_test
from jormungandr import utils
import json
import re  # https://stackoverflow.com/a/9312242/1614576
import requests_mock
import pytest


fake_response = """
    [
        {
            "id": "53e0ac4f-e693-4e29-8f69-eb1142d5b99e_2020-09-28",
            "journey_polyline": "svr_H}fyC{@g@[[o@u@Qc@[sBMm@Qe@[g@e@k@aBuAIKOI?AACCEEAIDAN@DMf@GZ@d@QbBOzAy@xFAJ]r@IPOQm@a@wDoB?C?ECIGAGD?@m@c@uCiCcCgBEKsB}AK?IGs@i@m@y@Wm@oAoDaB_EYk@y@kAyDmG_A}Ao@}AQeAIcAOiBUw@Yc@WWUKiAe@USSQKOACAIEQMOOGO@MHKPGd@CZK^INq@`A}@lAmEhGcIdLcPlUqGdJ[XE?K@KFK\AH?DQl@aFjH{IlMoCdEgBrCkG~JqDtFcLpPyb@pn@aHdKoHjKcAtAQLSLC?G@KFMZAVI`@eCvDoAnBs@rAe@~@{@pBg@tAk@fBcEpO{FpTeL~a@wBfIoAjEy@fBK?KDOPIXCd@F`@UxAY`AQn@o@zCwAxFiAbEMd@cCdJ_BxF[t@GAE?KDILAP@RHLBB]`Bw@fDYbAQn@gBrGqAnFq@rCyIj\qH|XMNe@vAq@dCcFpRaF|QuFbT}CdL{CbLgE`PiCjJcApDYWoB{@gBw@_EiBiAi@[KwBmAiF}CuAm@Q@}A{@sFeDkDqBsIwEkYmOyJ_FgEmBiEeB_EuAkCw@aCm@{AWaCa@aCYcF[yCG_D@iEPaDXeANmBZsCl@wDdA_DhAcEfBiHjDoLzF}h@fWoKbFcAh@]E_Cl@s@L_AJqA?}@IkAOcAUaA[y@e@{@i@SISA_@Fa@T]XYd@Gn@R|@X~@p@dAn@|@h@z@?B?D@HJNB?@?|@~ArE`IPl@t@tAt@hBdAvCzBxFj@tHn@|Iz@zL~@nMf@hGLb@F`@TzCJpB^@VERSN]@a@Ae@|By@bA[dCw@NKpDiAxAc@|@U",
            "dropoff_longitude": 0.69039,
            "pickup_latitude": 47.28696,
            "web_url": "https://blablalines.com",
            "departure_to_pickup_walking_time": 174,
            "duration": 1301,
            "pickup_longitude": 0.78981,
            "available_seats": 3,
            "dropoff_latitude": 47.38642,
            "price": {
                "currency": "EUR",
                "amount": 1
            },
            "dropoff_to_arrival_walking_time": 76,
            "distance": 18869
        },
        {
            "id": "124b38ca-9494-4743-a192-7bb8ad29bd05_2020-09-28",
            "journey_polyline": "svr_H}fyC{@g@[[o@u@Qc@[sBMm@Qe@[g@e@k@aBuAIKOI?AACCEEAIDAN@DMf@GZ@d@QbBOzAy@xFAJ]r@IPOQm@a@wDoB?C?ECIGAGD?@m@c@uCiCcCgBEKsB}AK?IGs@i@m@y@Wm@oAoDaB_EYk@y@kAyDmG_A}Ao@}AQeAIcAOiBUw@Yc@WWUKiAe@USSQKOACAIEQMOOGO@MHKPGd@CZK^INq@`A}@lAmEhGcIdLcPlUqGdJ[XE?K@KFK\AH?DQl@aFjH{IlMoCdEgBrCkG~JqDtFcLpPyb@pn@aHdKoHjKcAtAQLSLC?G@KFMZAVI`@eCvDoAnBs@rAe@~@{@pBg@tAk@fBcEpO{FpTeL~a@wBfIoAjEy@fBK?KDOPIXCd@F`@UxAY`AQn@o@zCwAxFiAbEMd@cCdJ_BxF[t@GAE?KDILAP@RHLBB]`Bw@fDYbAQn@gBrGqAnFq@rCyIj\qH|XMNe@vAq@dCcFpRaF|QuFbT}CdL{CbLgE`PiCjJcApDYWoB{@gBw@_EiBiAi@[KwBmAiF}CuAm@Q@}A{@sFeDkDqBsIwEkYmOyJ_FgEmBiEeB_EuAkCw@aCm@{AWaCa@aCYcF[yCG_D@iEPaDXeANmBZsCl@wDdA_DhAcEfBiHjDoLzF}h@fWoKbFcAh@]E_Cl@s@L_AJqA?}@IkAOcAUaA[y@e@{@i@SISA_@Fa@T]XYd@Gn@R|@X~@p@dAn@|@h@z@?B?D@HJNB?@?|@~ArE`IPl@t@tAt@hBdAvCzBxFj@tHn@|Iz@zL~@nMf@hGLb@F`@TzCJpB^@VERSN]@a@Ae@|By@bA[dCw@NKpDiAxAc@|@U",
            "dropoff_longitude": 0.69192,
            "pickup_latitude": 47.28696,
            "web_url": "https://blablalines.com",
            "departure_to_pickup_walking_time": 174,
            "duration": 1229,
            "pickup_longitude": 0.78981,
            "available_seats": 2,
            "dropoff_latitude": 47.38399,
            "price": {
                "currency": "EUR",
                "amount": 3
            },
            "dropoff_to_arrival_walking_time": 164,
            "distance": 16512
        }
    ]

"""

regex = re.compile(r'\\(?![/u"])')
fixed = regex.sub(r"\\\\", fake_response)

mock_get = mock.MagicMock(return_value=utils_test.MockResponse(json.loads(fixed), 200, '{}'))

DUMMY_BLABLACAR_FEED_PUBLISHER = {'id': '42', 'name': '42', 'license': 'I dunno', 'url': 'http://w.tf'}

# A hack class
class DummyInstance:
    name = ''


def get_ridesharing_service_test():
    configs = [
        {
            "class": "jormungandr.scenarios.ridesharing.blablacar.Blablacar",
            "args": {
                "service_url": "toto",
                "api_key": "toto key",
                "network": "N",
                "feed_publisher": DUMMY_BLABLACAR_FEED_PUBLISHER,
            },
        },
        {
            "class": "jormungandr.scenarios.ridesharing.blablacar.Blablacar",
            "args": {"service_url": "tata", "api_key": "tata key", "network": "M"},
        },
    ]
    instance = DummyInstance()
    ridesharing_service_manager = RidesharingServiceManager(instance, configs)
    ridesharing_service_manager.init_ridesharing_services()
    services = ridesharing_service_manager.get_all_ridesharing_services()

    assert len(services) == 2

    assert services[0].service_url == 'toto'
    assert services[0].api_key == 'toto key'
    assert services[0].network == 'N'
    assert services[0].system_id == 'Blablacar'
    assert services[0]._get_feed_publisher() == RsFeedPublisher(**DUMMY_BLABLACAR_FEED_PUBLISHER)

    assert services[1].service_url == 'tata'
    assert services[1].api_key == 'tata key'
    assert services[1].network == 'M'
    assert services[1].system_id == 'Blablacar'
    assert services[1]._get_feed_publisher() == RsFeedPublisher(**DEFAULT_BLABLACAR_FEED_PUBLISHER)


def blablacar_test():
    with mock.patch('requests.get', mock_get):

        blablacar = Blablacar(
            service_url='dummyUrl',
            api_key='dummyApiKey',
            network='dummyNetwork',
            feed_publisher=DUMMY_BLABLACAR_FEED_PUBLISHER,
        )
        from_coord = '47.28696,0.78981'
        to_coord = '47.38642,0.69039'

        period_extremity = utils.PeriodExtremity(
            datetime=utils.str_to_time_stamp("20171225T060000"), represents_start=True
        )
        ridesharing_journeys, feed_publisher = blablacar.request_journeys_with_feed_publisher(
            from_coord=from_coord, to_coord=to_coord, period_extremity=period_extremity
        )

        assert len(ridesharing_journeys) == 2
        assert ridesharing_journeys[0].metadata.network == 'dummyNetwork'
        assert ridesharing_journeys[0].metadata.system_id == 'Blablacar'
        assert ridesharing_journeys[0].ridesharing_ad == 'https://blablalines.com'

        assert ridesharing_journeys[0].pickup_place.addr == ""  # address is not provided in mock
        assert ridesharing_journeys[0].pickup_place.lat == 47.28696
        assert ridesharing_journeys[0].pickup_place.lon == 0.78981

        assert ridesharing_journeys[0].dropoff_place.addr == ""  # address is not provided in mock
        assert ridesharing_journeys[0].dropoff_place.lat == 47.38642
        assert ridesharing_journeys[0].dropoff_place.lon == 0.69039

        assert ridesharing_journeys[0].shape
        assert ridesharing_journeys[0].shape[0].lat == ridesharing_journeys[0].pickup_place.lat
        assert ridesharing_journeys[0].shape[0].lon == ridesharing_journeys[0].pickup_place.lon
        assert ridesharing_journeys[0].shape[1].lat == 47.28698  # test that we really load a shape
        assert ridesharing_journeys[0].shape[1].lon == 0.78975
        assert ridesharing_journeys[0].shape[-1].lat == ridesharing_journeys[0].dropoff_place.lat
        assert ridesharing_journeys[0].shape[-1].lon == ridesharing_journeys[0].dropoff_place.lon

        assert ridesharing_journeys[0].price == 1
        assert ridesharing_journeys[0].currency == 'centime'

        assert ridesharing_journeys[0].total_seats is None
        assert ridesharing_journeys[0].available_seats == 3

        assert ridesharing_journeys[1].metadata.network == 'dummyNetwork'
        assert ridesharing_journeys[1].metadata.system_id == 'Blablacar'
        assert ridesharing_journeys[1].shape
        assert ridesharing_journeys[1].ridesharing_ad == 'https://blablalines.com'

        assert ridesharing_journeys[1].pickup_place.addr == ""
        assert ridesharing_journeys[1].pickup_place.lat == 47.28696
        assert ridesharing_journeys[1].pickup_place.lon == 0.78981

        assert ridesharing_journeys[1].dropoff_place.addr == ""
        assert ridesharing_journeys[1].dropoff_place.lat == 47.38399
        assert ridesharing_journeys[1].dropoff_place.lon == 0.69192

        assert ridesharing_journeys[1].price == 3
        assert ridesharing_journeys[1].currency == 'centime'

        assert ridesharing_journeys[1].total_seats is None
        assert ridesharing_journeys[1].available_seats == 2

        assert feed_publisher == RsFeedPublisher(**DUMMY_BLABLACAR_FEED_PUBLISHER)


def test_request_journeys_should_raise_on_non_200():
    with requests_mock.Mocker() as mock:
        blablacar = Blablacar(service_url='http://blablacar', api_key='ApiKey', network='Network')

        mock.get('http://blablacar', status_code=401, text='{this is the http response}')

        with pytest.raises(RidesharingServiceError) as e:
            blablacar._request_journeys(
                '1.2,3.4',
                '5.6,7.8',
                utils.PeriodExtremity(
                    datetime=utils.str_to_time_stamp("20171225T060000"), represents_start=True
                ),
            )

        exception_params = e.value.get_params().values()
        assert 401 in exception_params
        assert '{this is the http response}' in exception_params
