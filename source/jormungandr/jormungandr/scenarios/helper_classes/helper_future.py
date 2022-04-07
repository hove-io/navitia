# Copyright (c) 2001-2017, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
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
from __future__ import absolute_import
import gevent
import gevent.pool
from jormungandr import app
from contextlib import contextmanager

# Using abc.ABCMeta in a way it is compatible both with Python 2.7 and Python 3.x
# http://stackoverflow.com/a/38668373/1614576
import abc

ABC = abc.ABCMeta(str("ABC"), (object,), {})
"""
This file encapsulates the implementation of future, one can easily change the implementation of future (ex.
use concurrent.futures ) by
1. creating a new future class of future and implementing methods 'get_future' and 'wait_and_get'
2. creating a new PoolManager and implementing __enter__ and __exit__ which allows all created future to be run
   (the way to do cleaning depends on the library of future to use)
"""


class _AbstractFuture(ABC):  # type: ignore
    @abc.abstractmethod
    def get_future(self):
        pass

    @abc.abstractmethod
    def wait_and_get(self):
        pass


class _AbstractPoolManager(ABC):  # type: ignore
    @abc.abstractmethod
    def create_future(self, fun, *args, **kwargs):
        pass

    @abc.abstractmethod
    def clean_futures(self):
        pass


class _GeventFuture(_AbstractFuture):
    def __init__(self, pool, fun, *args, **kwargs):
        self._future = pool.spawn(fun, *args, **kwargs)

    def get_future(self):
        return self._future

    def wait_and_get(self):
        return self._future.get()


class _GeventPoolManager(_AbstractPoolManager):
    def __init__(self, greenlet_pool_size=None):
        if greenlet_pool_size:
            self._pool = gevent.pool.Pool(greenlet_pool_size)
        else:
            self._pool = gevent.pool.Pool(8)  # Set a pool size default value if it is not specified
        self.is_within_context = False

    def create_future(self, fun, *args, **kwargs):
        assert (
            self.is_within_context
        ), "You are trying to create a Greenlet outside of it's context. Your FutureManager is already out of scope"
        return _GeventFuture(self._pool, fun, *args, **kwargs)

    def clean_futures(self):
        """
        All spawned futures must be started(if they're not yet started) when leaving the scope.

        We do this to prevent the programme from being blocked in case where some un-started futures may hold threading
        locks. If we leave the scope without starting these futures, they may hold locks forever.
        """
        self._pool.join()
        self.is_within_context = False


@contextmanager
def FutureManager(greenlet_pool_size=None):
    m = _GeventPoolManager(greenlet_pool_size)
    m.is_within_context = True
    try:
        yield m
    finally:
        m.clean_futures()
