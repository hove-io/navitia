# coding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
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

from __future__ import absolute_import, print_function, division
from tests.check_utils import api_get, api_post, api_delete, api_put
import json
import pytest
import mock
from navitiacommon import models
from navitiacommon.constants import DEFAULT_SHAPE_SCOPE
from tyr.rabbit_mq_handler import RabbitMqHandler
from tyr import app
from six.moves.urllib.parse import quote
import ujson


@pytest.fixture
def geojson_polygon():
    return {
        "type": "Feature",
        "geometry": {
            "type": "Polygon",
            "coordinates": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]]],
        },
    }


@pytest.fixture
def geojson_multipolygon():
    return {
        "type": "Feature",
        "geometry": {
            "type": "MultiPolygon",
            "coordinates": [
                [[[102.0, 2.0], [103.0, 2.0], [103.0, 3.0], [102.0, 3.0], [102.0, 2.0]]],
                [
                    [[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]],
                    [[100.2, 0.2], [100.8, 0.2], [100.8, 0.8], [100.2, 0.8], [100.2, 0.2]],
                ],
            ],
        },
    }


@pytest.fixture
def invalid_geojsonfixture():
    return {"type": "Feature", "geometry": {"type": "Point", "coordinates": []}}


@pytest.fixture
def create_user(geojson_polygon):
    with app.app_context():
        user = models.User('test', 'test@example.com')
        user.end_point = models.EndPoint.get_default()
        user.billing_plan = models.BillingPlan.get_default(user.end_point)
        user.shape = json.dumps(geojson_polygon)
        models.db.session.add(user)
        models.db.session.commit()
        return user.id


@pytest.fixture
def create_user_without_shape():
    with app.app_context():
        user = models.User('test', 'test@example.com')
        user.end_point = models.EndPoint.get_default()
        user.billing_plan = models.BillingPlan.get_default(user.end_point)
        models.db.session.add(user)
        models.db.session.commit()
        return user.id


@pytest.fixture
def create_instance():
    with app.app_context():
        instance = models.Instance('instance')
        models.db.session.add(instance)
        models.db.session.commit()
        return instance.id


@pytest.yield_fixture
def mock_rabbit():
    with mock.patch.object(RabbitMqHandler, 'publish') as m:
        yield m


@pytest.fixture
def create_multiple_users(request, geojson_polygon):
    with app.app_context():
        end_point = models.EndPoint()
        end_point.name = 'myEndPoint'
        billing_plan = models.BillingPlan()
        billing_plan.name = 'free'
        billing_plan.end_point = end_point

        user1 = models.User('foo', 'foo@example.com')
        user1.end_point = end_point
        user1.billing_plan = billing_plan
        user1.shape = json.dumps(geojson_polygon)

        user2 = models.User('foodefault', 'foo@example.com')
        user2.end_point = models.EndPoint.get_default()
        user2.billing_plan = models.BillingPlan.get_default(user2.end_point)

        models.db.session.add(end_point)
        models.db.session.add(billing_plan)
        models.db.session.add(user1)
        models.db.session.add(user2)
        models.db.session.commit()
        # we end the context but need to keep some id for later (object won't survive this lost!)
        d = {'user1': user1.id, 'user2': user2.id, 'end_point': end_point.id, 'billing_plan': billing_plan.id}

    # we can't truncate end_point and billing_plan, so we have to delete them explicitly
    def teardown():
        with app.app_context():
            end_point = models.EndPoint.query.get(d['end_point'])
            billing_plan = models.BillingPlan.query.get(d['billing_plan'])
            # sn_backend_authorization = models.SnBackendAuthorization.query.get(d['sn_backend_authorization'])
            models.db.session.delete(end_point)
            models.db.session.delete(billing_plan)
            # models.db.session.delete(sn_backend_authorization)
            models.db.session.commit()

    request.addfinalizer(teardown)

    return d


