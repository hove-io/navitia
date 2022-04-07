# coding: utf-8
# Copyright (c) 2001-2018, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
#     powered by Hove (www.kisio.com).
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

from __future__ import absolute_import, print_function, division
from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
from navitiacommon import models
from tyr import app


@pytest.fixture
def default_config():
    with app.app_context():
        velib = models.BssProvider('velib')
        velib.network = 'velib'
        velib.klass = 'jcdecaux'
        velib.args = {'operators': ['jcdecaux'], 'user': 'foo', 'password': 'bar'}
        models.db.session.add(velib)

        velov = models.BssProvider('velov')
        velov.network = 'velov'
        velov.klass = 'jcdecaux'
        velov.args = {'operators': ['jcdecaux'], 'user': 'foo2', 'password': 'bar2'}
        models.db.session.add(velov)
        models.db.session.commit()

        # refresh and detach the objets from the db before returning them
        models.db.session.refresh(velib)
        models.db.session.refresh(velov)
        models.db.session.expunge(velib)
        models.db.session.expunge(velov)

        return (velib, velov)


@pytest.fixture(scope="function", autouse=True)
def clean_db():
    with app.app_context():
        models.db.session.execute('truncate bss_provider;')
        models.db.session.commit()


def test_bss_provider_get(default_config):
    velib, velov = default_config
    resp = api_get('/v0/bss_providers')
    assert 'bss_providers' in resp
    assert len(resp['bss_providers']) == 2

    resp = api_get('/v0/bss_providers/velib')
    assert 'bss_providers' in resp
    assert len(resp['bss_providers']) == 1

    assert resp['bss_providers'][0]['network'] == 'velib'
    assert resp['bss_providers'][0]['discarded'] == False
    assert resp['bss_providers'][0]['args'] == velib.args


def test_bss_provider_post():
    input_json = {'network': 'foo', 'klass': 'jcdecaux', 'args': {'user': 'a'}}

    def check(resp):
        assert resp['network'] == input_json['network']
        assert resp['klass'] == input_json['klass']
        assert resp['args'] == input_json['args']
        assert resp['discarded'] == False

    _, status = api_post(
        'v0/bss_providers/', data=json.dumps(input_json), content_type='application/json', check=False
    )
    assert status == 400

    resp, status = api_post(
        'v0/bss_providers/foo', data=json.dumps(input_json), content_type='application/json', check=False
    )
    assert status == 201
    check(resp)

    resp = api_get('v0/bss_providers/foo')
    assert 'bss_providers' in resp
    assert len(resp['bss_providers']) == 1
    check(resp['bss_providers'][0])


def test_bss_provider_delete(default_config):
    resp = api_get('/v0/bss_providers')
    assert 'bss_providers' in resp
    assert len(resp['bss_providers']) == 2

    _, status = api_delete('/v0/bss_providers/velib', check=False, no_json=True)
    assert status == 204

    resp = api_get('/v0/bss_providers')
    assert 'bss_providers' in resp
    assert len(resp['bss_providers']) == 1
    assert resp['bss_providers'][0]['id'] == 'velov'

    # velib is still there, but not visible by default
    resp = api_get('/v0/bss_providers/velib')
    assert 'bss_providers' in resp
    assert len(resp['bss_providers']) == 1
    assert resp['bss_providers'][0]['id'] == 'velib'
    assert resp['bss_providers'][0]['discarded'] == True


def test_bss_provider_put(default_config):
    # we can use PUT to create a new configuration (like POST)
    input_json = {'network': 'foo', 'klass': 'jcdecaux', 'args': {'user': 'a'}}

    def check(resp):
        assert resp['network'] == input_json['network']
        assert resp['klass'] == input_json['klass']
        assert resp['args'] == input_json['args']
        assert resp['discarded'] == False

    resp, status = api_put(
        'v0/bss_providers/foo', data=json.dumps(input_json), content_type='application/json', check=False
    )
    assert status == 201
    check(resp)

    input_json['network'] = 'velib'
    resp = api_put('v0/bss_providers/foo', data=json.dumps(input_json), content_type='application/json')
    check(resp)
