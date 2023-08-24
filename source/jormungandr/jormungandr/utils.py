# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
import calendar
from collections import deque, namedtuple, defaultdict
from datetime import datetime
from google.protobuf.descriptor import FieldDescriptor
import pytz
from jormungandr.timezone import get_timezone
from navitiacommon import request_pb2, response_pb2, type_pb2, models
from navitiacommon.parser_args_type import DateTimeFormat
from importlib import import_module
import logging
from jormungandr.exceptions import ConfigException, UnableToParse, InvalidArguments
from six.moves.urllib.parse import urlparse
from jormungandr import new_relic, app
from six.moves import zip, range
from jormungandr.exceptions import TechnicalError
from flask import request, g
import re
import flask
from contextlib import contextmanager
import functools
import sys
import six
import csv
import os
import math

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3

DATETIME_FORMAT = "%Y%m%dT%H%M%S"
UTC_DATETIME_FORMAT = "%Y-%m-%dT%H:%M:%S%z"
NOT_A_DATE_TIME = "not-a-date-time"
WEEK_DAYS_MAPPING = ("monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday")
COVERAGE_ANY_BETA = "any-beta"
ORIGIN_DESTINATION_KEY = "{}-{}"
ONE_DAY = 86400
DATE_FORMAT = "%Y-%m-%d"

MAP_STRING_PTOBJECT_TYPE = {
    "stop_point": type_pb2.STOP_POINT,
    "stop_area": type_pb2.STOP_AREA,
    "address": type_pb2.ADDRESS,
    "administrative_region": type_pb2.ADMINISTRATIVE_REGION,
    "poi": type_pb2.POI,
    "access_point": type_pb2.ACCESS_POINT,
}


def get_uri_pt_object(pt_object):
    coord_format = "coord:{}:{}"
    if pt_object.embedded_type == type_pb2.ADDRESS:
        coords = pt_object.uri.split(';')
        return coord_format.format(coords[0], coords[1])
    if pt_object.embedded_type == type_pb2.ACCESS_POINT:
        return coord_format.format(pt_object.access_point.coord.lon, pt_object.access_point.coord.lat)
    if pt_object.embedded_type == type_pb2.POI:
        return coord_format.format(pt_object.poi.coord.lon, pt_object.poi.coord.lat)
    return pt_object.uri


def kilometers_to_meters(distance):
    # type: (float) -> int
    return int(round(distance * 1000.0))


class Coords:
    def __init__(self, lat, lon):
        self.lat = float(lat)
        self.lon = float(lon)


def is_coord(uri):
    # for the moment we do a simple check
    return get_lon_lat(uri) != (None, None)


def get_lon_lat(uri):
    """
    extract lon lat from an uri
    the uri should be formated as: 'lon;lat'
    >>> get_lon_lat('12.3;-5.3')
    (12.3, -5.3)
    >>> get_lon_lat('bob')
    (None, None)
    >>> get_lon_lat('5.3;bob')
    (None, None)
    >>> get_lon_lat('5.0;0.0')
    (5.0, 0.0)
    """
    if not uri:
        return None, None

    if uri.count(';') == 1:
        try:
            lon, lat = uri.split(';')
            # we check that both are float
            return float(lon), float(lat)
        except ValueError:
            return None, None
    return None, None


def is_url(url):
    if not url or url.strip() == '':
        return False
    url_parsed = urlparse(url)
    return url_parsed.scheme.strip() != '' and url_parsed.netloc.strip() != ''


def navitia_utcfromtimestamp(timestamp):
    try:
        if timestamp == 0:
            return None
        return datetime.utcfromtimestamp(timestamp)
    except ValueError:
        return None


def str_to_time_stamp(datetime_str):
    """
    convert a string to a posix timestamp
    the string must be in the YYYYMMDDTHHMMSS format
    like 20170534T124500
    """
    try:
        date = datetime.strptime(datetime_str, DATETIME_FORMAT)

        return date_to_timestamp(date)
    except Exception as e:
        logging.getLogger(__name__).exception(
            'Error while converting a string to a posix timestamp with exception: {}'.format(str(e))
        )
        return None


def str_to_dt(str):
    """
    convert a string to a datetime
    the string must be in the YYYYMMDDTHHMMSS format
    like 20170534T124500
    """
    return datetime.strptime(str, DATETIME_FORMAT)


def date_to_timestamp(date):
    """
    convert a datetime objet to a posix timestamp (number of seconds from 1970/1/1)
    """
    return int(calendar.timegm(date.utctimetuple()))


def str_datetime_utc_to_local(dt, timezone):
    if dt:
        utc_dt = DateTimeFormat()(dt)
    else:
        utc_dt = datetime.utcnow()
    local = pytz.timezone(timezone)
    return dt_to_str(utc_dt.replace(tzinfo=pytz.UTC).astimezone(local))


