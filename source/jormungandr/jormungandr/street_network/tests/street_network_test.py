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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io
import pytest
from jormungandr.street_network.street_network import StreetNetwork
from jormungandr.street_network.kraken import Kraken
from jormungandr.street_network.valhalla import Valhalla
from jormungandr.exceptions import ConfigException

KRAKEN_CLASS = 'jormungandr.street_network.kraken.Kraken'
VALHALLA_CLASS = 'jormungandr.street_network.valhalla.Valhalla'
ALL_MODES = ['walking', 'bike', 'bss', 'car']


def kraken_class_test():
    kraken_conf = [{
        'modes': ALL_MODES,
        'class': KRAKEN_CLASS
    }]
    services = StreetNetwork.get_street_network_services(None, kraken_conf)
    for mode in ALL_MODES :
        assert isinstance(services[mode], Kraken)


def valhalla_class_without_url_test():
    with pytest.raises(ValueError) as excinfo:
        valhalla_without_url = [{
            'modes': ALL_MODES,
            'class': VALHALLA_CLASS,
        }]
        StreetNetwork.get_street_network_services(None, valhalla_without_url)
    assert 'service_url is invalid, you give None' in str(excinfo.value)


def valhalla_class_wit_empty_url_test():
    with pytest.raises(ValueError) as excinfo:
        kraken_conf = [{
            'modes': ALL_MODES,
            'class': VALHALLA_CLASS,
            'args': {
                "service_url": ""
            }
        }]
        StreetNetwork.get_street_network_services(None, kraken_conf)
    assert 'service_url is invalid, you give ' in str(excinfo.value)


def valhalla_class_with_invalid_url_test():
    with pytest.raises(ValueError) as excinfo:
        kraken_conf = [{
            'modes': ALL_MODES,
            'class': VALHALLA_CLASS,
            'args': {
                "service_url": "bob"
            }
        }]
        StreetNetwork.get_street_network_services(None, kraken_conf)
    assert 'service_url is invalid, you give bob' in str(excinfo.value)


def valhalla_class_without_costing_options_test():
    kraken_conf = [{
        'modes': ALL_MODES,
        'class': VALHALLA_CLASS,
        'args': {
            "service_url": "http://localhost:8002"
        }
    }]
    services = StreetNetwork.get_street_network_services(None, kraken_conf)
    for mode in ALL_MODES:
        assert isinstance(services[mode], Valhalla)


def valhalla_class_with_empty_costing_options_test():
    kraken_conf = [{
        'modes': ALL_MODES,
        'class': VALHALLA_CLASS,
        'args': {
            "service_url": "http://localhost:8002",
            "costing_options": {}
        }
    }]
    services = StreetNetwork.get_street_network_services(None, kraken_conf)
    for mode in ALL_MODES:
        assert isinstance(services[mode], Valhalla)


def valhalla_class_with_url_valid_test():
    kraken_conf = [{
        'modes': ALL_MODES,
        'class': VALHALLA_CLASS,
        'args': {
            "service_url": "http://localhost:8002",
            "costing_options": {
                "pedestrian": {
                    "walking_speed": 50.1
                }
            }
        }
    }]
    services = StreetNetwork.get_street_network_services(None, kraken_conf)
    for mode in ALL_MODES:
        assert isinstance(services[mode], Valhalla)

def street_network_without_class_test():
    with pytest.raises(KeyError) as excinfo:
        kraken_conf = [{
            'modes': ['walking'],
            'args': {
                "service_url": "http://localhost:8002",
            "costing_options": {
                "pedestrian": {
                    "walking_speed": 50.1
                }
            }
            }
        }]
        StreetNetwork.get_street_network_services(None, kraken_conf)
    assert 'impossible to build a StreetNetwork, missing mandatory field in configuration: class' in str(excinfo.value)


def valhalla_class_with_class_invalid_test():
    with pytest.raises(ConfigException) as excinfo:
        kraken_conf = [{
            'class': 'jormungandr',
            'modes': ['walking'],
            'args': {
                "service_url": "http://localhost:8002",
                "costing_options": {
                    "pedestrian": {
                        "walking_speed": 50.1
                    }
                }
            }
        }]
        StreetNetwork.get_street_network_services(None, kraken_conf)
    assert 'impossible to build StreetNetwork, wrongly formated class: jormungandr' in str(excinfo.value)


def valhalla_class_with_class_not_exist_test():
    with pytest.raises(ConfigException) as excinfo:
        kraken_conf = [{
            'class': 'jormungandr.street_network.valhalla.bob',
            'modes': ['walking'],
            'args': {
                "service_url": "http://localhost:8002",
                "costing_options": {
                    "pedestrian": {
                        "walking_speed": 50.1
                    }
                }
            }
        }]
        StreetNetwork.get_street_network_services(None, kraken_conf)
    assert 'impossible to build StreetNetwork, wrongly formated class: jormungandr.street_network.valhalla.bob' \
           in str(excinfo.value)


def streetnetwork_service_without_modes_test():
    with pytest.raises(KeyError) as excinfo:
        kraken_conf = [{
            'class': 'jormungandr.street_network.valhalla.bob',
            'args': {
                "service_url": "http://localhost:8002",
                "costing_options": {
                    "pedestrian": {
                        "walking_speed": 50.1
                    }
                }
            }
        }]
        StreetNetwork.get_street_network_services(None, kraken_conf)
    assert 'impossible to build a StreetNetwork, missing mandatory field in configuration: modes' \
           in str(excinfo.value)
