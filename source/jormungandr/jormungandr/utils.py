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
from navitiacommon import response_pb2, type_pb2
from builtins import range, zip
from importlib import import_module
import logging
from jormungandr.exceptions import ConfigException

def str_to_time_stamp(str):
    """
    convert a string to a posix timestamp
    the string must be in the YYYYMMDDTHHMMSS format
    like 20170534T124500
    """
    date = datetime.strptime(str, "%Y%m%dT%H%M%S")

    return date_to_timestamp(date)


def date_to_timestamp(date):
    """
    convert a datatime objet to a posix timestamp (number of seconds from 1070/1/1)
    """
    return int(calendar.timegm(date.utctimetuple()))


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


def create_object(class_path, **kwargs):
    log = logging.getLogger(__name__)
    try:
        if '.' not in class_path:
            log.warn('impossible to build object {}, wrongly formated class'.format(class_path))
            raise ConfigException(class_path)

        module_path, name = class_path.rsplit('.', 1)
        module = import_module(module_path)
        attr = getattr(module, name)
    except ImportError:
        log.warn('impossible to build object {}, cannot find class'.format(class_path))
        raise ConfigException(class_path)

    try:
        obj = attr(**kwargs)  # call to the contructor, with all the args
    except TypeError as e:
        log.warn('impossible to build object {}, wrong arguments: {}'.format(class_path, e.message))
        raise ConfigException(class_path)

    return obj
