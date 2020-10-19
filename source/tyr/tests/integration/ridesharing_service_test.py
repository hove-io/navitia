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
def default_ridesharing_service_config():
    with app.app_context():
        ridesharing0 = models.RidesharingService('TestCovoiturage0')
        ridesharing0.klass = 'jormungandr.scenarios.ridesharing.instant_system.InstantSystem'
        ridesharing0.args = {
            'service_url': 'http://bob.com',
            "rating_scale_min": 0,
            "crowfly_radius": 500,
            "rating_scale_max": 5,
            "api_key": "aaa",
            "network": "Test Covoiturage",
            "feed_publisher": {
                "url": "url",
                "id": "test-coverage",
                "license": "Private",
                "name": "Test ABC - TEST",
            },
        }
        ridesharing1 = models.RidesharingService('TestCovoiturage1')
        ridesharing1.klass = 'jormungandr.scenarios.ridesharing.instant_system.InstantSystem'
        ridesharing1.args = {
            'service_url': 'http://bob.com',
            "rating_scale_min": 0,
            "crowfly_radius": 500,
            "rating_scale_max": 5,
            "api_key": "aaa",
            "network": "Test Covoiturage",
            "feed_publisher": {
                "url": "url",
                "id": "test-coverage",
                "license": "Private",
                "name": "Test ABC - TEST",
            },
        }
        models.db.session.add(ridesharing0)
        models.db.session.add(ridesharing1)
        models.db.session.commit()
        # refresh and detach the objets from the db before returning them
        models.db.session.refresh(ridesharing0)
        models.db.session.refresh(ridesharing1)
        models.db.session.expunge(ridesharing0)
        models.db.session.expunge(ridesharing1)
        # Create instance
        instance = models.Instance('default')
        models.db.session.add(instance)
        models.db.session.commit()
        models.db.session.refresh(instance)
        models.db.session.expunge(instance)

        yield instance, ridesharing0, ridesharing1

        models.db.session.delete(instance)
        models.db.session.delete(ridesharing0)
        models.db.session.delete(ridesharing1)
        models.db.session.commit()


@pytest.fixture(scope='function', autouse=True)
def clean_db():
    with app.app_context():
        models.db.session.execute('truncate ridesharing_service cascade;')
        models.db.session.commit()


def test_ridesharing_service_get(default_ridesharing_service_config):
    """
    Test that the list of services with their info is correctly returned when queried
    """
    _, ridesharing0, ridesharing1 = default_ridesharing_service_config
    resp = api_get('/v0/ridesharing_services')
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 2

    resp = api_get('/v0/ridesharing_services/TestCovoiturage0')
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 1

    assert resp['ridesharing_services'][0]['args'] == ridesharing0.args
    assert resp['ridesharing_services'][0]['id'] == ridesharing0.id
    assert not resp['ridesharing_services'][0]['discarded']


def test_ridesharing_service_put(default_ridesharing_service_config):
    """
    Test that a service is correctly created/updated in db and the info returned when queried
    """

    service = {
        'klass': 'jormungandr.scenarios.ridesharing.instant_system.InstantSystem',
        'args': {
            'service_url': 'https://new_url.io',
            "rating_scale_min": 5,
            "crowfly_radius": 60,
            "api_key": "abcd",
        },
    }
    resp, status = api_put(
        'v0/ridesharing_services/TestCovoiturage3',
        data=ujson.dumps(service),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 1
    for key in service.keys():
        assert resp['ridesharing_services'][0][key] == service[key]

    resp = api_get('/v0/ridesharing_services')
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 3

    # Update existing service
    service['args']['service_url'] = 'https://new_url_update.io'
    resp = api_put(
        'v0/ridesharing_services/TestCovoiturage3', data=ujson.dumps(service), content_type='application/json'
    )
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 1
    for key in service.keys():
        assert resp['ridesharing_services'][0][key] == service[key]

    resp = api_get('/v0/ridesharing_services/TestCovoiturage3')
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 1
    assert resp['ridesharing_services'][0]['args']['service_url'] == 'https://new_url_update.io'


def test_ridesharing_service_put_without_id():
    service = {
        'klass': 'jormungandr.scenarios.ridesharing.instant_system.InstantSystem',
        'args': {
            'service_url': 'https://new_url.io',
            "rating_scale_min": 0,
            "crowfly_radius": 600,
            "api_key": "1235",
        },
    }
    resp, status = api_put(
        'v0/ridesharing_services', data=ujson.dumps(service), content_type='application/json', check=False
    )
    assert status == 400
    assert resp["message"] == "id is required"


def test_equipments_provider_delete(default_ridesharing_service_config):
    """
    Test that a 'deleted' service isn't returned when querying all services,
    and that its 'discarded' parameter is set to True
    """
    resp = api_get('/v0/ridesharing_services')
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 2

    _, status_code = api_delete('v0/ridesharing_services/TestCovoiturage0', check=False, no_json=True)
    assert status_code == 204

    resp = api_get('/v0/ridesharing_services')
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 1

    resp = api_get('/v0/ridesharing_services/TestCovoiturage0')
    assert 'ridesharing_services' in resp
    assert len(resp['ridesharing_services']) == 1
    assert resp['ridesharing_services'][0]['discarded']


def test_associate_instance_ridesharing_service(default_ridesharing_service_config):
    instance, ridesharing0, ridesharing1 = default_ridesharing_service_config

    # Associate one ridesharing service
    resp = api_put('/v1/instances/{}?ridesharing_services={}'.format(instance.name, ridesharing0.id))
    assert len(resp["ridesharing_services"]) == 1
    assert resp["ridesharing_services"][0]["id"] == ridesharing0.id

    # Update associate ridesharing service
    resp = api_put('/v1/instances/{}?ridesharing_services={}'.format(instance.name, ridesharing1.id))
    assert len(resp["ridesharing_services"]) == 1
    assert resp["ridesharing_services"][0]["id"] == ridesharing1.id

    # Update associate ridesharing service Two services
    resp = api_put(
        '/v1/instances/{}?ridesharing_services={}&ridesharing_services={}'.format(
            instance.name, ridesharing0.id, ridesharing1.id
        )
    )
    assert len(resp["ridesharing_services"]) == 2


def test_ridesharing_service_schema():
    """
    Test that a service isn't created in db when missing required parameters
    """

    def send_and_check(serive_id, json_data, missing_param):
        resp, status = api_put(
            url='v0/ridesharing_services/{}'.format(serive_id),
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
    corrupted_provider = {'args': {'service_url': 'service.url', "api_key": "1235"}}
    send_and_check('Service0', corrupted_provider, 'klass')

    # args['service_url'] is missing
    corrupted_provider = {
        'klass': 'jormungandr.scenarios.ridesharing.instant_system.InstantSystemr',
        'args': {"api_key": "1235"},
    }
    send_and_check('Service0', corrupted_provider, 'service_url')

    # args['api_key'] is missing
    corrupted_provider = {
        'klass': 'jormungandr.scenarios.ridesharing.instant_system.InstantSystemr',
        'args': {"service_url": "1235"},
    }
    send_and_check('Service0', corrupted_provider, 'api_key')
