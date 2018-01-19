from tests.check_utils import api_get, api_delete
import pytest
from navitiacommon import models
from tyr import app


def create_dataset(dataset_type):

    dataset = models.DataSet()
    dataset.type = dataset_type
    dataset.family_type = '{}_family'.format(dataset_type)
    dataset.name = '/path/to/dataset_{}'.format(dataset_type)
    models.db.session.add(dataset)

    metric = models.Metric()
    metric.type = '{}2ed'.format(dataset_type)
    metric.dataset = dataset
    models.db.session.add(metric)

    return dataset, metric


def create_job_with_poi_only():

    dataset, metric = create_dataset('poi')

    job = models.Job()
    job.data_sets.append(dataset)
    job.metrics.append(metric)
    job.state = 'done'

    dataset, metric = create_dataset('poi')
    job.data_sets.append(dataset)
    job.metrics.append(metric)

    models.db.session.add(job)

    return job


def create_job_with_all_dataset_types():

    job = models.Job()
    job.state = 'done'

    for dataset_type in ['fusio', 'osm', 'poi']:
        dataset, metric = create_dataset(dataset_type)
        job.data_sets.append(dataset)
        job.metrics.append(metric)

    models.db.session.add(job)

    return job


@pytest.fixture
def create_instance():
    job_list = []
    with app.app_context():
        job_list.append(create_job_with_poi_only())
        instance = models.Instance(name='fr_instance', jobs=job_list)
        models.db.session.add(instance)
        models.db.session.commit()

        return instance.name


@pytest.fixture()
def create_tyr_update_jobs():
    with app.app_context():
        job_list = []
        job_list.append(create_job_with_all_dataset_types())
        job_list.append(create_job_with_poi_only())

        # Create instance to which the jobs are linked
        instance = models.Instance(name='tyr_instance', jobs=job_list)
        models.db.session.add(instance)
        models.db.session.commit()

        return instance.name


def test_migrate_poi_to_osm_wrong_instance(create_instance):

    resp, status_code = api_delete('/v0/instances/wrong_instance/actions/migrate_from_poi_to_osm', check=False)
    assert resp['action'] == 'No instance found for : wrong_instance'
    assert status_code == 404


def test_migrate_poi_to_osm_one_instance(create_tyr_update_jobs):

    resp = api_get('/v0/instances')
    assert len(resp) == 1

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp.values()[0]) == 2

    resp = api_delete('/v0/instances/{}/actions/migrate_from_poi_to_osm'.format(create_tyr_update_jobs))
    assert resp['action'] == 'All POI datasets deleted for instance {}'.format(create_tyr_update_jobs)

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp.values()[0]) == 1
    for dataset in resp.values()[0][0]['data_sets']:
        assert dataset['type'] != 'poi'


def test_migrate_poi_to_osm_two_instances(create_instance, create_tyr_update_jobs):

    resp = api_get('/v0/instances')
    assert len(resp) == 2

    resp = api_get('/v0/jobs/{}'.format(create_instance))
    for dataset in resp.values()[0][0]['data_sets']:
        assert dataset['type'] == 'poi'

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp.values()[0]) == 2

    resp = api_delete('/v0/instances/{}/actions/migrate_from_poi_to_osm'.format(create_tyr_update_jobs))
    assert resp['action'] == 'All POI datasets deleted for instance {}'.format(create_tyr_update_jobs)

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp.values()[0]) == 1
    for dataset in resp.values()[0][0]['data_sets']:
        assert dataset['type'] != 'poi'

    resp = api_get('/v0/jobs/{}'.format(create_instance))
    assert len(resp.values()[0]) == 1
    for dataset in resp.values()[0][0]['data_sets']:
        assert dataset['type'] == 'poi'
