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

from __future__ import absolute_import, print_function, unicode_literals, division
import calendar
from collections import deque
from datetime import datetime
from google.protobuf.descriptor import FieldDescriptor
import pytz
from jormungandr.timezone import get_timezone
from navitiacommon import response_pb2, type_pb2
from builtins import range, zip
from importlib import import_module
import logging
from jormungandr.exceptions import ConfigException, UnableToParse, InvalidArguments
from urlparse import urlparse
from jormungandr import new_relic


DATETIME_FORMAT = "%Y%m%dT%H%M%S"


def get_uri_pt_object(pt_object):
    if pt_object.embedded_type == type_pb2.ADDRESS:
        coords = pt_object.uri.split(';')
        return "coord:{}:{}".format(coords[0], coords[1])
    return pt_object.uri

def kilometers_to_meters(distance):
    return distance * 1000.0


def is_url(url):
    if not url or url.strip() == '':
        return False
    url_parsed = urlparse(url)
    return url_parsed.scheme.strip() != '' and url_parsed.netloc.strip() != ''


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
    convert a datatime objet to a posix timestamp (number of seconds from 1070/1/1)
    """
    return int(calendar.timegm(date.utctimetuple()))


def timestamp_to_datetime(timestamp):
    dt = datetime.utcfromtimestamp(timestamp)

    timezone = get_timezone()
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

    >>> bob = {'tutu': 1,
    ... 'tata': [1, 2],
    ... 'toto': {'bob':12, 'bobette': 13, 'nested_bob': {'bob': 3}},
    ... 'tete': ('tuple1', ['ltuple1', 'ltuple2']),
    ... 'titi': [{'a':1}, {'b':1}]}

    >>> def my_visitor(name, val):
    ...     print("{}={}".format(name, val))

    >>> walk_dict(bob, my_visitor)
    titi={u'b': 1}
    b=1
    titi={u'a': 1}
    a=1
    tete=ltuple2
    tete=ltuple1
    tete=tuple1
    tutu=1
    toto={u'bobette': 13, u'bob': 12, u'nested_bob': {u'bob': 3}}
    nested_bob={u'bob': 3}
    bob=3
    bob=12
    bobette=13
    tata=2
    tata=1

    >>> def my_stoper_visitor(name, val):
    ...     print("{}={}".format(name, val))
    ...     if name == 'tete':
    ...         return True

    >>> walk_dict(bob, my_stoper_visitor)
    titi={u'b': 1}
    b=1
    titi={u'a': 1}
    a=1
    tete=ltuple2
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
        #we don't want to visit the list, we'll visit each node separately
        if not isinstance(elem[1], (list, tuple)):
            if visitor(elem[0], elem[1]) is True:
                #we stop the search if the visitor returns True
                break
        #for list and tuple, the name is the parent's name
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


#we can't use reverse(enumerate(list)) without creating a temporary
#list, so we define our own reverse enumerate
def reverse_enumerate(l):
    return zip(range(len(l)-1, -1, -1), reversed(l))


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
    The dict must contains a 'class' key with the class path of the class we want to create
    It can contains also an 'args' key with a dictionary of arguments to pass to the constructor
    """
    class_path = configuration['class']
    kwargs = configuration.get('args', {})

    log = logging.getLogger(__name__)
    try:
        if '.' not in class_path:
            log.warn('impossible to build object {}, wrongly formated class'.format(class_path))
            raise ConfigException(class_path)

        module_path, name = class_path.rsplit('.', 1)
        module = import_module(module_path)
        attr = getattr(module, name)
    except AttributeError as e:
        log.warn('impossible to build object {} : {}'.format(class_path, e))
        raise ConfigException(class_path)
    except ImportError:
        log.warn('impossible to build object {}, cannot find class'.format(class_path))
        raise ConfigException(class_path)

    try:
        obj = attr(**kwargs)  # call to the contructor, with all the args
    except TypeError as e:
        log.warn('impossible to build object {}, wrong arguments: {}'.format(class_path, e.message))
        raise ConfigException(class_path)

    return obj


def generate_id():
    import uuid
    return uuid.uuid4()


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
        type_pb2.POI: "poi"
    }
    attr = getattr(pt_object,
                   map_coord.get(pt_object.embedded_type, ""),
                   None)
    coord = getattr(attr, "coord", None)

    if not coord:
        logging.getLogger(__name__).error('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
        raise UnableToParse('Invalid coord for ptobject type: {}'.format(pt_object.embedded_type))
    return coord


def record_external_failure(message, connector_type, connector_name):
    params = {'{}_system_id'.format(connector_type): repr(connector_name), 'message': message}
    new_relic.record_custom_event('{}_external_failure'.format(connector_type), params)


def decode_polyline(encoded):
    '''
    Version of : https://developers.google.com/maps/documentation/utilities/polylinealgorithm
    But with improved precision
    See: https://mapzen.com/documentation/mobility/decoding/#python (valhalla)
         http://developers.geovelo.fr/#/documentation/compute (geovelo)
    '''
    inv = 1.0 / 1e6
    decoded = []
    previous = [0, 0]
    i = 0
    #for each byte
    while i < len(encoded):
        #for each coord (lat, lon)
        ll = [0, 0]
        for j in [0, 1]:
            shift = 0
            byte = 0x20
            #keep decoding bytes until you have this coord
            while byte >= 0x20:
                byte = ord(encoded[i]) - 63
                i += 1
                ll[j] |= (byte & 0x1f) << shift
                shift += 5
            #get the final value adding the previous offset and remember it for the next
            ll[j] = previous[j] + (~(ll[j] >> 1) if ll[j] & 1 else (ll[j] >> 1))
            previous[j] = ll[j]
        #scale by the precision and chop off long coords also flip the positions so
        # #its the far more standard lon,lat instead of lat,lon
        decoded.append([float('%.6f' % (ll[1] * inv)), float('%.6f' % (ll[0] * inv))])
        #hand back the list of coordinates
    return decoded
