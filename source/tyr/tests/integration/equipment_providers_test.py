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

from tyr import app
from navitiacommon import models
from tests.check_utils import api_get, api_delete, api_put

import pytest
import ujson


@pytest.fixture
def default_equipments_config():
    with app.app_context():
        sytral = models.EquipmentsProvider('sytral')
        sytral.klass = 'sytral.klass'
        sytral.args = {'url': 'http://sytral.url', 'fail_max': 5, 'timeout': 2}
        models.db.session.add(sytral)

        sytral2 = models.EquipmentsProvider('sytral2')
        sytral2.klass = 'sytral.klass'
        sytral2.args = {'url': 'http://sytral2.url', 'fail_max': 10, 'timeout': 1}
        models.db.session.add(sytral2)

        models.db.session.commit()
        # refresh and detach the objets from the db before returning them
        models.db.session.refresh(sytral)
        models.db.session.refresh(sytral2)
        models.db.session.expunge(sytral)
        models.db.session.expunge(sytral2)

        yield sytral, sytral2

        models.db.session.delete(sytral)
        models.db.session.delete(sytral2)
        models.db.session.commit()


@pytest.fixture(scope='function', autouse=True)
def clean_db():
    with app.app_context():
        models.db.session.execute('truncate equipments_provider cascade;')
        models.db.session.commit()


def test_equipments_provider_get(default_equipments_config):
    """
    Test that the list of providers with their info is correctly returned when queried
    """
    sytral_provider, sytral2_provider = default_equipments_config
    resp = api_get('/v0/equipments_providers')
    assert 'equipments_providers' in resp
    assert len(resp['equipments_providers']) == 2

    resp = api_get('/v0/equipments_providers/sytral')
    assert 'equipments_providers' in resp
    assert len(resp['equipments_providers']) == 1

    assert resp['equipments_providers'][0]['args'] == sytral_provider.args
    assert not resp['equipments_providers'][0]['discarded']


def test_equipments_provider_put(default_equipments_config):
    """
    Test that a provider is correctly created/updated in db and the info returned when queried
    """

    # Create new provider
    new_provider = {
        'klass': 'jormungandr.equipments.sytral.SytralProvider',
        'args': {'url': 'sytral3.url', 'fail_max': 5, 'timeout': 1},
    }
    resp, status = api_put(
        'v0/equipments_providers/sytral3',
        data=ujson.dumps(new_provider),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert 'equipments_provider' in resp
    assert len(resp['equipments_provider']) == 1
    for key in new_provider.keys():
        assert resp['equipments_provider'][0][key] == new_provider[key]

    resp = api_get('/v0/equipments_providers')
    assert 'equipments_providers' in resp
    assert len(resp['equipments_providers']) == 3

    # Update existing provider
    new_provider['args']['url'] = 'sytral3.url.update'
    resp = api_put(
        'v0/equipments_providers/sytral3', data=ujson.dumps(new_provider), content_type='application/json'
    )
    assert 'equipments_provider' in resp
    assert len(resp['equipments_provider']) == 1
    for key in new_provider.keys():
        assert resp['equipments_provider'][0][key] == new_provider[key]

    resp = api_get('/v0/equipments_providers/sytral3')
    assert 'equipments_providers' in resp
    assert len(resp['equipments_providers']) == 1
    assert resp['equipments_providers'][0]['args']['url'] == 'sytral3.url.update'


def test_equipments_provider_put_without_id():
    new_provider = {
        'klass': 'jormungandr.equipments.sytral.SytralProvider',
        'args': {'url': 'sytral3.url', 'fail_max': 5, 'timeout': 1},
    }
    resp, status = api_put(
        'v0/equipments_providers', data=ujson.dumps(new_provider), content_type='application/json', check=False
    )
    assert status == 400
    assert resp["message"] == "id is required"


def test_equipments_provider_delete(default_equipments_config):
    """
    Test that a 'deleted' provider isn't returned when querying all providers, and that its 'discarded' parameter is set to True
    """
    resp = api_get('/v0/equipments_providers')
    assert 'equipments_providers' in resp
    assert len(resp['equipments_providers']) == 2

    _, status_code = api_delete('v0/equipments_providers/sytral', check=False, no_json=True)
    assert status_code == 204

    resp = api_get('/v0/equipments_providers')
    assert 'equipments_providers' in resp
    assert len(resp['equipments_providers']) == 1

    resp = api_get('/v0/equipments_providers/sytral')
    assert 'equipments_providers' in resp
    assert len(resp['equipments_providers']) == 1
    assert resp['equipments_providers'][0]['discarded']


def test_equipments_provider_schema():
    """
    Test that a provider isn't created in db when missing required parameters
    """

    def send_and_check(provider_id, json_data, missing_param):
        resp, status = api_put(
            url='v0/equipments_providers/{}'.format(provider_id),
            data=ujson.dumps(json_data),
            content_type='application/json',
            check=False,
        )
        assert status == 400
        assert 'message' in resp
        assert resp['message'] == "'{}' is a required property".format(missing_param)
        assert 'status' in resp
        assert resp['status'] == "invalid data"

    # 'klass' is missing
    corrupted_provider = {'args': {'url': 'sytral.url', 'fail_max': 5, 'timeout': 1}}
    send_and_check('sytral', corrupted_provider, 'klass')

    # args['url'] is missing
    corrupted_provider = {
        'klass': 'jormungandr.equipments.sytral.SytralProvider',
        'key': 'sytral',
        'args': {'fail_max': 5, 'timeout': 1},
    }
    send_and_check('sytral', corrupted_provider, 'url')
