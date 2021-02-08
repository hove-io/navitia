# encoding: utf-8
# Copyright (c) 2001-2021, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
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

from jormungandr.external_services.external_service_provider_manager import ExternalServiceManager
from navitiacommon.models import ExternalService
import datetime
import pytest

valid_configuration = [
    {
        "id": "forseti_free_floatings",
        "navitia_service": "free_floatings",
        "args": {
            "service_url": "http://config_from_file/free_floatings",
            "timeout": 10,
            "circuit_breaker_max_fail": 4,
            "circuit_breaker_reset_timeout": 60,
        },
        "class": "jormungandr.external_services.free_floating.FreeFloatingProvider",
    }
]

# in args the key 'url' is not correct. It should be 'service_url'
wrong_key_configuration = [
    {
        "id": "forseti_free_floatings",
        "navitia_service": "free_floatings",
        "args": {
            "url": "http://wtf/free_floatings",
            "timeout": 10,
            "circuit_breaker_max_fail": 4,
            "circuit_breaker_reset_timeout": 60,
        },
        "class": "jormungandr.external_services.free_floating.FreeFloatingProvider",
    }
]

wrong_class_configuration = [
    {
        "id": "forseti_free_floatings",
        "navitia_service": "free_floatings",
        "args": {
            "service_url": "http://wtf/free_floatings",
            "timeout": 10,
            "circuit_breaker_max_fail": 4,
            "circuit_breaker_reset_timeout": 60,
        },
        "class": "jormungandr.external_services.free_floating.toto",
    }
]


class MockInstance:
    def __init__(self, name="test1"):
        self.name = name


class ExternalServiceProviderMock(object):
    def __init__(self, service_url, timeout=2, **kwargs):
        self.service_url = service_url
        self.timeout = timeout


def external_service_provider_manager_config_from_file_test():
    """
    Test that external services are created from env when conditions are met
    """
    instance = MockInstance()

    # With configuration having wrong class no external service is initialized
    manager = ExternalServiceManager(instance, external_service_configuration=wrong_class_configuration)
    with pytest.raises(Exception):
        manager.init_external_services()

    # With configuration missing a key in "args" external service will be initialized
    # with a right key without any value
    manager = ExternalServiceManager(instance, external_service_configuration=wrong_key_configuration)
    manager.init_external_services()
    assert len(manager._external_services_legacy) == 1
    assert len(manager._external_services_legacy.get('free_floatings', [])) == 1
    service = manager._external_services_legacy.get('free_floatings', [])[0]
    assert service.service_url is None
    assert service.timeout == 10

    # With a valid configuration a proper external service will be initialized
    manager = ExternalServiceManager(instance, external_service_configuration=valid_configuration)
    manager.init_external_services()
    assert len(manager._external_services_legacy) == 1
    assert len(manager._external_services_legacy.get('free_floatings', [])) == 1
    service = manager._external_services_legacy.get('free_floatings', [])[0]
    assert service.service_url == "http://config_from_file/free_floatings"
    assert service.timeout == 10
    assert service.breaker.reset_timeout == 60
    assert service.breaker.fail_max == 4


def services_getter_ok():
    service1 = ExternalService(id='forseti_free_floatings')
    service1.navitia_service = 'free_floatings'
    service1.klass = 'jormungandr.external_services.free_floating.FreeFloatingProvider'
    service1.args = {'service_url': 'http://config_from_db/free_floatings', 'timeout': 5}
    service1.created_at = datetime.datetime.utcnow()

    service2 = ExternalService(id='forseti_vehicle_occupancies')
    service2.navitia_service = 'vehicle_occupancies'
    service2.klass = 'jormungandr.external_services.free_floating.FreeFloatingProvider'
    service2.args = {'service_url': 'http://127.0.0.1:8080/vehicle_occupancies', 'timeout': 5}
    return [service1, service2]


def services_getter_update():
    service = ExternalService(id='forseti_free_floatings')
    service.navitia_service = 'free_floatings'
    service.klass = 'jormungandr.external_services.free_floating.FreeFloatingProvider'
    service.args = {'service_url': 'http://update/free_floatings', 'timeout': 10}
    service.created_at = datetime.datetime.utcnow()
    return [service]


def services_getter_update_with_wrong_class():
    service = ExternalService(id='forseti_free_floatings')
    service.navitia_service = 'free_floatings'
    service.klass = 'jormungandr.external_services.free_floating.Toto'
    service.args = {'service_url': 'http://update/free_floatings', 'timeout': 10}
    service.created_at = datetime.datetime.utcnow()
    return [service]


def external_service_provider_manager_db_test():
    """
    # Test that external services are created from db when conditions are met
    """
    instance = MockInstance()
    manager = ExternalServiceManager(instance, [], services_getter_ok, -1)

    # We use env or file configuration only for debug purpose. No external service available
    # as we haven't yet updated services from the DB
    manager.init_external_services()
    assert not manager._external_services_legacy

    # 2 external services defined in the db are associated to the coverage
    # one with navitia_service=free_floatings where as another with navitia_service=vehicle_occupancies
    # For api /free_floatings the service associated to navitia_service=free_floatings will be used
    manager.update_config()
    assert len(manager._external_services_legacy) == 2
    service = manager._external_services_legacy.get('free_floatings', [])[0]
    assert service.service_url == "http://config_from_db/free_floatings"
    assert service.timeout == 5
    assert service.breaker.reset_timeout == 60
    assert service.breaker.fail_max == 4
    manager_update = manager._last_update

    # services are re-initialized from the new configurations in the db (services_getter_update)
    manager._external_service_getter = services_getter_update
    manager.update_config()

    assert manager._last_update > manager_update
    assert len(manager._external_services_legacy) == 1
    service = manager._external_services_legacy.get('free_floatings', [])[0]
    assert service.service_url == 'http://update/free_floatings'
    assert service.timeout == 10
    assert service.breaker.reset_timeout == 60
    assert service.breaker.fail_max == 4

    # No service is re-initialized from the new configurations in the db as the class is wrong one
    manager_update = manager._last_update
    manager._external_service_getter = services_getter_update_with_wrong_class
    manager.update_config()
    assert len(manager._external_services_legacy) == 0


def external_service_provider_manager_config_from_file_and_db_test():
    """
    # Test that external services are created from configuration but replaced by services form db
    """
    instance = MockInstance()
    manager = ExternalServiceManager(instance, valid_configuration, services_getter_ok, -1)

    # We use env or file configuration only for debug purpose while will be replaced by services
    # from the DB after
    manager.init_external_services()
    assert len(manager._external_services_legacy) == 1
    assert len(manager._external_services_legacy.get('free_floatings', [])) == 1
    service = manager._external_services_legacy.get('free_floatings', [])[0]
    assert service.service_url == "http://config_from_file/free_floatings"

    # 2 external services defined in the db are associated to the coverage
    # one with navitia_service=free_floatings where as another with navitia_service=vehicle_occupancies
    # For api /free_floatings the service associated to navitia_service=free_floatings will be used
    # All the services from configuration file are all removed.
    manager.update_config()
    assert len(manager._external_services_legacy) == 2
    service = manager._external_services_legacy.get('free_floatings', [])[0]
    assert service.service_url == "http://config_from_db/free_floatings"
    assert service.timeout == 5
    assert service.breaker.reset_timeout == 60
    assert service.breaker.fail_max == 4
