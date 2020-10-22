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

from jormungandr.scenarios.ridesharing.klaxit import Klaxit, DEFAULT_KLAXIT_FEED_PUBLISHER
from jormungandr.scenarios.ridesharing.ridesharing_service_manager import RidesharingServiceManager
from jormungandr.scenarios.ridesharing.ridesharing_service import RsFeedPublisher, RidesharingServiceError

import mock
from jormungandr.tests import utils_test
from jormungandr import utils
import json


fake_response = """
[
  {
    "departureToPickupWalkingDistance": 475,
    "departureToPickupWalkingPolyline": "keliHiyoMqAxCjFvHIRdA~BjE`G",
    "departureToPickupWalkingTime": 344,
    "distance": 5166,
    "driver": {
      "alias": "Mohamed M",
      "grade": null,
      "picture": "https://res.cloudinary.com/wayzup/image/upload/v5/image/upload/production/mate/picture/5fa81667-a603-4114-b669-80870e6107fd.png"
    },
    "driverArrivalLat": 48.85658,
    "driverArrivalLng": 2.37729,
    "driverDepartureDate": 1601988149,
    "driverDepartureLat": 48.8898252102323,
    "driverDepartureLng": 2.37550085289956,
    "dropoffToArrivalWalkingDistance": 1288,
    "dropoffToArrivalWalkingPolyline": "sheiHaioMd@dBjIkGnTlKrBnAvMw@fEjCXu@[]`CkG",
    "dropoffToArrivalWalkingTime": 938,
    "duration": 770,
    "id": "7f296dee-8781-4f70-9b95-061c5d899821",
    "journeyPolyline": "ixkiHa~nMhc@tk@\\k@jF{FrYk@rCUbb@`C~@`IbM{GpBjHj@s@dO}Ko@`GjcA{Dl@gBbD}E_Mmc@",
    "price": {
      "amount": 0.51,
      "type": "PAYING"
    },
    "type": "PLANNED",
    "webUrl": "https://klaxit.app.link"
  },
  {
    "departureToPickupWalkingDistance": 642,
    "departureToPickupWalkingPolyline": "keliHiyoMeKrh@yBpB",
    "departureToPickupWalkingTime": 462,
    "distance": 5666,
    "driver": {
      "alias": "Mohamed M",
      "grade": null,
      "picture": "https://res.cloudinary.com/wayzup/image/upload/v5/image/upload/production/mate/picture/5fa81667-a603-4114-b669-80870e6107fd.png"
    },
    "driverArrivalLat": 48.8559454289402,
    "driverArrivalLng": 2.37563525508835,
    "driverDepartureDate": 1601988043,
    "driverDepartureLat": 48.8943912703919,
    "driverDepartureLng": 2.3724889755249,
    "dropoffToArrivalWalkingDistance": 1237,
    "dropoffToArrivalWalkingPolyline": "{deiHq~nMh@hBrNaK?u@jKdFrBnAvMw@fEjCXu@[]`CkG",
    "dropoffToArrivalWalkingTime": 903,
    "duration": 852,
    "id": "4abb2d30-2f96-4612-949d-fda8942cc904",
    "journeyPolyline": "kuliHclnMlVaX|h@xr@\\k@jF{FrYk@rCUbb@`C~@`IbM{GpBjHj@s@dO}Ko@`GjcA{Dl@gBbD}EgI}X",
    "price": {
      "amount": 0.56,
      "type": "PAYING"
    },
    "type": "PLANNED",
    "webUrl": "https://klaxit.app.link"
  }
]
"""

# https://stackoverflow.com/a/9312242/1614576
import re

regex = re.compile(r'\\(?![/u"])')
fixed = regex.sub(r"\\\\", fake_response)

mock_get = mock.MagicMock(return_value=utils_test.MockResponse(json.loads(fixed), 200, '{}'))

DUMMY_KLAXIT_FEED_PUBLISHER = {'id': '42', 'name': '42', 'license': 'I dunno', 'url': 'http://w.tf'}


# A hack class
class DummyInstance:
    name = ''


