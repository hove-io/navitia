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

from __future__ import absolute_import, print_function, division, unicode_literals
from tyr import app
from navitiacommon import models
from navitiacommon.constants import ENUM_EXTERNAL_SERVICE
from tests.check_utils import api_get, api_delete, api_put
import pytest
import ujson


@pytest.fixture
def default_external_service_config():
    with app.app_context():
        external_service_1 = models.ExternalService('forseti_free_floatings')
        external_service_1.klass = "jormungandr.external_services.free_floating.FreeFloatingProvider"
        external_service_1.navitia_service = "free_floatings"
        external_service_1.args = {
            "service_url": "http://my_external_service.com",
            "timeout": 10,
            "circuit_breaker_max_fail": 4,
            "circuit_breaker_reset_timeout": 60,
        }
        external_service_2 = models.ExternalService('forseti_vehicle_occupancies')
        external_service_2.klass = "jormungandr.external_services.vehicle_occupancies.VehicleOccupancyProvider"
        external_service_2.navitia_service = "vehicle_occupancies"
        external_service_2.args = {
            "service_url": "http://my_external_service.com",
            "timeout": 5,
            "circuit_breaker_max_fail": 2,
            "circuit_breaker_reset_timeout": 30,
        }
        external_service_3 = models.ExternalService('Timeo_Horizon')
        external_service_3.klass = "jormungandr.realtime_schedule.timeo.Timeo"
        external_service_3.navitia_service = "realtime_proxies"
        external_service_3.args = {
            "service_url": "http://my_external_service.com",
            "timeout": 5,
            "timezone": "aa",
            "destination_id_tag": "",
            "service_args": {'serviceID': "10", 'EntityID': "658", 'Media': "spec_navit_comp"},
        }
        external_service_4 = models.ExternalService('gtfs_rt_sch')
        external_service_4.klass = "bob"
        external_service_4.navitia_service = "vehicle_positions"
        external_service_4.args = {"service_url": "http://vehicle_positions.com", "timeout": 5, "timezone": "aa"}

        models.db.session.add(external_service_1)
        models.db.session.add(external_service_2)
        models.db.session.add(external_service_3)
        models.db.session.add(external_service_4)
        models.db.session.commit()
        # refresh and detach the objets from the db before returning them
        models.db.session.refresh(external_service_1)
        models.db.session.refresh(external_service_2)
        models.db.session.refresh(external_service_3)
        models.db.session.refresh(external_service_4)

        models.db.session.expunge(external_service_1)
        models.db.session.expunge(external_service_2)
        models.db.session.expunge(external_service_3)
        models.db.session.expunge(external_service_4)
        # Create instance
        instance = models.Instance('default')
        models.db.session.add(instance)
        models.db.session.commit()
        models.db.session.refresh(instance)
        models.db.session.expunge(instance)

        yield instance, external_service_1, external_service_2, external_service_3, external_service_4

        models.db.session.delete(instance)
        models.db.session.delete(external_service_1)
        models.db.session.delete(external_service_2)
        models.db.session.delete(external_service_3)
        models.db.session.delete(external_service_4)
        models.db.session.commit()


@pytest.fixture(scope='function', autouse=True)
def clean_db():
    with app.app_context():
        models.db.session.execute('truncate external_service cascade;')
        models.db.session.commit()


def test_external_service_get(default_external_service_config):
    """
    Test that the list of services with their info is correctly returned when queried
    """
    _, external_service_1, external_service_2, external_service_3, external_service_4 = (
        default_external_service_config
    )
    resp = api_get('/v0/external_services')
    assert "external_services" in resp
    assert len(resp['external_services']) == 4

    resp = api_get('/v0/external_services/forseti_free_floatings')
    assert "external_services" in resp
    assert len(resp['external_services']) == 1

    assert resp['external_services'][0]['id'] == external_service_1.id
    assert resp['external_services'][0]['navitia_service'] == external_service_1.navitia_service
    assert resp['external_services'][0]['klass'] == external_service_1.klass
    assert resp['external_services'][0]['args'] == external_service_1.args
    assert not resp['external_services'][0]['discarded']

    resp = api_get('/v0/external_services/Timeo_Horizon')
    assert "external_services" in resp
    assert len(resp['external_services']) == 1
    assert resp['external_services'][0]['id'] == external_service_3.id
    assert resp['external_services'][0]['navitia_service'] == external_service_3.navitia_service
    assert resp['external_services'][0]['klass'] == external_service_3.klass
    assert resp['external_services'][0]['args'] == external_service_3.args
    assert not resp['external_services'][0]['discarded']

    resp = api_get('/v0/external_services/gtfs_rt_sch')
    assert "external_services" in resp
    assert len(resp['external_services']) == 1
    assert resp['external_services'][0]['id'] == external_service_4.id
    assert resp['external_services'][0]['navitia_service'] == external_service_4.navitia_service
    assert resp['external_services'][0]['klass'] == external_service_4.klass
    assert resp['external_services'][0]['args'] == external_service_4.args
    assert not resp['external_services'][0]['discarded']


