# coding: utf-8
# Copyright (c) 2001-2020, Hove and/or its affiliates. All rights reserved.
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
import datetime


def create_dataset(dataset_type):
    dataset = models.DataSet()
    dataset.type = dataset_type
    dataset.family_type = '{}_family'.format(dataset_type)
    dataset.name = '/path/to/dataset_{}'.format(dataset_type)
    models.db.session.add(dataset)

    metric = models.Metric()
    metric.type = '{}2ed'.format(dataset_type)
    metric.duration = datetime.timedelta(seconds=9.0001)
    metric.dataset = dataset
    models.db.session.add(metric)

    return dataset, metric


def create_job_with_state(state):
    job = models.Job()
    job.state = state

    dataset, metric = create_dataset("fusio")
    job.data_sets.append(dataset)
    job.metrics.append(metric)

    models.db.session.add(job)
    models.db.session.commit()

    return job


def create_job_with_all_states():
    job_list = []
    for state in ["running", "done", "failed"]:
        job_list.append(create_job_with_state(state))
    return job_list


def create_instance(name):
    job_list = []
    with app.app_context():
        job_list = create_job_with_all_states()
        instance = models.Instance(name=name, jobs=job_list)
        models.db.session.add(instance)
        models.db.session.commit()

        return instance.name


@pytest.fixture
def create_instances():
    instances_list = []
    for name in ["Instance_1", "Instance_2", "Instance_3"]:
        instances_list.append(create_instance(name))
    return instances_list


def test_jobs_deletion(create_instances):
    """
    Test DELETE method for /jobs with various filters
    """
    instances = create_instances
    # 3 jobs are created for each instance
    for instance in instances:
        resp = api_get("/v0/jobs/{}".format(instance))
        assert len(resp["jobs"]) == 3

    # get a job and verify attributes
    resp = api_get("/v0/jobs/1")
    assert len(resp["jobs"]) == 1
    job = resp["jobs"][0]
    assert job["created_at"] is not None
    assert job["updated_at"] is not None
    assert len(job["metrics"]) == 1
    assert job["metrics"][0]["type"] == "fusio2ed"
    assert job["metrics"][0]["duration"] == 9.0001
    assert job["instance"]["name"] == "Instance_1"
    assert len(job["data_sets"]) == 1

    # ---1--- DELETE WITH FILTER BY INSTANCE AND BY STATE
    # delete "running" job for 'Instance_1'
    api_delete("/v0/jobs/{}?confirm=yes&state=running".format("Instance_1"))

    # two jobs remaining for 'Instance_1'
    resp = api_get("/v0/jobs/Instance_1?confirm=yes")
    assert len(resp["jobs"]) == 2

    # other instances still have 3 jobs
    for instance in ["Instance_2", "Instance_3"]:
        resp = api_get("/v0/jobs/{}".format(instance))
        assert len(resp["jobs"]) == 3

    # --- 2 --- DELETE WITH FILTER BY STATE
    api_delete("/v0/jobs?confirm=yes&state=running")

    # two jobs remaining for each instance
    for instance in instances:
        resp = api_get("/v0/jobs/{}".format(instance))
        assert len(resp["jobs"]) == 2

    # --- 3 --- DELETE WITH FILTER BY INSTANCE
    api_delete("/v0/jobs/Instance_1?confirm=yes")

    # no job remaining for 'Instance_1'
    resp = api_get("/v0/jobs/Instance_1")
    assert len(resp["jobs"]) == 0

    # other instances still have 2 jobs
    for instance in ["Instance_2", "Instance_3"]:
        resp = api_get("/v0/jobs/{}".format(instance))
        assert len(resp["jobs"]) == 2

    # --- 4 --- DELETE WITH FILTER BY ID
    job_id = api_get("/v0/jobs/Instance_2")["jobs"][0]["id"]
    api_delete("/v0/jobs/{}?confirm=yes".format(job_id))

    # one job remaining for 'Instance_2'
    resp = api_get("/v0/jobs/Instance_2")
    assert len(resp["jobs"]) == 1

    # --- 5 --- DELETE WITHOUT CONFIRMATION
    query_no_confirm = "/v0/jobs"
    query_confirm_not_yes = "/v0/jobs?confirm=no"
    query_confirm_wrong_case = "/v0/jobs?Confirm=no"

    for query in [query_no_confirm, query_confirm_not_yes, query_confirm_wrong_case]:
        _, status_code = api_delete(query, check=False)
        assert status_code == 400

        # jobs still remaining
        resp = api_get("/v0/jobs")
        assert len(resp["jobs"])

    # --- 6 --- DELETE WITHOUT FILTER - ARMAGEDDON
    api_delete("/v0/jobs?confirm=yes")

    # no job remaining
    resp = api_get("/v0/jobs")
    assert len(resp["jobs"]) == 0

    # --- 7 --- BONUS: WHEN NO JOB TO DELETE, STATUS = 204
    resp, status_code = api_delete("/v0/jobs?confirm=yes", check=False, no_json=True)
    assert status_code == 204