def klaxit_service_config_test():
    configs = [
        {
            "class": "jormungandr.scenarios.ridesharing.klaxit.Klaxit",
            "args": {
                "service_url": "toto",
                "api_key": "toto key",
                "network": "N",
                "feed_publisher": DUMMY_KLAXIT_FEED_PUBLISHER,
            },
        },
        {
            "class": "jormungandr.scenarios.ridesharing.klaxit.Klaxit",
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
    assert services[0].system_id == 'Klaxit VIA API'
    assert services[0]._get_feed_publisher() == RsFeedPublisher(**DUMMY_KLAXIT_FEED_PUBLISHER)

    assert services[1].service_url == 'tata'
    assert services[1].api_key == 'tata key'
    assert services[1].network == 'M'
    assert services[1].system_id == 'Klaxit VIA API'
    assert services[1]._get_feed_publisher() == RsFeedPublisher(**DEFAULT_KLAXIT_FEED_PUBLISHER)


def klaxit_service_test():
    with mock.patch('requests.get', mock_get):

        klaxit = Klaxit(
            service_url='dummyUrl',
            api_key='dummyApiKey',
            network='dummyNetwork',
            feed_publisher=DUMMY_KLAXIT_FEED_PUBLISHER,
        )
        from_coord = '47.28696,0.78981'
        to_coord = '47.38642,0.69039'

        period_extremity = utils.PeriodExtremity(
            datetime=utils.str_to_time_stamp("20171225T060000"), represents_start=True
        )

        ridesharing_journeys, feed_publisher = klaxit.request_journeys_with_feed_publisher(
            from_coord=from_coord, to_coord=to_coord, period_extremity=period_extremity
        )

        assert len(ridesharing_journeys) == 2
        assert ridesharing_journeys[0].metadata.network == 'dummyNetwork'
        assert ridesharing_journeys[0].metadata.system_id == 'Klaxit VIA API'
        assert ridesharing_journeys[0].ridesharing_ad == 'https://klaxit.app.link'

        assert ridesharing_journeys[0].pickup_place.addr == ""  # address is not provided in mock
        assert ridesharing_journeys[0].pickup_place.lat == 48.8898252102323
        assert ridesharing_journeys[0].pickup_place.lon == 2.37550085289956

        assert ridesharing_journeys[0].dropoff_place.addr == ""  # address is not provided in mock
        assert ridesharing_journeys[0].dropoff_place.lat == 48.85658
        assert ridesharing_journeys[0].dropoff_place.lon == 2.37729

        assert ridesharing_journeys[0].shape
        assert ridesharing_journeys[0].shape[0].lat == ridesharing_journeys[0].pickup_place.lat
        assert ridesharing_journeys[0].shape[0].lon == ridesharing_journeys[0].pickup_place.lon
        assert ridesharing_journeys[0].shape[1].lat == 48.88981  # test that we really load a shape
        assert ridesharing_journeys[0].shape[1].lon == 2.37553
        assert ridesharing_journeys[0].shape[-1].lat == ridesharing_journeys[0].dropoff_place.lat
        assert ridesharing_journeys[0].shape[-1].lon == ridesharing_journeys[0].dropoff_place.lon

        assert ridesharing_journeys[0].price == 0.51
        assert ridesharing_journeys[0].currency == 'centime'

        # Klaxit doesn't have any information on seats
        assert ridesharing_journeys[0].total_seats is None
        assert ridesharing_journeys[0].available_seats is None

        assert ridesharing_journeys[1].metadata.network == 'dummyNetwork'
        assert ridesharing_journeys[1].metadata.system_id == 'Klaxit VIA API'
        assert ridesharing_journeys[1].shape
        assert ridesharing_journeys[1].ridesharing_ad == 'https://klaxit.app.link'

        assert ridesharing_journeys[1].pickup_place.addr == ""
        assert ridesharing_journeys[1].pickup_place.lat == 48.8943912703919
        assert ridesharing_journeys[1].pickup_place.lon == 2.3724889755249

        assert ridesharing_journeys[1].dropoff_place.addr == ""
        assert ridesharing_journeys[1].dropoff_place.lat == 48.8559454289402
        assert ridesharing_journeys[1].dropoff_place.lon == 2.37563525508835

        assert ridesharing_journeys[1].price == 0.56
        assert ridesharing_journeys[1].currency == 'centime'

        # Klaxit doesn't have any information on seats
        assert ridesharing_journeys[1].total_seats is None
        assert ridesharing_journeys[1].available_seats is None

        assert feed_publisher == RsFeedPublisher(**DUMMY_KLAXIT_FEED_PUBLISHER)


import requests_mock
import pytest


def test_request_journeys_should_raise_on_non_200():
    with requests_mock.Mocker() as mock:
        klaxit = Klaxit(service_url='http://klaxit.com', api_key='ApiKey', network='Network')

        mock.get('http://klaxit.com', status_code=401, text='{this is the http response}')

        with pytest.raises(RidesharingServiceError) as e:
            klaxit._request_journeys(
                '1.2,3.4',
                '5.6,7.8',
                utils.PeriodExtremity(
                    datetime=utils.str_to_time_stamp("20171225T060000"), represents_start=True
                ),
            )

        exception_params = e.value.get_params().values()
        assert 401 in exception_params
        assert '{this is the http response}' in exception_params