def test_external_service_put(default_external_service_config):
    """
    Test that a service is correctly created/updated in db and the info returned when queried
    """

    service = {
        "klass": "jormungandr.external_services.free_floating.FreeFloatingProvider",
        "navitia_service": "free_floatings",
        "args": {
            "service_url": "http://my_external_service_free_floating.com",
            "timeout": 10,
            "circuit_breaker_max_fail": 4,
            "circuit_breaker_reset_timeout": 60,
        },
    }
    resp, status = api_put(
        'v0/external_services/forseti_free_floatings_paris',
        data=ujson.dumps(service),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1
    for key in service.keys():
        assert resp['external_services'][0][key] == service[key]

    resp = api_get('/v0/external_services')
    assert 'external_services' in resp
    assert len(resp['external_services']) == 5

    # Update existing service
    service['args']['service_url'] = "http://my_external_service_free_floating_update.com"
    resp = api_put(
        'v0/external_services/forseti_free_floatings_paris',
        data=ujson.dumps(service),
        content_type='application/json',
    )
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1
    for key in service.keys():
        assert resp['external_services'][0][key] == service[key]

    resp = api_get('/v0/external_services/forseti_free_floatings_paris')
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1
    assert (
        resp['external_services'][0]['args']['service_url']
        == "http://my_external_service_free_floating_update.com"
    )


def test_realtime_proxies_external_service_put(default_external_service_config):
    """
    Test that a service is correctly created/updated in db and the info returned when queried
    """

    service = {
        "klass": "jormungandr.realtime_schedule.timeo.Timeo",
        "navitia_service": "realtime_proxies",
        "args": {
            "service_url": "http://my_external_service_free_floating.com",
            "timeout": 5,
            "timezone": "aa",
            "destination_id_tag": "",
            "service_args": {'serviceID': "10", 'EntityID': "658", 'Media': "spec_navit_comp"},
        },
    }
    resp, status = api_put(
        'v0/external_services/Timeo_Amelys',
        data=ujson.dumps(service),
        content_type='application/json',
        check=False,
    )
    assert status == 201
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1

    # Update existing service
    service['args']['service_url'] = "http://new_timeo_url.com"
    resp = api_put(
        'v0/external_services/Timeo_Amelys', data=ujson.dumps(service), content_type='application/json'
    )
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1

    resp = api_get('/v0/external_services/Timeo_Amelys')
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1
    assert resp['external_services'][0]['args']['service_url'] == "http://new_timeo_url.com"


def test_external_service_put_without_id():
    service = {
        "klass": "jormungandr.external_services.free_floating.FreeFloatingProvider",
        "navitia_service": "free_floatings",
        "args": {
            "service_url": "http://my_external_service_free_floating.com",
            "timeout": 10,
            "circuit_breaker_max_fail": 4,
            "circuit_breaker_reset_timeout": 60,
        },
    }
    resp, status = api_put(
        'v0/external_services', data=ujson.dumps(service), content_type='application/json', check=False
    )
    assert status == 400
    assert resp["message"] == "id is required"


def test_external_service_delete(default_external_service_config):
    """
    Test that a 'deleted' service isn't returned when querying all services,
    and that its 'discarded' parameter is set to True
    """
    resp = api_get('/v0/external_services')
    assert 'external_services' in resp
    assert len(resp['external_services']) == 4

    _, status_code = api_delete('v0/external_services/forseti_free_floatings', check=False, no_json=True)
    assert status_code == 204

    resp = api_get('/v0/external_services')
    assert 'external_services' in resp
    assert len(resp['external_services']) == 3

    resp = api_get('/v0/external_services/forseti_free_floatings')
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1
    assert resp['external_services'][0]['discarded']

    resp = api_get('/v0/external_services/Timeo_Horizon')
    assert 'external_services' in resp
    assert len(resp['external_services']) == 1
    assert not resp['external_services'][0]['discarded']


def test_associate_instance_external_service(default_external_service_config):
    instance, external_service_1, external_service_2, external_service_3, external_service_4 = (
        default_external_service_config
    )

    # Associate one external service
    resp = api_put('/v1/instances/{}?external_services={}'.format(instance.name, external_service_1.id))
    assert len(resp["external_services"]) == 1
    assert resp["external_services"][0]["id"] == external_service_1.id

    # Check that instance contains only one external service
    resp = api_get('/v1/instances')
    assert len(resp['instances']) == 1
    assert len(resp['instances'][0]['external_services']) == 1
    assert resp['instances'][0]['external_services'][0]['id'] == external_service_1.id

    # Update associated external service by external_service_2
    resp = api_put('/v1/instances/{}?external_services={}'.format(instance.name, external_service_2.id))
    assert len(resp["external_services"]) == 1
    assert resp["external_services"][0]["id"] == external_service_2.id

    # Check that instance contains only one external service
    resp = api_get('/v1/instances')
    assert len(resp['instances']) == 1
    assert len(resp['instances'][0]['external_services']) == 1

    # Associate two external services to the same instance
    resp = api_put(
        '/v1/instances/{}?external_services={}&external_services={}'.format(
            instance.name, external_service_1.id, external_service_2.id
        )
    )
    assert len(resp["external_services"]) == 2

    # An update with one external service deletes all existing associations and
    # re associates this external service to the instance
    resp = api_put('/v1/instances/{}?external_services={}'.format(instance.name, external_service_1.id))
    assert len(resp["external_services"]) == 1
    assert resp["external_services"][0]["id"] == external_service_1.id

    # Check that instance contains only one external service
    resp = api_get('/v1/instances')
    assert len(resp['instances']) == 1
    assert len(resp['instances'][0]['external_services']) == 1
    assert resp['instances'][0]['external_services'][0]['id'] == external_service_1.id

    # associate Timeo service
    resp = api_put(
        '/v1/instances/{}?external_services={}&external_services={}&external_services={}'.format(
            instance.name, external_service_1.id, external_service_2.id, external_service_3.id
        )
    )
    assert len(resp["external_services"]) == 3

    # Update associate without Timeo service
    resp = api_put(
        '/v1/instances/{}?external_services={}&external_services={}'.format(
            instance.name, external_service_1.id, external_service_2.id
        )
    )
    assert len(resp["external_services"]) == 2


def test_external_service_schema():
    """
    Test that a service isn't created in db when missing required parameters
    """

    def send_and_check(serive_id, json_data, missing_param=None):
        resp, status = api_put(
            url='v0/external_services/{}'.format(serive_id),
            data=ujson.dumps(json_data),
            content_type='application/json',
            check=False,
        )
        assert status == 400
        assert 'message' in resp
        assert 'status' in resp
        assert resp['status'] == "invalid data"
        if missing_param is None:
            return resp['message']
        assert resp['message'] == "'{}' is a required property".format(missing_param)

    # 'klass' is missing
    corrupted_provider = {
        "args": {"service_url": "service.url", "timeout": 10},
        "navitia_service": "free_floatings",
    }
    send_and_check('external_service_1', corrupted_provider, 'klass')

    # 'navitia_service' is missing
    corrupted_provider = {
        "args": {"service_url": "service.url", "timeout": 10},
        "klass": "jormungandr.external_services.free_floating.FreeFloatingProvider",
    }
    send_and_check('external_service_1', corrupted_provider, 'navitia_service')

    # 'service_url' is missing
    corrupted_provider = {
        "args": {"timeout": 10, "circuit_breaker_max_fail": 4, "circuit_breaker_reset_timeout": 60},
        "klass": "jormungandr.external_services.free_floating.FreeFloatingProvider",
        "navitia_service": "free_floatings",
    }
    send_and_check('external_service_1', corrupted_provider, 'service_url')

    # wrong 'navitia_service' other than 'free_floatings' and 'vehicle_occupancies'
    corrupted_provider = {
        "args": {"service_url": "service.url", "timeout": 10},
        "klass": "jormungandr.external_services.free_floating.FreeFloatingProvider",
        "navitia_service": "navitia_service",
    }
    massage = send_and_check('external_service_1', corrupted_provider)
    assert "'{}' is not one of {}".format('navitia_service', list(ENUM_EXTERNAL_SERVICE)) in massage
