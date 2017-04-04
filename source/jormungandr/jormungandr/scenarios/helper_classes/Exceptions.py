import copy


class PtException(Exception):
    def __init__(self, pt_journey_with_error):
        super(PtException, self).__init__()
        self._response = copy.deepcopy(pt_journey_with_error)

    def get(self):
        return self._response


def _make_error_response(message, error_id):
    from navitiacommon import response_pb2
    r = response_pb2.Response()
    r.error.message = message
    r.error.id = error_id
    return r

class EntryPointException(Exception):
    def __init__(self, error_id, error_message):
        super(EntryPointException, self).__init__()
        self._response = _make_error_response(error_id, error_message)

    def get(self):
        return self._response
