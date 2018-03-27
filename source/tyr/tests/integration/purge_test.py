# coding: utf-8
# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
#     powered by Canal TP (www.canaltp.fr).
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
# IRC #navitia on freenode
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
from tests.check_utils import api_get
try:
    import ConfigParser
except:
    import configparser as ConfigParser


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


def create_job(creation_date, dataset_type, backup_dir):
    job = models.Job()
    job.state = 'done'
    dataset_backup_dir = tempfile.mkdtemp(dir=backup_dir)
    dataset, metric = create_dataset(dataset_type, dataset_backup_dir)
    job.data_sets.append(dataset)
    job.metrics.append(metric)
    job.created_at = creation_date
    models.db.session.add(job)

    return job


def create_jobs_with_same_datasets(name, backup_dir):
    with app.app_context():
        job_list = []

        dataset_backup_dir = tempfile.mkdtemp(dir=backup_dir)
        for i in range(3):
            dataset, metric = create_dataset('poi', dataset_backup_dir)
            job = models.Job()
            job.state = 'done'
            job.data_sets.append(dataset)
            job.metrics.append(metric)
            job.created_at = datetime.utcnow()-timedelta(days=i)
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


def create_instance_with_one_type_dataset(name, backup_dir):
    with app.app_context():
        job_list = []
        for index, dataset_type in enumerate(['fusio', 'osm', 'poi']):
            job_list.append(create_job(datetime.utcnow()-timedelta(days=index), dataset_type, backup_dir))
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
    print('Folders = ', folders)

    tasks.purge_jobs()

    jobs_resp = api_get('/v0/jobs/{}'.format(instances_resp[0]['name']))
    assert len(jobs_resp['jobs']) == 1

    folders = set(glob.glob('{}/*'.format(backup_dir)))
    assert len(folders) == 1

@pytest.mark.usefixtures("init_instances_dir")
def test_purge_old_jobs_no_delete():
    """
    The jobs related to the instance are the only ones of their type.
    So, even if they are older than the time limit, they won't be deleted
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
    So, delete the job in db but the dataset file on disc
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
