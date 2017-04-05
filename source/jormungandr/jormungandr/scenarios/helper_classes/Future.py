import gevent
import gevent.pool
from jormungandr import app

class GeventFuture:
    _pool = gevent.pool.Pool(4)

    def __init__(self, fun, *args, **kwargs):
        self._future = self._pool.spawn(fun, *args, **kwargs)

    def get_future(self):
        return self._future

    def wait_and_get(self):
        return self._future.get()


def create_future(fun, *args, **kwargs):
    return GeventFuture(fun, *args, **kwargs)


def wait_all(futures):
    gevent.joinall([f.get_future() for f in futures])
