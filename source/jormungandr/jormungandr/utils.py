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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
import calendar
from collections import deque, namedtuple
from datetime import datetime
from google.protobuf.descriptor import FieldDescriptor
import pytz
from jormungandr.timezone import get_timezone
from navitiacommon import response_pb2, type_pb2
from navitiacommon.parser_args_type import DateTimeFormat
from importlib import import_module
import logging
from jormungandr.exceptions import ConfigException, UnableToParse, InvalidArguments
from six.moves.urllib.parse import urlparse
from jormungandr import new_relic
from six.moves import zip, range
from jormungandr.exceptions import TechnicalError
from flask import request
import re
import flask
from contextlib import contextmanager
import functools
import sys
import six

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3

DATETIME_FORMAT = "%Y%m%dT%H%M%S"
NOT_A_DATE_TIME = "not-a-date-time"


def get_uri_pt_object(pt_object):
    if pt_object.embedded_type == type_pb2.ADDRESS:
        coords = pt_object.uri.split(';')
        return "coord:{}:{}".format(coords[0], coords[1])
    return pt_object.uri


def kilometers_to_meters(distance):
    # type: (float) -> float
    return distance * 1000.0


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


def str_to_time_stamp(str):
    """
    convert a string to a posix timestamp
    the string must be in the YYYYMMDDTHHMMSS format
    like 20170534T124500
    """
    date = datetime.strptime(str, DATETIME_FORMAT)

    return date_to_timestamp(date)


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


def dt_to_str(dt):
    return dt.strftime(DATETIME_FORMAT)


def timestamp_to_str(timestamp):
    dt = timestamp_to_datetime(timestamp)
    if dt:
        return dt_to_str(dt)
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
        log.warning('impossible to build object {}, wrong arguments: {}'.format(class_path, e.message))
        raise ConfigException(class_path)

    return obj


def generate_id():
    import shortuuid

    return shortuuid.uuid()


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
    }
    attr = getattr(pt_object, map_coord.get(pt_object.embedded_type, ""), None)
    coord = getattr(attr, "coord", None)

    if not coord:
        logging.getLogger(__name__).error('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
        raise UnableToParse('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
    return coord


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
    inv = 10 ** -precision
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
        else:
            return -1 if a.end_date_time < b.end_date_time else 1


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
    >>> portable_min(iter(()), default=43) # empty iterable
    43

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