@pytest.fixture
def create_billing_plan():
    with app.app_context():
        billing_plan = models.BillingPlan(
            name='test',
            max_request_count=10,
            max_object_count=100,
            end_point_id=models.EndPoint.get_default().id,
        )
        models.db.session.add(billing_plan)
        models.db.session.commit()
        return billing_plan.id


def test_get_users_empty():
    resp = api_get('/v0/users/')
    assert resp == []


def assert_default_scop_shape(response):
    assert "shape_scope" in response
    assert len(response['shape_scope']) == len(DEFAULT_SHAPE_SCOPE)
    for ss in response['shape_scope']:
        assert ss in DEFAULT_SHAPE_SCOPE


def test_add_user_without_shape(mock_rabbit):
    """
    creation of a user without shape
    When we get this user, we should see
    shape = None and has_shape = False
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    data = json.dumps(user)
    resp = api_post('/v0/users/', data=data, content_type='application/json')

    def check(u):
        gen = (k for k in user if k != 'shape')
        for k in gen:
            assert u[k] == user[k]
        assert u['end_point']['name'] == 'navitia.io'
        assert u['type'] == 'with_free_instances'
        assert u['block_until'] is None
        # User created but not modified yet
        assert u['created_at'] is not None
        assert u['updated_at'] is None
        assert u['blocked_at'] is None

    check(resp)
    assert resp['shape'] is None
    assert resp['has_shape'] is False
    assert mock_rabbit.called
    assert_default_scop_shape(resp)

    # we did not give any coord, so we don't have some
    assert resp['default_coord'] is None

    # with disable_geojson=true by default
    resp = api_get('/v0/users/')
    assert len(resp) == 1
    check(resp[0])
    assert resp[0]['shape'] is None
    assert resp[0]['has_shape'] is False
    assert_default_scop_shape(resp[0])

    # with disable_geojson=false
    resp = api_get('/v0/users/?disable_geojson=false')
    assert len(resp) == 1
    check(resp[0])
    assert resp[0]['shape'] is None
    assert resp[0]['has_shape'] is False
    assert_default_scop_shape(resp[0])


def test_add_user(mock_rabbit, geojson_polygon):
    """
    creation of a user passing arguments as a json
    """
    coord = '2.37730;48.84550'
    user = {
        'login': 'user1',
        'email': 'user1@example.com',
        'shape': geojson_polygon,
        'has_shape': True,
        'default_coord': coord,
    }
    data = json.dumps(user)
    resp = api_post('/v0/users/', data=data, content_type='application/json')

    def check(u):
        gen = (k for k in user if k != 'shape')
        for k in gen:
            assert u[k] == user[k]
        assert u['end_point']['name'] == 'navitia.io'
        assert u['type'] == 'with_free_instances'
        assert u['block_until'] is None

    check(resp)
    assert resp['shape'] == geojson_polygon
    assert resp['default_coord'] == coord
    assert_default_scop_shape(resp)

    resp = api_get('/v0/users/')
    assert len(resp) == 1
    check(resp[0])
    assert resp[0]['shape'] == {}
    assert mock_rabbit.called
    assert_default_scop_shape(resp[0])


def test_add_user_with_multipolygon(mock_rabbit, geojson_multipolygon):
    """
    creation of a user with multipolygon shape
    status must be 200 when bragi will accept multipolygon shape
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'shape': geojson_multipolygon, 'has_shape': True}
    data = json.dumps(user)
    resp, status = api_post('/v0/users/', check=False, data=data, content_type='application/json')
    assert status == 400
    assert mock_rabbit.call_count == 0


def test_add_user_with_invalid_geojson(mock_rabbit, invalid_geojsonfixture):
    """
    creation of a user passing arguments as a json
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'shape': invalid_geojsonfixture, 'has_shape': True}
    data = json.dumps(user)
    resp, status = api_post('/v0/users/', check=False, data=data, content_type='application/json')
    assert status == 400
    assert mock_rabbit.call_count == 0


def test_add_user_with_invalid_coord(mock_rabbit):
    """
    creation of a user passing wrongly formated coord
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'default_coord': 'bob'}
    data = json.dumps(user)
    resp, status = api_post('/v0/users/', check=False, data=data, content_type='application/json')
    assert status == 400
    assert mock_rabbit.call_count == 0


