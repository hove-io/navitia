from __future__ import absolute_import, print_function, unicode_literals, division
from tests.check_utils import api_get
import pytest
from navitiacommon import models
from tyr import app


def get_instance_from_db(name=None):
    with app.app_context():
        return models.Instance.get_by_name(name=name)


@pytest.fixture
def create_basic_job_with_data_sets():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()

        job = models.Job()
        job.instance = instance

        # we also create 2 datasets, one for fusio, one for synonym
        for i, dset_type in enumerate(['fusio', 'synonym']):
            dataset = models.DataSet()
            dataset.type = dset_type
            dataset.family_type = dataset.type
            if dataset.type == 'fusio':
                dataset.family_type = 'pt'
            dataset.name = '/path/to/dataset_{}'.format(i)
            models.db.session.add(dataset)

            job.data_sets.append(dataset)

        job.state = 'done'
        models.db.session.add(job)
        models.db.session.commit()


@pytest.fixture
def create_autocomplete_job_with_data_sets():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()

        job = models.Job()
        job.instance = instance

        # we also create 2 datasets, one for fusio, one for autocomplete_cosmogony
        for i, dset_type in enumerate(['fusio', 'cosmogony']):
            dataset = models.DataSet()
            dataset.type = dset_type
            dataset.family_type = dataset.type
            if dataset.type == 'fusio':
                dataset.family_type = 'pt'
                dataset.name = '/path/to/dataset_{}'.format(i)
            else:
                dataset.family_type = 'autocomplete_cosmogony'
                dataset.name = '/path/to/dataset_cosmogony/cosmogony_europe.jsonl.gz'

            models.db.session.add(dataset)

            job.data_sets.append(dataset)

        job.state = 'done'
        models.db.session.add(job)
        models.db.session.commit()


def add_job_with_data_set_mimir(create_basic_job_with_data_sets):
    with app.app_context():
        # we also create 1 job with a dataset for mimir
        instance = get_instance_from_db(name='fr')
        job = models.Job()
        job.instance = instance
        dataset = models.DataSet()
        dataset.family_type = 'mimir'
        dataset.type = 'stop2mimir'
        dataset.name = '/path/to/dataset_3'
        models.db.session.add(dataset)
        job.data_sets.append(dataset)
        job.state = 'done'
        models.db.session.add(job)

        models.db.session.commit()


def add_job_and_data_set_with_jobstate_running(create_basic_job_with_data_sets):
    with app.app_context():
        # we add a new job with a dataset for mimir
        instance = get_instance_from_db(name='fr')
        job = models.Job()
        job.instance = instance
        dataset = models.DataSet()
        dataset.family_type = 'osm'
        dataset.type = 'osm'
        dataset.name = '/path/to/dataset_osm'
        models.db.session.add(dataset)
        job.data_sets.append(dataset)
        job.state = 'running'
        models.db.session.add(job)

        models.db.session.commit()


def test_basic_datasets(create_basic_job_with_data_sets):
    """
    we query the loaded datasets of fr
    we loaded 2 datasets, one for pt, one for synonym
    """
    resp = api_get('/v0/instances/fr/last_datasets')

    assert len(resp) == 2
    fusio = next((d for d in resp if d['type'] == 'fusio'), None)
    assert fusio
    assert fusio['family_type'] == 'pt'
    assert fusio['name'] == '/path/to/dataset_0'

    synonym = next((d for d in resp if d['type'] == 'synonym'), None)
    assert synonym
    assert synonym['family_type'] == 'synonym'
    assert synonym['name'] == '/path/to/dataset_1'

    # There is only one job
    resp = api_get('/v0/jobs/fr')
    assert len(resp['jobs']) == 1

    # Here we add a new job with mimir as data_set
    add_job_with_data_set_mimir(create_basic_job_with_data_sets)
    resp = api_get('/v0/instances/fr/last_datasets')
    assert len(resp) == 2
    assert resp[0]['family_type'] == 'pt'
    assert resp[0]['name'] == '/path/to/dataset_0'
    assert resp[1]['family_type'] == 'synonym'
    assert resp[1]['name'] == '/path/to/dataset_1'

    # we have two jobs: one with 1 data_set and another with 2 data_sets
    resp = api_get('/v0/jobs/fr')
    assert len(resp['jobs']) == 2
    job_with_pt = resp['jobs'][1]['data_sets']
    assert job_with_pt[0]['family_type'] == 'pt'
    assert job_with_pt[0]['name'] == '/path/to/dataset_0'
    assert job_with_pt[0]['type'] == 'fusio'
    assert job_with_pt[1]['family_type'] == 'synonym'
    assert job_with_pt[1]['name'] == '/path/to/dataset_1'
    assert job_with_pt[1]['type'] == 'synonym'

    job_with_mimir = resp['jobs'][0]['data_sets']
    assert job_with_mimir[0]['family_type'] == 'mimir'
    assert job_with_mimir[0]['name'] == '/path/to/dataset_3'
    assert job_with_mimir[0]['type'] == 'stop2mimir'

    # We add one job and a data_set with job state = 'running'.
    add_job_and_data_set_with_jobstate_running(create_basic_job_with_data_sets)

    # Here we should have the same data_sets as before since the new job added has state 'running'
    resp = api_get('/v0/instances/fr/last_datasets')
    assert len(resp) == 2
    assert resp[0]['family_type'] == 'pt'
    assert resp[0]['name'] == '/path/to/dataset_0'
    assert resp[1]['family_type'] == 'synonym'
    assert resp[1]['name'] == '/path/to/dataset_1'

    # Here we should have the same data_sets as before since the new job added has state 'running'
    resp = api_get('/v0/jobs/fr')
    assert len(resp['jobs']) == 3


def test_cosmogony_file_path(create_autocomplete_job_with_data_sets):
    with app.app_context():
        # retrieve the cosmogony file path
        assert models.DataSet.get_cosmogony_file_path() == '/path/to/dataset_cosmogony/cosmogony_europe.jsonl.gz'
