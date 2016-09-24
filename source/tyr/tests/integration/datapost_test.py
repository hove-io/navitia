from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
import os
from navitiacommon import models
from tyr import app
import logging

@pytest.fixture(scope="module")
def create_instance_fr():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()
        return instance.id

def test_post_pbf(create_instance_fr):
    filename = 'empty_pbf.osm.pbf'
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename)
    files = {'file': (open(path, 'rb'), filename)}

    tester = app.test_client()
    resp = tester.post('/v0/data/instances/fr', data=files)
    assert resp.status_code == 200
    assert os.path.isfile('/tmp/empty_pbf.osm.pbf')
    os.remove('/tmp/empty_pbf.osm.pbf')
    assert (not os.path.isfile('/tmp/empty_pbf.osm.pbf'))

def test_post_no_file(create_instance_fr):
    tester = app.test_client()
    resp = tester.post('/v0/data/instances/fr')
    assert resp.status_code == 404

def test_get(create_instance_fr):
    tester = app.test_client()
    resp = tester.get('/v0/data/instances/fr')
    assert resp.status_code == 400
