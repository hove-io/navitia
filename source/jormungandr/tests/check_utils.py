from nose.tools import *
import json
from navitiacommon import request_pb2, response_pb2
from datetime import datetime
import logging
import re

"""
Some small functions to check the service responses
"""


def check_url(tester, url, might_have_additional_args=False):
    """
    Test url status code to 200 and if valid format response as json
    if might_have_additional_args is set to True,
        we just don't want a 404 error (the url might be in error because of mandatory params not provided)
    else
        we don't want an error on the url
    """
    #tester = app.test_client(tester)
    response = tester.get(url)

    assert response, "response for url {} is null".format(url)
    if might_have_additional_args:
        assert response.status_code != 404, "unreachable url {}"\
            .format(json.dumps(json.loads(response.data), indent=2))
    else:
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


def is_valid_float(str):
    if type(str) is float:
        return True

    try:
        float(str)
    except ValueError:
        return False
    return True


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
       * 'internal' --> optional but must be a boolean
       * 'href' --> valid url if not templated, empty if internal
       * 'rel' --> not empty
       * 'title' --> optional
       * 'templated' --> optional but must be a boolean
       * 'type' --> not empty if internal
    """
    links = get_links_dict(object)

    for link_name, link in links.iteritems():
        def get_bool(name):
            """ give boolean if in dict, else False"""
            if name in link:
                assert is_valid_bool(link[name])
                if bool(link[name]):
                    return True
            return False
        internal = get_bool('internal')
        templated = get_bool('templated')

        if not internal:
            assert 'href' in link, "no href in link"

        if not templated and not internal:
            #we check that the url is valid
            assert check_url(tester, link['href'], might_have_additional_args=True), "href's link must be a valid url"

        assert 'rel' in link
        assert link['rel']

        if internal:
            assert 'type' in link
            assert link['type']

    return links


class unique_dict(dict):
    """
    We often have to check that a set of values are uniq, this container is there to do the job

    >>> d = unique_dict('id')
    >>> d['bob'] = 1
    >>> d['bobette'] = 1
    >>> d['bob'] = 2
    Traceback (most recent call last):
        ...
    AssertionError: the id if must be unique, but 'bob' is not
    """

    def __init__(self, key_name):
        self.key_name = key_name

    def __setitem__(self, key, value):
        assert not key in self, \
            "the {} if must be unique, but '{}' is not".format(self.key_name, key)
        dict.__setitem__(self, key, value)