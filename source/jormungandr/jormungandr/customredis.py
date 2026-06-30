from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr.customrediscache import CustomRedisCache
import logging
import pybreaker

logger = logging.getLogger(__name__)


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
        try:
            return self.breaker.call(super(RedisCache, self).get, key)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on get for key '%s': %s", key, e)
            return None
        except Exception as e:
            logger.error("Redis cache error on get for key '%s': %s", key, e)
            return None

    def get_many(self, *keys):
        try:
            return self.breaker.call(super(RedisCache, self).get_many, *keys)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on get_many for keys '%s': %s", keys, e)
            return [None] * len(keys)
        except Exception as e:
            logger.error("Redis cache error on get_many for keys '%s': %s", keys, e)
            return [None] * len(keys)

    def set(self, key, value, timeout=None):
        try:
            return self.breaker.call(super(RedisCache, self).set, key, value, timeout)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on set for key '%s': %s", key, e)
            return False
        except Exception as e:
            logger.error("Redis cache error on set for key '%s': %s", key, e)
            return False

    def add(self, key, value, timeout=None):
        try:
            return self.breaker.call(super(RedisCache, self).add, key, value, timeout)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on add for key '%s': %s", key, e)
            return False
        except Exception as e:
            logger.error("Redis cache error on add for key '%s': %s", key, e)
            return False

    def set_many(self, mapping, timeout=None):
        try:
            return self.breaker.call(super(RedisCache, self).set_many, mapping, timeout)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on set_many: %s", e)
            return False
        except Exception as e:
            logger.error("Redis cache error on set_many: %s", e)
            return False

    def delete(self, key):
        try:
            return self.breaker.call(super(RedisCache, self).delete, key)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on delete for key '%s': %s", key, e)
            return False
        except Exception as e:
            logger.error("Redis cache error on delete for key '%s': %s", key, e)
            return False

    def delete_many(self, *keys):
        try:
            return self.breaker.call(super(RedisCache, self).delete_many, *keys)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on delete_many for keys '%s': %s", keys, e)
            return False
        except Exception as e:
            logger.error("Redis cache error on delete_many for keys '%s': %s", keys, e)
            return False

    def has(self, key):
        try:
            return self.breaker.call(super(RedisCache, self).has, key)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on has for key '%s': %s", key, e)
            return False
        except Exception as e:
            logger.error("Redis cache error on has for key '%s': %s", key, e)
            return False

    def clear(self):
        try:
            return self.breaker.call(super(RedisCache, self).clear)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on clear: %s", e)
            return False
        except Exception as e:
            logger.error("Redis cache error on clear: %s", e)
            return False

    def inc(self, key, delta=1):
        try:
            return self.breaker.call(super(RedisCache, self).inc, key, delta)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on inc for key '%s': %s", key, e)
            return None
        except Exception as e:
            logger.error("Redis cache error on inc for key '%s': %s", key, e)
            return None

    def dec(self, key, delta=1):
        try:
            return self.breaker.call(super(RedisCache, self).dec, key, delta)
        except pybreaker.CircuitBreakerError as e:
            logger.error("Redis cache error: circuit breaker open on dec for key '%s': %s", key, e)
            return None
        except Exception as e:
            logger.error("Redis cache error on dec for key '%s': %s", key, e)
            return None

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
