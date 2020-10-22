# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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

from __future__ import absolute_import
import pytest
from jormungandr.instance import Instance
from jormungandr.street_network.streetnetwork_backend_manager import StreetNetworkBackendManager
from navitiacommon.models.streetnetwork_backend import StreetNetworkBackend
from jormungandr.street_network.kraken import Kraken
from jormungandr.street_network.valhalla import Valhalla
from jormungandr.exceptions import ConfigException, TechnicalError
from jormungandr.tests.utils_test import compare_list_of_dicts

import datetime

KRAKEN_CLASS = 'jormungandr.street_network.kraken.Kraken'
VALHALLA_CLASS = 'jormungandr.street_network.valhalla.Valhalla'
ALL_MODES = ['walking', 'bike', 'bss', 'car']


def _init_and_create_backend_without_default(conf):
    sn_manager = StreetNetworkBackendManager()
    sn_manager._create_street_network_backends("instance", conf)
    return sn_manager.get_all_street_networks_legacy("instance")


def kraken_class_test():
    kraken_conf = [{'modes': ALL_MODES, 'class': KRAKEN_CLASS}]
    services = _init_and_create_backend_without_default(kraken_conf)
    assert len(services) == 1
    assert isinstance(services[0], Kraken)


def kraken_klass_test():
    kraken_conf = [{'modes': ALL_MODES, 'klass': KRAKEN_CLASS}]
    services = _init_and_create_backend_without_default(kraken_conf)
    assert len(services) == 1
    assert isinstance(services[0], Kraken)


def valhalla_class_without_url_test():
    with pytest.raises(ValueError) as excinfo:
        valhalla_without_url = [{'modes': ALL_MODES, 'class': VALHALLA_CLASS}]
        _init_and_create_backend_without_default(valhalla_without_url)
    assert 'service_url None is not a valid url' in str(excinfo.value)


def valhalla_class_wit_empty_url_test():
    with pytest.raises(ValueError) as excinfo:
        kraken_conf = [{'modes': ALL_MODES, 'class': VALHALLA_CLASS, 'args': {"service_url": ""}}]
        _init_and_create_backend_without_default(kraken_conf)
    assert 'service_url  is not a valid url' in str(excinfo.value)


def valhalla_class_with_invalid_url_test():
    with pytest.raises(ValueError) as excinfo:
        kraken_conf = [{'modes': ALL_MODES, 'class': VALHALLA_CLASS, 'args': {"service_url": "bob"}}]
        _init_and_create_backend_without_default(kraken_conf)
    assert 'service_url bob is not a valid url' in str(excinfo.value)


def valhalla_class_without_costing_options_test():
    kraken_conf = [
        {'modes': ALL_MODES, 'class': VALHALLA_CLASS, 'args': {"service_url": "http://localhost:8002"}}
    ]
    services = _init_and_create_backend_without_default(kraken_conf)
    assert len(services) == 1
    assert isinstance(services[0], Valhalla)


def valhalla_class_with_empty_costing_options_test():
    kraken_conf = [
        {
            'modes': ALL_MODES,
            'class': VALHALLA_CLASS,
            'args': {"service_url": "http://localhost:8002", "costing_options": {}},
        }
    ]
    services = _init_and_create_backend_without_default(kraken_conf)
    assert len(services) == 1
    assert isinstance(services[0], Valhalla)


def valhalla_class_with_url_valid_test():
    kraken_conf = [
        {
            'modes': ALL_MODES,
            'class': VALHALLA_CLASS,
            'args': {
                "service_url": "http://localhost:8002",
                "costing_options": {"pedestrian": {"walking_speed": 50.1}},
            },
        }
    ]
    services = _init_and_create_backend_without_default(kraken_conf)
    assert len(services) == 1
    assert isinstance(services[0], Valhalla)


def street_network_without_class_test():
    with pytest.raises(KeyError) as excinfo:
        kraken_conf = [
            {
                'modes': ['walking'],
                'args': {
                    "service_url": "http://localhost:8002",
                    "costing_options": {"pedestrian": {"walking_speed": 50.1}},
                },
            }
        ]
        _init_and_create_backend_without_default(kraken_conf)
    assert (
        'impossible to build a StreetNetwork, missing mandatory field in configuration: class or klass'
        in str(excinfo.value)
    )


def valhalla_class_with_class_invalid_test():
    with pytest.raises(ConfigException) as excinfo:
        kraken_conf = [
            {
                'class': 'jormungandr',
                'modes': ['walking'],
                'args': {
                    "service_url": "http://localhost:8002",
                    "costing_options": {"pedestrian": {"walking_speed": 50.1}},
                },
            }
        ]
        _init_and_create_backend_without_default(kraken_conf)


def valhalla_class_with_class_not_exist_test():
    with pytest.raises(ConfigException) as excinfo:
        kraken_conf = [
            {
                'class': 'jormungandr.street_network.valhalla.bob',
                'modes': ['walking'],
                'args': {
                    "service_url": "http://localhost:8002",
                    "costing_options": {"pedestrian": {"walking_speed": 50.1}},
                },
            }
        ]
        _init_and_create_backend_without_default(kraken_conf)


