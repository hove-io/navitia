# coding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals
import os
import glob
import pytest
import tempfile
from datetime import datetime, timedelta

from tyr import app, tasks
from navitiacommon import models
from tests.check_utils import api_get, api_delete

# TODO : need to clean that after full migration to python3
try:
    import ConfigParser
except:
    import configparser as ConfigParser  # type: ignore


def create_dataset(dataset_type, dir):
    dataset = models.DataSet()
    dataset.type = dataset_type
    dataset.family_type = '{}_family'.format(dataset_type)
    dataset.name = '{}/dataset'.format(dir)
    models.db.session.add(dataset)

    metric = models.Metric()
    metric.type = '{}2ed'.format(dataset_type)
    metric.dataset = dataset
    models.db.session.add(metric)

    return dataset, metric


def create_job(creation_date, dataset_type, backup_dir, state='done'):
    job = models.Job()
    job.state = state
    dataset_backup_dir = tempfile.mkdtemp(dir=backup_dir)
    dataset, metric = create_dataset(dataset_type, dataset_backup_dir)
    job.data_sets.append(dataset)
    job.metrics.append(metric)
    job.created_at = creation_date
    models.db.session.add(job)

    return job


def create_jobs_with_same_datasets(name, backup_dir, job_state='done'):
    with app.app_context():
        job_list = []

        dataset_backup_dir = tempfile.mkdtemp(dir=backup_dir)
        for i in range(3):
            dataset, metric = create_dataset('poi', dataset_backup_dir)
            job = models.Job()
            job.state = job_state
            job.data_sets.append(dataset)
            job.metrics.append(metric)
            job.created_at = datetime.utcnow() - timedelta(days=i)
            models.db.session.add(job)
            job_list.append(job)

        create_instance(name, job_list)


def create_instance(name, jobs):
    instance = models.Instance(name=name, jobs=jobs)
    models.db.session.add(instance)
    models.db.session.commit()


def create_instance_with_same_type_datasets(name, backup_dir):
    with app.app_context():
        job_list = []
        for index in range(3):
            job_list.append(create_job(datetime.utcnow() - timedelta(days=index), 'poi', backup_dir))
        create_instance(name, job_list)


def create_instance_with_different_dataset_types_and_job_state(name, backup_dir):
    with app.app_context():
        job_list = []

        # Add jobs with type = 'poi', state = 'done' and 'running'
        job_list.append(create_job(datetime.utcnow() - timedelta(days=1), 'poi', backup_dir))
        job_list.append(create_job(datetime.utcnow() - timedelta(days=2), 'poi', backup_dir))
        job_list.append(create_job(datetime.utcnow() - timedelta(days=3), 'poi', backup_dir, state='running'))

        # Add jobs with type = 'fusio', state = 'done' and 'running'
        job_list.append(create_job(datetime.utcnow() - timedelta(days=4), 'fusio', backup_dir))
        job_list.append(create_job(datetime.utcnow() - timedelta(days=5), 'fusio', backup_dir))
        job_list.append(create_job(datetime.utcnow() - timedelta(days=6), 'fusio', backup_dir, state='running'))
        create_instance(name, job_list)


def create_instance_with_one_type_dataset(name, backup_dir):
    with app.app_context():
        job_list = []
        for index, dataset_type in enumerate(['fusio', 'osm', 'poi']):
            job_list.append(create_job(datetime.utcnow() - timedelta(days=index), dataset_type, backup_dir))
        create_instance(name, job_list)


def init_test():
    config = ConfigParser.ConfigParser()

    files = os.listdir(app.config['INSTANCES_DIR'])
    for index, f in enumerate(files):
        if os.path.splitext(f)[1] == '.ini':
            instance_name = os.path.splitext(f)[0]
            config.read(os.path.realpath('{}/{}'.format(app.config['INSTANCES_DIR'], f)))
            backup_dir = config.get('instance', 'backup-directory')

    return instance_name, backup_dir


@pytest.mark.usefixtures("init_instances_dir")
def test_purge_old_jobs():
    """
    Delete old jobs created before the time limit
    """
    app.config['JOB_MAX_PERIOD_TO_KEEP'] = 1

    instance_name, backup_dir = init_test()
    create_instance_with_same_type_datasets(instance_name, backup_dir)

    instances_resp = api_get('/v0/instances')
    assert len(instances_resp) == 1

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 3

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 3

    tasks.purge_jobs()

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 1

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 1


@pytest.mark.usefixtures("init_instances_dir")
def test_purge_old_jobs_no_delete():
    """
    The jobs related to the instance are the only ones of their type(having one data_set par job).
    So, even if they are older than the time limit, they won't be deleted and hence last_datasets
    always contains a full set of valid data_sets
    """
    app.config['JOB_MAX_PERIOD_TO_KEEP'] = 1

    instance_name, backup_dir = init_test()
    create_instance_with_one_type_dataset(instance_name, backup_dir)

    instances_resp = api_get('/v0/instances')
    assert len(instances_resp) == 1

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 3

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 3

    tasks.purge_jobs()

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 3

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 3


@pytest.mark.usefixtures("init_instances_dir")
def test_purge_old_jobs_same_dataset():
    """
    An old job to be deleted uses the same dataset as one to keep.
    So, delete the job keeping at least one in db but the dataset file on disc
    """
    app.config['JOB_MAX_PERIOD_TO_KEEP'] = 1

    instance_name, backup_dir = init_test()
    create_jobs_with_same_datasets(instance_name, backup_dir)

    instances_resp = api_get('/v0/instances')
    assert len(instances_resp) == 1

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 3

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 1

    tasks.purge_jobs()

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 1

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 1