def local_str_date_to_utc(str, tz=None):
    timezone = tz or get_timezone()
    dt = str_to_dt(str)
    if timezone:
        local = pytz.timezone(timezone)
        local_dt = local.localize(dt, is_dst=None)
        utc_date = local_dt.astimezone(pytz.UTC)
        return utc_date
    return None


def local_str_date_to_str_date_with_offset(str, tz):
    timezone = tz or get_timezone()
    dt = str_to_dt(str)
    local = pytz.timezone(timezone)
    local_dt = local.localize(dt, is_dst=None)
    return local_dt.isoformat()


def timestamp_to_datetime(timestamp, tz=None):
    """
    Convert a timestamp to datetime
    if timestamp > MAX_INT we return None
    """
    maxint = 9223372036854775807
    # when a date is > 2038-01-19 03:14:07
    # we receive a timestamp = 18446744071562142720 (64 bits) > 9223372036854775807 (MAX_INT 32 bits)
    # And ValueError: timestamp out of range for platform time_t is raised
    if timestamp >= maxint:
        return None

    dt = navitia_utcfromtimestamp(timestamp)
    if not dt:
        return None

    timezone = tz or get_timezone()
    if timezone:
        dt = pytz.utc.localize(dt)
        return dt.astimezone(timezone)
    return None


def dt_to_str(dt, _format=DATETIME_FORMAT):
    return dt.strftime(_format)


def timestamp_to_str(timestamp):
    dt = timestamp_to_datetime(timestamp)
    if dt:
        return dt_to_str(dt)
    return None


def dt_to_date_str(dt, _format=DATE_FORMAT):
    return dt.strftime(_format)


def timestamp_to_date_str(timestamp, tz=None, _format=DATE_FORMAT):
    dt = timestamp_to_datetime(timestamp, tz=tz)
    if dt:
        return dt_to_date_str(dt, _format=_format)
    return None


def walk_dict(tree, visitor):
    """
    depth first search on a dict.
    call the visit(elem) method on the visitor for each node

    if the visitor returns True, stop the search

    """
    queue = deque()

    def add_elt(name, elt, first=False):
        if isinstance(elt, (list, tuple)):
            for val in elt:
                queue.append((name, val))
        elif hasattr(elt, 'items'):
            for k, v in elt.items():
                queue.append((k, v))
        elif first:  # for the first elt, we add it even if it is no collection
            queue.append((name, elt))

    add_elt("main", tree, first=True)
    while queue:
        elem = queue.pop()
        # we don't want to visit the list, we'll visit each node separately
        if not isinstance(elem[1], (list, tuple)):
            if visitor(elem[0], elem[1]) is True:
                # we stop the search if the visitor returns True
                break
        # for list and tuple, the name is the parent's name
        add_elt(elem[0], elem[1])


def walk_protobuf(pb_object, visitor):
    """
    Walk on a protobuf and call the visitor for each nodes
    >>> journeys = response_pb2.Response()
    >>> journey_standard = journeys.journeys.add()
    >>> journey_standard.type = "none"
    >>> journey_standard.duration = 1
    >>> journey_standard.nb_transfers = 2
    >>> s = journey_standard.sections.add()
    >>> s.duration = 3
    >>> s = journey_standard.sections.add()
    >>> s.duration = 4
    >>> journey_rapid = journeys.journeys.add()
    >>> journey_rapid.duration = 5
    >>> journey_rapid.nb_transfers = 6
    >>> s = journey_rapid.sections.add()
    >>> s.duration = 7
    >>>
    >>> from collections import defaultdict
    >>> types_counter = defaultdict(int)
    >>> def visitor(name, val):
    ...     types_counter[type(val)] +=1
    >>>
    >>> walk_protobuf(journeys, visitor)
    >>> types_counter[response_pb2.Response]
    1
    >>> types_counter[response_pb2.Journey]
    2
    >>> types_counter[response_pb2.Section]
    3
    >>> types_counter[int]  # and 7 int in all
    7
    """
    queue = deque()

    def add_elt(name, elt):
        try:
            fields = elt.ListFields()
        except AttributeError:
            return
        for field, value in fields:
            if field.label == FieldDescriptor.LABEL_REPEATED:
                for v in value:
                    queue.append((field.name, v))
            else:
                queue.append((field.name, value))

    # add_elt("main", pb_object)
    queue.append(('main', pb_object))
    while queue:
        elem = queue.pop()

        visitor(elem[0], elem[1])

        add_elt(elem[0], elem[1])


