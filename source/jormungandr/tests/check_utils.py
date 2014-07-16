# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from collections import deque
from nose.tools import *
import json
from navitiacommon import request_pb2, response_pb2
from datetime import datetime
import logging
import re

"""
Some small functions to check the service responses
"""


def check_url(tester, url, might_have_additional_args=False, **kwargs):
    """
    Test url status code to 200 and if valid format response as json
    if might_have_additional_args is set to True,
        we just don't want a 404 error (the url might be in error because of mandatory params not provided)
    else
        we don't want an error on the url
    """
    #tester = app.test_client(tester)
    response = tester.get(url, **kwargs)

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


def get_valid_datetime(str):
    """
    Check is the string is a valid date
    >>> get_valid_datetime("bob")
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("")
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("20123101T215030")  # month is badly set
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("20120131T215030")
    datetime.datetime(2012, 1, 31, 21, 50, 30)
    """
    assert str

    try:
        return datetime.strptime(str, "%Y%m%dT%H%M%S")
    except ValueError:
        logging.error("string '{}' is no valid date".format(str))
        assert False


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


def get_valid_int(str):
    assert str != ""
    if type(str) is int:
        return str

    try:
        return int(str)
    except ValueError:
        assert False


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


def walk_dict(tree, visitor):
    """
    depth first search on a dict.
    call the visit(elem) method on the visitor for each node

    >>> bob = {'tutu': 1,
    ... 'tata': [1, 2],
    ... 'toto': {'bob':12, 'bobette': 13, 'nested_bob': {'bob': 3}},
    ... 'tete': ('tuple1', ['ltuple1', 'ltuple2']),
    ... 'titi': [{'a':1}, {'b':1}]}

    >>> def my_visitor(name, val):
    ...     print "{}={}".format(name, val)

    >>> walk_dict(bob, my_visitor)
    titi={'b': 1}
    b=1
    titi={'a': 1}
    a=1
    tete=ltuple2
    tete=ltuple1
    tete=tuple1
    tutu=1
    toto={'bobette': 13, 'bob': 12, 'nested_bob': {'bob': 3}}
    nested_bob={'bob': 3}
    bob=3
    bob=12
    bobette=13
    tata=2
    tata=1
    """
    queue = deque()

    def add_elt(name, elt, first=False):
        if isinstance(elt, (list, tuple)):
            for val in elt:
                queue.append((name, val))
        elif hasattr(elt, 'iteritems'):
            for k, v in elt.iteritems():
                queue.append((k, v))
        elif first:  # for the first elt, we add it even if it is no collection
            queue.append((name, elt))

    add_elt("main", tree, first=True)
    while queue:
        elem = queue.pop()
        #we don't want to visit the list, we'll visit each node separatly
        if not isinstance(elem[1], (list, tuple)):
            visitor(elem[0], elem[1])
        #for list and tuple, the name is the parent's name
        add_elt(elem[0], elem[1])


def check_internal_links(response, tester):
    """
    We want to check that all 'internal' link are correctly linked to an element in the response

    for that we first collect all internal link
    then iterate on all node and remove a link if we find a node with
     * a name equals to link.'rel'
     * an id equals to link.'id'

     At the end the internal link list must be empty
    """
    internal_links_id = set()
    internal_link_types = set()  # set with the types we look for

    def add_link_visitor(name, val):
        if name == 'links':
            if 'internal' in val and bool(val['internal']):
                internal_links_id.add(val['id'])
                internal_link_types.add(val['rel'])

    walk_dict(response, add_link_visitor)

    logging.info('links: {}'.format(len(internal_links_id)))

    for l in internal_links_id:
        logging.info('--> {}'.format(l))
    for l in internal_link_types:
        logging.info('type --> {}'.format(l))

    def check_node(name, val):

        if name in internal_link_types:
            logging.info("found a good node {}".format(name))

            if 'id' in val and val['id'] in internal_links_id:
                #found one ref, we can remove the link
                internal_links_id.remove(val['id'])

    walk_dict(response, check_node)

    assert not internal_links_id, "cannot find correct ref for internal links : {}".\
        format([lid for lid in internal_links_id])

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


def query_from_str(str):
    """
    for convenience, convert a url to a dict

    >>> query_from_str("toto/tata?bob=toto&bobette=tata&bobinos=tutu")
    {'bobette': 'tata', 'bobinos': 'tutu', 'bob': 'toto'}
    """
    query = {}
    last_elt = str.split("?")[-1]
    for s in last_elt.split("&"):
        k, v = s.split("=")
        query[k] = v

    return query


def is_valid_journey_response(response, tester, query_str):
    query_dict = query_from_str(query_str)
    journeys = get_not_null(response, "journeys")

    all_sections = unique_dict('id')
    assert len(journeys) > 0, "we must at least have one journey"
    for j in journeys:
        is_valid_journey(j, tester, query_dict)

        for s in j['sections']:
            all_sections[s['id']] = s

    # check the fare section
    # the fares must be structurally valid and all link to sections must be ok
    all_tickets = unique_dict('id')
    fares = get_not_null(response, "tickets")
    for f in fares:
        is_valid_ticket(f, tester)
        all_tickets[f['id']] = f

    check_internal_links(response, tester)


    #TODO check journey links (prev/next)


def is_valid_journey(journey, tester, query):
    arrival = get_valid_datetime(journey['arrival_date_time'])
    departure = get_valid_datetime(journey['departure_date_time'])
    request = get_valid_datetime(journey['requested_date_time'])

    assert arrival >= departure

    if 'datetime_represents' not in query or query['datetime_represents'] == "departure":
        #for 'departure after' query, the departure must be... after \o/
        assert departure >= request
    else:
        assert arrival <= request

    #we want to test that all departure match de previous section arrival
    last_arrival = departure
    for s in journey['sections']:
        is_valid_section(s, query)
        section_departure = get_valid_datetime(s['departure_date_time'])
        assert (section_departure - last_arrival).seconds <= 1  # there cannot be more than one second between the 2

        last_arrival = get_valid_datetime(s['arrival_date_time'])

    assert get_valid_datetime(journey['sections'][-1]['arrival_date_time']) == last_arrival


def is_valid_section(section, query):
    arrival = get_valid_datetime(section['arrival_date_time'])
    departure = get_valid_datetime(section['departure_date_time'])

    assert (arrival - departure).seconds == section['duration']

    assert section['type']  # type cannot be empty

    #for street network section, we must have a valid path
    if section['type'] == 'street_network':
        assert section['mode']  # mode cannot be empty for street network
        total_duration = 0
        for p in section['path']:
            assert get_valid_int(p['length']) >= 0
            assert -180 <= get_valid_int(p['direction']) <= 180  # direction is an angle
            #No constraint on name, it can be empty
            dur = get_valid_int(p['duration'])
            assert dur >= 0
            total_duration += dur

        assert total_duration == section['duration']

    #TODO check geojson
    #TODO check stop_date_times
    #TODO check from/to


def is_valid_ticket(ticket, tester):
    found = get_not_null(ticket, 'found')
    assert is_valid_bool(found)

    get_not_null(ticket, 'id')
    get_not_null(ticket, 'name')
    cost = get_not_null(ticket, 'cost')
    if found:
        #for found ticket, we must have a non empty currency
        get_not_null(cost, 'currency')

    assert is_valid_float(get_not_null(cost, 'value'))

    check_links(ticket, tester)


#default journey query used in varius test
journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
    .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20120614T080000")
