#encoding: utf-8

from redis import Redis
import cPickle

class Cache(object):
    def __init__(self, host, port, db, password):
        self._redis = Redis(host=host, port=port, db=db, password=password)

    def get(self, key):
        cached = self._redis.get(key)
        if cached:
            return cPickle.loads(cached)
        else:
            return None

    def set(self, key, obj, ttl=None):
        self._redis.set(key, cPickle.dumps(obj))
        if ttl:
            self._redis.expire(key, ttl)

