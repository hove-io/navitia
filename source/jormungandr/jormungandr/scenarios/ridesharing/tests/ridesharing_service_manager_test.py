# encoding: utf-8
# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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

from jormungandr.scenarios.ridesharing.ridesharing_service_manager import RidesharingServiceManager
from jormungandr.scenarios.ridesharing.blablacar import Blablacar
from jormungandr.scenarios.ridesharing.instant_system import InstantSystem
from navitiacommon.models.ridesharing_service import RidesharingService


class MockInstance:
    def __init__(self, name="test1"):
        self.name = name


config_instant_system = {
    "class": "jormungandr.scenarios.ridesharing.instant_system.InstantSystem",
    "args": {
        "service_url": "tata",
        "api_key": "tata key",
        "network": "M",
        "rating_scale_min": 1,
        "rating_scale_max": 5,
        "crowfly_radius": 200,
        "timeframe_duration": 1800,
    },
}
config_blablacar = {
    "class": "jormungandr.scenarios.ridesharing.blablacar.Blablacar",
    "args": {"service_url": "tata", "api_key": "tata key", "network": "MM"},
}


def instant_system_ridesharing_service_manager_config_from_file_test():
    """
    creation of a ridesharing service from the configuration file: InstantSystem
    Result: We must find the service InstantSystem
    """
    instance = MockInstance()
    ridesharing_services_config = [config_instant_system]
    ridesharing_manager = RidesharingServiceManager(instance, ridesharing_services_config)
    ridesharing_manager.init_ridesharing_services()
    assert len(ridesharing_manager.ridesharing_services_configuration) == 1
    assert not ridesharing_manager._ridesharing_services
    assert not ridesharing_manager._rs_services_getter
    assert ridesharing_manager._update_interval == 60
    assert len(ridesharing_manager._ridesharing_services_legacy) == 1


def blablacar_ridesharing_service_manager_config_from_file_test():
    """
    creation of a ridesharing service from the configuration file: Blablacar
    Result: We must find the service Blablacar
    """
    instance = MockInstance()
    ridesharing_services_config = [config_blablacar]
    ridesharing_manager = RidesharingServiceManager(instance, ridesharing_services_config)
    ridesharing_manager.init_ridesharing_services()
    assert len(ridesharing_manager.ridesharing_services_configuration) == 1
    assert not ridesharing_manager._ridesharing_services
    assert not ridesharing_manager._rs_services_getter
    assert ridesharing_manager._update_interval == 60
    assert len(ridesharing_manager._ridesharing_services_legacy) == 1


def two_ridesharing_service_manager_config_from_file_test():
    """
    creation of a ride sharing services from the configuration file: Blablacar and InstantSystym
    Result: We have to find both services Blablacar and InstantSystem
    """
    instance = MockInstance()
    ridesharing_services_config = [config_blablacar, config_instant_system]
    ridesharing_manager = RidesharingServiceManager(instance, ridesharing_services_config)
    ridesharing_manager.init_ridesharing_services()
    assert len(ridesharing_manager.ridesharing_services_configuration) == 2
    assert not ridesharing_manager._ridesharing_services
    assert not ridesharing_manager._rs_services_getter
    assert ridesharing_manager._update_interval == 60
    assert len(ridesharing_manager._ridesharing_services_legacy) == 2


def mock_get_attr_instant_system():
    json = {"klass": config_instant_system["class"], "args": config_instant_system["args"]}
    service = RidesharingService(id="InstantSystem", json=json)
    return [service]


def mock_get_attr_instant_system_and_blablacar():
    json = {"klass": config_blablacar["class"], "args": config_blablacar["args"]}
    Blablacar = RidesharingService(id="Blablacar", json=json)
    return [Blablacar] + mock_get_attr_instant_system()


