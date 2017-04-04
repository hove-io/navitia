import gevent
import gevent.pool


class GeventFuture:
    _pool = gevent.pool.Pool(8)

    def __init__(self, fun, *args, **kwargs):
        self._future = self._pool.spawn(fun, *args, **kwargs)

    def get_future(self):
        return self._future

    def wait_and_get(self):
        return self._future.get()

    def wait_and_set(self, value):
        self._future.join()
        self._future.value = value


def create_future(fun, *args, **kwargs):
    return GeventFuture(fun, *args, **kwargs)


def wait_all(futures):
    gevent.joinall([f.get_future() for f in futures])
