from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr.customrediscache import CustomRedisCache
import pybreaker


class RedisCache(CustomRedisCache):
    """
    Override the base RedisCache to add a circuitbreaker, to prevent slowdown in case of a redis failure
    """

    def __init__(
        self,
        write_client='localhost',
        read_client='localhost',
        port=6379,
        password=None,
        db=0,
        default_timeout=300,
        key_prefix=None,
        **kwargs
    ):
        fail_max = kwargs.pop('fail_max', 5)
        reset_timeout = kwargs.pop('reset_timeout', 60)
        CustomRedisCache.__init__(
            self, write_client, read_client, port, password, db, default_timeout, key_prefix, **kwargs
        )
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)

    def get(self, key):
        return self.breaker.call(super(RedisCache, self).get, key)

    def get_many(self, *keys):
        return self.breaker.call(super(RedisCache, self).get_many, *keys)

    def set(self, key, value, timeout=None):
        return self.breaker.call(super(RedisCache, self).set, key, value, timeout)

    def add(self, key, value, timeout=None):
        return self.breaker.call(super(RedisCache, self).add, key, value, timeout)

    def set_many(self, mapping, timeout=None):
        return self.breaker.call(super(RedisCache, self).set_many, mapping, timeout)

    def delete(self, key):
        return self.breaker.call(super(RedisCache, self).delete, key)

    def delete_many(self, *keys):
        return self.breaker.call(super(RedisCache, self).delete_many, *keys)

    def has(self, key):
        return self.breaker.call(super(RedisCache, self).has, key)

    def clear(self):
        return self.breaker.call(super(RedisCache, self).clear)

    def inc(self, key, delta=1):
        return self.breaker.call(super(RedisCache, self).inc, key, delta)

    def dec(self, key, delta=1):
        return self.breaker.call(super(RedisCache, self).dec, key, delta)

    def status(self):
        return {
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            }
        }


# Taken from flask caching without any change except the usage of our own RedisCache class
def redis(app, config, args, kwargs):
    try:
        from redis import from_url as redis_from_url
    except ImportError:
        raise RuntimeError('no redis module found')

    kwargs.update(
        dict(
            write_client=config.get('CACHE_REDIS_PRIMARY', config.get('CACHE_REDIS_HOST', 'localhost')),
            read_client=config.get('CACHE_REDIS_READER', config.get('CACHE_REDIS_HOST', 'localhost')),
            port=config.get('CACHE_REDIS_PORT', 6379),
        )
    )
    password = config.get('CACHE_REDIS_PASSWORD')
    if password:
        kwargs['password'] = password

    key_prefix = config.get('CACHE_KEY_PREFIX')
    if key_prefix:
        kwargs['key_prefix'] = key_prefix

    db_number = config.get('CACHE_REDIS_DB')
    if db_number:
        kwargs['db'] = db_number

    redis_primary_url = config.get('CACHE_REDIS_PRIMARY_URL', config.get('CACHE_REDIS_URL'))
    if redis_primary_url:
        kwargs['write_client'] = redis_from_url(redis_primary_url, db=kwargs.pop('db', None))
    redis_reader_url = config.get('CACHE_REDIS_READER_URL')
    if redis_reader_url:
        kwargs['read_client'] = redis_from_url(redis_reader_url, db=kwargs.pop('db', None))

    return RedisCache(*args, **kwargs)
