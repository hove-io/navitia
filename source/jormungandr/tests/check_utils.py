# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from collections import defaultdict, namedtuple
from functools import partial
from future.moves.itertools import zip_longest
import json
from jormungandr.scenarios.qualifier import compare_field, reverse_compare_field
from navitiacommon.parser_args_type import UnsignedInteger
from datetime import datetime
import logging
import re
from shapely.geometry import shape
import sys
from six.moves.urllib.parse import unquote
import six


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
    response = tester.get(url, **kwargs)

    assert response, "response for url {} is null".format(url)
    if might_have_additional_args:
        assert response.status_code != 404, "unreachable url {}".format(
            json.dumps(json.loads(response.data), indent=2)
        )
    else:
        assert response.status_code == 200, "invalid return code, response : {}".format(
            json.dumps(json.loads(response.data, encoding='utf-8'), indent=2)
        )
    return response


def get_not_null(dict, field):
    assert field in dict

    val = dict[field]
    if type(val) == bool:
        return val  # no check for booleans
    if type(val) == int:
        return val  # no check for integer

    assert val, "value of field {} is null".format(field)
    return val


days_regexp = re.compile("^(0|1){366}$")


def is_valid_days(days):
    m = days_regexp.match(days)
    return m is not None


version_number_regexp = re.compile("v[0-9]+\.[0-9]+\.[0-9]+[-.*]?")


def is_valid_navitia_version_number(str):
    """
    check that the version number is valid
    it must contains at least v{major}.{minor}.{hotfix}
    it can also contains the git sha1 at the end
    >>> is_valid_navitia_version_number("v1.12.126")
    True
    >>> is_valid_navitia_version_number("v1.3.1-73-g4c7524b")
    True
    >>> is_valid_navitia_version_number("1.12.126")
    Traceback (most recent call last):
    AssertionError
    >>> is_valid_navitia_version_number("v12.126-73-g4c7524b")
    Traceback (most recent call last):
    AssertionError
    """
    m = version_number_regexp.match(str)
    assert m
    return True


def get_valid_datetime(str, possible_errors=False):
    """
    Check is the string is a valid date and return it
    if possible_errors, the string might be equals to
    "not-a-date-time"

    >>> get_valid_datetime("bob")
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("")
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("20123101T215030")  # month is badly set
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("20123101T215030", possible_errors=True)
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("not-a-date-time", possible_errors=True)

    >>> get_valid_datetime("not-a-date-time")
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_datetime("20120131T215030")
    datetime.datetime(2012, 1, 31, 21, 50, 30)
    """
    assert str

    try:
        return datetime.strptime(str, "%Y%m%dT%H%M%S")
    except ValueError:
        if possible_errors:
            assert str == "not-a-date-time"
            return None
        logging.error("string '{}' is no valid datetime".format(str))
        assert False


def get_valid_time(str):
    """
    Check is the string is a valid time and return it
    >>> get_valid_time("bob")
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_time("")
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_time("20120131T215030")  # it's a datetime, not valid
    Traceback (most recent call last):
    AssertionError
    >>> get_valid_time("215030")  #time is HHMMSS
    datetime.datetime(1900, 1, 1, 21, 50, 30)
    >>> get_valid_time("501230")  # MMHHSS, not valid
    Traceback (most recent call last):
    AssertionError
    """
    assert str

    try:
        # AD:we use a datetime anyway because I don't know what to use instead
        return datetime.strptime(str, "%H%M%S")
    except ValueError:
        logging.error("string '{}' is no valid time".format(str))
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
    # else check as string
    lower = str.lower()
    return lower == "true" or lower == "false"


def get_valid_float(str):
    if type(str) is float:
        return str

    try:
        return float(str)
    except ValueError:
        assert "cannot convert {} to float".format(str)


def get_valid_int(str):
    assert str != ""
    if type(str) is int:
        return str

    try:
        return int(str)
    except ValueError:
        assert False


def get_valid_unsigned_int(str):
    assert str != ""
    try:
        return UnsignedInteger()(str)
    except ValueError:
        assert False


def is_valid_lat(str):
    lat = get_valid_float(str)

    assert -90.0 <= lat <= 90.0, "lat should be between -90 and 90"


def is_valid_lon(str):
    lon = get_valid_float(str)

    assert 180.0 >= lon >= -180.0, "lon should be between -180 and 180"


def is_valid_coord(coord):
    lat = get_not_null(coord, "lat")
    lon = get_not_null(coord, "lon")
    is_valid_lat(lat)
    is_valid_lon(lon)


def get_links_dict(response):
    """
    get links as dict ordered by 'rel' or 'type"
    """
    raw_links = get_not_null(response, "links")

    # create a dict with the 'rel' field as key
    links = {link.get('rel', link.get('type', None)): link for link in raw_links}

    return links


def check_links(object, tester, href_mandatory=True):
    """
    get the links as dict ordered by 'rel' and check:
     - all links must have the attributes:
       * 'internal' --> optional but must be a boolean
       * 'href' --> valid url if not templated, empty if internal
       * 'rel' --> not empty if internal
       * 'title' --> optional
       * 'templated' --> optional but must be a boolean
       * 'type' --> not empty
    """
    links = get_links_dict(object)

    for link_name, link in links.items():

        def get_bool(name):
            """ give boolean if in dict, else False"""
            if name in link:
                assert is_valid_bool(link[name])
                if bool(link[name]):
                    return True
            return False

        internal = get_bool('internal')
        templated = get_bool('templated')

        if href_mandatory:
            if not internal:
                assert 'href' in link, "no href in link"

            if not templated and not internal:
                # we check that the url is valid
                assert check_url(
                    tester, link['href'].replace('http://localhost', ''), might_have_additional_args=False
                ), "href's link must be a valid url"

        if internal:
            assert 'rel' in link
            assert link['rel']

        assert 'type' in link
        assert link['type']

    return links