@pytest.mark.usefixtures("init_instances_dir")
def test_purge_old_jobs_of_multi_instance():
    """
    We should delete jobs, data_sets as well as metrics of all instances
    including discarded = True
    """
    app.config['JOB_MAX_PERIOD_TO_KEEP'] = 1

    instance_name, backup_dir = init_test()
    create_jobs_with_same_datasets(instance_name, backup_dir)

    create_jobs_with_same_datasets('fr-discarded', backup_dir)

    instances_resp = api_get('/v0/instances')
    assert len(instances_resp) == 2

    # Delete the second instance (discarded = True)
    instances_resp = api_delete('/v0/instances/fr-discarded')
    assert instances_resp['discarded'] is True

    jobs_resp = api_get('/v0/jobs/fr-discarded')
    assert len(jobs_resp['jobs']) == 3
    jobs_resp = api_get('/v0/jobs/fr')
    assert len(jobs_resp['jobs']) == 3
    tasks.purge_jobs()
    jobs_resp = api_get('/v0/jobs/fr-discarded')
    assert len(jobs_resp['jobs']) == 1
    jobs_resp = api_get('/v0/jobs/fr')
    assert len(jobs_resp['jobs']) == 1


@pytest.mark.usefixtures("init_instances_dir")
def test_purge_old_jobs_with_diff_job_states():
    """
    we shouldn't delete jobs with status 'running'. This is to avoid
    deleting jobs and their files on running state.
    """
    app.config['JOB_MAX_PERIOD_TO_KEEP'] = 1

    instance_name, backup_dir = init_test()
    create_jobs_with_same_datasets(instance_name, backup_dir)

    instances_resp = api_get('/v0/instances')
    assert len(instances_resp) == 1

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 3

    last_datasets = api_get('/v0/instances/{}/last_datasets'.format(instances_resp[0]['name']))
    assert len(last_datasets) == 1
    # Purge old jobs
    tasks.purge_jobs()
    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 1

    # Add three more jobs with state = 'running'. Purge shouldn't delete these jobs
    create_jobs_with_same_datasets('fr-npdc', backup_dir, 'running')
    jobs_resp = api_get('/v0/jobs')
    assert len(jobs_resp['jobs']) == 4
    tasks.purge_jobs()
    jobs_resp = api_get('/v0/jobs')
    assert len(jobs_resp['jobs']) == 4


@pytest.mark.usefixtures("init_instances_dir")
def test_purge_instance_jobs():
    """
    Delete old jobs created before the time limit
    Do not delete jobs with state = 'running'
    Deletes only directories of delete able jobs but not in the table 'job'
    Is used by the job 'purge_datasets' planned daily to purge old jobs
    """
    app.config['JOB_MAX_PERIOD_TO_KEEP'] = 1

    instance_name, backup_dir = init_test()
    create_instance_with_different_dataset_types_and_job_state(instance_name, backup_dir)

    instances_resp = api_get('/v0/instances')
    assert len(instances_resp) == 1

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 6

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 6

    last_datasets = api_get('/v0/instances/{}/last_datasets'.format(instances_resp[0]['name']))
    assert len(last_datasets) == 2

    tasks.purge_instance(instances_resp[0]['id'], 1)

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 4

    # No job is deleted in the table 'job'
    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 6

    # if we use the task purge_jobs, data related to old jobs also deleted from the table 'job'
    tasks.purge_jobs()
    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 4


@pytest.mark.usefixtures("init_cities_dir")
def test_purge_cities_job():
    """
    Test that 'cities' jobs and associated datasets are correctly purged
    'cities' file should also be deleted unless used by a job to keep
    """

    def create_cities_job(creation_date, path, state):
        job = models.Job()
        job.state = state
        dataset_backup_dir = path

        dataset = models.DataSet()
        dataset.type = 'cities'
        dataset.family_type = 'cities_family'
        dataset.name = '{}'.format(dataset_backup_dir)
        models.db.session.add(dataset)

        job.data_sets.append(dataset)
        job.created_at = creation_date
        models.db.session.add(job)

    cities_file_dir = app.config['CITIES_OSM_FILE_PATH']
    # Have 2 jobs with the same dataset to test that it isn't deleted if one of the jobs is kept
    common_dataset_folder = tempfile.mkdtemp(dir=cities_file_dir)
    with app.app_context():
        for i in range(2):
            create_cities_job(datetime.utcnow() - timedelta(days=i), common_dataset_folder, state='done')
        for j in range(2):
            create_cities_job(
                datetime.utcnow() - timedelta(days=j + 2), tempfile.mkdtemp(dir=cities_file_dir), state='done'
            )
        models.db.session.commit()

        jobs_resp = api_get('/v0/jobs')
        assert 'jobs' in jobs_resp
        assert len(jobs_resp['jobs']) == 4

        folders = set(glob.glob('{}/*'.format(cities_file_dir)))
        assert len(folders) == 3

        app.config['DATASET_MAX_BACKUPS_TO_KEEP'] = 3
        tasks.purge_cities()

        jobs_resp = api_get('/v0/jobs')
        assert 'jobs' in jobs_resp
        assert len(jobs_resp['jobs']) == 3

        folders = set(glob.glob('{}/*'.format(cities_file_dir)))
        assert len(folders) == 2

        app.config['DATASET_MAX_BACKUPS_TO_KEEP'] = 1
        tasks.purge_cities()

        jobs_resp = api_get('/v0/jobs')
        assert 'jobs' in jobs_resp
        assert len(jobs_resp['jobs']) == 1

        folders = set(glob.glob('{}/*'.format(cities_file_dir)))
        assert len(folders) == 1