def realtime_level_to_pbf(level):
    if level == 'base_schedule':
        return type_pb2.BASE_SCHEDULE
    elif level == 'adapted_schedule':
        return type_pb2.ADAPTED_SCHEDULE
    elif level == 'realtime':
        return type_pb2.REALTIME
    else:
        raise ValueError('Impossible to convert in pbf')


# we can't use reverse(enumerate(list)) without creating a temporary
# list, so we define our own reverse enumerate
def reverse_enumerate(l):
    return zip(range(len(l) - 1, -1, -1), reversed(l))


def pb_del_if(l, pred):
    '''
    Delete the elements such as pred(e) is true in a protobuf list.
    Return the number of elements deleted.
    '''
    nb = 0
    for i, e in reverse_enumerate(l):
        if pred(e):
            del l[i]
            nb += 1
    return nb


def create_object(configuration):
    """
    Create an object from a dict
    The dict must contains a 'class' or 'klass' key with the class path of the class we want to create
    It can contains also an 'args' key with a dictionary of arguments to pass to the constructor
    """
    log = logging.getLogger(__name__)
    class_path = configuration.get('class') or configuration.get('klass')
    if class_path is None:
        log.warning('impossible to build object, class_path is empty')
        raise KeyError(
            'impossible to build a StreetNetwork, missing mandatory field in configuration: class or klass'
        )

    kwargs = configuration.get('args', {})
    if "id" in configuration:
        kwargs["id"] = configuration["id"]

    try:
        if '.' not in class_path:
            log.warning('impossible to build object {}, wrongly formated class'.format(class_path))
            raise ConfigException(class_path)

        module_path, name = class_path.rsplit('.', 1)
        module = import_module(module_path)
        attr = getattr(module, name)
    except AttributeError as e:
        log.warning('impossible to build object {} : {}'.format(class_path, e))
        raise ConfigException(class_path)
    except ImportError:
        log.exception('impossible to build object {}, cannot find class'.format(class_path))
        raise ConfigException(class_path)

    try:
        obj = attr(**kwargs)  # call to the contructor, with all the args
    except TypeError as e:
        log.warning('impossible to build object {}, wrong arguments: {}'.format(class_path, e))
        raise ConfigException(class_path)

    return obj


def generate_id():
    import shortuuid

    return shortuuid.uuid()


def add_children(pt_object, dict_pt_object):
    for ch in dict_pt_object.get("poi", {}).get("children", []):
        ch_poi = pt_object.poi.children.add()
        ch_poi.uri = ch["id"]
        ch_poi.name = ch.get("name", "")
        coord = Coords(ch["coord"]["lat"], ch["coord"]["lon"])
        ch_poi.coord.lon = coord.lon
        ch_poi.coord.lat = coord.lat


def add_properties(pt_object, dict_pt_object):
    for key, value in dict_pt_object.get("poi", {}).get("properties", {}).items():
        property = pt_object.poi.properties.add()
        property.type = key
        property.value = value


def check_dict_object(dict_pt_object):
    if not isinstance(dict_pt_object, dict):
        logging.getLogger(__name__).error('Invalid dict_pt_object')
        raise InvalidArguments('dict_pt_object')
    embedded_type = MAP_STRING_PTOBJECT_TYPE.get(dict_pt_object.get("embedded_type"))
    if not embedded_type:
        logging.getLogger(__name__).error('Invalid embedded_type')
        raise InvalidArguments('embedded_type')


def populate_pt_object(pt_object, dict_pt_object):
    pt_object.uri = dict_pt_object["id"]
    pt_object.name = dict_pt_object.get("name", "")
    text_embedded_type = dict_pt_object.get("embedded_type")
    embedded_type = MAP_STRING_PTOBJECT_TYPE.get(text_embedded_type)
    pt_object.embedded_type = embedded_type

    map_pt_object_type_to_pt_object = {
        type_pb2.STOP_POINT: pt_object.stop_point,
        type_pb2.STOP_AREA: pt_object.stop_area,
        type_pb2.ADDRESS: pt_object.address,
        type_pb2.ADMINISTRATIVE_REGION: pt_object.administrative_region,
        type_pb2.POI: pt_object.poi,
        type_pb2.ACCESS_POINT: pt_object.access_point,
    }

    obj = map_pt_object_type_to_pt_object.get(embedded_type)
    obj.uri = dict_pt_object[text_embedded_type]["id"]
    obj.name = dict_pt_object[text_embedded_type].get("name", "")
    coord = Coords(
        dict_pt_object[text_embedded_type]["coord"]["lat"], dict_pt_object[text_embedded_type]["coord"]["lon"]
    )
    obj.coord.lon = coord.lon
    obj.coord.lat = coord.lat
    if embedded_type == type_pb2.POI:
        add_children(pt_object, dict_pt_object)
        add_properties(pt_object, dict_pt_object)


