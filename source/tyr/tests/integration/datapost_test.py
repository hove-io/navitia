from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
import os
from navitiacommon import models
from tyr import app

@pytest.fixture
def create_instance2():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()
        return instance.id

def test_post_pbf(create_instance2):
    filename = 'empty_pbf.osm.pbf'
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename)
    files = {'file': (open(path, 'rb'), filename)}

    tester = app.test_client()
    resp = tester.post('/v0/data/instances/fr', data=files)
    assert resp.status_code == 200
    #assert os.path.isfile()
