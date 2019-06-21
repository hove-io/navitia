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
from tyr import app
from navitiacommon import models

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


def create_5_users():
    with app.app_context():
        for i in range(5):
            user_name = 'user{}'.format(str(i))
            models.db.session.add(models.User(user_name, '{}@example.com'.format(user_name)))
        models.db.session.commit()


def test_users():
    create_5_users()
    check_v1_response('users')


def test_users_pagination():
    def check_resp_page(page_num, resp, is_last_page=False):
        assert len(resp['users']) == 2 if not is_last_page else 1
        assert 'pagination' in resp
        assert resp['pagination']['current_page'] == page_num
        assert resp['pagination']['total_items'] == 5
        assert resp['pagination']['items_per_page'] == 2 if not is_last_page else 1
        if not is_last_page:
            assert 'next' in resp['pagination']
            return resp['pagination']['next'].split('/')[-1]
        else:
            assert 'next' not in resp['pagination']

    create_5_users()
    app.config['MAX_ITEMS_PER_PAGE'] = 2

    first_page_resp = api_get('/v1/users')
    second_page_link = check_resp_page(1, first_page_resp)
    assert second_page_link

    second_page_resp = api_get('/v1/{}'.format(second_page_link))
    last_page_link = check_resp_page(2, second_page_resp)
    assert last_page_link

    last_page_resp = api_get('/v1/{}'.format(last_page_link))
    no_more_page_link = check_resp_page(3, last_page_resp, is_last_page=True)
    assert not no_more_page_link
