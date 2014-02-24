from utils import make_filename
import os
from jormungandr import app
from nose.tools import *
import json
from navitiacommon import request_pb2, response_pb2
from datetime import datetime
import logging


def mock_read_send_and_receive(*args, **kwargs):
    """
    Mock send_and_receive function for integration tests
    This just read the previously serialized file for the given request
    """
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


def check_and_get_as_json(tester, url):
    """Test url status code to 200 and if valid format response as json"""
    tester = app.test_client(tester)
    response = tester.get(url)

    assert response is not None
    eq_(response.status_code, 200)

    assert response.data
    json_response = json.loads(response.data)

    assert json_response
    return json_response


def get_not_null(dict, field):
    assert field in dict and dict[field]

    return dict[field]


def is_valid_date(str):
    assert str

    print str
    try:
        datetime.strptime(str, "%Y%m%dT%H%M%S")
    except ValueError:
        logging.error("string '%s' is no valid date" % str)
        return False
    return True

def read(request):
    file_name = make_filename(request)
    assert(os.path.exists(file_name))

    file_ = open(file_name, 'rb')
    to_return = file_.read()
    file_.close()
    return to_return
