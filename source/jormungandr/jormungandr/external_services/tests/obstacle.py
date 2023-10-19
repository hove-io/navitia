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
from jormungandr.external_services.obstacle import ObstacleProvider

mock_data = {
    "obstacles": [
        {
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "random",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
            "photo": "https://imgur.com",
            "distance": 200,
        },
                {
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "random",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
            "photo": "https://imgur.com",
            "distance": 200,
        },
                {
            "id": "dmVsaWI6U1RBVElPTjoxMjAwOQ==",
            "type": "random",
            "coord": {"lat": 48.84617146118711, "lon": 2.3793068155646324},
            "photo": "https://imgur.com",
            "distance": 200,
        },
       ]
}


def obstacle_get_information_test():
    """
    Test that 'obstacles'
    """
    service = ObstacleProvider(id="obstacle", service_url="obstacle.url", timeout=3)
    brut_result = service._call_webservice = MagicMock(return_value=mock_data)
    obstacles = brut_result.return_value.get('obstacles', [])
    assert len(obstacles) == 3