def sn_backends_getter_ok():
    sn_backend1 = StreetNetworkBackend(id='kraken')
    sn_backend1.klass = 'jormungandr.street_network.tests.StreetNetworkBackendMock'
    sn_backend1.args = {'url': 'kraken.url'}
    sn_backend1.created_at = datetime.datetime.utcnow()

    sn_backend2 = StreetNetworkBackend(id='asgard')
    sn_backend2.klass = 'jormungandr.street_network.tests.StreetNetworkBackendMock'
    sn_backend2.args = {'url': 'asgard.url'}
    sn_backend2.created_at = datetime.datetime.utcnow()

    return [sn_backend1, sn_backend2]


def sn_backends_getter_update():
    sn_backend = StreetNetworkBackend(id='kraken')
    sn_backend.klass = 'jormungandr.street_network.tests.StreetNetworkBackendMock'
    sn_backend.args = {'url': 'kraken.url.UPDATE'}
    sn_backend.updated_at = datetime.datetime.utcnow()
    return [sn_backend]


def sn_backends_getter_wrong_class():
    sn_backend = StreetNetworkBackend(id='kraken')
    sn_backend.klass = 'jormungandr/street_network/tests/StreetNetworkBackendMock'
    sn_backend.args = {'url': 'kraken.url'}
    return [sn_backend]


def streetnetwork_backend_manager_db_test():
    """
    Test that streetnetwork backends are created from db when conditions are met
    """
    manager = StreetNetworkBackendManager(sn_backends_getter_ok, -1)

    # 2 sn_backends defined in db are associated to the coverage
    # -> 2 sn_backends created
    manager._update_config("instance")

    assert not manager._streetnetwork_backends_by_instance_legacy
    assert len(manager._streetnetwork_backends) == 2
    assert 'kraken' in manager._streetnetwork_backends
    assert manager._streetnetwork_backends['kraken'].url == 'kraken.url'
    assert 'asgard' in manager._streetnetwork_backends
    assert manager._streetnetwork_backends['asgard'].url == 'asgard.url'

    manager_update = manager._last_update
    assert 'kraken' in manager._streetnetwork_backends_last_update
    kraken_update = manager._streetnetwork_backends_last_update['kraken']

    # Sn_backend already existing is updated
    manager._sn_backends_getter = sn_backends_getter_update
    manager._update_config("instance")
    assert manager._last_update > manager_update
    assert not manager._streetnetwork_backends_by_instance_legacy
    assert len(manager._streetnetwork_backends) == 2
    assert 'kraken' in manager._streetnetwork_backends
    assert manager._streetnetwork_backends['kraken'].url == 'kraken.url.UPDATE'
    assert 'kraken' in manager._streetnetwork_backends_last_update
    assert manager._streetnetwork_backends_last_update['kraken'] > kraken_update

    # Long update interval so sn_backend shouldn't be updated
    manager = StreetNetworkBackendManager(sn_backends_getter_ok, 600)
    manager._update_config("instance")
    assert not manager._streetnetwork_backends_by_instance_legacy
    assert len(manager._streetnetwork_backends) == 2
    assert 'kraken' in manager._streetnetwork_backends
    assert manager._streetnetwork_backends['kraken'].url == 'kraken.url'
    manager_update = manager._last_update

    manager.sn_backends_getter = sn_backends_getter_update
    manager._update_config("instance")
    assert manager._last_update == manager_update
    assert not manager._streetnetwork_backends_by_instance_legacy
    assert len(manager._streetnetwork_backends) == 2
    assert 'kraken' in manager._streetnetwork_backends
    assert manager._streetnetwork_backends['kraken'].url == 'kraken.url'


def wrong_streetnetwork_backend_test():
    """
    Test that streetnetwork backends with wrong parameters aren't created
    """

    # Sn_backend has a class wrongly formatted
    manager = StreetNetworkBackendManager(sn_backends_getter_wrong_class, -1)
    manager._update_config("instance")
    assert not manager._streetnetwork_backends_by_instance_legacy
    assert not manager._streetnetwork_backends

    # No sn_backends available in db
    manager._sn_backends_getter = []
    manager._update_config("instance")
    assert not manager._streetnetwork_backends_by_instance_legacy
    assert not manager._streetnetwork_backends


