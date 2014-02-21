from utils import make_filename
import os
from navitiacommon import request_pb2, response_pb2



def mock_read_send_and_receive(*args, **kwargs):
    request = None
    if "request" in kwargs:
        request = kwargs["request"]
    else:
        for arg in args:
            if type(arg) == request_pb2.Request:
                request = arg
    if request:
        pb = read(request)
        if pb:
            resp = response_pb2.Response()
            resp.ParseFromString(pb)
            return resp
    return None

def read(request):
    file_name = make_filename(request)
    assert(os.path.exists(file_name))

    file_ = open(file_name, 'rb')
    to_return = file_.read()
    file_.close()
    return to_return
