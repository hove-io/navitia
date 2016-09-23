from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
from navitiacommon import models
from tyr import app

@pytest.fixture
def create_instance():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()
        return instance.id

def test_post_pbf(create_instance):
    filename = 'empty_pbf.osm.pbf'
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'fixtures/', filename)
    files = {'file': (open(path, 'rb'), filename)}

    resp = api_post('/v0/data/instances/{}'.fomat(create_instance), data=files)
    assert resp.status_code == 200
    assert len(resp) == 1
    assert resp[0]['name'] == 'fr'
    assert resp[0]['id'] == create_instance