def check_internal_links(response, tester):
    """
    We want to check that all 'internal' link are correctly linked to an element in the response

    for that we first collect all internal link
    then iterate on all node and remove a link if we find a node with
     * a name equals to link.'rel'
     * an id equals to link.'id'

     At the end the internal link list must be empty
    """
    from jormungandr import utils  # import late not to load it before updating the conf for the tests

    internal_links_id = set()
    internal_link_types = set()  # set with the types we look for

    def add_link_visitor(name, val):
        if val and name == 'links':
            if 'internal' in val and bool(val['internal']):
                internal_links_id.add(val['id'])
                internal_link_types.add(val['rel'])

    utils.walk_dict(response, add_link_visitor)

    def check_node(name, val):

        if name in internal_link_types:

            if 'id' in val and val['id'] in internal_links_id:
                # found one ref, we can remove the link
                internal_links_id.remove(val['id'])

    utils.walk_dict(response, check_node)

    assert not internal_links_id, "cannot find correct ref for internal links : {}".format(
        [lid for lid in internal_links_id]
    )


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
        assert not key in self, "the {} if must be unique, but '{}' is not".format(self.key_name, key)
        dict.__setitem__(self, key, value)


def query_from_str(str):
    """
    for convenience, convert a url to a dict

    Note: the query can be encoded, so the split it either on the encoded or the decoded value
    """
    query = {}
    last_elt = str.split('?' if '?' in str else '%3F')[-1]

    for s in last_elt.split('&' if '&' in last_elt else '%26'):
        k, v = s.split("=" if '=' in s else '%3D')
        k = unquote(k)
        v = unquote(v)

        if k in query:
            old_val = query[k]
            if isinstance(old_val, list):
                old_val.append(v)
            else:
                query[k] = [old_val, v]
        else:
            query[k] = v

    return query


def is_valid_feed_publisher(feed_publisher):
    get_not_null(feed_publisher, 'id')
    get_not_null(feed_publisher, 'name')
    get_not_null(feed_publisher, 'license')
    get_not_null(feed_publisher, 'url')


def is_valid_isochrone_response(response, tester, query_str):
    if isinstance(query_str, six.string_types):
        query_dict = query_from_str(query_str)
    else:
        query_dict = query_str

    journeys = get_not_null(response, "journeys")

    assert len(journeys) > 0, "we must at least have one journey"
    for j in journeys:
        is_valid_isochrone(j, tester, query_dict)

    check_internal_links(response, tester)

    # check other links
    # Don't work for now :'(
    # check_links(response, tester)

    # more checks on links, we want the prev/next/first/last,
    # to have forwarded all params, (and the time must be right)

    feed_publishers = response.get("feed_publishers", [])
    for feed_publisher in feed_publishers:
        is_valid_feed_publisher(feed_publisher)

    if query_dict.get('debug', False):
        assert 'debug' in response
    else:
        assert 'debug' not in response


def is_valid_isochrone(journey, tester, query):
    arrival = get_valid_datetime(journey['arrival_date_time'])
    departure = get_valid_datetime(journey['departure_date_time'])
    request = get_valid_datetime(journey['requested_date_time'])

    assert arrival >= departure

    if 'datetime_represents' not in query or query['datetime_represents'] == "departure":
        # for 'departure after' query, the departure must be... after \o/
        assert departure >= request
    else:
        assert arrival <= request

    journey_links = get_links_dict(journey)

    assert 'journeys' in journey_links

    additional_args = query_from_str(journey_links['journeys']['href'])
    for k, v in query.items():
        assert additional_args[k] == v


def is_valid_journey(journey, tester, query):
    arrival = get_valid_datetime(journey['arrival_date_time'])
    departure = get_valid_datetime(journey['departure_date_time'])
    request = get_valid_datetime(journey['requested_date_time'])
    if query.get('debug', 'false') != 'true':
        assert journey["type"]

    assert arrival >= departure
    # test if duration time is consistent with arrival and departure
    # as we sometimes loose a second in rounding section duration, tolerance is added
    assert (arrival - departure).seconds - journey['duration'] <= len(journey['sections']) - 1

    if 'datetime_represents' not in query or query['datetime_represents'] == "departure":
        # for 'departure after' query, the departure must be... after \o/
        assert departure >= request
    else:
        assert arrival <= request

    # we want to test that all departure match de previous section arrival
    last_arrival = departure

    previous_destination_uri = None
    if journey['sections']:
        previous_destination_uri = journey['sections'][0]['from']['id']
    for s in journey['sections']:
        is_valid_section(s, query)
        section_departure = get_valid_datetime(s['departure_date_time'])
        delta = section_departure - last_arrival
        assert delta.seconds <= 1  # there cannot be more than one second between the 2

        last_arrival = get_valid_datetime(s['arrival_date_time'])

        # test if geojson is valid
        g = s.get('geojson')
        g is None or shape(g)

        if s['type'] != 'waiting':
            assert s['from']['id'] == previous_destination_uri
            previous_destination_uri = s['to']['id']
    assert last_arrival == arrival
    assert get_valid_datetime(journey['sections'][-1]['arrival_date_time']) == last_arrival

    if query.get('debug', False):
        assert 'debug' in journey
    else:
        assert 'debug' not in journey


