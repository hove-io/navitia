# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
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

from __future__ import absolute_import, print_function, unicode_literals, division

from jormungandr import InstanceManager
from pytest import fixture
from pytest_mock import mocker

from jormungandr import app
from jormungandr.instance_manager import choose_best_instance


class FakeInstance:
    def __init__(self, name, is_free=False, priority=0):
        self.name = name
        self.is_free = is_free
        self.priority = priority


@fixture
def manager():
    instance_manager = InstanceManager(None)
    instance_manager.instances['paris'] = FakeInstance('paris')
    instance_manager.instances['pdl'] = FakeInstance('pdl')
    return instance_manager


def get_instances_test(manager, mocker):
    mock = mocker.patch.object(
        manager,
        'get_all_available_instances_names',
        return_value=['paris', 'pdl'],
    )
    with app.test_request_context('/'):
        instances = manager.get_instances()
        assert len(instances) == 2
        assert {'paris', 'pdl'} == {i.name for i in instances}

        instances = manager.get_instances('paris')
        assert len(instances) == 1
        assert 'paris' == instances[0].name


def get_instances_by_coord_test(manager, mocker):
    mock = mocker.patch.object(
        manager, '_all_keys_of_coord_in_instances', return_value=[manager.instances['paris']]
    )
    mock = mocker.patch.object(
        manager,
        'get_all_available_instances_names',
        return_value=['paris', 'pdl'],
    )
    with app.test_request_context('/'):
        instances = manager.get_instances(lon=4, lat=3)
        assert len(instances) == 1
        assert 'paris' == instances[0].name
        assert mock.called


def get_instances_by_object_id_test(manager, mocker):
    mock = mocker.patch.object(manager, '_all_keys_of_id_in_instances', return_value=[manager.instances['pdl']])
    mock = mocker.patch.object(
        manager,
        'get_all_available_instances_names',
        return_value=['paris', 'pdl'],
    )
    with app.test_request_context('/'):
        instances = manager.get_instances(object_id='sa:pdl')
        assert len(instances) == 1
        assert 'pdl' == instances[0].name
        assert mock.called


def choose_best_instance_test():
    """
    Test to choose the best instance according to comparator : priority > is_free=False > is_free=True
    """
    instances_list = [
        FakeInstance('fr-nw', is_free=True, priority=0),
        FakeInstance('fr-nw-c', is_free=True, priority=0),
        FakeInstance('fr-auv', is_free=True, priority=0),
    ]
    assert choose_best_instance(instances_list).name == 'fr-auv'

    instances_list[1].is_free = False
    assert choose_best_instance(instances_list).name == 'fr-nw-c'

    instances_list.append(FakeInstance('fr-bre', is_free=True, priority=1000))
    assert choose_best_instance(instances_list).name == 'fr-bre'
