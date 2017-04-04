import Future


class PlaceByUri:
    def __init__(self, instance, requested_place):
        self._instance = instance
        self._requested_place = requested_place
        self._value = None
        self.async_request()

    def _do_request(self):
        return self._instance.georef.place(self._requested_place)

    def async_request(self):
        self._value = Future.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get() if self._value else None
