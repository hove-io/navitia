from response_pb2 import Response

def generate_error(str, status=400):
    r = Response()
    r.error.comment = str
    r.status_code = status
    return r
