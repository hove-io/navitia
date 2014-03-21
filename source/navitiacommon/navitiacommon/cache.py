# encoding: utf-8

import cPickle
import logging

class Cache(object):

    def __init__(self, host='localhost', port=6379, db=0,
                 password=None, disabled=False, separator="."):

        self._disabled = disabled
        self.separator = separator
        self.logger = logging.getLogger(__name__)
        if not disabled:
            self.logger.info('connection to redis')
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
        cached = self._redis.get(full_key)
        if cached:
            return cPickle.loads(cached)
        else:
            return None

    def set(self, prefix, key, obj, ttl=None):
        full_key = self._build_key(key, prefix)
        if self._disabled:
            return
        self.logger.debug('set %s to redis with %s ttl', full_key, ttl)
        self._redis.set(full_key, cPickle.dumps(obj))
        if ttl:
            self._redis.expire(full_key, ttl)


cache = Cache(disabled=True)

def get_cache():
    return cache

def init_cache(host='localhost', port=6379, db=0, password=None):
    """
    initiate the global cache object, by default it is disabled
    """
    global cache
    logging.debug('avant: %s', cache)
    cache = Cache(host=host, port=port, db=db, password=password)
    logging.debug('aprés: %s', cache)
