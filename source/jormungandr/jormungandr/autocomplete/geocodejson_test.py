# coding=utf-8

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
from .geocodejson import GeocodeJson, GeocodeJsonError

import pytest
import requests_mock


class FakeInstance:
    def __init__(self, name="test", poi_dataset="priv.test"):
        self.name = name
        self.poi_dataset = poi_dataset


def test_geocodejson_check_response():
    url = 'https://geocode.json'
    with requests_mock.Mocker() as m:
        m.get('https://geocode.json/features/my_method', status_code=42, text='{"msg": "this is a response"}')

        gj = GeocodeJson(host=url)

        with pytest.raises(GeocodeJsonError) as exc:
            gj.get_by_uri('my_method', request_id="123")

        assert "42" in str(exc)
        assert "this is a response" in str(exc)


def test_geocodejson_config_default_timeouts():
    url = 'https://geocode.json'
    gj = GeocodeJson(host=url)
    assert gj.timeout == 2
    assert gj.timeout_bragi_es == 2

    assert gj.fast_timeout == 0.2
    assert gj.fast_timeout_bragi_es == 0.2

    request = {"q": "abcd", "count": 10, "type[]": ["stop_area"], "request_id": "001"}
    params = gj.make_params(request, [FakeInstance()])

    timeout_bragi_es = next((param[1] for param in params if param[0] == 'timeout'), None)
    assert timeout_bragi_es == 2000


def test_geocodejson_config_timeout_bragi_es():
    url = 'https://geocode.json'
    gj = GeocodeJson(host=url, timeout_bragi_es=1.8)
    assert gj.timeout == 2
    assert gj.timeout_bragi_es == 1.8

    assert gj.fast_timeout == 0.2
    assert gj.fast_timeout_bragi_es == 0.2

    request = {"q": "abcd", "count": 10, "type[]": ["stop_area"], "request_id": "001"}
    params = gj.make_params(request, [FakeInstance()])

    timeout_bragi_es = next((param[1] for param in params if param[0] == 'timeout'), None)
    assert timeout_bragi_es == 1800
