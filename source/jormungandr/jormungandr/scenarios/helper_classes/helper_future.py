# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io
import gevent
import gevent.pool
from jormungandr import app

"""
This file encapsulates the implementation of future, one can easily change the implementation of future (ex.
use concurrent.futures ) by creating a new future class of future and implementing methods 'get_future' and
'wait_and_get' and use that new class in 'create_future'
"""

class GeventFuture:
    _pool = gevent.pool.Pool(app.config.get('GREENLET_POOL_SIZE', 4))

    def __init__(self, fun, *args, **kwargs):
        self._future = self._pool.spawn(fun, *args, **kwargs)

    def get_future(self):
        return self._future

    def wait_and_get(self):
        return self._future.get()


def create_future(fun, *args, **kwargs):
    return GeventFuture(fun, *args, **kwargs)