def append_default_street_network_to_config_test():
    manager = StreetNetworkBackendManager(sn_backends_getter=None, update_interval=-1)

    # Kraken is used for all the modes.
    config_full_default = [
        {
            'args': {
                'street_network': {
                    'args': {'timeout': 10},
                    'class': 'jormungandr.street_network.Kraken',
                    'modes': [],
                }
            },
            'class': 'jormungandr.street_network.Taxi',
            'modes': ['taxi'],
        },
        {
            'args': {
                'street_network': {
                    'args': {'timeout': 10},
                    'class': 'jormungandr.street_network.Kraken',
                    'modes': [],
                }
            },
            'class': 'jormungandr.street_network.Ridesharing',
            'modes': ['ridesharing'],
        },
        {
            'args': {'timeout': 10},
            'class': 'jormungandr.street_network.Kraken',
            'modes': ['car', 'walking', 'bike', 'bss'],
        },
    ]

    response = manager._append_default_street_network_to_config(None)
    compare_list_of_dicts("class", response, config_full_default)

    # Asgard is used for car, Kraken for all the other modes.
    config_car_asgard = [{'class': 'jormungandr.street_network.Asgard', 'modes': ['car']}]
    response = manager._append_default_street_network_to_config(config_car_asgard)

    config_asgard_plus_default = [
        {'class': 'jormungandr.street_network.Asgard', 'modes': ['car']},
        {
            'args': {
                'street_network': {
                    'args': {'timeout': 10},
                    'class': 'jormungandr.street_network.Kraken',
                    'modes': [],
                }
            },
            'class': 'jormungandr.street_network.Taxi',
            'modes': ['taxi'],
        },
        {
            'args': {
                'street_network': {
                    'args': {'timeout': 10},
                    'class': 'jormungandr.street_network.Kraken',
                    'modes': [],
                }
            },
            'class': 'jormungandr.street_network.Ridesharing',
            'modes': ['ridesharing'],
        },
        {
            'args': {'timeout': 10},
            'class': 'jormungandr.street_network.Kraken',
            'modes': ['walking', 'bike', 'bss'],
        },
    ]
    compare_list_of_dicts("class", response, config_asgard_plus_default)

    # Surf is used for surf, Kraken for all the other modes.
    # Surf stay in the config but is not used.
    wrong_config = [{'class': 'jormungandr.street_network.Surf', 'modes': ['surf']}]
    response = manager._append_default_street_network_to_config(wrong_config)

    wrong_plus_default_config = [
        {'class': 'jormungandr.street_network.Surf', 'modes': ['surf']},
        {
            'args': {
                'street_network': {
                    'args': {'timeout': 10},
                    'class': 'jormungandr.street_network.Kraken',
                    'modes': [],
                }
            },
            'class': 'jormungandr.street_network.Taxi',
            'modes': ['taxi'],
        },
        {
            'args': {
                'street_network': {
                    'args': {'timeout': 10},
                    'class': 'jormungandr.street_network.Kraken',
                    'modes': [],
                }
            },
            'class': 'jormungandr.street_network.Ridesharing',
            'modes': ['ridesharing'],
        },
        {
            'args': {'timeout': 10},
            'class': 'jormungandr.street_network.Kraken',
            'modes': ['car', 'walking', 'bike', 'bss'],
        },
    ]
    compare_list_of_dicts("class", response, wrong_plus_default_config)


def get_street_network_db_test():
    manager = StreetNetworkBackendManager(sn_backends_getter_ok, -1)

    sn = manager.get_street_network_db("instance", "kraken")
    assert sn is not None
    assert sn.timeout == 2
    assert sn.url == 'kraken.url'

    sn = manager.get_street_network_db("instance", "asgard")
    assert sn is not None
    assert sn.timeout == 2
    assert sn.url == 'asgard.url'

    with pytest.raises(TechnicalError) as excinfo:
        sn = manager.get_street_network_db("instance", "plopi")
    assert (
        str(excinfo.value.data['message'])
        == 'impossible to find a streetnetwork module for instance instance with configuration plopi'
    )
    assert 'TechnicalError' == str(excinfo.typename)


class FakeInstance(Instance):
    street_network_car = "asgard"
    street_network_car_no_park = "asgard"
    street_network_walking = "asgard"
    street_network_bike = "geovelo"
    street_network_bss = "kraken"
    street_network_taxi = None
    street_network_ridesharing = None

    def __init__(self):
        super(FakeInstance, self).__init__(
            context=None,
            name="instance",
            zmq_socket=None,
            street_network_configurations=[],
            ridesharing_configurations=[],
            realtime_proxies_configuration=[],
            zmq_socket_type=None,
            autocomplete_type='kraken',
            instance_equipment_providers=[],
            streetnetwork_backend_manager=StreetNetworkBackendManager(),
        )


def get_all_street_networks_db_test():
    manager = StreetNetworkBackendManager(sn_backends_getter_ok, -1)
    instance = FakeInstance()

    all_sn = manager.get_all_street_networks_db(instance)
    assert len(all_sn) == 2

    # So that Asgard and kraken are always in the same order
    all_sn_sorted = sorted(all_sn, key=lambda sn: sn.url)

    assert all_sn_sorted[0].url == "asgard.url"
    assert sorted(all_sn_sorted[0].modes) == sorted(["walking", "car", "car_no_park"])

    assert all_sn_sorted[1].url == "kraken.url"
    assert all_sn_sorted[1].modes == ["bss"]