def is_valid_geojson_coord(coord):
    is_valid_lon(coord[0])
    is_valid_lat(coord[1])


def is_valid_multipolygon_geojson(geojson):
    assert geojson['type'] == 'MultiPolygon'
    for p in get_not_null(geojson, 'coordinates'):
        outer = p[0]
        for c in outer:
            is_valid_geojson_coord(c)
        for inner in p[1:]:
            for c in inner:
                is_valid_geojson_coord(c)


def is_valid_graphical_isochrone(isochrone, tester, query):

    for g in get_not_null(isochrone, 'isochrones'):
        get_valid_datetime(g['requested_date_time'])
        get_valid_datetime(g['min_date_time'])
        get_valid_datetime(g['max_date_time'])
        geojson = g['geojson']
        assert geojson
        is_valid_multipolygon_geojson(geojson)
        get_valid_unsigned_int(g['max_duration'])
        get_valid_unsigned_int(g['min_duration'])
        if 'from' in g:
            is_valid_place(g['from'])
        if 'to' in g:
            is_valid_place(g['to'])
        assert ('from' in g) ^ ('to' in g)

    for feed_publisher in get_not_null(isochrone, 'feed_publishers'):
        is_valid_feed_publisher(feed_publisher)


def is_valid_single_coord(coord, is_valid_type, name):
    min_type = get_not_null(coord, 'min_' + name)
    center_type = get_not_null(coord, 'center_' + name)
    max_type = get_not_null(coord, 'max_' + name)
    for t in [min_type, center_type, max_type]:
        is_valid_type(t)
    assert min_type <= center_type <= max_type


def is_valid_header(header):
    max_type = header[0]['cell_lat']['min_lat']
    for lat in header:
        is_valid_single_coord(lat['cell_lat'], is_valid_lat, 'lat')
        assert lat['cell_lat']['min_lat'] == max_type
        max_type = lat['cell_lat']['max_lat']
    return True


def is_valid_duration(duration):
    if duration is not None:
        get_valid_unsigned_int(duration)
    return True


def is_valid_body(body):
    max_type = body[0]['cell_lon']['min_lon']
    length = len(body[0]['duration'])
    for pair in body:
        is_valid_single_coord(pair['cell_lon'], is_valid_lon, 'lon')
        assert pair['cell_lon']['min_lon'] == max_type
        max_type = pair['cell_lon']['max_lon']
        assert length == len(pair['duration'])
        for duration in pair['duration']:
            is_valid_duration(duration)
    return True


def is_valid_matrix(matrix):
    header = matrix['line_headers']
    assert header
    is_valid_header(header)
    body = matrix['lines']
    assert body
    is_valid_body(body)
    return True


def is_valid_heat_maps(heat_map, tester, query):

    for g in get_not_null(heat_map, 'heat_maps'):
        get_valid_datetime(g['requested_date_time'])
        matrix = g['heat_matrix']
        assert matrix
        is_valid_matrix(matrix)
        if 'from' in g:
            is_valid_place(g['from'])
        if 'to' in g:
            is_valid_place(g['to'])
        assert ('from' in g) ^ ('to' in g)
    return True


def is_rt_dt_coherent(data_freshness, base_dt, amended_dt):
    if base_dt is None:
        return
    if data_freshness == 'base_schedule':
        assert base_dt == amended_dt
    elif base_dt != amended_dt:
        assert data_freshness != 'base_schedule'


def is_valid_section(section, query):
    arrival = get_valid_datetime(section['arrival_date_time'])
    departure = get_valid_datetime(section['departure_date_time'])

    assert (arrival - departure).seconds == section['duration']

    assert section['type']  # type cannot be empty

    if section['type'] in {'public_transport', 'on_demand_transport'}:
        is_valid_rt_level(section['data_freshness'])
        is_rt_dt_coherent(
            section['data_freshness'], section.get('base_arrival_date_time'), section['arrival_date_time']
        )
        is_rt_dt_coherent(
            section['data_freshness'], section.get('base_departure_date_time'), section['departure_date_time']
        )

    # for street network section, we must have a valid path
    if section['type'] == 'street_network':
        assert section['mode']  # mode cannot be empty for street network
        total_duration = 0
        for p in section['path']:
            assert get_valid_int(p['length']) >= 0
            assert -180 <= get_valid_int(p['direction']) <= 180  # direction is an angle
            # No constraint on name, it can be empty
            dur = get_valid_int(p['duration'])
            assert dur >= 0
            total_duration += dur

        assert abs(total_duration - section['duration']) <= 0.5 * len(section['path']) + 1

    if section['type'] in ('crow_fly', 'street_network', 'transfer', 'public_transport'):
        assert section['geojson']
        geojson = section['geojson']
        assert geojson['type'] == 'LineString'
        assert len(geojson['coordinates']) >= 2
        assert geojson['properties']

    # TODO check geojson
    # TODO check stop_date_times
    # TODO check from/to


def is_valid_ticket(ticket, tester):
    found = get_not_null(ticket, 'found')
    assert is_valid_bool(found)

    get_not_null(ticket, 'id')
    get_not_null(ticket, 'name')
    cost = get_not_null(ticket, 'cost')
    if found:
        # for found ticket, we must have a non empty currency
        get_not_null(cost, 'currency')

    get_valid_float(get_not_null(cost, 'value'))

    check_links(ticket, tester)


