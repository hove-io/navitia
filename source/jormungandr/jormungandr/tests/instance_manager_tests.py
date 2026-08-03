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
import gevent


class FakeInstance:
    def __init__(self, name, is_free=False, priority=0, geom=None, is_initialized=True):
        self.name = name
        self.is_free = is_free
        self.priority = priority
        self.geom = geom
        self.is_initialized = is_initialized
        self.init_call_count = 0

    def has_point(self, p):
        return bool(self.geom and self.geom.contains(p))

    def init(self):
        # simulate kraken finishing its load: geom becomes available
        self.init_call_count += 1
        from shapely.geometry import box

        self.geom = box(0, 0, 10, 10)  # covers (4, 3)
        self.is_initialized = True
        return True


def init_kraken_repings_instance_with_empty_geom_test():
    """
    init_kraken_instances must re-ping an instance that is initialized
    but has geom == None (kraken was still loading at first ping).
    """
    im = InstanceManager(None)
    im.instances['paris'] = FakeInstance('paris', geom=None, is_initialized=True)
    im.init_kraken_instances()
    assert im.instances['paris'].init_call_count == 1
    assert im.instances['paris'].geom is not None


def all_keys_of_coord_refreshes_stale_instance_test():
    """
    _all_keys_of_coord_in_instances must trigger a fresh init() on an
    instance whose geom is None, then succeed once the geom is loaded.
    """
    im = InstanceManager(None)
    paris = FakeInstance('paris', geom=None, is_initialized=True)
    im.instances['paris'] = paris
    result = im._all_keys_of_coord_in_instances([paris], lon=4, lat=3)
    assert paris.init_call_count == 1
    assert result == [paris]


def all_keys_of_coord_raises_when_no_instance_has_geom_test():
    """
    If no candidate has (or can load) a geom containing the point,
    RegionNotFound is still raised.
    """
    from jormungandr.exceptions import RegionNotFound
    from pytest import raises

    im = InstanceManager(None)
    far = FakeInstance('paris', geom=None, is_initialized=True)
    # override init so geom stays empty (kraken has no data)
    far.init = lambda: None
    far.geom = None
    im.instances['paris'] = far
    with raises(RegionNotFound):
        im._all_keys_of_coord_in_instances([far], lon=200, lat=200)


def thread_ping_keeps_running_until_stopped_test(mocker):
    """
    thread_ping must keep re-fetching metadata on a timer and only stop
    when thread_event is set (it no longer exits once all are initialized).
    """
    im = InstanceManager(None)
    im.instances['paris'] = FakeInstance('paris', geom=None, is_initialized=True)

    call_count = {'n': 0}

    def fake_init_kraken():
        call_count['n'] += 1
        if call_count['n'] >= 3:
            im.stop()  # sets thread_event, breaks the loop

    mocker.patch.object(im, 'init_kraken_instances', side_effect=fake_init_kraken)

    g = gevent.spawn(im.thread_ping, 0.01)
    g.join(timeout=5)

    assert call_count['n'] >= 3
    assert im.thread_event.is_set()


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
