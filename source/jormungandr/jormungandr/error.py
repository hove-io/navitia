from jormungandr.response_pb2 import Response

def generate_error(str_, status=400):
    r = Response()
    r.error.message = str_
    r.status_code = status
    return r