def is_valid_stop_area(stop_area, depth_check=1):
    """
    check the structure of a stop area
    """
    get_not_null(stop_area, "name")
    coord = get_not_null(stop_area, "coord")
    is_valid_label(get_not_null(stop_area, "label"))
    is_valid_coord(coord)

    for c in stop_area.get('comments', []):
        is_valid_comment(c)
    for physical_mode in stop_area.get("physical_modes", []):
        is_valid_physical_mode(physical_mode)
    for commercial_mode in stop_area.get("commercial_modes", []):
        is_valid_commercial_mode(commercial_mode)


def is_valid_stop_point(stop_point, depth_check=1):
    """
    check the structure of a stop point
    """
    get_not_null(stop_point, "name")
    is_valid_label(get_not_null(stop_point, "label"))
    coord = get_not_null(stop_point, "coord")
    is_valid_coord(coord)

    for c in stop_point.get('comments', []):
        is_valid_comment(c)
    for physical_mode in stop_point.get("physical_modes", []):
        is_valid_physical_mode(physical_mode)
    for commercial_mode in stop_point.get("commercial_modes", []):
        is_valid_commercial_mode(commercial_mode)

    if depth_check > 0:
        is_valid_stop_area(get_not_null(stop_point, "stop_area"), depth_check - 1)
    else:
        assert "stop_area" not in stop_point

    if depth_check == 3:
        is_valid_address(get_not_null(stop_point, "address"))


def is_valid_route(route, depth_check=1):
    get_not_null(route, "name")
    is_valid_bool(get_not_null(route, "is_frequence"))

    direction = get_not_null(route, "direction")
    is_valid_place(direction, depth_check - 1)
    # the direction of the route must always be a stop point
    assert get_not_null(direction, "embedded_type") == "stop_area"
    is_valid_stop_area(get_not_null(direction, "stop_area"), depth_check - 1)

    if depth_check > 0:
        is_valid_line(get_not_null(route, "line"), depth_check - 1)
    else:
        assert 'line' not in route

    for c in route.get('comments', []):
        is_valid_comment(c)

    # test if geojson is valid
    g = route.get('geojson')
    g is None or shape(g)  # TODO check length


def is_valid_company(company, depth_check=1):
    get_not_null(company, "name")
    get_not_null(company, "id")


def is_valid_physical_mode(physical_mode, depth_check=1):
    get_not_null(physical_mode, "name")
    get_not_null(physical_mode, "id")


def is_valid_commercial_mode(commercial_mode, depth_check=1):
    get_not_null(commercial_mode, "name")
    get_not_null(commercial_mode, "id")


def is_valid_line(line, depth_check=1):
    get_not_null(line, "name")
    get_not_null(line, "id")

    for physical_mode in line.get("physical_modes", []):
        is_valid_physical_mode(physical_mode)
    is_valid_commercial_mode(get_not_null(line, "commercial_mode"))

    for c in line.get('comments', []):
        is_valid_comment(c)

    if depth_check > 0:
        is_valid_network(get_not_null(line, 'network'), depth_check - 1)

        routes = get_not_null(line, 'routes')
        for r in routes:
            is_valid_route(r, depth_check - 1)
    else:
        assert 'network' not in line
        assert 'routes' not in line

    # test if geojson is valid
    g = line.get('geojson')
    g is None or shape(g)  # TODO check length


def is_valid_line_group(line_group, depth_check=1):
    get_not_null(line_group, "name")
    get_not_null(line_group, "id")

    if depth_check > 0:
        # the main_line is always displayed with a depth of 0 to reduce duplicated informations
        is_valid_line(get_not_null(line_group, "main_line"), 0)
        for l in line_group.get('lines', []):
            is_valid_line(l, depth_check - 1)


def is_valid_line_report(line_report, depth_check=1):
    is_valid_line(get_not_null(line_report, 'line'), depth_check - 1)
    pt_objects = get_not_null(line_report, 'pt_objects')
    for pt_object in pt_objects:
        is_valid_pt_object(pt_object, 0)


def is_valid_poi(poi, depth_check=1):
    get_not_null(poi, 'name')
    poi_type = get_not_null(poi, 'poi_type')
    get_not_null(poi_type, 'id')
    get_not_null(poi_type, 'name')
    get_not_null(poi, 'label')
    get_not_null(poi, 'id')
    is_valid_coord(get_not_null(poi, 'coord'))
    for admin in get_not_null(poi, 'administrative_regions'):
        is_valid_admin(admin, depth_check - 1)
    is_valid_address(get_not_null(poi, 'address'), depth_check - 1)


def is_valid_admin(admin, depth_check=1):
    if depth_check < 0:
        return
    get_not_null(admin, 'insee')
    name = get_not_null(admin, 'name')
    zip_code = get_not_null(admin, 'zip_code')
    lbl = get_not_null(admin, 'label')
    is_valid_label(lbl)
    zips = sorted(zip_code.split(';'))
    # name of the admin and it's min and max zip codes must be in the label
    assert name in lbl and zips[0] in lbl and zips[-1] in lbl

    get_not_null(admin, 'id')
    get_valid_int(get_not_null(admin, 'level'))
    is_valid_coord(get_not_null(admin, 'coord'))


def is_valid_codes(codes):
    for code in codes:
        get_not_null(code, "type")
        get_not_null(code, "value")


def is_valid_notes(notes):
    for note in notes:
        is_valid_note(note)


def is_valid_note(note):
    get_not_null(note, "id")
    get_not_null(note, "value")
    assert get_not_null(note, "type") == "notes"
    assert get_not_null(note, "category") in ["comment", "terminus"]
    if note["category"] == "comment":
        # TODO: migrate realtime proxy to use "terminus" note
        # assert get_not_null(note, "comment_type")
        pass


