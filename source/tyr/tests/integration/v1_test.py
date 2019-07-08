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
from tests.check_utils import api_get, api_post, api_put, api_delete
from tyr import app
from navitiacommon import models

import pytest
import ujson as json

"""
Tests the differences of endpoints responses for API v1
"""


@pytest.fixture
def create_5_users():
    with app.app_context():
        for i in range(5):
            user_name = 'user{}'.format(str(i))
            user = models.User(user_name, '{}@example.com'.format(user_name))
            models.db.session.add(user)
        models.db.session.commit()

        # Return only 1 id for test purpose
        yield user.id

        for user in models.User.query.all():
            models.db.session.delete(user)
        models.db.session.commit()


@pytest.fixture
def create_instance():
    with app.app_context():
        instance = models.Instance('test_instance')
        models.db.session.add(instance)
        models.db.session.commit()

        yield instance.id

        models.db.session.delete(instance)


@pytest.fixture
def create_api():
    with app.app_context():
        api = models.Api('test_api')
        models.db.session.add(api)
        models.db.session.commit()

        yield api.id

        models.db.session.delete(api)


def check_v1_response(endpoint, request=None):
    if not request:
        request = endpoint
    resp_v0 = api_get('/v0/{}'.format(request))
    assert endpoint not in resp_v0

    resp_v1 = api_get('/v1/{}'.format(request))
    assert type(resp_v1) == dict
    assert endpoint in resp_v1
    assert sorted(resp_v1[endpoint]) == sorted(resp_v0)


def test_api():
    check_v1_response('api')


def test_users(create_5_users):
    check_v1_response('users')
    check_v1_response('users', 'users/{}'.format(create_5_users))


def test_users_methods():
    user_data = {'login': 'user1', 'email': 'user1@example.com'}
    resp_post = api_post('/v1/users', data=json.dumps(user_data), content_type='application/json')
    assert 'user' in resp_post
    assert resp_post['user']['login'] == 'user1'
    assert resp_post['user']['email'] == 'user1@example.com'
    user_id = resp_post['user']['id']

    user_data_update = {'type': 'super_user'}
    resp_put = api_put(
        '/v1/users/{}'.format(user_id), data=json.dumps(user_data_update), content_type='application/json'
    )
    assert 'user' in resp_put
    assert resp_put['user']['login'] == 'user1'
    assert resp_put['user']['type'] == 'super_user'

    resp_delete, status_delete = api_delete('/v1/users/{}'.format(user_id), check=False, no_json=True)
    assert status_delete == 204

    resp_get, status_get = api_get('/v1/users/{}'.format(user_id), check=False)
    assert status_get == 404


def test_users_pagination(create_5_users):
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

    app.config['MAX_ITEMS_PER_PAGE'] = 2

    first_page_resp = api_get('/v1/users')
    check_resp_page(1, first_page_resp)

    second_page_resp = api_get(first_page_resp['pagination']['next'])
    check_resp_page(2, second_page_resp)

    last_page_resp = api_get(second_page_resp['pagination']['next'])
    check_resp_page(3, last_page_resp, is_last_page=True)


def test_keys(create_5_users):
    check_v1_response('keys', 'users/{}/keys'.format(create_5_users))


def test_keys_methods(create_5_users):
    user_id = create_5_users
    resp_post = api_post(
        '/v1/users/{}/keys'.format(create_5_users),
        data=json.dumps({'app_name': 'testApp', 'valid_until': '2020-01-01'}),
        content_type='application/json',
    )
    assert 'keys' in resp_post['user']
    assert len(resp_post['user']['keys']) == 1
    assert resp_post['user']['keys'][0]['app_name'] == 'testApp'
    assert resp_post['user']['keys'][0]['valid_until'] == '2020-01-01'

    key_id = resp_post['user']['keys'][0]['id']
    resp_put = api_put(
        '/v1/users/{}/keys/{}'.format(user_id, key_id),
        data=json.dumps({'app_name': 'testApp', 'valid_until': '2021-01-01'}),
        content_type='application/json',
    )
    assert len(resp_put['user']['keys']) == 1
    assert resp_put['user']['keys'][0]['app_name'] == 'testApp'
    assert resp_put['user']['keys'][0]['valid_until'] == '2021-01-01'

    resp_delete = api_delete('/v1/users/{}/keys/{}'.format(user_id, key_id))
    assert len(resp_delete['user']['keys']) == 0


def test_authorization_methods(create_5_users, create_instance, create_api):
    user_id = create_5_users
    full_user = api_get('/v1/users/{}'.format(user_id))
    assert len(full_user['users']['authorizations']) == 0
    auth = {'instance_id': create_instance, 'api_id': create_api}
    resp_post = api_post(
        '/v1/users/{}/authorizations'.format(user_id), data=json.dumps(auth), content_type='application/json'
    )
    assert len(resp_post['user']['authorizations']) == 1

    assert resp_post['user']['authorizations'][0]['api']['name'] == 'test_api'
    assert resp_post['user']['authorizations'][0]['instance']['name'] == 'test_instance'

    resp_delete = api_delete(
        '/v1/users/{}/authorizations'.format(user_id), data=json.dumps(auth), content_type='application/json'
    )
    assert len(resp_delete['user']['authorizations']) == 0
