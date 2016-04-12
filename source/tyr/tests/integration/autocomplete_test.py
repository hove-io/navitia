from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
from navitiacommon import models
from tyr import app

@pytest.fixture
def create_autocomplete_parameter():
    with app.app_context():
        autocomplete_param = models.AutocompleteParameter('idf', 'OSM', 'BANO','FUSIO', 'OSM')
        models.db.session.add(autocomplete_param)
        models.db.session.commit()

@pytest.fixture
def create_two_autocomplete_parameters():
    with app.app_context():
        autocomplete_param1 = models.AutocompleteParameter('europe', 'OSM', 'BANO', 'OSM', 'OSM')
        autocomplete_param2 = models.AutocompleteParameter('france', 'OSM', 'OSM', 'FUSIO', 'OSM')
        models.db.session.add(autocomplete_param1)
        models.db.session.add(autocomplete_param2)
        models.db.session.commit()


@pytest.fixture
def autocomplete_parameter_json():
    return {"name": "peru", "street": "OSM", "address": "BANO", "poi": "FUSIO", "admin": "OSM"}


def test_get_autocomplete_parameters_empty():
    resp = api_get('/v0/autocomplete_parameters/')
    assert resp == []


def test_get_all_autocomplete(create_autocomplete_parameter):
    resp = api_get('/v0/autocomplete_parameters/')
    assert len(resp) == 1
    assert resp[0]['name'] == 'idf'
    assert resp[0]['street'] == 'OSM'
    assert resp[0]['address'] == 'BANO'
    assert resp[0]['poi'] == 'FUSIO'
    assert resp[0]['admin'] == 'OSM'


def test_get_autocomplete_by_name(create_two_autocomplete_parameters):
    resp = api_get('/v0/autocomplete_parameters/')
    assert len(resp) == 3

    resp = api_get('/v0/autocomplete_parameters/france')
    assert resp['name'] == 'france'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'OSM'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'


def test_post_autocomplete(autocomplete_parameter_json):
    resp = api_post('/v0/autocomplete_parameters', data=json.dumps(autocomplete_parameter_json), content_type='application/json')

    assert resp['name'] == 'peru'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'BANO'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'

def test_put_autocomplete(autocomplete_parameter_json):
    resp = api_get('/v0/autocomplete_parameters/france')
    assert resp['name'] == 'france'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'OSM'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'

    resp = api_put('/v0/autocomplete_parameters/france', data=json.dumps(autocomplete_parameter_json), content_type='application/json')

    assert resp['street'] == 'OSM'
    assert resp['address'] == 'BANO'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'


def test_delete_autocomplete():
    resp = resp = api_get('/v0/autocomplete_parameters/')
    assert len(resp) == 4
    resp = api_get('/v0/autocomplete_parameters/france')
    assert resp['name'] == 'france'

    _, status = api_delete('/v0/autocomplete_parameters/france', check=False, no_json=True)
    assert status == 204

    _, status = api_get('/v0/autocomplete_parameters/france', check=False)
    assert status == 404