def is_valid_places(places, depth_check=1):
    for p in places:
        is_valid_place(p, depth_check)


def is_valid_place(place, depth_check=1):
    if depth_check < 0:
        return
    n = get_not_null(place, "name")
    get_not_null(place, "id")
    type = get_not_null(place, "embedded_type")
    if type == "address":
        address = get_not_null(place, "address")
        is_valid_address(address, depth_check)
    elif type == "stop_area":
        stop_area = get_not_null(place, "stop_area")
        is_valid_stop_area(stop_area, depth_check)

        # for stops name should be the label
        is_valid_label(n)
        assert stop_area['label'] == n
    elif type == "stop_point":
        stop_point = get_not_null(place, "stop_point")
        is_valid_stop_point(stop_point, depth_check)

        is_valid_label(n)
        assert stop_point['label'] == n
    elif type == "poi":
        is_valid_poi(get_not_null(place, "poi"), depth_check)
    elif type == "administrative_region":
        is_valid_admin(get_not_null(place, "administrative_region"), depth_check)
    else:
        assert False, "invalid type {}".format(type)


def is_valid_pt_objects_response(response, depth_check=1):
    for pt_obj in get_not_null(response, 'pt_objects'):
        is_valid_pt_object(pt_obj, depth_check)


def is_valid_pt_object(pt_object, depth_check=1):
    n = get_not_null(pt_object, "name")
    get_not_null(pt_object, "id")
    get_not_null(pt_object, "quality")
    pt_obj_type = get_not_null(pt_object, "embedded_type")

    assert pt_obj_type in (
        'line',
        'route',
        'network',
        'commercial_mode',
        'admin',
        'vehicle_journey',
        'calendar',
        'company',
        'stop_area',
        'stop_point',
        'poi',
        'address',
        'trip',
    )

    # if it's a line, it should pass the 'is_valid_line' test,
    # if it's a stop_area, it should pass the 'is_valid_stop_area' test ...
    check_method_to_call = getattr(sys.modules[__name__], 'is_valid_' + pt_obj_type)
    check_method_to_call(get_not_null(pt_object, pt_obj_type), depth_check)

    # check the pt_object label
    if pt_obj_type in ('stop_area', 'stop_point'):
        # for stops name should be the label
        assert get_not_null(pt_object, pt_obj_type)['label'] == n
    if pt_obj_type == 'line':
        # the line network, commercial_mode, code and name should be in the label
        check_embedded_line_label(n, pt_object['line'], depth_check)
    if pt_obj_type == 'route':
        # the line network, commercial_mode, code and name should be in the label
        check_embedded_route_label(n, pt_object['route'], depth_check)


def is_valid_period(period, depth_check=1):
    get_not_null(period, "begin")
    get_not_null(period, "end")


def is_valid_current_availability(current_availability, depth_check=1):
    get_not_null(current_availability, "status")
    periods = get_not_null(current_availability, 'periods')
    for period in periods:
        is_valid_period(period, 0)
    get_not_null(current_availability, "updated_at")
    cause = get_not_null(current_availability, "cause")
    get_not_null(cause, "label")
    effect = get_not_null(current_availability, "effect")
    get_not_null(effect, "label")


def is_valid_equipment_detail(equipment_detail, depth_check=1):
    get_not_null(equipment_detail, "id")
    get_not_null(equipment_detail, "embedded_type")
    if equipment_detail['current_availability']['status'] != "unknown":
        get_not_null(equipment_detail, "name")
        is_valid_current_availability(get_not_null(equipment_detail, 'current_availability'), depth_check - 1)


def is_valid_stop_area_equipment(stop_area_equipment, depth_check=1):
    is_valid_stop_area(get_not_null(stop_area_equipment, 'stop_area'), depth_check - 1)
    # equipment_details can be empty, if there is not filter into the request
    if 'equipment_details' in stop_area_equipment:
        equipment_details = stop_area_equipment['equipment_details']
        for equipment_detail in equipment_details:
            is_valid_equipment_detail(equipment_detail, 0)


def is_valid_equipment_report(equipment_report, depth_check=1):
    is_valid_line(get_not_null(equipment_report, 'line'), 1)
    stop_area_equipments = get_not_null(equipment_report, 'stop_area_equipments')
    for stop_area_equipment in stop_area_equipments:
        is_valid_stop_area_equipment(stop_area_equipment, 0)


def check_embedded_line_label(label, line, depth_check):
    is_valid_label(label)
    # the label must contains
    # the network name, the commercial mode name, the line id, and the line name
    if depth_check > 0:
        assert get_not_null(line, 'commercial_mode')['name'] in label
        assert get_not_null(line, 'network')['name'] in label
    assert line.get('code', '') in label
    assert get_not_null(line, 'name') in label


def check_embedded_route_label(label, route, depth_check):
    is_valid_label(label)
    # the label must contains
    # the line's network name, the commercial mode name, the line id, and the route name
    assert get_not_null(route, 'name') in label
    if depth_check > 0:
        line = get_not_null(route, 'line')
        assert get_not_null(line, 'code') in label

        if depth_check > 1:
            assert get_not_null(line, 'commercial_mode')['name'] in label
            assert get_not_null(line, 'network')['name'] in label


def is_valid_address(address, depth_check=1):
    if depth_check < 0:
        return
    id = get_not_null(address, "id")
    lon, lat = id.split(';')
    is_valid_lon(lon)
    is_valid_lat(lat)
    get_not_null(address, "house_number")
    get_not_null(address, "name")
    if depth_check >= 1:
        for admin in get_not_null(address, "administrative_regions"):
            is_valid_admin(admin, depth_check)
    coord = get_not_null(address, "coord")
    is_valid_coord(coord)