def get_pt_object_from_json(dict_pt_object, instance):
    check_dict_object(dict_pt_object)
    embedded_type = MAP_STRING_PTOBJECT_TYPE.get(dict_pt_object.get("embedded_type"))
    if embedded_type == type_pb2.ADMINISTRATIVE_REGION:
        # In this case we need the main_stop_area
        pt_object = instance.georef.place(dict_pt_object["id"])
        if pt_object:
            return pt_object
    pt_object = type_pb2.PtObject()
    populate_pt_object(pt_object, dict_pt_object)

    within_zones = dict_pt_object.get("within_zones", [])
    if pt_object.embedded_type == type_pb2.ADDRESS and within_zones:
        for within_zone in within_zones:
            pt_object_within_zone = pt_object.address.within_zones.add()
            populate_pt_object(pt_object_within_zone, within_zone)
    return pt_object


def replace_address_with_custom_poi(dict_pt_object, uri):
    new_poi = next(
        (
            wz
            for wz in dict_pt_object.get("within_zones", [])
            if MAP_STRING_PTOBJECT_TYPE.get(wz.get("embedded_type")) == type_pb2.POI
            and wz.get("poi", {}).get("children", [])
        ),
        None,
    )
    if not new_poi:
        return dict_pt_object
    if not is_coord(uri):
        return dict_pt_object
    lon, lat = get_lon_lat(uri)
    # We're putting back the coordinate of the end-user, who is in a POI area (with entrypoints).
    # If we don't, barycenter of the POI area will be displayed which means nothing for the end user.
    new_poi["poi"]["coord"]["lon"] = "{}".format(lon)
    new_poi["poi"]["coord"]["lat"] = "{}".format(lat)
    return new_poi


def entrypoint_uri_refocus(dict_pt_object, uri):
    if not dict_pt_object:
        return None
    if MAP_STRING_PTOBJECT_TYPE.get(dict_pt_object.get("embedded_type")) == type_pb2.ADDRESS:
        return replace_address_with_custom_poi(dict_pt_object, uri)
    return dict_pt_object


def json_address_from_uri(uri):
    if is_coord(uri):
        lon, lat = get_lon_lat(uri)
        return {
            "id": uri,
            "embedded_type": "address",
            "address": {"id": uri, "coord": {"lon": "{}".format(lon), "lat": "{}".format(lat)}},
        }
    return None


