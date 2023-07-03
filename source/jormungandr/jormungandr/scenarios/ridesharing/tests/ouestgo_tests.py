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

from jormungandr.scenarios.ridesharing.ouestgo import Ouestgo, DEFAULT_OUESTGO_FEED_PUBLISHER
from jormungandr.scenarios.ridesharing.ridesharing_service_manager import RidesharingServiceManager
from jormungandr.scenarios.ridesharing.ridesharing_journey import Gender
from jormungandr.scenarios.ridesharing.ridesharing_service import RsFeedPublisher, RidesharingServiceError

import mock
from jormungandr.tests import utils_test
from jormungandr import utils
import pytz
import json
import re  # https://stackoverflow.com/a/9312242/1614576
import requests_mock
import pytest


fake_response = """
    [
        {
            "journeys": {
                "uuid": 225,
                "operator": "Ouestgo",
                "origin": "test.ouestgo.mobicoop.io",
                "url": "https://test.ouestgo.mobicoop.io/covoiturage/rdex/9e2922c777e583cfae11",
                "driver": {
                    "uuid": 12,
                    "alias": "Corentin K.",
                    "image": null,
                    "gender": "male",
                    "seats": 3,
                    "state": 1
                },
            "passenger": {
                "uuid": 12,
                "alias": "Corentin K.",
                "image": null,
                "gender": "male",
                "persons": 0,
                "state": 0
            },
            "from": {
                "address": null,
                "city": "Nancy",
                "postalcode": "54100",
                "country": "France",
                "latitude": "48.687930",
                "longitude": "6.171514"
            },
            "to": {
                "address": null,
                "city": "Metz",
                "postalcode": "57000",
                "country": "France",
                "latitude": "49.108385",
                "longitude": "6.194897"
            },
            "distance": 56546,
            "duration": 2345,
            "route": null,
            "number_of_waypoints": 0,
            "waypoints": {},
            "cost": {
                "variable": "0.059649"
            },
            "details": null,
            "vehicle": null,
            "frequency": "punctual",
            "type": "one-way",
            "real_time": null,
            "stopped": null,
            "days": {
                "monday": 1,
                "tuesday": 0,
                "wednesday": 0,
                "thursday": 0,
                "friday": 0,
                "saturday": 0,
                "sunday": 0
            },
            "outward": {
                "mindate": "2022-11-22",
                "maxdate": "2022-11-22",
                "monday": {
                    "mintime": "08:45:00",
                    "maxtime": "09:15:00"
                },
                "tuesday": null,
                "wednesday": null,
                "thursday": null,
                "friday": null,
                "saturday": null,
                "sunday": null
            },
            "return": null
        }
      }
    ]
"""

regex = re.compile(r'\\(?![/u"])')
fixed = regex.sub(r"\\\\", fake_response)

mock_get = mock.MagicMock(return_value=utils_test.MockResponse(json.loads(fixed), 200, '{}'))

DUMMY_OUESTGO_FEED_PUBLISHER = {'id': '42', 'name': '42', 'license': 'I dunno', 'url': 'http://w.tf'}


# A hack class
class DummyInstance:
    name = ''
    walking_speed = 1.12
    timezone = pytz.utc


