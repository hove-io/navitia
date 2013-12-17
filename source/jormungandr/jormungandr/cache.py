# encoding: utf-8

from redis import Redis
import cPickle


class Cache(object):

    def __init__(self, host, port, db, password, disabled=False):
        self._disabled = disabled
        if not disabled:
            self._redis = Redis(host=host, port=port, db=db, password=password)
        else:
            self._redis = None

    def get(self, key):
        if self._disabled:
            return None

        cached = self._redis.get(key)
        if cached:
            return cPickle.loads(cached)
        else:
            return None

    def set(self, key, obj, ttl=None):
        if self._disabled:
            return

        self._redis.set(key, cPickle.dumps(obj))
        if ttl:
            self._redis.expire(key, ttl)
