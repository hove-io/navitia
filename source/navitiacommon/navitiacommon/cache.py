# encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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


import cPickle
import logging

try:
    from redis.exceptions import ConnectionError
except ImportError:
    pass

class Cache(object):

    def __init__(self, host='localhost', port=6379, db=0, password=None,
                 disabled=False, separator='.', default_ttl=None):

        self._disabled = disabled
        self.separator = separator
        self._default_ttl = default_ttl
        self.logger = logging.getLogger(__name__)
        if not disabled:
            self.logger.info('connection to redis://{host}:{port}/{db}'.format(
                        host=host, port=port, db=db))
            from redis import Redis
            self._redis = Redis(host=host, port=port, db=db, password=password)
        else:
            self._redis = None

    def _build_key(self, key, prefix):
        return '{0}{1}{2}'.format(prefix, self.separator, key)


    def get(self, prefix, key):
        if self._disabled:
            return None
        full_key = self._build_key(key, prefix)
        self.logger.debug('get %s from redis', full_key)
        try:
            cached = self._redis.get(full_key)
        except ConnectionError, e:
            self.logger.error('Redis is down, cache is disabled: %s', e)
            return None
        if cached:
            return cPickle.loads(cached)
        else:
            return None

    def set(self, prefix, key, obj, ttl=None):
        full_key = self._build_key(key, prefix)
        if self._disabled:
            return
        self.logger.debug('set %s to redis with %s ttl', full_key, ttl)
        try:
            self._redis.set(full_key, cPickle.dumps(obj))
        except ConnectionError, e:
            self.logger.error('Redis is down, cache is disabled: %s', e)
            return
        ttl = (ttl or self._default_ttl)
        if ttl:
            self._redis.expire(full_key, ttl)


cache = Cache(disabled=True)

def get_cache():
    return cache

def init_cache(host='localhost', port=6379, db=0, password=None,
               default_ttl=None):
    """
    initiate the global cache object, by default it is disabled
    """
    global cache
    cache = Cache(host=host, port=port, db=db, password=password,
                  default_ttl=default_ttl)