def is_valid_validity_pattern(validity_pattern, depth_check=1):
    beginning_date = get_not_null(validity_pattern, "beginning_date")
    assert is_valid_date(beginning_date)

    days = get_not_null(validity_pattern, "days")
    assert is_valid_days(days)


def is_valid_network(network, depth_check=1):
    get_not_null(network, "id")
    get_not_null(network, "name")


def is_valid_vehicle_journey(vj, depth_check=1):
    if depth_check < 0:
        return
    get_not_null(vj, "id")
    get_not_null(vj, "name")

    for c in vj.get('comments', []):
        is_valid_comment(c)

    if depth_check > 0:
        is_valid_trip(get_not_null(vj, "trip"), depth_check=depth_check - 1)

        is_valid_journey_pattern(get_not_null(vj, 'journey_pattern'), depth_check=depth_check - 1)
        is_valid_validity_pattern(get_not_null(vj, 'validity_pattern'), depth_check=depth_check - 1)

        stoptimes = get_not_null(vj, 'stop_times')

        for st in stoptimes:
            get_valid_time(get_not_null(st, 'arrival_time'))
            get_valid_time(get_not_null(st, 'departure_time'))
            get_valid_time(get_not_null(st, 'utc_arrival_time'))
            get_valid_time(get_not_null(st, 'utc_departure_time'))
            is_valid_stop_point(get_not_null(st, 'stop_point'), depth_check=depth_check - 1)

            # the JPP are kept only for backward compatibility
            if depth_check > 1:
                # with depth > 1 (we are already in the stoptime nested object), we don't want jpp
                is_valid_journey_pattern_point(get_not_null(st, 'journey_pattern_point'), depth_check - 2)
            else:
                assert 'journey_pattern_point' not in st
    else:
        # with depth = 0, we don't want the stop times, the jp, vp, ...
        assert 'stop_times' not in vj
        assert 'journey_pattern' not in vj
        assert 'validity_pattern' not in vj


def is_valid_trip(trip, depth_check=1):
    get_not_null(trip, "id")
    get_not_null(trip, "name")


def is_valid_journey_pattern(jp, depth_check=1):
    if depth_check < 0:
        return
    get_not_null(jp, "id")
    get_not_null(jp, "name")


def is_valid_journey_pattern_point(jpp, depth_check=1):
    get_not_null(jpp, "id")
    if depth_check > 0:
        is_valid_stop_point(get_not_null(jpp, 'stop_point'), depth_check=depth_check - 1)
    else:
        assert 'stop_point' not in jpp


def is_valid_comment(comment):
    get_not_null(comment, 'type')
    get_not_null(comment, 'value')


def is_valid_region_status(status):
    get_not_null(status, 'status')
    get_valid_int(get_not_null(status, 'data_version'))
    get_valid_int(get_not_null(status, 'nb_threads'))
    is_valid_bool(get_not_null(status, 'last_load_status'))
    is_valid_bool(get_not_null(status, 'is_connected_to_rabbitmq'))
    is_valid_date(get_not_null(status, 'end_production_date'))
    is_valid_date(get_not_null(status, 'start_production_date'))
    get_valid_datetime(get_not_null(status, 'last_load_at'), possible_errors=True)
    get_valid_datetime(get_not_null(status, 'publication_date'), possible_errors=True)
    get_not_null(status, 'is_realtime_loaded')


# for () are mandatory for the label even if is reality it is not
# (for example if the admin has no post code or a stop no admin)
# This make the test data a bit more difficult to create, but that way we can check the label creation
label_regexp = re.compile(".* \(.*\)")


def is_valid_label(label):
    m = label_regexp.match(label)
    return m is not None


def is_valid_rt_level(level):
    assert level in ('base_schedule', 'adapted_schedule', 'realtime')


def get_disruptions(obj, response):
    """
    unref disruption links are return the list of disruptions
    """
    all_disruptions = {d['id']: d for d in response['disruptions']}

    # !! uggly hack !!
    # for vehicle journeys we do not respect the right way to represent disruption,
    # we do not put the disruption link in a generic `links` section but in a `disruptions` sections...
    link_section = obj.get('links', obj.get('disruptions', []))

    return [all_disruptions[d['id']] for d in link_section if d['type'] == 'disruption']


def is_valid_line_section_disruption(disruption):
    """
    a line section disruption is a classic disruption with it's line as the impacted object
    and aside this a 'line_section' section with additional information on the line section
    """
    is_valid_disruption(disruption, chaos_disrup=False)

    line_section_impacted = next(
        (
            d
            for d in get_not_null(disruption, 'impacted_objects')
            if d['pt_object']['embedded_type'] == 'line' and d.get('impacted_section')
        ),
        None,
    )

    assert line_section_impacted
    line_section = get_not_null(line_section_impacted, 'impacted_section')
    is_valid_pt_object(get_not_null(line_section, 'from'))
    is_valid_pt_object(get_not_null(line_section, 'to'))


def is_valid_rail_section_disruption(disruption):
    """
    a rail section disruption is a classic disruption with it's line as the impacted object
    and aside this a 'rail_section' section with additional information on the rail section
    """
    is_valid_disruption(disruption, chaos_disrup=False)

    rail_section_impacted = next(
        (
            d
            for d in get_not_null(disruption, 'impacted_objects')
            if d['pt_object']['embedded_type'] == 'line' and d.get('impacted_rail_section')
        ),
        None,
    )

    assert rail_section_impacted
    rail_section = get_not_null(rail_section_impacted, 'impacted_rail_section')
    is_valid_pt_object(get_not_null(rail_section, 'from'))
    is_valid_pt_object(get_not_null(rail_section, 'to'))


