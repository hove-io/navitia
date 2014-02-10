# encoding: utf-8

import cPickle


class Cache(object):

    def __init__(self, host='localhost', port=6379, db=0,
                 password=None, disabled=False, separator="."):
        self._disabled = disabled
        self.separator = separator
        if not disabled:
            from redis import Redis
            self._redis = Redis(host=host, port=port, db=db, password=password)
        else:
            self._redis = None

    def get(self, prefix, key):
        if self._disabled:
            return None

        cached = self._redis.get("%s%s%s" % (prefix, self.separator, key))
        if cached:
            return cPickle.loads(cached)
        else:
            return None

    def set(self, prefix, key, obj, ttl=None):
        complete_key = "%s%s%s" % (prefix, self.separator, key)
        if self._disabled:
            return
        self._redis.set(complete_key, cPickle.dumps(obj))
        if ttl:
            self._redis.expire(complete_key, ttl)


cache = Cache(disabled=True)

def init_cache(host='localhost', port=6379, db=0, password=None):
    """
    initiate the global cache object, by default it is disabled
    """
    global cache
    cache = Cache(host=host, port=port, db=db, password=password)
