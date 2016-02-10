from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
from navitiacommon import models
from tyr import app


@pytest.fixture
def create_user():
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

@pytest.fixture
def create_multiple_users(request):
    with app.app_context():
        end_point = models.EndPoint()
        end_point.name = 'myEndPoint'
        billing_plan = models.BillingPlan()
        billing_plan.name = 'free'
        billing_plan.end_point = end_point

        user1 = models.User('foo', 'foo@example.com')
        user1.end_point = end_point
        user1.billing_plan = billing_plan

        user2 = models.User('foodefault', 'foo@example.com')
        user2.end_point = models.EndPoint.get_default()
        user2.billing_plan = models.BillingPlan.get_default(user2.end_point)

        models.db.session.add(end_point)
        models.db.session.add(billing_plan)
        models.db.session.add(user1)
        models.db.session.add(user2)
        models.db.session.commit()
        #we end the context but need to keep some id for later (object won't survive this lost!)
        d = {'user1': user1.id, 'user2': user2.id, 'end_point': end_point.id, 'billing_plan': billing_plan.id}

    #we can't truncate end_point and billing_plan, so we have to delete them explicitly
    def teardown():
        with app.app_context():
            end_point = models.EndPoint.query.get(d['end_point'])
            billing_plan = models.BillingPlan.query.get(d['billing_plan'])
            models.db.session.delete(end_point)
            models.db.session.delete(billing_plan)
            models.db.session.commit()
    request.addfinalizer(teardown)

    return d


def test_get_users_empty():
    resp = api_get('/v0/users/')
    assert resp == []

def test_add_user():
    """
    creation of a user passing arguments as a json
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp = api_post('/v0/users/', data=json.dumps(user), content_type='application/json')
    def check(u):
        for k,_ in user.iteritems():
            assert u[k] == user[k]
        assert u['end_point']['name'] == 'navitia.io'
        assert u['type'] == 'with_free_instances'
        assert u['block_until'] == None
    check(resp)

    resp = api_get('/v0/users/')
    assert len(resp) == 1
    check(resp[0])

def test_add_duplicate_login_user(create_user):
    user = {'login': 'test', 'email': 'user1@example.com'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 409

def test_add_duplicate_email_user(create_user):
    user = {'login': 'user', 'email': 'test@example.com'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 409

def test_add_user_invalid_email():
    """
    creation of a user with an invalid email
    """
    user = {'login': 'user1', 'email': 'user1'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400

def test_add_user_invalid_endpoint():
    """
    creation of a user with an invalid endpoint
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'end_point_id': 100}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400

def test_add_user_invalid_billingplan():
    """
    creation of a user with an invalid endpoint
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'billing_plan_id': 100}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400

def test_add_user_invalid_type():
    """
    creation of a user with an invalid endpoint
    """
    user = {'login': 'user1', 'email': 'user1@example.com', 'type': 'foo'}
    resp, status = api_post('/v0/users/', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 400


def test_multiple_users(create_multiple_users):
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

        if u['id'] == create_multiple_users['user2']:
            user2_found = True
            assert u['login'] == 'foodefault'
            assert u['email'] == 'foo@example.com'
            assert u['end_point']['name'] == 'navitia.io'
            assert u['billing_plan']['name'] == 'nav_ctp'

    assert user1_found
    assert user2_found


def test_delete_user(create_multiple_users):
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

def test_delete_invalid_user(create_multiple_users):
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

def test_update_invalid_user():
    """
    we try to update a user who dosn't exist
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp, status = api_put('/v0/users/10', check=False, data=json.dumps(user), content_type='application/json')
    assert status == 404

def test_update_user(create_multiple_users):
    """
    we update a user
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp = api_put('/v0/users/{}'.format(create_multiple_users['user1']), data=json.dumps(user),
                   content_type='application/json')
    assert resp['id'] == create_multiple_users['user1']
    #login can't be updated, we keep the original value
    assert resp['login'] == 'foo'
    assert resp['email'] == user['email']

def test_update_block_until(create_multiple_users):
    """
    we update a user
    """
    user = {'block_until': '20160128T111200Z'}
    resp = api_put('/v0/users/{}'.format(create_multiple_users['user1']), data=json.dumps(user),
                   content_type='application/json')
    assert resp['id'] == create_multiple_users['user1']
    assert resp['block_until'] == '2016-01-28T11:12:00'

def test_full_registration_then_deletion(create_instance):
    """
    we create a user, then a token for him, and finaly we give to him some authorization
    after that we delete him
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp_user = api_post('/v0/users/', data=json.dumps(user), content_type='application/json')

    resp_key = api_post('/v0/users/{}/keys'.format(resp_user['id']), data=json.dumps({'app_name': 'myApp'}),
                        content_type='application/json')
    auth = {'instance_id': create_instance, 'api_id': 1}
    resp_auth = api_post('/v0/users/{}/authorizations'.format(resp_user['id']), data=json.dumps(auth),
                         content_type='application/json')


    resp = api_get('/v0/users/{}'.format(resp_user['id']))

    assert len(resp['keys']) == 1
    assert resp['keys'][0]['app_name'] == 'myApp'
    assert len(resp['authorizations']) == 1
    assert resp['authorizations'][0]['instance']['name'] == 'instance'

    _, status = api_delete('/v0/users/{}'.format(resp_user['id']), check=False, no_json=True)
    assert status == 204
    _, status = api_get('/v0/users/{}'.format(resp_user['id']), check=False)
    assert status == 404


def test_deletion_keys_and_auth(create_instance):
    """
    We start by creating the user, it's easier than using a fixture, then we delete the auth and the key
    """
    user = {'login': 'user1', 'email': 'user1@example.com'}
    resp_user = api_post('/v0/users/', data=json.dumps(user), content_type='application/json')
    api_post('/v0/users/{}/keys'.format(resp_user['id']), data=json.dumps({'app_name': 'myApp'}),
                        content_type='application/json')
    auth = {'instance_id': create_instance, 'api_id': 1}
    api_post('/v0/users/{}/authorizations'.format(resp_user['id']), data=json.dumps(auth),
                         content_type='application/json')
    resp = api_get('/v0/users/{}'.format(resp_user['id']))

    resp_key = api_delete('/v0/users/{user_id}/keys/{key_id}'.format(user_id=resp['id'],
                                                                     key_id=resp['keys'][0]['id']))
    assert len(resp_key['keys']) == 0
    resp_auth = api_delete('/v0/users/{}/authorizations/'.format(resp['id']), data=json.dumps(auth),
                           content_type='application/json')
    assert len(resp_auth['authorizations']) == 0