def is_valid_disruption(disruption, chaos_disrup=True):
    get_not_null(disruption, 'id')
    get_not_null(disruption, 'disruption_id')
    s = get_not_null(disruption, 'severity')
    get_not_null(s, 'name')
    get_not_null(s, 'color')
    effect = get_not_null(s, 'effect')
    msg = disruption.get('messages', [])

    if chaos_disrup:
        # for chaos message is mandatory
        assert len(msg) > 0

    for m in msg:
        get_not_null(m, "text")
        channel = get_not_null(m, 'channel')
        get_not_null(channel, "content_type")
        get_not_null(channel, "id")
        get_not_null(channel, "name")

    for impacted_obj in get_not_null(disruption, 'impacted_objects'):
        pt_obj = get_not_null(impacted_obj, 'pt_object')
        is_valid_pt_object(pt_obj, depth_check=0)

        # for the vj, if it's not a cancellation we need to have the list of impacted stoptimes
        if get_not_null(pt_obj, 'embedded_type') == 'trip' and effect != 'NO_SERVICE':
            impacted_stops = get_not_null(impacted_obj, 'impacted_stops')

            assert len(impacted_stops) > 0
            for impacted_stop in impacted_stops:
                is_valid_stop_point(get_not_null(impacted_stop, 'stop_point'), depth_check=0)

                effect_values = ('added', 'deleted', 'delayed', 'unchanged')
                assert get_not_null(impacted_stop, "stop_time_effect") in effect_values
                assert get_not_null(impacted_stop, "departure_status") in effect_values
                assert get_not_null(impacted_stop, "arrival_status") in effect_values

                if 'base_arrival_time' in impacted_stop:
                    get_valid_time(impacted_stop['base_arrival_time'])
                if 'base_departure_time' in impacted_stop:
                    get_valid_time(impacted_stop['base_departure_time'])
                if 'amended_arrival_time' in impacted_stop:
                    get_valid_time(impacted_stop['amended_arrival_time'])
                if 'amended_departure_time' in impacted_stop:
                    get_valid_time(impacted_stop['amended_departure_time'])

                # if the stop has been deleted, we do not output it's amended times
                if get_not_null(impacted_stop, "departure_status") == 'deleted':
                    assert 'amended_departure_time' not in impacted_stop
                if get_not_null(impacted_stop, "arrival_status") == 'deleted':
                    assert 'amended_arrival_time' not in impacted_stop

                # we need at least either the base or the departure information
                assert (
                    'base_arrival_time' in impacted_stop
                    and 'base_departure_time' in impacted_stop
                    or 'amended_arrival_time' in impacted_stop
                    and 'amended_departure_time' in impacted_stop
                )


ObjGetter = namedtuple('ObjGetter', ['collection', 'uri'])


def has_disruption(response, object_get, disruption_uri):
    """
    Little helper to check if a specific object is linked to a specific disruption

    object_spec is a ObjGetter
    """
    o = next((s for s in response[object_get.collection] if s['id'] == object_get.uri), None)
    assert o, 'impossible to find object {}'.format(object_get)

    disruptions = get_disruptions(o, response) or []

    return disruption_uri in (d['disruption_uri'] for d in disruptions)


s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset
stopA_coord = "0.00107797;0.00071865"
stopB_coord = "0.0000898312;0.00026949"

# default journey query used in various test
journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
    from_coord=s_coord, to_coord=r_coord, datetime="20120614T080000"
)
isochrone_basic_query = "isochrones?from={from_coord}&datetime={datetime}&max_duration={max_duration}".format(
    from_coord=s_coord, datetime="20120614T080000", max_duration="3600"
)
heat_map_basic_query = "heat_maps?from={from_coord}&datetime={datetime}&max_duration={max_duration}".format(
    from_coord=s_coord, datetime="20120614T080000", max_duration="3600"
)
sub_query = 'journeys?from={from_coord}&to={to_coord}'.format(from_coord=s_coord, to_coord=r_coord)


def get_all_element_disruptions(elem, response):
    """
    return a map with the disruption id as key and the list of disruption + impacted object as value for a item of the response
    """
    DisruptAndElt = namedtuple('DisruptAndElt', ['disruption', 'impacted_object'])
    disruption_by_obj = defaultdict(list)

    all_disruptions = {d['id']: d for d in response['disruptions']}

    def disruptions_filler(_, obj):
        try:
            if 'links' not in obj:
                return
        except TypeError:
            return

        real_disruptions = [all_disruptions[d['id']] for d in obj['links'] if d['type'] == 'disruption']

        for d in real_disruptions:
            disruption_by_obj[d['id']].append(DisruptAndElt(disruption=d, impacted_object=obj))

    # we import utils here else it will import jormungandr too early in the test
    from jormungandr import utils

    utils.walk_dict(elem, disruptions_filler)

    return disruption_by_obj


def is_valid_stop_date_time(stop_date_time):
    get_not_null(stop_date_time, 'arrival_date_time')
    assert get_valid_datetime(stop_date_time['arrival_date_time'])
    get_not_null(stop_date_time, 'departure_date_time')
    assert get_valid_datetime(stop_date_time['departure_date_time'])
    get_not_null(stop_date_time, 'base_departure_date_time')
    assert get_valid_datetime(stop_date_time['base_departure_date_time'])
    get_not_null(stop_date_time, 'base_arrival_date_time')
    assert get_valid_datetime(stop_date_time['base_arrival_date_time'])


