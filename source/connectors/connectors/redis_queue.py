#encoding: utf-8
from redis import Redis

class RedisQueue():

    def __init__(self, host = "localhost", port = 6379, db = 0, password = None, prefix='REDIS'):
        try:
            self._redis = Redis(host=host,port=port,db=db,password=password)
            self._prefix = prefix
            self._separator = "|"
        except:
            raise ValueError("Redis : Connecting redis failed" )

    def set_prefix(self, prefix):
        self._prefix = prefix

    def set(self, key, value):
        if self._redis != None:
            return self._redis.set(self._prefix + self._separator + key, value)
        else:
            return False

    def get(self,key):
        if self._redis != None:
            return self._redis.get(self._prefix + self._separator + key)
        else:
            return None

