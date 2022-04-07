# encoding: utf-8
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
from mock import MagicMock
from jormungandr.external_services.free_floating import FreeFloatingProvider

mock_data = {
    "free_floatings": [
        {
            "public_id": "12009",
            "provider_name": "Velib",
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "STATION",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
        },
        {
            "public_id": "FB-435-HD",
            "provider_name": "Cityscoot",
            "id": "Y2l0eXNjb290Ok1PVE9SU0NPT1RFUjpGQi00MzUtSEQ=",
            "type": "MOTORSCOOTER",
            "coord": {"lat": 48.848715, "lon": 2.37618},
            "propulsion": "ELECTRIC",
            "battery": 91,
            "attributes": ["ELECTRIC"],
            "deeplink": "https://cityscoot.onelink.me/8GT9?pid=FLUCTUO&c=CS-API-FLUCTUO&af_dp=cityscoot%3A%2F%2Fcities%2F4%2Fscooters%2F18055&af_web_dp=https%3A%2F%2Fwww.cityscoot.eu%2F&af_force_deeplink=true",
        },
        {
            "public_id": "FH-116-WN",
            "provider_name": "Cityscoot",
            "id": "Y2l0eXNjb290Ok1PVE9SU0NPT1RFUjpGSC0xMTYtV04=",
            "type": "MOTORSCOOTER",
            "coord": {"lat": 48.84902833333333, "lon": 2.376105},
            "propulsion": "ELECTRIC",
            "battery": 91,
            "attributes": ["ELECTRIC"],
            "deeplink": "https://cityscoot.onelink.me/8GT9?pid=FLUCTUO&c=CS-API-FLUCTUO&af_dp=cityscoot%3A%2F%2Fcities%2F4%2Fscooters%2F20840&af_web_dp=https%3A%2F%2Fwww.cityscoot.eu%2F&af_force_deeplink=true",
        },
    ]
}


def free_floating_get_information_test():
    """
    Test that 'free_floatings'
    """
    service = FreeFloatingProvider(service_url="free_floating.url", timeout=3)
    brut_result = service._call_webservice = MagicMock(return_value=mock_data)
    free_floatings = brut_result.return_value.get('free_floatings', [])
    assert len(free_floatings) == 3