def two_ridesharing_service_manager_config_from_file_and_db_test():
    """
    creation of a ridesharing service from the configuration file: Blablacar
    creation of a ridesharing service from database : InstantSystem
    Result: We have to find both services Blablacar and InstantSystem
    """
    instance = MockInstance()
    ridesharing_services_config = [config_blablacar]
    ridesharing_manager = RidesharingServiceManager(
        instance, ridesharing_services_config, rs_services_getter=mock_get_attr_instant_system
    )
    ridesharing_manager.init_ridesharing_services()
    ridesharing_manager.update_config()
    assert len(ridesharing_manager.ridesharing_services_configuration) == 1
    assert len(list(ridesharing_manager._ridesharing_services.values())) == 1
    assert ridesharing_manager._ridesharing_services["InstantSystem"].system_id == "Instant System"
    assert ridesharing_manager._rs_services_getter
    assert ridesharing_manager._update_interval == 60
    assert len(ridesharing_manager._ridesharing_services_legacy) == 1
    services = ridesharing_manager.get_all_ridesharing_services()
    assert len(services) == 2


def two_same_ridesharing_service_manager_config_from_file_and_db_test():
    """
    creation of a ridesharing service from the configuration file: InstantSystem
    creation of a ridesharing service from database : InstantSystem
    Result: We must find the service InstantSystem
    """
    instance = MockInstance()
    ridesharing_services_config = [config_instant_system]
    ridesharing_manager = RidesharingServiceManager(
        instance, ridesharing_services_config, rs_services_getter=mock_get_attr_instant_system
    )
    ridesharing_manager.init_ridesharing_services()
    ridesharing_manager.update_config()
    assert len(ridesharing_manager.ridesharing_services_configuration) == 1
    assert len(list(ridesharing_manager._ridesharing_services.values())) == 1
    assert ridesharing_manager._ridesharing_services["InstantSystem"].system_id == "Instant System"
    assert ridesharing_manager._rs_services_getter
    assert ridesharing_manager._update_interval == 60
    assert ridesharing_manager._update_interval == 60
    assert len(ridesharing_manager._ridesharing_services_legacy) == 0
    services = ridesharing_manager.get_all_ridesharing_services()
    assert len(services) == 1


def ridesharing_service_manager_config_from_file_and_db_test():
    """
    creation of a ridesharing service from database: InstantSystem
    creation of a ridesharing service from database : Blablacar
    Result: We have to find both services Blablacar and InstantSystem
    """
    instance = MockInstance()
    ridesharing_manager = RidesharingServiceManager(
        instance, [], rs_services_getter=mock_get_attr_instant_system_and_blablacar
    )
    ridesharing_manager.init_ridesharing_services()
    ridesharing_manager.update_config()
    assert len(ridesharing_manager.ridesharing_services_configuration) == 0
    assert len(list(ridesharing_manager._ridesharing_services.values())) == 2
    assert ridesharing_manager._ridesharing_services["InstantSystem"].system_id == "Instant System"
    assert ridesharing_manager._ridesharing_services["Blablacar"].system_id == "Blablacar"
    assert ridesharing_manager._rs_services_getter
    assert ridesharing_manager._update_interval == 60
    assert ridesharing_manager._update_interval == 60
    assert len(ridesharing_manager._ridesharing_services_legacy) == 0
    services = ridesharing_manager.get_all_ridesharing_services()
    assert len(services) == 2


def blablacar_init_class_test():
    instance = MockInstance()
    ridesharing_manager = RidesharingServiceManager(instance, [])
    service = ridesharing_manager._init_class(config_blablacar["class"], config_blablacar["args"])
    assert isinstance(service, Blablacar)
    assert service.system_id == "Blablacar"


def instant_system_init_class_test():
    instance = MockInstance()
    ridesharing_manager = RidesharingServiceManager(instance, [])
    service = ridesharing_manager._init_class(config_instant_system["class"], config_instant_system["args"])
    assert isinstance(service, InstantSystem)
    assert service.system_id == 'Instant System'
