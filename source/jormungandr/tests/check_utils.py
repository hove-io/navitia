from jormungandr import app
from nose.tools import *
import json
from navitiacommon import request_pb2, response_pb2
from datetime import datetime
import logging

"""
some small functions to check the service responses
"""

def check_and_get_as_dict(tester, url):
    """Test url status code to 200 and if valid format response as json"""
    tester = app.test_client(tester)
    response = tester.get(url)

    assert response is not None
    eq_(response.status_code, 200)

    assert response.data
    json_response = json.loads(response.data)

    logging.debug("loaded response : " + str(json_response))

    assert json_response
    return json_response


def get_not_null(dict, field):
    assert field in dict and dict[field]

    return dict[field]


def is_valid_date(str):
    assert str

    try:
        datetime.strptime(str, "%Y%m%dT%H%M%S")
    except ValueError:
        logging.error("string '%s' is no valid date" % str)
        return False
    return True


def is_valid_bool(str):
    assert str
    if type(str) is bool:
        return True

    #else check as string
    lower = str.lower()
    return lower == "true" or lower == "false"


def get_links_dict(response):
    raw_links = get_not_null(response, "links")

    #create a dict with the 'rel' field as key
    links = {get_not_null(link, "rel"): link for link in raw_links}

    return links


def check_valid_calendar(cal):
    get_not_null(cal, "id")
    get_not_null(cal, "name")
    pattern = get_not_null(cal, "week_pattern")
    is_valid_bool(get_not_null(pattern, "monday"))  # check one field in pattern

    active_periods = get_not_null(cal, "active_periods")
    assert len(active_periods) > 0

    beg = get_not_null(active_periods[0], "begin")
    assert is_valid_date(beg)

    end = get_not_null(active_periods[0], "end")
    assert is_valid_date(end)

    #check links
