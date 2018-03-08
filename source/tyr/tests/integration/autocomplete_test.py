# -*- coding: utf-8 -*-
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

from __future__ import absolute_import, print_function, unicode_literals, division
from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
from navitiacommon import models
from tyr import app

@pytest.fixture
def create_autocomplete_parameter():
    with app.app_context():
        autocomplete_param = models.AutocompleteParameter('idf', 'OSM', 'BANO','FUSIO', 'OSM', [8, 9])
        models.db.session.add(autocomplete_param)
        models.db.session.commit()

        # we also create 3 datasets, one for bano, 2 for osm
        for i, dset_type in enumerate(['bano', 'osm', 'osm']):
            job = models.Job()

            dataset = models.DataSet()
            dataset.type = dset_type
            dataset.family_type = 'autocomplete_{}'.format(dataset.type)
            dataset.name = '/path/to/dataset_{}'.format(i)
            models.db.session.add(dataset)

            job.autocomplete_params_id = autocomplete_param.id
            job.data_sets.append(dataset)
            job.state = 'done'
            models.db.session.add(job)
            models.db.session.commit()


@pytest.fixture
def create_two_autocomplete_parameters():
    with app.app_context():
        autocomplete_param1 = models.AutocompleteParameter('europe', 'OSM', 'BANO', 'OSM', 'OSM', [8, 9])
        autocomplete_param2 = models.AutocompleteParameter('france', 'OSM', 'OSM', 'FUSIO', 'OSM', [8, 9])
        models.db.session.add(autocomplete_param1)
        models.db.session.add(autocomplete_param2)
        models.db.session.commit()


@pytest.fixture
def autocomplete_parameter_json():
    return {"name": "peru", "street": "OSM", "address": "BANO", "poi": "FUSIO", "admin": "OSM",
            "admin_level": [8]}


def test_get_autocomplete_parameters_empty():
    resp = api_get('/v0/autocomplete_parameters/')
    assert resp == []


def test_get_all_autocomplete(create_autocomplete_parameter):
    resp = api_get('/v0/autocomplete_parameters/')
    assert len(resp) == 1
    assert resp[0]['name'] == 'idf'
    assert resp[0]['street'] == 'OSM'
    assert resp[0]['address'] == 'BANO'
    assert resp[0]['poi'] == 'FUSIO'
    assert resp[0]['admin'] == 'OSM'
    assert resp[0]['admin_level'] == [8, 9]


def test_get_autocomplete_by_name(create_two_autocomplete_parameters):
    resp = api_get('/v0/autocomplete_parameters/')
    assert len(resp) == 2

    resp = api_get('/v0/autocomplete_parameters/france')
    assert resp['name'] == 'france'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'OSM'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'
    assert resp['admin_level'] == [8, 9]


def test_post_autocomplete(autocomplete_parameter_json):
    resp = api_post('/v0/autocomplete_parameters', data=json.dumps(autocomplete_parameter_json), content_type='application/json')

    assert resp['name'] == 'peru'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'BANO'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'
    assert resp['admin_level'] == [8]

def test_put_autocomplete(create_two_autocomplete_parameters, autocomplete_parameter_json):
    resp = api_get('/v0/autocomplete_parameters/france')
    assert resp['name'] == 'france'
    assert resp['street'] == 'OSM'
    assert resp['address'] == 'OSM'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'
    assert resp['admin_level'] == [8, 9]

    resp = api_put('/v0/autocomplete_parameters/france', data=json.dumps(autocomplete_parameter_json), content_type='application/json')

    assert resp['street'] == 'OSM'
    assert resp['address'] == 'BANO'
    assert resp['poi'] == 'FUSIO'
    assert resp['admin'] == 'OSM'
    assert resp['admin_level'] == [8]


def test_delete_autocomplete(create_two_autocomplete_parameters):
    resp = api_get('/v0/autocomplete_parameters/')
    assert len(resp) == 2
    resp = api_get('/v0/autocomplete_parameters/france')
    assert resp['name'] == 'france'

    _, status = api_delete('/v0/autocomplete_parameters/france', check=False, no_json=True)
    assert status == 204

    _, status = api_get('/v0/autocomplete_parameters/france', check=False)
    assert status == 404

    resp = api_get('/v0/autocomplete_parameters/')
    assert len(resp) == 1


def test_get_last_datasets_autocomplete(create_autocomplete_parameter):
    """
    we query the loaded datasets of idf
    we loaded 3 datasets, but by default we should get one by family_type, so one for bano, one for osm
    """
    resp = api_get('/v0/autocomplete_parameters/idf/last_datasets')

    assert len(resp) == 2
    bano = next((d for d in resp if d['type'] == 'bano'), None)
    assert bano
    assert bano['family_type'] == 'autocomplete_bano'
    assert bano['name'] == '/path/to/dataset_0'

    osm = next((d for d in resp if d['type'] == 'osm'), None)
    assert osm
    assert osm['family_type'] == 'autocomplete_osm'
    assert osm['name'] == '/path/to/dataset_2'  # we should have the last one

    # if we ask for the 2 last datasets per type, we got all of them
    resp = api_get('/v0/autocomplete_parameters/idf/last_datasets?count=2')
    assert len(resp) == 3
