# coding=utf-8

# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.io).
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

from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr import pt_planners_manager
import pytest
from jormungandr.exceptions import ConfigException


def test_with_kraken_loki():
    configs = {
        "kraken": {
            "class": "jormungandr.pt_planners.kraken.Kraken",
            "args": {"zmq_socket": "ipc:///tmp/fr-idf_loki", "timeout": 4242},
        },
        "loki": {
            "class": "jormungandr.pt_planners.loki.Loki",
            "args": {"zmq_socket": "ipc:///tmp/fr-idf_loki", "timeout": 4242},
        },
    }
    manager = pt_planners_manager.PtPlannersManager(
        configs, 'dummy', 'zmq_socket_type', 'context', default_socket_path='zmq_socket', db_configs_getter=None
    )
    assert manager.get_pt_planner('kraken')
    assert manager.get_pt_planner('loki')


def test_with_only_kraken():
    configs = {
        "kraken": {
            "class": "jormungandr.pt_planners.kraken.Kraken",
            "args": {"zmq_socket": "ipc:///tmp/fr-idf_loki", "timeout": 4242},
        }
    }
    manager = pt_planners_manager.PtPlannersManager(
        configs, 'dummy', 'zmq_socket_type', 'context', default_socket_path='zmq_socket', db_configs_getter=None
    )

    assert manager.get_pt_planner('kraken')
    with pytest.raises(pt_planners_manager.NoRequestedPtPlanner):
        manager.get_pt_planner('loki')


def test_with_only_loki():
    configs = {
        "loki": {"class": "jormungandr.pt_planners.loki.Loki", "args": {"zmq_socket": "ipc:///tmp/fr-idf_loki"}}
    }
    manager = pt_planners_manager.PtPlannersManager(
        configs, 'dummy', 'zmq_socket_type', 'context', default_socket_path='zmq_socket', db_configs_getter=None
    )

    assert manager.get_pt_planner('kraken')
    assert manager.get_pt_planner('loki')


def test_with_dummy_config():
    configs = {"loki": {"class": "jormungandr.pt_planners.loki.Loki", "args": {"timeout": 10000}}}
    with pytest.raises(ConfigException):
        pt_planners_manager.PtPlannersManager(
            configs,
            'dummy',
            'zmq_socket_type',
            'context',
            default_socket_path='zmq_socket',
            db_configs_getter=None,
        )