def get_pt_object_coord(pt_object):
    """
    Given a PtObject, return the coord according to its embedded_type
    :param pt_object: type_pb2.PtObject
    :return: coord: type_pb2.GeographicalCoord

    >>> pt_object = type_pb2.PtObject()
    >>> pt_object.embedded_type = type_pb2.POI
    >>> pt_object.poi.coord.lon = 42.42
    >>> pt_object.poi.coord.lat = 41.41
    >>> coord = get_pt_object_coord(pt_object)
    >>> coord.lon
    42.42
    >>> coord.lat
    41.41
    """
    if not isinstance(pt_object, type_pb2.PtObject):
        logging.getLogger(__name__).error('Invalid pt_object')
        raise InvalidArguments('Invalid pt_object')

    map_coord = {
        type_pb2.STOP_POINT: "stop_point",
        type_pb2.STOP_AREA: "stop_area",
        type_pb2.ADDRESS: "address",
        type_pb2.ADMINISTRATIVE_REGION: "administrative_region",
        type_pb2.POI: "poi",
        type_pb2.ACCESS_POINT: "access_point",
    }
    attr = getattr(pt_object, map_coord.get(pt_object.embedded_type, ""), None)
    coord = getattr(attr, "coord", None)

    if not coord:
        logging.getLogger(__name__).error('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
        raise UnableToParse('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
    return coord


def is_olympic_poi(pt_object, instance):
    if pt_object.embedded_type != type_pb2.POI:
        return False
    if not (hasattr(pt_object.poi, 'properties') and pt_object.poi.properties):
        return False
    olympic_site = next(
        (
            p
            for p in pt_object.poi.properties
            if p.type == instance.olympics_forbidden_uris.poi_property_key
            and p.value == instance.olympics_forbidden_uris.poi_property_value
        ),
        None,
    )
    return olympic_site


def is_olympic_site(entry_point, instance):
    if not entry_point:
        return False
    if is_olympic_poi(entry_point, instance):
        return True
    if entry_point.embedded_type == type_pb2.ADDRESS:
        if not (hasattr(entry_point.address, 'within_zones') and entry_point.address.within_zones):
            return False
        for within_zone in entry_point.address.within_zones:
            if is_olympic_poi(within_zone, instance):
                return True
    return False


def get_olympic_site(entry_point, instance):
    if not instance.olympics_forbidden_uris:
        return None
    if not entry_point:
        return None
    if is_olympic_site(entry_point, instance):
        return entry_point
    if entry_point.embedded_type == type_pb2.ADDRESS:
        if not (hasattr(entry_point.address, 'within_zones') and entry_point.address.within_zones):
            return None
        for within_zone in entry_point.address.within_zones:
            if is_olympic_poi(within_zone, instance):
                return within_zone
    return None


def get_last_pt_section(journey):
    return next((s for s in reversed(journey.sections) if s.type == response_pb2.PUBLIC_TRANSPORT), None)


def get_first_pt_section(journey):
    return next((s for s in journey.sections if s.type == response_pb2.PUBLIC_TRANSPORT), None)


def record_external_failure(message, connector_type, connector_name):
    params = {'{}_system_id'.format(connector_type): six.text_type(connector_name), 'message': message}
    new_relic.record_custom_event('{}_external_failure'.format(connector_type), params)


def decode_polyline(encoded, precision=6):
    '''
    Version of : https://developers.google.com/maps/documentation/utilities/polylinealgorithm
    But with improved precision
    See: https://mapzen.com/documentation/mobility/decoding/#python (valhalla)
         http://developers.geovelo.fr/#/documentation/compute (geovelo)
    '''
    inv = 10**-precision
    decoded = []
    previous = [0, 0]
    i = 0
    # for each byte
    while i < len(encoded):
        # for each coord (lat, lon)
        ll = [0, 0]
        for j in [0, 1]:
            shift = 0
            byte = 0x20
            # keep decoding bytes until you have this coord
            while byte >= 0x20:
                byte = ord(encoded[i]) - 63
                i += 1
                ll[j] |= (byte & 0x1F) << shift
                shift += 5
            # get the final value adding the previous offset and remember it for the next
            ll[j] = previous[j] + (~(ll[j] >> 1) if ll[j] & 1 else (ll[j] >> 1))
            previous[j] = ll[j]
        # scale by the precision and chop off long coords also flip the positions so
        # #its the far more standard lon,lat instead of lat,lon
        decoded.append([float('%.6f' % (ll[1] * inv)), float('%.6f' % (ll[0] * inv))])
        # hand back the list of coordinates
    return decoded


# PeriodExtremity is used to provide a datetime and it's meaning
#     datetime: given datetime (obviously)
#     represents_start: is True if it's start of period, False if it's the end of period
# (mostly used for fallback management in experimental scenario)
PeriodExtremity = namedtuple('PeriodExtremity', ['datetime', 'represents_start'])


class SectionSorter(object):
    def __call__(self, a, b):
        if a.begin_date_time != b.begin_date_time:
            return -1 if a.begin_date_time < b.begin_date_time else 1
        elif a.end_date_time != b.end_date_time:
            return -1 if a.end_date_time < b.end_date_time else 1
        elif a.destination.uri == b.origin.uri:
            return -1
        elif a.origin.uri == b.destination.uri:
            return 1
        else:
            return 0


def make_namedtuple(typename, *fields, **fields_with_default):
    """
    helper to create a named tuple with some default values
    :param typename: name of the type
    :param fields: required argument of the named tuple
    :param fields_with_default: positional arguments with fields and their default value
    :return: the namedtuple

    """
    import collections

    field_names = list(fields) + list(fields_with_default.keys())
    T = collections.namedtuple(typename, field_names)
    T.__new__.__defaults__ = tuple(fields_with_default.values())
    return T


def get_timezone_str(default='Africa/Abidjan'):
    try:
        timezone = get_timezone()
    except TechnicalError:
        return default
    else:
        return timezone.zone if timezone else default


def get_current_datetime_str(is_utc=False):
    timezone = 'Africa/Abidjan' if is_utc else get_timezone_str()
    current_datetime = request.args.get('_current_datetime')
    return str_datetime_utc_to_local(current_datetime, timezone)


def make_timestamp_from_str(strftime):
    """

    :param strftime:
    :return: double

    >>> make_timestamp_from_str("2017-12-25T08:07:59 +01:00")
    1514185679
    >>> make_timestamp_from_str("20171225T080759+01:00")
    1514185679
    >>> make_timestamp_from_str("2017-12-25 08:07:59 +01:00")
    1514185679
    >>> make_timestamp_from_str("20171225T080759Z")
    1514189279
    """
    from dateutil import parser
    import calendar

    return calendar.timegm(parser.parse(strftime).utctimetuple())


def get_house_number(housenumber):
    hn = 0
    numbers = re.findall(r'^\d+', housenumber or "0")
    if len(numbers) > 0:
        hn = numbers[0]
    return int(hn)


# The two following functions allow to use flask request context in greenlet
# The decorator provided by flask (@copy_current_request_context) will generate an assertion error with multiple greenlets


def copy_flask_request_context():
    """
    Make a copy of the 'main' flask request conquest to be used with the context manager below
    :return: a copy of the current flask request context
    """
    # Copy flask request context to be used in greenlet
    top = flask._request_ctx_stack.top
    if top is None:
        raise RuntimeError(
            'This function can only be used at local scopes '
            'when a request context is on the stack.  For instance within '
            'view functions.'
        )
    return top.copy()


@contextmanager
def copy_context_in_greenlet_stack(request_context):
    """
    Push a copy of the 'main' flask request context in a global stack created for it.
    Pop the copied request context to discard it

    ex:
        request_context = utils.copy_flask_request_context()

        def worker():
            with utils.copy_context_in_greenlet_stack(request_context):
                # do some work here with flask request context available

        gevent.spawn(worker) # Multiples times

    :param request_context: a copy of the 'main' flask request context
    """
    flask.globals._request_ctx_stack.push(request_context)
    yield
    flask.globals._request_ctx_stack.pop()


def compose(*funs):
    """
    compose functions and return a callable object

    example 1:
    f(x) = x + 1
    g(x) = 2*x

    compose(f,g) = g(f(x)) = 2 * (x + 1 )

    example 2:
    f(a list of integer): returns multiples of 3
    g(a list of integer): returns multiples of 5

    compose(f,g): returns multiples of 3 AND 5

    :param funs:
    :return: a lambda

    >>> c = compose(lambda x: x+1, lambda x: 2*x)
    >>> c(42)
    86

    >>> f = lambda l: (x for x in l if x%3 == 0)
    >>> g = lambda l: (x for x in l if x%5 == 0)
    >>> c = compose(f, g)
    >>> list(c(range(45)))
    [0, 15, 30]
    """
    return lambda obj: functools.reduce(lambda prev, f: f(prev), funs, obj)


class ComposedFilter(object):
    """
    Compose several filters with convenient interfaces
    All filters are evaluated lazily, the first added is the first tested

    >>> F = ComposedFilter()
    >>> f = F.add_filter(lambda x: x % 2 == 0).add_filter(lambda x: x % 5 == 0).compose_filters()
    >>> list(f(range(40)))
    [0, 10, 20, 30]
    >>> list(f(range(20))) # we can reuse the composed filter
    [0, 10]
    >>> f = F.add_filter(lambda x: x % 3 == 0).compose_filters() # we can continue on adding new filter
    >>> list(f(range(40)))
    [0, 30]
    """

    def __init__(self):
        self.filters = []

    def add_filter(self, pred):
        self.filters.append(lambda iterable: (i for i in iterable if pred(i)))
        return self

    def compose_filters(self):
        return compose(*self.filters)


def portable_min(*args, **kwargs):
    """
    a portable min() for python2 which takes a default value when
    the iterable is empty

    >>> portable_min([1], default=42)
    1
    >>> portable_min([], default=42)
    42
    >>> portable_min([]) # only behavioral change compared to py3's min: default to None (could be reconsidered, forcing caller to provide default)

    >>> portable_min((j for j in [])) # same, default to None

    >>> portable_min((j for j in []), key=lambda j: j) # same, default to None

    >>> portable_min(iter(()), default=43) # empty iterable
    43
    >>> portable_min((j for j in [{"s": 5}, {"s": 9},{"s": 1}]), key=lambda j: j["s"]) # not comparable without key
    {'s': 1}

    """
    if PY2:
        default = kwargs.pop('default', None)
        try:
            return min(*args, **kwargs)
        except ValueError:
            return default
        except Exception:
            raise
    if PY3:
        kwargs.setdefault('default', None)  # may be changed before really switching to py3's min().
        return min(*args, **kwargs)


def mps_to_kmph(speed):
    return round(3.6 * speed)


def get_poi_params(codes):
    poi_params = set()
    for code in codes or []:
        if code.startswith('poi:'):
            poi_params.add(code)
    return poi_params


def get_overriding_mode(main_mode, modes):
    if main_mode == 'bss' and 'walking' in modes:
        return ['walking']
    return []


def has_invalid_reponse_code(objects):
    return objects[1] != 200


def journeys_absent(objects):
    return 'journeys' not in objects[0]


def can_connect_to_database():
    # Why testing the db connection when the db is disabled?
    if app.config['DISABLE_DATABASE']:
        return False

    # If g is not initialized we are out of app_context. This never happens for any service jormungandr
    # in this case we return true to have retro-compatibility.
    if not g:
        return True
    if hasattr(g, 'can_connect_to_database'):
        return g.can_connect_to_database
    try:
        engine = models.db.engine
        connection = engine.connect()
        connection.close()
        g.can_connect_to_database = True
    except Exception:
        logging.getLogger(__name__).exception('Connection to database has failed')
        g.can_connect_to_database = False
        return False

    return True


def create_journeys_request(origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt):
    req = request_pb2.Request()
    req.requested_api = type_pb2.pt_planner

    def _set_departure_attractivity(stop_point_id, location):
        attractivity_virtual_duration = journey_parameters.olympic_site_params.get("departure", {}).get(
            stop_point_id
        )
        if attractivity_virtual_duration:
            location.attractivity = attractivity_virtual_duration.attractivity

    def _set_arrival_attractivity(stop_point_id, location):
        attractivity_virtual_duration = journey_parameters.olympic_site_params.get("arrival", {}).get(
            stop_point_id
        )
        if attractivity_virtual_duration:
            location.attractivity = attractivity_virtual_duration.attractivity

    for stop_point_id, access_duration in origins.items():
        location = req.journeys.origin.add()
        location.place = stop_point_id
        location.access_duration = access_duration
        _set_departure_attractivity(stop_point_id, location)

    for stop_point_id, access_duration in destinations.items():
        location = req.journeys.destination.add()
        location.place = stop_point_id
        location.access_duration = access_duration
        _set_arrival_attractivity(stop_point_id, location)

    req.journeys.night_bus_filter_max_factor = journey_parameters.night_bus_filter_max_factor
    req.journeys.night_bus_filter_base_factor = journey_parameters.night_bus_filter_base_factor

    req.journeys.datetimes.append(datetime)
    req.journeys.clockwise = clockwise
    req.journeys.realtime_level = realtime_level_to_pbf(journey_parameters.realtime_level)
    req.journeys.max_duration = journey_parameters.max_duration
    req.journeys.max_transfers = journey_parameters.max_transfers
    req.journeys.wheelchair = journey_parameters.wheelchair
    if journey_parameters.max_extra_second_pass:
        req.journeys.max_extra_second_pass = journey_parameters.max_extra_second_pass

    for uri in journey_parameters.forbidden_uris:
        req.journeys.forbidden_uris.append(uri)

    for id in journey_parameters.allowed_id:
        req.journeys.allowed_id.append(id)

    if journey_parameters.direct_path_duration is not None:
        req.journeys.direct_path_duration = journey_parameters.direct_path_duration

    req.journeys.bike_in_pt = bike_in_pt

    if journey_parameters.min_nb_journeys:
        req.journeys.min_nb_journeys = journey_parameters.min_nb_journeys

    if journey_parameters.timeframe:
        req.journeys.timeframe_duration = int(journey_parameters.timeframe)

    if journey_parameters.depth:
        req.journeys.depth = journey_parameters.depth

    if journey_parameters.isochrone_center:
        req.journeys.isochrone_center.place = journey_parameters.isochrone_center.uri
        req.journeys.isochrone_center.access_duration = 0
        req.requested_api = type_pb2.ISOCHRONE

    if journey_parameters.sn_params:
        sn_params_request = req.journeys.streetnetwork_params
        sn_params_request.origin_mode = journey_parameters.sn_params.origin_mode
        sn_params_request.destination_mode = journey_parameters.sn_params.destination_mode
        sn_params_request.walking_speed = journey_parameters.sn_params.walking_speed
        sn_params_request.bike_speed = journey_parameters.sn_params.bike_speed
        sn_params_request.car_speed = journey_parameters.sn_params.car_speed
        sn_params_request.bss_speed = journey_parameters.sn_params.bss_speed
        sn_params_request.car_no_park_speed = journey_parameters.sn_params.car_no_park_speed

    if journey_parameters.current_datetime:
        req._current_datetime = journey_parameters.current_datetime

    if journey_parameters.walking_transfer_penalty:
        req.journeys.walking_transfer_penalty = journey_parameters.walking_transfer_penalty

    if journey_parameters.arrival_transfer_penalty:
        req.journeys.arrival_transfer_penalty = journey_parameters.arrival_transfer_penalty

    if journey_parameters.criteria == "robustness":
        req.journeys.criteria = request_pb2.Robustness
    elif journey_parameters.criteria == "occupancy":
        req.journeys.criteria = request_pb2.Occupancy
    elif journey_parameters.criteria == "classic":
        req.journeys.criteria = request_pb2.Classic
    elif journey_parameters.criteria == "arrival_stop_attractivity":
        req.journeys.criteria = request_pb2.ArrivalStopAttractivity
    elif journey_parameters.criteria == "departure_stop_attractivity":
        req.journeys.criteria = request_pb2.DepartureStopAttractivity

    return req


def create_graphical_isochrones_request(
    origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
):
    req = create_journeys_request(
        origins,
        destinations,
        datetime,
        clockwise,
        graphical_isochrones_parameters.journeys_parameters,
        bike_in_pt,
    )
    req.requested_api = type_pb2.graphical_isochrone
    req.journeys.max_duration = graphical_isochrones_parameters.journeys_parameters.max_duration
    if graphical_isochrones_parameters.boundary_duration:
        for duration in sorted(graphical_isochrones_parameters.boundary_duration, key=int, reverse=True):
            if graphical_isochrones_parameters.min_duration < duration < req.journeys.max_duration:
                req.isochrone.boundary_duration.append(duration)
    req.isochrone.boundary_duration.insert(0, req.journeys.max_duration)
    req.isochrone.boundary_duration.append(graphical_isochrones_parameters.min_duration)

    if req.journeys.origin:
        req.journeys.clockwise = True
    else:
        req.journeys.clockwise = False

    req.isochrone.journeys_request.CopyFrom(req.journeys)
    return req


def remove_ghost_words(query_string, ghost_words):
    for gw in ghost_words:
        query_string = re.sub(gw, '', query_string, flags=re.IGNORECASE)
    return query_string


def get_weekday(timestamp, timezone):
    try:
        date_time = datetime.fromtimestamp(timestamp, tz=timezone)
        return WEEK_DAYS_MAPPING[date_time.weekday()]
    except ValueError:
        return None


def is_stop_point(uri):
    return uri.startswith("stop_point") if uri else False


def make_origin_destination_key(from_id, to_id):
    return ORIGIN_DESTINATION_KEY.format(from_id, to_id)


def read_best_boarding_positions(file_path):
    logger = logging.getLogger(__name__)
    if not os.path.exists(file_path):
        logger.warning("file: %s does not exist", file_path)
        return None

    logger.info("reading best boarding position from file: %s", file_path)
    position_str_to_enum = {
        'front': response_pb2.BoardingPosition.FRONT,
        'middle': response_pb2.BoardingPosition.MIDDLE,
        'back': response_pb2.BoardingPosition.BACK,
    }
    try:
        my_dict = defaultdict(set)
        fieldnames = ['from_id', 'to_id', 'positionnement_navitia']
        with open(file_path) as f:
            csv_reader = csv.DictReader(f, fieldnames)
            # skip the header
            next(csv_reader)

            for line in csv_reader:
                key = make_origin_destination_key(line['from_id'], line['to_id'])
                pos_str = line['positionnement_navitia']
                pos_enum = position_str_to_enum.get(pos_str.lower())
                if pos_enum is None:
                    logger.warning(
                        "Error occurs when loading best_boarding_positions, wrong position string: %s", pos_str
                    )
                    continue
                my_dict[key].add(pos_enum)

        return my_dict
    except Exception as e:
        logger.exception(
            'Error while loading best_boarding_positions file: {} with exception: {}'.format(file_path, str(e))
        )
        return None


def read_origin_destination_data(file_path):
    logger = logging.getLogger(__name__)
    if not os.path.exists(file_path):
        logger.warning("file: %s does not exist", file_path)
        return None, None, None

    logger.info("reading origin destination and allowed ids from file: %s", file_path)
    try:
        my_dict = defaultdict(set)
        stop_areas = set()
        allowed_ids = set()
        fieldnames = ['origin', 'destination', 'od_value']
        with open(file_path) as f:
            csv_reader = csv.DictReader(f, fieldnames)
            # skip the header
            next(csv_reader)

            for line in csv_reader:
                allowed_id = line['od_value']
                if allowed_id:
                    key = make_origin_destination_key(line['origin'], line['destination'])
                    my_dict[key].add(allowed_id)

                    stop_areas.add(line['origin'])
                    stop_areas.add(line['destination'])
                    allowed_ids.add(allowed_id)

        return my_dict, stop_areas, allowed_ids
    except Exception as e:
        logger.exception(
            'Error while loading od_allowed_ids file: {} with exception: {}'.format(file_path, str(e))
        )
        return None, None, None


def ceil_by_half(f):
    """Return f ceiled by 0.5.
    >>> ceil_by_half(1.0)
    1.0
    >>> ceil_by_half(0.6)
    1.0
    >>> ceil_by_half(0.5)
    0.5
    >>> ceil_by_half(0.1)
    0.5
    >>> ceil_by_half(0.0)
    0.0
    """
    return 0.5 * math.ceil(2.0 * float(f))


def content_is_too_large(instance, endpoint, response):
    if not instance:
        return False
    if instance.resp_content_limit_bytes is None:
        return False
    if endpoint in instance.resp_content_limit_endpoints_whitelist:
        return False
    if response.content_length is None:
        return False
    if response.content_length <= instance.resp_content_limit_bytes:
        return False

    return True
