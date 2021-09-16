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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, division, unicode_literals
import pytest
import os
from navitiacommon import models
from tyr import app
from flask_restful import current_app
from tests.check_utils import api_post


@pytest.fixture
def create_instance_fr():
    with app.app_context():
        navitia_instance = models.Instance('fr')
        autocomplete_instance = models.AutocompleteParameter(
            name='fr', street='OSM', address='BANO', poi='OSM', admin='OSM', admin_level=[8]
        )
        models.db.session.add(navitia_instance)
        models.db.session.add(autocomplete_instance)
        models.db.session.commit()


@pytest.yield_fixture(scope="function", autouse=True)
def autocomplete_path(tmpdir):
    with app.app_context():
        current_ac_path = current_app.config['TYR_AUTOCOMPLETE_DIR']
        current_app.config['TYR_AUTOCOMPLETE_DIR'] = tmpdir.strpath
        yield
        current_app.config['TYR_AUTOCOMPLETE_DIR'] = current_ac_path


def get_jobs_from_db(id=None):
    with app.app_context():
        return models.Job.get(id=id)


def test_post_pbf(create_instance_fr):
    assert not os.path.isfile('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_empty_pbf.osm.pbf')
    assert not os.path.isfile('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_fusio.zip')

    filenames = ['empty_pbf.osm.pbf', 'fusio.zip']
    files = []
    for filename in filenames:
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        files.append((open(path, 'rb'), filename))

    try:
        tester = app.test_client()
        resp = tester.post('/v0/jobs/fr', data={'file': files})
    finally:
        for f in files:
            f[0].close()
    assert resp.status_code == 200

    assert os.path.isfile('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_empty_pbf.osm.pbf')
    assert os.path.isfile('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_fusio.zip')

    os.remove('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_empty_pbf.osm.pbf')
    os.remove('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_fusio.zip')

    assert not os.path.isfile('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_empty_pbf.osm.pbf')
    assert not os.path.isfile('/tmp/_JOB_ID_DELIMITER_1_JOB_ID_DELIMITER_fusio.zip')


def test_post_no_file(create_instance_fr):
    tester = app.test_client()
    resp = tester.post('/v0/jobs/fr')
    assert resp.status_code == 400


def test_post_bad_instance(create_instance_fr):
    tester = app.test_client()
    resp = tester.post('/v0/jobs/fr_ko')
    assert resp.status_code == 404


def test_post_pbf_autocomplete(create_instance_fr):
    with app.app_context():
        filename = 'empty_pbf.osm.pbf'
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        with open(path, 'rb') as f:
            files = {'file': (f, filename)}
            json_response = api_post('/v0/autocomplete_parameters/fr/update_data', data=files)
            assert 'job' in json_response
            job = json_response['job']
            assert 'id' in job
            assert type(job['id']) == int
            assert len(job['data_sets']) == 1
            assert 'empty_pbf.osm.pbf' in job['data_sets'][0]['name']

            job = get_jobs_from_db(id=job['id'])
            assert job


def test_post_pbf_autocomplete_with_parenthesis(create_instance_fr):
    with app.app_context():
        filename = 'empty_pbf.osm(1).pbf'
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        with open(path, 'rb') as f:
            files = {'file': (f, filename)}
            json_response = api_post('/v0/autocomplete_parameters/fr/update_data', data=files)
            assert 'job' in json_response
            job = json_response['job']
            assert 'id' in job
            assert type(job['id']) == int
            assert len(job['data_sets']) == 1
            assert 'empty_pbf.osm(1).pbf' not in job['data_sets'][0]['name']
            assert 'empty_pbf.osm_' in job['data_sets'][0]['name']

            job = get_jobs_from_db(id=job['id'])
            assert job


def test_post_bano_autocomplete(create_instance_fr):
    with app.app_context():
        filename = 'sample-bano.csv'
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        with open(path, 'rb') as f:
            files = {'file': (f, filename)}
            json_response = api_post('/v0/autocomplete_parameters/fr/update_data', data=files)
            assert 'job' in json_response
            job = json_response['job']
            assert 'id' in job
            assert type(job['id']) == int

            job = get_jobs_from_db(id=job['id'])
            assert job


def test_post_pbf_autocomplete_without_files(create_instance_fr):
    with app.app_context():
        filename = 'empty_pbf.osm.pbf'
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        with open(path, 'rb'):
            response, status = api_post('/v0/autocomplete_parameters/fr/update_data', check=False)
            assert status == 400


def test_post_pbf_autocomplete_instance_not_exist(create_instance_fr):
    with app.app_context():
        filename = 'empty_pbf.osm.pbf'
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        with open(path, 'rb'):
            response, status = api_post('/v0/autocomplete_parameters/bob/update_data', check=False)
            assert status == 404


def test_post_zip_file_on_job_should_succeed(create_instance_fr):
    with app.app_context():
        filename = 'fusio.zip'
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        with open(path, 'rb') as f:
            files = {'file': (f, filename)}
            resp, status = api_post('/v0/jobs/fr', data=files, check=False)
            assert status == 200


def test_post_zip_file_on_job_with_wrong_extension(create_instance_fr):
    with app.app_context():
        filename = 'empty_file_without_extension'
        path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'tests/fixtures/', filename
        )
        with open(path, 'rb') as f:
            files = {'file': (f, filename)}
            resp, status = api_post('/v0/jobs/fr', data=files, check=False)
            assert status == 400
