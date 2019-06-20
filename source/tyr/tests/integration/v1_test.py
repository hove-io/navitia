# coding: utf-8
# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, division
from tests.check_utils import api_get

"""
Tests the differences of endpoints responses for API v1
"""


def check_v1_response(endpoint):
    resp_v0 = api_get('/v0/{}'.format(endpoint))
    assert type(resp_v0) == list
    resp_v1 = api_get('/v1/{}'.format(endpoint))
    assert type(resp_v1) == dict
    assert endpoint in resp_v1
    assert resp_v1[endpoint] == resp_v0


def test_api():
    check_v1_response('api')


def test_users():
    check_v1_response('users')