def is_valid_autocomplete(response, depth, mandatory_links=True):
    if mandatory_links:
        links = get_not_null(response, 'links')
        for link in links:
            assert 'href' in link
            assert 'rel' in link
            assert 'templated' in link
    places = get_not_null(response, 'places')

    for place in places:
        is_valid_place(place, depth_check=depth)


def is_valid_global_autocomplete(response, depth):
    return is_valid_autocomplete(response, depth, mandatory_links=False)


def get_used_vj(response):
    """
    return for each journeys the list of taken vj
    """
    journeys_vj = []
    for j in get_not_null(response, 'journeys'):
        vjs = []
        for s in get_not_null(j, 'sections'):
            for l in s.get('links', []):
                if l['type'] == 'vehicle_journey':
                    vjs.append(l['id'])
                    break
        journeys_vj.append(vjs)

    return journeys_vj


def get_arrivals(response):
    """
    return a list with the journeys arrival times
    """
    return [j['arrival_date_time'] for j in get_not_null(response, 'journeys')]


Journey = namedtuple('Journey', ['sections'])
Section = namedtuple(
    'Section',
    [
        'departure_date_time',
        'arrival_date_time',
        'base_departure_date_time',
        'base_arrival_date_time',
        'stop_date_times',
    ],
)
SectionStopDT = namedtuple(
    'SectionStopDT',
    ['departure_date_time', 'arrival_date_time', 'base_departure_date_time', 'base_arrival_date_time'],
)


def check_journey(journey, ref_journey):
    """
    check the values in a journey
    """
    for section, ref_section in zip_longest(journey['sections'], ref_journey.sections):
        assert section.get('departure_date_time') == ref_section.departure_date_time
        assert section.get('arrival_date_time') == ref_section.arrival_date_time
        assert section.get('base_departure_date_time') == ref_section.base_departure_date_time
        assert section.get('base_arrival_date_time') == ref_section.base_arrival_date_time
        for stop_dt, ref_stop_dt in zip_longest(section.get('stop_date_times', []), ref_section.stop_date_times):
            assert stop_dt.get('departure_date_time') == ref_stop_dt.departure_date_time
            assert stop_dt.get('arrival_date_time') == ref_stop_dt.arrival_date_time
            assert stop_dt.get('base_departure_date_time') == ref_stop_dt.base_departure_date_time
            assert stop_dt.get('base_arrival_date_time') == ref_stop_dt.base_arrival_date_time


def generate_pt_journeys(response):
    """ generate all journeys with at least a public transport section """
    for j in response.get('journeys', []):
        if any(s for s in j.get('sections', []) if s['type'] == 'public_transport'):
            yield j


def new_default_pagination_journey_comparator(clockwise):
    """same as new_default.__get_best_for_criteria but for python dict
    compare first on departure (or arrival for non clockwise), then on:
    - duration
    - number of transfers
    """

    def make_crit(func, reverse=False):
        if not reverse:
            return partial(compare_field, func=func)
        return partial(reverse_compare_field, func=func)

    main_criteria = (
        make_crit(lambda j: get_valid_datetime(j['arrival_date_time']))
        if clockwise
        else make_crit(lambda j: get_valid_datetime(j['departure_date_time']), reverse=True)
    )

    return [
        main_criteria,
        make_crit(lambda j: get_valid_int(j['duration'])),
        make_crit(lambda j: len(j.get('sections', []))),
    ]


def get_disruptions_by_id(response, disrupt_id):
    return [d for d in response['disruptions'] if d['id'] == disrupt_id]


def has_the_disruption(response, disrupt_id):
    return any(get_disruptions_by_id(response, disrupt_id))


def get_departure(dep, sp_uri, line_code):
    """ small helper that extract the information from a route point departures """
    return [
        {
            'rt': r['stop_date_time']['data_freshness'] == 'realtime',
            'dt': r['stop_date_time']['departure_date_time'],
        }
        for r in dep
        if r['stop_point']['id'] == sp_uri and r['route']['line']['code'] == line_code
    ]


def get_schedule(scs, sp_uri, line_code):
    """ small helper that extract the information from a route point stop schedule """
    return [
        {'rt': r['data_freshness'] == 'realtime', 'dt': r['date_time']}
        for r in next(
            rp_sched['date_times']
            for rp_sched in scs
            if rp_sched['stop_point']['id'] == sp_uri and rp_sched['route']['line']['code'] == line_code
        )
    ]


def get_disruption(disruptions, disrupt_id):
    return next((d for d in disruptions if d['id'] == disrupt_id), None)


# Looking for (identifiable) objects impacted by 'disruptions' inputed
def impacted_ids(disruptions):
    # for the impacted object returns:
    #  * id
    #  * or the id of the vehicle_journey for stop_schedule.date_time
    def get_id(obj):
        id = obj.impacted_object.get('id')
        if id is None:
            # stop_schedule.date_time case
            assert obj.impacted_object['links'][0]['type'] == 'vehicle_journey'
            id = obj.impacted_object['links'][0]['id']
        return id

    ids = set()
    for d in disruptions.values():
        ids.update(set(get_id(o) for o in d))

    return ids


def impacted_headsigns(disruptions):
    # for the impacted object returns the headsign (for the display_information field)

    ids = set()
    for d in disruptions.values():
        ids.update(set(o.impacted_object.get('headsign') for o in d))

    return ids