def _check_user_resp(user_input, user_output):
    for key in user_input:
        assert user_output[key] == user_input[key]
    assert user_output['end_point']['name'] == 'navitia.io'
    assert user_output['type'] == 'with_free_instances'
    assert user_output['block_until'] is None


def test_add_user_with_plus(mock_rabbit):
    """
    creation of a user with a "+" in the email
    """
    user = {'login': 'user1+test@example.com', 'email': 'user1+test@example.com'}
    resp = api_post('/v0/users/', data=json.dumps(user), content_type='application/json')
    _check_user_resp(user, resp)

    resp = api_get('/v0/users/')
    assert len(resp) == 1
    _check_user_resp(user, resp[0])
    assert mock_rabbit.called
    assert_default_scop_shape(resp[0])


def test_add_user_with_plus_no_json(mock_rabbit):
    """
    creation of a user with a "+" in the email
    """
    user = {'login': 'user1+test@example.com', 'email': 'user1+test@example.com'}
    resp = api_post('/v0/users/', data=user)
    _check_user_resp(user, resp)
    assert_default_scop_shape(resp)

    resp = api_get('/v0/users/')
    assert len(resp) == 1
    _check_user_resp(user, resp[0])
    assert mock_rabbit.called
    assert_default_scop_shape(resp[0])


def test_add_user_with_plus_in_query(mock_rabbit):
    """
    creation of a user with a "+" in the email
    """
    user = {'email': 'user1+test@example.com', 'login': 'user1+test@example.com'}
    resp, status = api_post('/v0/users/?login={email}&email={email}'.format(email=user['email']), check=False)
    assert status == 400
    assert resp["error"] == 'email invalid, you give "user1 test******"'

    resp = api_post('/v0/users/?login={email}&email={email}'.format(email=quote(user['email'])))
    _check_user_resp(user, resp)
    assert_default_scop_shape(resp)

    resp = api_get('/v0/users/')
    assert len(resp) == 1
    _check_user_resp(user, resp[0])
    assert mock_rabbit.called
    assert_default_scop_shape(resp[0])


