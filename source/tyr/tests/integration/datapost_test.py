import json
import pytest
import os
from navitiacommon import models
from tyr import app
from flask_restful import current_app
import shutil
from tests.check_utils import api_post

@pytest.fixture
def create_instance_fr():
    with app.app_context():
        navitia_instance = models.Instance('fr')
        autocomplete_instance = models.AutocompleteParameter(name='fr',
                                                             street='OSM', address='BANO', poi='OSM', admin='OSM',
                                                             admin_level=[8])
        models.db.session.add(navitia_instance)
        models.db.session.add(autocomplete_instance)
        models.db.session.commit()


def get_jobs_from_db():
    with app.app_context():
        return models.Job.get_all()

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
    with app.app_context():
        ac_path = current_app.config['TYR_AUTOCOMPLETE_DIR']
        assert not os.path.exists(ac_path)
        filename = 'empty_pbf.osm.pbf'
        path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename)
        with open(path, 'rb') as f:
            files = {'file': (f, filename)}
            json_response = api_post('/v0/autocomplete_parameters/fr/update_data', data=files)
            assert 'job' in json_response
            job = json_response['job']
            assert 'id' in job
            assert type(job['id']) == int

            jobs = get_jobs_from_db()
            assert len(jobs) == 1
            assert jobs[0].id == job['id']
            shutil.rmtree(ac_path)#Remove autocomplete directory


def test_post_pbf_autocomplete_without_files(create_instance_fr):
    with app.app_context():
        ac_path = current_app.config['TYR_AUTOCOMPLETE_DIR']
        assert not os.path.exists(ac_path)
        filename = 'empty_pbf.osm.pbf'
        path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename)
        with open(path, 'rb') as f:
            response = api_post('/v0/autocomplete_parameters/fr/update_data', check=False)
            assert response[1] == 404


def test_post_pbf_autocomplete_instance_not_exist(create_instance_fr):
    with app.app_context():
        ac_path = current_app.config['TYR_AUTOCOMPLETE_DIR']
        assert not os.path.exists(ac_path)
        filename = 'empty_pbf.osm.pbf'
        path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename)
        with open(path, 'rb') as f:
            response = api_post('/v0/autocomplete_parameters/bob/update_data', check=False)
            assert response[1] == 404
