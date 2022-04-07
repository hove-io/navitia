# coding: utf-8
# Copyright (c) 2001-2018, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
#     powered by Hove (www.hove.com).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io


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


def test_delete_for_wrong_instance(create_instance):

    resp, status_code = api_delete('/v0/instances/wrong_instance/actions/delete_dataset/poi', check=False)
    assert resp['action'] == 'No instance found for : wrong_instance'
    assert status_code == 404


def test_delete_for_wrong_data_set_type(create_instance):

    resp, status_code = api_delete('/v0/instances/fr_instance/actions/delete_dataset/toto', check=False)
    assert resp['action'] == 'No toto dataset to be deleted for instance fr_instance'
    assert status_code == 200


def test_delete_poi_type_for_one_instance(create_tyr_update_jobs):

    resp = api_get('/v0/instances')
    assert len(resp) == 1

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp["jobs"]) == 2

    # Here a job having two poi data_sets is deleted
    resp = api_delete('/v0/instances/{}/actions/delete_dataset/poi'.format(create_tyr_update_jobs))
    assert resp['action'] == 'All poi datasets deleted for instance {}'.format(create_tyr_update_jobs)

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp["jobs"]) == 1
    for dataset in resp["jobs"][0]['data_sets']:
        assert dataset['type'] != 'poi'


# job_1 -> data_sets ['fusio','osm','poi']
# job_2 -> data_sets ['poi','poi']
def test_delete_fusio_type_for_one_instance(create_tyr_update_jobs):

    resp = api_get('/v0/instances')
    assert len(resp) == 1

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp["jobs"]) == 2

    # Here no empty job is deleted
    resp = api_delete('/v0/instances/{}/actions/delete_dataset/fusio'.format(create_tyr_update_jobs))
    assert resp['action'] == 'All fusio datasets deleted for instance {}'.format(create_tyr_update_jobs)

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp["jobs"]) == 2
    for dataset in resp["jobs"][0]['data_sets']:
        assert dataset['type'] != 'fusio'
    for dataset in resp["jobs"][0]['data_sets']:
        assert dataset['type'] == 'poi'


def test_delete_poi_type_for_two_instances(create_instance, create_tyr_update_jobs):

    resp = api_get('/v0/instances')
    assert len(resp) == 2

    resp = api_get('/v0/jobs/{}'.format(create_instance))
    for dataset in resp["jobs"][0]['data_sets']:
        assert dataset['type'] == 'poi'

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp["jobs"]) == 2

    resp = api_delete('/v0/instances/{}/actions/delete_dataset/poi'.format(create_tyr_update_jobs))
    assert resp['action'] == 'All poi datasets deleted for instance {}'.format(create_tyr_update_jobs)

    resp = api_get('/v0/jobs/{}'.format(create_tyr_update_jobs))
    assert len(resp["jobs"]) == 1
    for dataset in resp["jobs"][0]['data_sets']:
        assert dataset['type'] != 'poi'

    resp = api_get('/v0/jobs/{}'.format(create_instance))
    assert len(resp["jobs"]) == 1
    for dataset in resp["jobs"][0]['data_sets']:
        assert dataset['type'] == 'poi'
