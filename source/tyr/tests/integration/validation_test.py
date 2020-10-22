# coding: utf-8
# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
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

from __future__ import absolute_import, print_function, unicode_literals
import pytest
import mock
from tyr.validations import InputJsonValidator
from tyr import app
from werkzeug.exceptions import BadRequest
from tyr.formats import ridesharing_service_format
from flask import request


def func_mock(a, **kwargs):
    pass


def test_without_id():
    with pytest.raises(BadRequest) as info:
        kwargs = dict()
        InputJsonValidator(json_format=None)(func_mock)(**kwargs)
    assert info.value.code == 400
    assert info.value.data["message"] == "id is required"
    assert info.value.data["status"] == "error"


def test_with_empty_id():
    with pytest.raises(BadRequest) as info:
        kwargs = {"id": ""}
        InputJsonValidator(json_format=None)(func_mock)(**kwargs)
    assert info.value.code == 400
    assert info.value.data["message"] == "id is required"
    assert info.value.data["status"] == "error"


def test_empty_json():
    with app.test_request_context():
        with mock.patch.object(request, 'get_json', return_value=None):
            with pytest.raises(BadRequest) as info:
                kwargs = {"id": 10}
                InputJsonValidator(json_format=ridesharing_service_format)(func_mock)(**kwargs)
            assert info.value.code == 400
            assert info.value.data["status"] == "invalid data"


def test_json_without_klass():
    service = {
        'args': {
            'service_url': 'https://new_url.io',
            "rating_scale_min": 0,
            "crowfly_radius": 600,
            "api_key": "1235",
        }
    }
    with app.test_request_context():
        with mock.patch.object(request, 'get_json', return_value=service):
            kwargs = {"id": 10}
            with pytest.raises(BadRequest) as info:
                InputJsonValidator(json_format=ridesharing_service_format)(func_mock)(**kwargs)
            assert info.value.code == 400
            assert info.value.data["status"] == "invalid data"
            assert info.value.data["message"] == "'klass' is a required property"


def test_json_without_api_key():
    service = {
        'klass': 'jormungandr.scenarios.ridesharing.instant_system.InstantSystem',
        'args': {'service_url': 'https://new_url.io', "rating_scale_min": 0, "crowfly_radius": 600},
    }
    with app.test_request_context():
        with mock.patch.object(request, 'get_json', return_value=service):
            kwargs = {"id": 10}
            with pytest.raises(BadRequest) as info:
                InputJsonValidator(json_format=ridesharing_service_format)(func_mock)(**kwargs)
            assert info.value.code == 400
            assert info.value.data["status"] == "invalid data"
            assert info.value.data["message"] == "'api_key' is a required property"
