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

from jormungandr.scenarios.ridesharing.karos import Karos, DEFAULT_KAROS_FEED_PUBLISHER
from jormungandr.scenarios.ridesharing.ridesharing_journey import Gender
from jormungandr.scenarios.ridesharing.ridesharing_service import (
    Ridesharing,
    RsFeedPublisher,
    RidesharingServiceError,
)

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
    "availableSeats": 3,
    "car": {
      "brand": "206",
      "model": "Peugeot"
    },
    "deepLink": {
      "android": {
        "storeUrl": "https://play.google.com/store/apps/details?id=fr.karos.karos.app",
        "uri": "https://go.karos.fr/vianavigo?v=698b7506-6337-4236-8934-2276c60ee6ed"
      },
      "ios": {
        "universalLink": "https://go.karos.fr/vianavigo?v=698b7506-6337-4236-8934-2276c60ee6ed"
      }
    },
    "departureToPickupWalkingDistance": 608.5296776855071,
    "departureToPickupWalkingPolyline": "ijzhHig_MaYt`@",
    "departureToPickupWalkingTime": 457,
    "distance": 10629.35565078014,
    "driver": {
      "firstName": "Antonello",
      "gender": "M",
      "grade": 5,
      "id": "f72fe5cd-d287-45b3-b35b-7d2af56f9a66",
      "lastName": "M",
      "picture": "https://storage-download.googleapis.com/karos-de.appspot.com/p/20609877-29db-4ce5-bb03-76840c4868fa.jpg"
    },
    "driverArrivalAddress": "",
    "driverArrivalLat": 48.7849,
    "driverArrivalLng": 2.1832,
    "driverDepartureAddress": "",
    "driverDepartureDate": 1602068631,
    "driverDepartureLat": 48.8181,
    "driverDepartureLng": 2.398,
    "dropoffToArrivalWalkingDistance": 7650.494187651163,
    "dropoffToArrivalWalkingPolyline": "cwvhHgqgL~fBbxR",
    "dropoffToArrivalWalkingTime": 5752,
    "duration": 696,
    "id": "698b7506-6337-4236-8934-2276c60ee6ed",
    "journeyPolyline": "kd{hHse~LRf@Rf@bB~CjHjMjCnFz@nAbBrD?RRf@R?RSzEzJRf@z@nAjCbGz@bBRRz@bB~HzO~CjHRRnAvBf@bBvBbBz@nA?f@RvBrDrI?f@~CfJRRnAbGnArDRf@nArDf@vBz@jC?RRf@f@bBf@vBbBzE~CfJRz@?R~CnKRf@z@vBf@jCz@jCRf@Rf@Rf@RnARf@nA~Cf@bB?f@Rf@vBvG?Rz@vBf@vBz@jCRnAz@jCRRRz@bBnF?RbBnFbBzE?f@rDbLnArDRf@?f@?RnArDRnAnArDvBjH???f@f@R?RnFjRz@vBRnA?R?f@RbBf@?R{@S{@g@{@?Sg@{@kHcV??Rg@SSSSSR?f@f@R?RnFjRz@vBRnA?R?f@RbB?z@S??bGSfJoAr]g@~WoAz^wLv`Eg@fJoAjWg@bGsDrD",
    "passengerDropLat": 48.7821,
    "passengerDropLng": 2.1738,
    "passengerPickupLat": 48.8047,
    "passengerPickupLng": 2.2897,
    "pickupTime": 0,
    "preferences": {
      "isTalker": true,
      "music": true,
      "smoking": true
    },
    "price": {
      "amount": 2,
      "type": "PAYING"
    },
    "webUrl": "https://www.karos.fr/vianavigo?data=eyJmaXJzdE5hbWUiOiAiQW50b25lbGxvIiwgImlkIjogImY3MmZlNWNkLWQyODctNDViMy1iMzViLTdkMmFmNTZmOWE2NiIsICJncmFkZSI6IDUsICJhZ2UiOiAyMCwgImxhc3ROYW1lIjogIk0iLCAiZHVyYXRpb25fc2luY2VfbGFzdF9sb2dpbiI6IDk0LCAiZGVwYXJ0dXJlVG9QaWNrdXBXYWxraW5nUG9seWxpbmUiOiAiaWp6aEhpZ19NYVl0YEAiLCAicGljdHVyZSI6ICJodHRwczovL3N0b3JhZ2UtZG93bmxvYWQuZ29vZ2xlYXBpcy5jb20va2Fyb3MtZGUuYXBwc3BvdC5jb20vcC8yMDYwOTg3Ny0yOWRiLTRjZTUtYmIwMy03Njg0MGM0ODY4ZmEuanBnIiwgImRyb3BvZmZUb0Fycml2YWxXYWxraW5nVGltZSI6IDU3NTIsICJwaG9uZV92ZXJpZmllZCI6IHRydWUsICJkdXJhdGlvbiI6IDY5NiwgInByaWNlIjogeyJhbW91bnQiOiAyLjAsICJkaXNjb3VudCI6IDAuMH0sICJnZW5kZXIiOiAiTSIsICJyZXNwb25zZV9yYXRlIjogMC44NDg0ODQ4NDg0ODQ4NDg1LCAiZHJvcG9mZlRvQXJyaXZhbFdhbGtpbmdQb2x5bGluZSI6ICJjd3ZoSGdxZ0x+ZkJieFIiLCAiZGVwYXJ0dXJlX3RpbWUiOiAxNjAyMDY4NTM5LCAibmJfY2FycG9vbCI6IDQzLCAiZGVwYXJ0dXJlVG9QaWNrdXBXYWxraW5nVGltZSI6IDQ1NywgImpvdXJuZXlQb2x5bGluZSI6ICJrZHtoSHNlfkxSZkBSZkBiQn5DakhqTWpDbkZ6QG5BYkJyRD9SUmZAUj9SU3pFekpSZkB6QG5BakNiR3pAYkJSUnpAYkJ+SHpPfkNqSFJSbkF2QmZAYkJ2QmJCekBuQT9mQFJ2QnJEckk/ZkB+Q2ZKUlJuQWJHbkFyRFJmQG5BckRmQHZCekBqQz9SUmZAZkBiQmZAdkJiQnpFfkNmSlJ6QD9SfkNuS1JmQHpAdkJmQGpDekBqQ1JmQFJmQFJmQFJuQVJmQG5BfkNmQGJCP2ZAUmZAdkJ2Rz9SekB2QmZAdkJ6QGpDUm5BekBqQ1JSUnpAYkJuRj9SYkJuRmJCekU/ZkByRGJMbkFyRFJmQD9mQD9SbkFyRFJuQW5BckR2QmpIPz8/ZkBmQFI/Um5GalJ6QHZCUm5BP1I/ZkBSYkJmQD9Se0BTe0BnQHtAP1NnQHtAa0hjVj8/UmdAU1NTU1NSP2ZAZkBSP1JuRmpSekB2QlJuQT9SP2ZAUmJCP3pAUz8/YkdTZkpvQXJdZ0B+V29Bel53THZgRWdAZkpvQWpXZ0BiR3NEckQifQ==",
    "type": "PLANNED"
  },
  {
    "availableSeats": 3,
    "car": {
      "brand": "C3",
      "model": "Citroën"
    },
    "deepLink": {
      "android": {
        "storeUrl": "https://play.google.com/store/apps/details?id=fr.karos.karos.app",
        "uri": "https://go.karos.fr/vianavigo?v=07ac0ddc-ba55-49d6-b0bc-9bf6d3325e36"
      },
      "ios": {
        "universalLink": "https://go.karos.fr/vianavigo?v=07ac0ddc-ba55-49d6-b0bc-9bf6d3325e36"
      }
    },
    "departureToPickupWalkingDistance": 761.3410564385107,
    "departureToPickupWalkingPolyline": "ijzhHig_M|Vyt@",
    "departureToPickupWalkingTime": 572,
    "distance": 18048.637194930703,
    "driver": {
      "firstName": "caroline",
      "gender": "F",
      "grade": 5,
      "id": "19071ee5-f76a-4130-90ff-33551f91ed0f",
      "lastName": "t",
      "picture": null
    },
    "driverArrivalAddress": "",
    "driverArrivalLat": 48.7028,
    "driverArrivalLng": 2.1029,
    "driverDepartureAddress": "",
    "driverDepartureDate": 1602074700,
    "driverDepartureLat": 48.7967,
    "driverDepartureLng": 2.3037,
    "dropoffToArrivalWalkingDistance": 7315.830400003977,
    "dropoffToArrivalWalkingPolyline": "ogghHcvyKsfK~|D",
    "dropoffToArrivalWalkingTime": 5500,
    "duration": 1181,
    "id": "07ac0ddc-ba55-49d6-b0bc-9bf6d3325e36",
    "journeyPolyline": "kryhHc}`MziQ~ef@",
    "passengerDropLat": 48.7028,
    "passengerDropLng": 2.1029,
    "passengerPickupLat": 48.7967,
    "passengerPickupLng": 2.3037,
    "pickupTime": 0,
    "preferences": {
      "isTalker": true,
      "music": true,
      "smoking": false
    },
    "price": {
      "amount": 2,
      "type": "PAYING"
    },
    "type": "PLANNED",
    "webUrl": "https://www.karos.fr/vianavigo?data=eyJmaXJzdE5hbWUiOiAiY2Fyb2xpbmUiLCAiaWQiOiAiMTkwNzFlZTUtZjc2YS00MTMwLTkwZmYtMzM1NTFmOTFlZDBmIiwgImdyYWRlIjogNSwgImFnZSI6IDIwLCAibGFzdE5hbWUiOiAidCIsICJkdXJhdGlvbl9zaW5jZV9sYXN0X2xvZ2luIjogNjIzNCwgImRlcGFydHVyZVRvUGlja3VwV2Fsa2luZ1BvbHlsaW5lIjogImlqemhIaWdfTXxWeXRAIiwgInBpY3R1cmUiOiBudWxsLCAiZHJvcG9mZlRvQXJyaXZhbFdhbGtpbmdUaW1lIjogNTUwMCwgInBob25lX3ZlcmlmaWVkIjogdHJ1ZSwgImR1cmF0aW9uIjogMTE4MSwgInByaWNlIjogeyJhbW91bnQiOiAyLjAsICJkaXNjb3VudCI6IDAuMH0sICJnZW5kZXIiOiAiRiIsICJyZXNwb25zZV9yYXRlIjogMC45NjI5NjI5NjI5NjI5NjI5LCAiZHJvcG9mZlRvQXJyaXZhbFdhbGtpbmdQb2x5bGluZSI6ICJvZ2doSGN2eUtzZkt+fEQiLCAiZGVwYXJ0dXJlX3RpbWUiOiAxNjAyMDc0MTI3LCAibmJfY2FycG9vbCI6IDE0NiwgImRlcGFydHVyZVRvUGlja3VwV2Fsa2luZ1RpbWUiOiA1NzIsICJqb3VybmV5UG9seWxpbmUiOiAia3J5aEhjfWBNemlRfmVmQCJ9"
  }
]

