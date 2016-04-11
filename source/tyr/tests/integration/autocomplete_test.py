from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
from navitiacommon import models
from tyr import app

@pytest.fixture
def create_autocomplete():
    with app.app_context():
        autocomplete = models.Autocomplete('idf', 'BANO', 'BANO', 'FUSIO', 'FUSIO', 'OSM')
        models.db.session.add(autocomplete)
        models.db.session.commit()

@pytest.fixture
def create_two_autocompletes():
    with app.app_context():
        autocomplete1 = models.Autocomplete('europe', 'BANO', 'BANO', 'FUSIO', 'FUSIO', 'OSM')
        autocomplete2 = models.Autocomplete('france', 'OSM', 'OpenAddresses', 'BANO', 'PagesJaunes', 'FUSIO')
        models.db.session.add(autocomplete1)
        models.db.session.add(autocomplete2)
        models.db.session.commit()


@pytest.fixture
def autocomplete_json():
    return {"shape": "peru", "street": "BANO", "address": "BANO", "stop_area": "FUSIO", "poi": "FUSIO", "admin": "OSM"}


def test_get_autocompletes_empty():
    resp = api_get('/v0/autocompletes/')
    assert resp == []


def test_get_all_autocomplete(create_autocomplete):
    resp = api_get('/v0/autocompletes/')
    assert len(resp) == 1
    assert resp[0]['shape'] == 'idf'
    assert resp[0]['street'] == 'BANO'
    assert resp[0]['address'] == 'BANO'
    assert resp[0]['stop_area'] == 'FUSIO'
    assert resp[0]['poi'] == 'FUSIO'
    assert resp[0]['admin'] == 'OSM'


def test_get_autocomplete_by_shape(create_two_autocompletes):
    resp = api_get('/v0/autocompletes/')
    assert len(resp) == 3

    resp = api_get('/v0/autocompletes/france')
    assert resp['shape'] == 'france'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'OpenAddresses'
    assert resp['stop_area'] == 'BANO'
    assert resp['poi'] == 'PagesJaunes'
    assert resp['admin'] == 'FUSIO'


def test_post_autocomplete(autocomplete_json):
    resp = api_post('/v0/autocompletes', data=json.dumps(autocomplete_json), content_type='application/json')

    assert resp['shape'] == 'peru'
    assert resp['street'] == 'BANO'
    assert resp['address'] == 'BANO'
    assert resp['stop_area'] == 'FUSIO'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'

def test_put_autocomplete(autocomplete_json):
    resp = api_get('/v0/autocompletes/france')
    assert resp['shape'] == 'france'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'OpenAddresses'
    assert resp['stop_area'] == 'BANO'
    assert resp['poi'] == 'PagesJaunes'
    assert resp['admin'] == 'FUSIO'

    resp = api_put('/v0/autocompletes/france', data=json.dumps(autocomplete_json), content_type='application/json')

    assert resp['street'] == 'BANO'
    assert resp['address'] == 'BANO'
    assert resp['stop_area'] == 'FUSIO'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'


def test_delete_autocomplete():
    resp = resp = api_get('/v0/autocompletes/')
    assert len(resp) == 4
    resp = api_get('/v0/autocompletes/france')
    assert resp['shape'] == 'france'

    _, status = api_delete('/v0/autocompletes/france', check=False, no_json=True)
    assert status == 204

    _, status = api_get('/v0/autocompletes/france', check=False)
    assert status == 404