def test_add_duplicate_login_user(create_user, mock_rabbit):
    user = {'login': 'test', 'email': 'user1@example.com'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 409
    assert mock_rabbit.call_count == 0


def test_add_duplicate_email_user(create_user, mock_rabbit):
    user = {'login': 'user', 'email': 'test@example.com'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 409
    assert mock_rabbit.call_count == 0


def test_add_user_invalid_email(mock_rabbit):
    """
    creation of a user with an invalid email
    """
    user = {'login': 'user1', 'email': 'user1'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400
    assert resp["error"] == 'email invalid, you give "user1"'
    assert mock_rabbit.call_count == 0


def test_add_user_invalid_endpoint(mock_rabbit):
    """
    creation of a user with an invalid endpoint
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'end_point_id': 100}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400
    assert resp["error"] == 'end_point doesn\'t exist for user email user1******, you give "100"'
    assert mock_rabbit.call_count == 0


def test_add_user_invalid_billingplan(mock_rabbit):
    """
    creation of a user with an invalid endpoint
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'billing_plan_id': 100}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400
    assert resp["error"] == 'billing plan doesn\'t exist for user email user1******, you give "100"'
    assert mock_rabbit.call_count == 0


def test_add_user_invalid_type(mock_rabbit):
    """
    creation of a user with an invalid endpoint
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'type': 'foo'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400
    assert mock_rabbit.call_count == 0


def test_add_user_invalid_shape_scope(mock_rabbit):
    """
    creation of a user with an invalid shape_scope
    """
    user = {'login': 'user1', 'email': 'user1@example.com', "shape_scope": ["ab"]}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400
    assert "message" in resp
    message = resp["message"]
    assert "shape_scope" in message
    assert (
        message["shape_scope"]
        == u"The shape_scope argument must be in list ('admin', 'street', 'addr', 'poi', 'stop'), you gave ab"
    )
    assert mock_rabbit.call_count == 0


def test_add_user_valid_shape_scope(mock_rabbit):
    """
    creation of a user with an invalid shape_scope
    """
    user = {'login': 'user1', 'email': 'user1@example.com', "shape_scope": ["stop", "admin"]}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 200
    assert "shape_scope" in resp
    shape_scope = resp["shape_scope"]
    assert len(shape_scope) == len(user["shape_scope"])
    for ss in shape_scope:
        assert ss in user["shape_scope"]
    assert mock_rabbit.call_count == 1


def test_update_user_valid_shape_scope(create_multiple_users):
    """
    Update of a user with valid shape_scope
    """

    def check(u):
        for usr in u:
            # User created but not modified yet
            assert usr['created_at'] is not None
            assert usr['block_until'] is None
            assert usr['updated_at'] is None
            assert usr['blocked_at'] is None

    resp = api_get('/v0/users/')
    assert len(resp) == 2
    check(resp)

    for user in resp:
        assert_default_scop_shape(user)
    user = {'login': 'user1', 'email': 'user1@example.com', "shape_scope": ["poi", "stop"]}
    resp, status = api_put(
        '/v0/users/{}'.format(create_multiple_users['user1']),
        check=False,
        data=json.dumps(user),
        content_type='application/json',
    )
    assert status == 200
    assert len(resp["shape_scope"]) == len(user["shape_scope"])
    for ss in resp["shape_scope"]:
        assert ss in user["shape_scope"]
    # User modified but not blocked
    assert resp['created_at'] is not None
    assert resp['updated_at'] is not None
    assert resp['block_until'] is None
    assert resp['blocked_at'] is None


def test_update_user_invalid_shape_scope(create_multiple_users, mock_rabbit):
    """
    Update of a user with invalid shape_scope
    """
    resp = api_get('/v0/users/')
    assert len(resp) == 2

    for user in resp:
        assert_default_scop_shape(user)
    user = {'login': 'user1', 'email': 'user1@example.com', "shape_scope": ["bob"]}
    resp, status = api_put(
        '/v0/users/{}'.format(create_multiple_users['user1']),
        check=False,
        data=json.dumps(user),
        content_type='application/json',
    )
    assert status == 400
    assert "message" in resp
    message = resp["message"]
    assert "shape_scope" in message
    assert (
        message["shape_scope"]
        == u"The shape_scope argument must be in list ('admin', 'street', 'addr', 'poi', 'stop'), you gave bob"
    )
    assert mock_rabbit.call_count == 0


def test_multiple_users(create_multiple_users, mock_rabbit):
    """
    check the list
    """
    resp = api_get('/v0/users/')
    assert len(resp) == 2
    user1_found = False
    user2_found = False
    for u in resp:
        if u['id'] == create_multiple_users['user1']:
            user1_found = True
            assert u['login'] == 'foo'
            assert u['email'] == 'foo@example.com'
            assert u['end_point']['name'] == 'myEndPoint'
            assert u['billing_plan']['name'] == 'free'
            # Billing plan created but not modified yet
            assert u['billing_plan']['created_at'] is not None
            assert u['billing_plan']['updated_at'] is None

        if u['id'] == create_multiple_users['user2']:
            user2_found = True
            assert u['login'] == 'foodefault'
            assert u['email'] == 'foo@example.com'
            assert u['end_point']['name'] == 'navitia.io'
            assert u['billing_plan']['name'] == 'nav_ctp'
            # Billing plan created but not modified yet
            assert u['billing_plan']['created_at'] is not None
            assert u['billing_plan']['updated_at'] is None

    assert user1_found
    assert user2_found
    assert mock_rabbit.call_count == 0


def test_delete_user(create_multiple_users, mock_rabbit):
    """
    delete a user
    """
    resp, status = api_delete('/v0/users/{}'.format(create_multiple_users['user1']), check=False, no_json=True)
    assert status == 204

    resp, status = api_get('/v0/users/{}'.format(create_multiple_users['user1']), check=False)
    assert status == 404

    resp = api_get('/v0/users/')
    assert len(resp) == 1
    u = resp[0]
    assert u['id'] == create_multiple_users['user2']
    assert u['login'] == 'foodefault'
    assert u['email'] == 'foo@example.com'
    assert u['end_point']['name'] == 'navitia.io'
    assert u['billing_plan']['name'] == 'nav_ctp'
    # Billing plan created but not modified yet
    assert u['billing_plan']['created_at'] is not None
    assert u['billing_plan']['updated_at'] is None
    assert mock_rabbit.call_count == 1
    assert_default_scop_shape(u)


def test_delete_invalid_user(create_multiple_users, mock_rabbit):
    """
    we try to delete an invalid users, this must fail and after that we check out users to be sure
    """
    to_delete = 0
    while to_delete in create_multiple_users.values():
        to_delete = to_delete + 1
    resp, status = api_delete('/v0/users/{}'.format(to_delete), check=False, no_json=True)
    assert status == 404

    resp = api_get('/v0/users/')
    assert len(resp) == 2
    assert mock_rabbit.call_count == 0


def test_update_invalid_user(mock_rabbit):
    """
    we try to update a user who dosn't exist
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp, status = api_put('/v0/users/10', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 404
    assert mock_rabbit.call_count == 0


def test_update_user(create_multiple_users, mock_rabbit, geojson_polygon):
    """
    we update a user
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'shape': geojson_polygon}
    resp = api_put(
        '/v0/users/{}'.format(create_multiple_users['user1']),
        data=json.dumps(user),
        content_type='application/json',
    )

    def check(u):
        for key in user:
            assert u[key] == user[key]
        assert resp['id'] == create_multiple_users['user1']
        assert resp['login'] == user['login']
        assert resp['email'] == user['email']
        # user modified but not blocked
        assert resp['created_at'] is not None
        assert resp['updated_at'] is not None
        assert resp['block_until'] is None
        assert resp['blocked_at'] is None

    check(resp)
    assert mock_rabbit.called


def test_update_block_until(create_multiple_users, mock_rabbit, geojson_polygon):
    """
    we update a user
    """
    user = {'block_until': '20160128T111200Z'}
    resp = api_put(
        '/v0/users/{}'.format(create_multiple_users['user1']),
        data=json.dumps(user),
        content_type='application/json',
    )
    assert resp['id'] == create_multiple_users['user1']
    assert resp['block_until'] == '2016-01-28T11:12:00'
    # user modified and blocked
    assert resp['created_at'] is not None
    assert resp['updated_at'] is not None
    assert resp['blocked_at'] is not None
    assert resp['shape'] == geojson_polygon
    assert mock_rabbit.called


def test_update_shape(create_multiple_users, mock_rabbit, geojson_polygon):
    """
    we update a user
    """
    user = {'shape': geojson_polygon}
    resp = api_put(
        '/v0/users/{}'.format(create_multiple_users['user1']),
        data=json.dumps(user),
        content_type='application/json',
    )

    def check(u):
        for key in user:
            assert u[key] == user[key]
        assert resp['id'] == create_multiple_users['user1']

    check(resp)
    assert mock_rabbit.called


def test_update_shape_with_none(create_multiple_users, mock_rabbit):
    """
    we update a user
    """
    user = {'shape': None}
    resp = api_put(
        '/v0/users/{}'.format(create_multiple_users['user1']),
        data=json.dumps(user),
        content_type='application/json',
    )
    assert resp['id'] == create_multiple_users['user1']
    assert resp['shape'] is None
    assert mock_rabbit.called


def test_update_shape_with_empty(create_multiple_users, mock_rabbit, geojson_polygon):
    """
    we update a user
    """
    user = {'shape': {}}
    resp = api_put(
        '/v0/users/{}'.format(create_multiple_users['user1']),
        data=json.dumps(user),
        content_type='application/json',
    )
    assert resp['id'] == create_multiple_users['user1']
    assert resp['shape'] == geojson_polygon
    assert mock_rabbit.called


def test_full_registration_then_deletion(create_instance, mock_rabbit):
    """
    we create a user, then a token for him, and finaly we give to him some authorization
    after that we delete him
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp_user = api_post('/v0/users/', data=json.dumps(user), content_type='application/json')

    api_post(
        '/v0/users/{}/keys'.format(resp_user['id']),
        data=json.dumps({'app_name': 'myApp'}),
        content_type='application/json',
    )
    auth = {'instance_id': create_instance, 'api_id': 1}
    api_post(
        '/v0/users/{}/authorizations'.format(resp_user['id']),
        data=json.dumps(auth),
        content_type='application/json',
    )

    resp = api_get('/v0/users/{}'.format(resp_user['id']))

    assert len(resp['keys']) == 1
    assert resp['keys'][0]['app_name'] == 'myApp'
    assert len(resp['authorizations']) == 1
    assert resp['authorizations'][0]['instance']['name'] == 'instance'

    _, status = api_delete('/v0/users/{}'.format(resp_user['id']), check=False, no_json=True)
    assert status == 204
    assert mock_rabbit.called

    _, status = api_get('/v0/users/{}'.format(resp_user['id']), check=False)
    assert status == 404


def test_deletion_keys_and_auth(create_instance, mock_rabbit):
    """
    We start by creating the user, it's easier than using a fixture, then we delete the auth and the key
    """

    # first, test that with an unknown user, we get a 404
    _, status = api_delete('/v0/users/75/keys/1', check=False, no_json=True)
    assert status == 404

    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp_user = api_post('/v0/users/', data=json.dumps(user), content_type='application/json')
    api_post(
        '/v0/users/{}/keys'.format(resp_user['id']),
        data=json.dumps({'app_name': 'myApp'}),
        content_type='application/json',
    )
    auth = {'instance_id': create_instance, 'api_id': 1}
    api_post(
        '/v0/users/{}/authorizations'.format(resp_user['id']),
        data=json.dumps(auth),
        content_type='application/json',
    )
    resp = api_get('/v0/users/{}'.format(resp_user['id']))
    assert len(resp['keys']) == 1
    # key created but not modified yet
    assert resp['keys'][0]['created_at'] is not None
    assert resp['keys'][0]['updated_at'] is None
    resp_key = api_delete(
        '/v0/users/{user_id}/keys/{key_id}'.format(user_id=resp['id'], key_id=resp['keys'][0]['id'])
    )
    assert len(resp_key['keys']) == 0
    resp_auth = api_delete(
        '/v0/users/{}/authorizations/'.format(resp['id']), data=json.dumps(auth), content_type='application/json'
    )
    assert len(resp_auth['authorizations']) == 0
    assert mock_rabbit.called


def test_get_user_with_shape(create_user, geojson_polygon):
    """
    We start by creating the user with a shape,
    and we test that the attribute shape={} and has_shape = True
    """
    print(api_get('/v0/users'))
    resp = api_get('/v0/users/{}'.format(create_user))

    assert resp['has_shape'] is True
    assert resp['shape'] == {}


def test_get_user_with_shape_and_disable_geojson_param_false(create_user, geojson_polygon):
    """
    We start by creating the user with a shape.
    We request the user with parameter disable_geojson=true
    We test that shape = geojson and has_shape = True
    """
    resp = api_get('/v0/users/{}?disable_geojson=false'.format(create_user))

    assert resp['has_shape'] is True
    assert resp['shape'] == geojson_polygon


def test_get_user_without_shape(create_user_without_shape):
    """
    We start by creating the user without shape,
    and we test that  shape = None and has_shape = False
    """
    resp = api_get('/v0/users/{}'.format(create_user_without_shape))
    print(resp['shape'])
    assert resp['has_shape'] is False
    assert resp['shape'] is None
    assert resp['shape'] is None


def test_get_user_without_shape_and_disable_geojson_param_false(create_user_without_shape):
    """
    We start by creating the user without shape.
    We request the user with parameter disable_geojson=true
    We test that shape = None and has_shape = False
    """
    resp = api_get('/v0/users/{}?disable_geojson=false'.format(create_user_without_shape))

    assert resp['has_shape'] is False
    assert resp['shape'] is None


def test_get_users(create_multiple_users):
    """
    We start by creating a user with a shape and a user without shape,
    we test that:
    user1.has_shape = True
    user1.shape = {}
    user2.has_shape = False
    user2.shape = None
    """
    resp = api_get('/v0/users')

    foo = next((u for u in resp if u.get('login') == 'foo'), None)
    assert foo
    assert foo.get('has_shape') is True
    assert foo.get('shape') == {}

    foodefault = next((u for u in resp if u.get('login') == 'foodefault'), None)
    assert foodefault
    assert foodefault.get('has_shape') is False
    assert foodefault.get('shape') is None


def test_get_users_with_disable_geojson_false(create_multiple_users, geojson_polygon):
    """
    We start by creating a user with a shape and a user without shape,
    we test that requesting /users?disable_geojson=false:
    user1.has_shape = True
    user1.shape = geojson
    user2.has_shape = False
    user2.shape = None
    """
    resp = api_get('/v0/users?disable_geojson=false')

    foo = next((u for u in resp if u.get('login') == 'foo'), None)
    assert foo
    assert foo.get('has_shape') is True
    assert foo.get('shape') == geojson_polygon

    foodefault = next((u for u in resp if u.get('login') == 'foodefault'), None)
    assert foodefault
    assert foodefault.get('has_shape') is False
    assert foodefault.get('shape') is None


def test_get_billing_plan(create_billing_plan):
    """
    We create a billing_plan.
    """
    resp = api_get('/v0/billing_plans/{}'.format(create_billing_plan))

    assert resp['name'] == 'test'
    assert resp['max_request_count'] == 10
    assert resp['max_object_count'] == 100


def test_delete_billing_plan(create_billing_plan):
    """
    We start by creating a billing_plan.
    Delete the billing_plan
    """
    resp = api_get('/v0/billing_plans/{}'.format(create_billing_plan))

    _, status = api_delete('/v0/billing_plans/{}'.format(resp['id']), check=False, no_json=True)
    assert status == 204


def test_delete_billing_plan_used_by_an_user(create_user, geojson_polygon):
    """
    We start by creating the user with a shape.
    We request the user with parameter disable_geojson=false
    A default billing_plan is created and used with name = 'nav_ctp'
    We try to delete the billing_plan of this user but in vain.
    """
    resp = api_get('/v0/users/{}?disable_geojson=false'.format(create_user))

    assert resp['billing_plan']['name'] == 'nav_ctp'
    assert resp['has_shape'] is True
    assert resp['shape'] == geojson_polygon

    _, status = api_delete('/v0/billing_plans/{}'.format(resp['billing_plan']['id']), check=False, no_json=True)
    assert status == 409


def test_filter_users_by_key(create_user, create_multiple_users):
    resp_users = api_get('/v0/users')
    assert len(resp_users) == 3
    for user in resp_users:
        api_post(
            '/v0/users/{}/keys'.format(user['id']),
            data=json.dumps({'app_name': 'myApp'}),
            content_type='application/json',
        )

        resp_user = api_get('/v0/users/{}'.format(user['id']))
        assert len(resp_user['keys']) == 1
        assert 'token' in resp_user['keys'][0]

        token = resp_user['keys'][0]['token']
        resp_token = api_get('/v0/users?key={}'.format(token))
        assert resp_token['id'] == user['id']


def test_streetnetwork_backend_authorization_for_a_user(create_user, create_multiple_users):
    resp_users = api_get('/v1/users')
    assert len(resp_users['users']) == 3
    user_id = resp_users['users'][0]['id']
    assert user_id == 47

    # No street network backend is added to the user yet.
    assert resp_users['users'][0]['has_sn_backend'] is False

    # Add three streetnetwork backends for the test
    new_backend = {'klass': 'valhalla.klass'}
    resp, status = api_post(
        'v0/streetnetwork_backends/valhallaDev',
        data=ujson.dumps(new_backend),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert resp['streetnetwork_backend']['id'] == "valhallaDev"

    new_backend = {'klass': 'asgard.klass'}
    resp, status = api_post(
        'v0/streetnetwork_backends/asgardDev',
        data=ujson.dumps(new_backend),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert resp['streetnetwork_backend']['id'] == "asgardDev"

    # Add sn_backend_authorization for the user
    new_obj = {'sn_backend_id': 'valhallaDev', 'mode': 'walking'}
    resp, status = api_post(
        'v1/users/{}/sn_backend_authorizations'.format(user_id),
        data=ujson.dumps(new_obj),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert resp['sn_backend_id'] == 'valhallaDev'
    assert resp['mode'] == 'walking'
    assert resp['user_id'] == user_id

    # We cannot add another sn_backend for the same user + mode
    new_obj = {'sn_backend_id': 'asgardDev', 'mode': 'walking'}
    resp, status = api_post(
        'v1/users/{}/sn_backend_authorizations'.format(user_id),
        data=ujson.dumps(new_obj),
        content_type='application/json',
        check=False,
    )
    assert status == 409
    assert 'duplicate key value' in resp['error']

    new_obj = {'sn_backend_id': 'asgardDev', 'mode': 'car'}
    resp, status = api_post(
        'v1/users/{}/sn_backend_authorizations'.format(user_id),
        data=ujson.dumps(new_obj),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert resp['sn_backend_id'] == 'asgardDev'
    assert resp['mode'] == 'car'
    assert resp['user_id'] == user_id

    # # Calling api /users/user_id/sn_backend_authorizations to verify street_network backends
    resp, status = api_get('/v1/users/{}/sn_backend_authorizations'.format(user_id), check=False)
    assert status == 200
    assert len(resp['sn_backend_authorizations']) == 2

    # Calling /user/user_id to verify that has_sn_backend is True and street_network backends
    resp = api_get('/v1/users/{}'.format(user_id))
    assert resp['users']['id'] == user_id
    assert resp['users']['has_sn_backend'] is True
    assert len(resp['users']['sn_backend_authorizations']) == 2

    # Testing api delete()
    resp, status = api_delete(
        "/v1/users/{}/sn_backend_authorizations?mode={}".format(user_id, "car"),
        check=False,
        no_json=True,
    )
    assert status == 204
    resp, status = api_delete(
        "/v1/users/{}/sn_backend_authorizations?mode={}".format(user_id, "walking"),
        check=False,
        no_json=True,
    )
    assert status == 204

    # Calling api /users/user_id/sn_backend_authorizations to verify that sn_backend is absent
    resp, status = api_get('/v1/users/{}/sn_backend_authorizations'.format(user_id), check=False)
    assert status == 200
    assert len(resp['sn_backend_authorizations']) == 0

    # Calling /user/user_id to verify that has_sn_backend is False and sn_backend is absent
    resp = api_get('/v1/users/{}'.format(user_id))
    assert resp['users']['id'] == user_id
    assert resp['users']['has_sn_backend'] is False
    assert len(resp['users']['sn_backend_authorizations']) == 0