"""

regex = re.compile(r'\\(?![/u"])')
fixed = regex.sub(r"\\\\", fake_response)

mock_get = mock.MagicMock(return_value=utils_test.MockResponse(json.loads(fixed), 200, '{}'))

DUMMY_KAROS_FEED_PUBLISHER = {'id': '42', 'name': '42', 'license': 'I dunno', 'url': 'http://w.tf'}

# A hack class
class DummyInstance:
    name = ''


def get_ridesharing_service_test():
    configs = [
        {
            "class": "jormungandr.scenarios.ridesharing.karos.Karos",
            "args": {
                "service_url": "toto",
                "api_key": "toto key",
                "network": "N",
                "timeout": 10,
                "timedelta": 7000,
                "departure_radius": 15,
                "arrival_radius": 15,
                "feed_publisher": DUMMY_KAROS_FEED_PUBLISHER,
            },
        },
        {
            "class": "jormungandr.scenarios.ridesharing.karos.Karos",
            "args": {"service_url": "tata", "api_key": "tata key", "network": "M"},
        },
    ]

    services = Ridesharing.get_ridesharing_services(DummyInstance(), configs)
    assert len(services) == 2

    assert services[0].service_url == 'toto'
    assert services[0].api_key == 'toto key'
    assert services[0].network == 'N'
    assert services[0].system_id == 'Karos'
    assert services[0].timeout == 10
    assert services[0].timedelta == 7000
    assert services[0].departure_radius == 15
    assert services[0].arrival_radius == 15
    assert services[0]._get_feed_publisher() == RsFeedPublisher(**DUMMY_KAROS_FEED_PUBLISHER)

    assert services[1].service_url == 'tata'
    assert services[1].api_key == 'tata key'
    assert services[1].network == 'M'
    assert services[1].system_id == 'Karos'
    # Default values
    assert services[1].timeout == 2
    assert services[1].timedelta == 3600
    assert services[1].departure_radius == 2
    assert services[1].arrival_radius == 2

    assert services[1]._get_feed_publisher() == RsFeedPublisher(**DEFAULT_KAROS_FEED_PUBLISHER)
