from nose.tools import *
import json
from navitiacommon import request_pb2, response_pb2
from datetime import datetime
import logging
import re

"""
Some small functions to check the service responses
"""


def check_url(tester, url):
    """Test url status code to 200 and if valid format response as json"""
    #tester = app.test_client(tester)
    response = tester.get(url)

    assert response, "response for url {} is null".format(url)
    eq_(response.status_code, 200, "invalid return code, response : {}"
        .format(json.dumps(json.loads(response.data), indent=2)))
    return response


def get_not_null(dict, field):
    assert field in dict

    val = dict[field]
    if type(val) == bool:
        return val  # no check for booleans

    assert val, "value of field {} is null".format(field)
    return val


days_regexp = re.compile("^(0|1){366}$")


def is_valid_days(days):
    m = days_regexp.match(days)
    return m


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
    """
    get links as dict ordered by 'rel'
    """
    raw_links = get_not_null(response, "links")

    #create a dict with the 'rel' field as key
    links = {get_not_null(link, "rel"): link for link in raw_links}

    return links


def check_links(object, tester):
    """
    get the links as dict ordered by 'rel' and check:
     - all links must have the attributes:
       * 'href' --> valid url if not templated
       * 'rel' --> not empty
       * 'title' --> optional (? don't we have to ensure the title ?)
       * templated --> optional but must be a boolean
    """
    links = get_links_dict(object)

    for link_name, link in links.iteritems():
        assert 'href' in link, "no href in link"

        if 'templated' in link and not link['templated']:
            assert is_valid_bool(link['templated'])
            assert check_url(tester, link['href']), "href's link must be a valid url"

        assert 'rel' in link
        assert link['rel']

        #assert 'title' in link
        #assert link['title']

    return links
