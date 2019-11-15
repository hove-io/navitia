# coding=utf-8

# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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
from .geocodejson import GeocodeJson, GeocodeJsonError

import pytest
import requests_mock


@requests_mock.mock()
def test_geocodejson_check_response(mock):
    url = 'https://geocode.json'
    mock.get('https://geocode.json/features/my_method', status_code=42, text='{"msg": "this is a response"}')

    gj = GeocodeJson(host=url)

    with pytest.raises(GeocodeJsonError) as exc:
        gj.get_by_uri('my_method')

    assert "42" in str(exc)
    assert "this is a response" in str(exc)