def get_ridesharing_service_test():
    configs = [
        {
            "class": "jormungandr.scenarios.ridesharing.ouestgo.Ouestgo",
            "args": {
                "service_url": "toto",
                "api_key": "toto key",
                "network": "N",
                "driver_state": 1,
                "passenger_state": 0,
                "feed_publisher": DUMMY_OUESTGO_FEED_PUBLISHER,
            },
        },
        {
            "class": "jormungandr.scenarios.ridesharing.ouestgo.Ouestgo",
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
    assert services[0].system_id == 'ouestgo'
    assert services[0]._get_feed_publisher() == RsFeedPublisher(**DUMMY_OUESTGO_FEED_PUBLISHER)

    assert services[1].service_url == 'tata'
    assert services[1].api_key == 'tata key'
    assert services[1].network == 'M'
    assert services[1].system_id == 'ouestgo'
    assert services[1]._get_feed_publisher() == RsFeedPublisher(**DEFAULT_OUESTGO_FEED_PUBLISHER)


def ouestgo_basic_test():
    with mock.patch('requests.get', mock_get):

        ouestgo = Ouestgo(
            service_url='dummyUrl',
            api_key='dummyApiKey',
            network='dummyNetwork',
            feed_publisher=DUMMY_OUESTGO_FEED_PUBLISHER,
        )
        from_coord = '48.68793,6.171514'
        to_coord = '49.108385,6.194897'

        period_extremity = utils.PeriodExtremity(
            datetime=utils.make_timestamp_from_str("20221121T084122"), represents_start=True
        )
        ridesharing_journeys, feed_publisher = ouestgo.request_journeys_with_feed_publisher(
            from_coord=from_coord,
            to_coord=to_coord,
            period_extremity=period_extremity,
            instance_params=DummyInstance(),
        )

        assert len(ridesharing_journeys) == 1
        assert ridesharing_journeys[0].metadata.network == 'dummyNetwork'
        assert ridesharing_journeys[0].metadata.system_id == 'ouestgo'
        assert (
            ridesharing_journeys[0].ridesharing_ad
            == 'https://test.ouestgo.mobicoop.io/covoiturage/rdex/9e2922c777e583cfae11'
        )

        assert ridesharing_journeys[0].pickup_place.addr == ""  # address is not provided in mock
        assert ridesharing_journeys[0].pickup_place.lat == 48.687930
        assert ridesharing_journeys[0].pickup_place.lon == 6.171514

        assert ridesharing_journeys[0].dropoff_place.addr == ""  # address is not provided in mock
        assert ridesharing_journeys[0].dropoff_place.lat == 49.108385
        assert ridesharing_journeys[0].dropoff_place.lon == 6.194897

        assert ridesharing_journeys[0].shape is None
        assert ridesharing_journeys[0].price == 350.0
        assert ridesharing_journeys[0].currency == 'centime'

        assert ridesharing_journeys[0].total_seats is None
        assert ridesharing_journeys[0].available_seats == 3

        assert ridesharing_journeys[0].driver.alias == 'Corentin K.'
        assert ridesharing_journeys[0].driver.gender == Gender.MALE
        assert ridesharing_journeys[0].driver.image is None
        assert ridesharing_journeys[0].driver.rate is None
        assert ridesharing_journeys[0].driver.rate_count is None

        assert feed_publisher == RsFeedPublisher(**DUMMY_OUESTGO_FEED_PUBLISHER)


def test_request_journeys_should_raise_on_non_200():
    with requests_mock.Mocker() as mock:
        ouestgo = Ouestgo(service_url='http://ouestgo', api_key='ApiKey', network='Network')

        mock.get('http://ouestgo', status_code=401, text='{this is the http response}')

        with pytest.raises(RidesharingServiceError) as e:
            ouestgo._request_journeys(
                '1.2,3.4',
                '5.6,7.8',
                utils.PeriodExtremity(
                    datetime=utils.make_timestamp_from_str("20221121T084122"), represents_start=True
                ),
                DummyInstance(),
            )

        exception_params = e.value.get_params().values()
        assert 401 in exception_params
        assert '{this is the http response}' in exception_params


def ouestgo_status_test():
    with mock.patch('requests.get', mock_get):

        ouestgo = Ouestgo(
            service_url='dummyUrl',
            api_key='dummyApiKey',
            network='dummyNetwork',
            feed_publisher=DUMMY_OUESTGO_FEED_PUBLISHER,
        )
        resp = ouestgo.status()
        assert resp["id"] == ouestgo.system_id
        assert resp["network"] == ouestgo.network


def get_mean_pickup_datetime_empty_json_outward_test():
    ouestgo = Ouestgo(
        service_url='dummyUrl',
        api_key='dummyApiKey',
        network='dummyNetwork',
        feed_publisher=DUMMY_OUESTGO_FEED_PUBLISHER,
    )
    mean_pickup_datetime = ouestgo.get_mean_pickup_datetime(
        json_outward={}, circulation_day="wednesday", timezone="toto"
    )
    assert not mean_pickup_datetime


def get_mean_pickup_datetime_test():
    ouestgo = Ouestgo(
        service_url='dummyUrl',
        api_key='dummyApiKey',
        network='dummyNetwork',
        feed_publisher=DUMMY_OUESTGO_FEED_PUBLISHER,
    )
    json_outward = {
        "mindate": "2022-11-22",
        "maxdate": "2022-11-22",
        "monday": {"mintime": "08:45:00", "maxtime": "09:15:00"},
        "tuesday": None,
        "wednesday": None,
        "thursday": None,
        "friday": None,
        "saturday": None,
        "sunday": None,
    }

    mean_pickup_datetime = ouestgo.get_mean_pickup_datetime(
        json_outward=json_outward, circulation_day="monday", timezone=pytz.timezone("Europe/Paris")
    )

    assert utils.dt_to_str(utils.navitia_utcfromtimestamp(mean_pickup_datetime)) == "20221122T080000"


def make_response_empty_raw_json_test():
    ouestgo = Ouestgo(
        service_url='dummyUrl',
        api_key='dummyApiKey',
        network='dummyNetwork',
        feed_publisher=DUMMY_OUESTGO_FEED_PUBLISHER,
    )
    resp = ouestgo._make_response([], None, None, None, pytz.timezone("UTC"))
    assert not resp


def make_response_pikup_datetime_invalid_test():
    ouestgo = Ouestgo(
        service_url='dummyUrl',
        api_key='dummyApiKey',
        network='dummyNetwork',
        feed_publisher=DUMMY_OUESTGO_FEED_PUBLISHER,
    )
    # Thursday 25 May 2023 12:00:00
    request_datetime = 1685016000
    resp = ouestgo._make_response([{"toto": "tata"}], request_datetime, None, None, pytz.timezone("UTC"))
    assert not resp
