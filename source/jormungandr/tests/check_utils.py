from jormungandr import app
from nose.tools import *
import json
from navitiacommon import request_pb2, response_pb2
from datetime import datetime
import logging
import re

"""
some small functions to check the service responses
"""

def check_and_get_as_dict(tester, url):
    """Test url status code to 200 and if valid format response as json"""
    tester = app.test_client(tester)
    response = tester.get(url)

    assert response
    eq_(response.status_code, 200)

    assert response.data
    json_response = json.loads(response.data)

    logging.debug("loaded response : " + str(json_response))

    assert json_response
    return json_response


def get_not_null(dict, field):
    assert field in dict

    val = dict[field]
    if type(val) == bool:
        return val  # no check for booleans

    assert val
    return val

days_regexp = re.compile("^(0|1){366}$")

def is_valid_days(days):
   match = days_regexp.match(days)
   if not match:
        return None
   return match

def is_valid_datetime(str):
    """
    Check is the string is a valid date
    >>> is_valid_datetime("bob")
    False
    >>> is_valid_datetime("")
    Traceback (most recent call last):
    AssertionError
    >>> is_valid_datetime("20123101T215030")  # month is badly set
    False
    >>> is_valid_datetime("20120131T215030")
    True
    """
    assert str

    try:
        datetime.strptime(str, "%Y%m%dT%H%M%S")
    except ValueError:
        logging.error("string '{}' is no valid date".format(str))
        return False
    return True


def is_valid_date(str):
    """
    Check is the string is a valid date
    >>> is_valid_date("bob")
    False
    >>> is_valid_date("")
    Traceback (most recent call last):
    AssertionError
    >>> is_valid_date("20123101")  # month is badly set
    False
    >>> is_valid_date("20120131")
    True
    """
    assert str

    try:
        datetime.strptime(str, "%Y%m%d")
    except ValueError:
        logging.error("string '{}' is no valid date".format(str))
        return False
    return True


def is_valid_bool(str):
    if type(str) is bool:
        return True

    assert str
    #else check as string
    lower = str.lower()
    return lower == "true" or lower == "false"


def get_links_dict(response):
    raw_links = get_not_null(response, "links")

    #create a dict with the 'rel' field as key
    links = {get_not_null(link, "rel"): link for link in raw_links}

    return links


