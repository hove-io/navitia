from tests.check_utils import api_get, api_post, api_delete, api_put, _dt, _to_json
import json
import pytest
import os
from navitiacommon import models
from tyr import app
import logging

@pytest.fixture
def create_instance_fr():
    with app.app_context():
        instance = models.Instance('fr')
        ac_instance = models.AutocompleteParameter(name='fr', street='OSM', address='BANO', poi='OSM', admin='OSM',
                                                   admin_level=[8])
        models.db.session.add(instance)
        models.db.session.add(ac_instance)
        models.db.session.commit()
        return instance.id


def test_post_pbf(create_instance_fr):
    assert (not os.path.isfile('/tmp/empty_pbf.osm.pbf'))

    filename = 'empty_pbf.osm.pbf'
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename)
    f = open(path, 'rb')
    try:
        files = {'file': (f, filename)}
        tester = app.test_client()
        resp = tester.post('/v0/jobs/fr', data=files)
    finally:
        f.close()
    assert resp.status_code == 200
    assert os.path.isfile('/tmp/empty_pbf.osm.pbf')

    os.remove('/tmp/empty_pbf.osm.pbf')
    assert (not os.path.isfile('/tmp/empty_pbf.osm.pbf'))


def test_post_no_file(create_instance_fr):
    tester = app.test_client()
    resp = tester.post('/v0/jobs/fr')
    assert resp.status_code == 404


def test_post_bad_instance(create_instance_fr):
    tester = app.test_client()
    resp = tester.post('/v0/jobs/fr_ko')
    assert resp.status_code == 404


def test_post_pbf_autocomplete(create_instance_fr):
    assert (not os.path.isfile('/tmp/empty_pbf.osm.pbf'))

    filename = 'empty_pbf.osm.pbf'
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename)
    f = open(path, 'rb')
    try:
        files = {'file': (f, filename)}
        tester = app.test_client()
        response = tester.post('/v0/autocomplete_parameters/fr/update_data', data=files)
        json_response = json.loads(response.data)
        assert 'job' in json_response
        job = json_response['job']
        assert 'id' in job
        assert type(job['id']) == int
    finally:
        f.close()
