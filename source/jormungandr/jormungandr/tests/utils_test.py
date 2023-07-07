# coding=utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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

from __future__ import absolute_import, unicode_literals
from contextlib import contextmanager
from flask import appcontext_pushed, g
from jormungandr.utils import (
    timestamp_to_datetime,
    make_namedtuple,
    walk_dict,
    get_pt_object_from_json,
    read_best_boarding_positions,
    read_origin_destination_data,
    make_origin_destination_key,
    portable_min,
    get_first_or_last_pt_section,
)
import pytz
from jormungandr import app
import datetime
import io
from operator import itemgetter
from jormungandr.exceptions import InvalidArguments
from navitiacommon import models, response_pb2
from navitiacommon.constants import DEFAULT_SHAPE_SCOPE
import pytest
import csv
import os


class MockResponse(object):
    """
    small class to mock an http response
    """

    def __init__(self, data, status_code, url=None, *args, **kwargs):
        self.data = data
        self.status_code = status_code
        self.url = url

    def json(self):
        return self.data

    def raise_for_status(self):
        return self.status_code

    @property
    def text(self):
        return self.data

    @property
    def content(self):
        return self.data


class MockRequests(object):
    """
    small class to mock an http request
    """

    def __init__(self, responses):
        self.responses = responses

    def get(self, url, *args, **kwargs):
        params = kwargs.get('params')
        if params:
            from six.moves.urllib.parse import urlencode

            params.sort()
            url += "?{}".format(urlencode(params, doseq=True))

        r = self.responses.get(url)
        if r:
            return MockResponse(r[0], r[1], url)
        else:
            raise Exception("impossible to find mock response for url {}".format(url))

    def post(self, *args, **kwargs):
        return self.get(*args, **kwargs)


class FakeUser(models.User):
    """
    We create a user independent from a database
    """

    def __init__(
        self,
        name,
        id,
        have_access_to_free_instances=True,
        is_super_user=False,
        is_blocked=False,
        shape=None,
        shape_scope=None,
        default_coord=None,
        block_until=None,
    ):
        """
        We just need a fake user, we don't really care about its identity
        """
        self.id = id
        self.login = name
        self.type = 'with_free_instances'
        if not have_access_to_free_instances:
            self.type = 'without_free_instances'
        if is_super_user:
            self.type = 'super_user'
        self.end_point_id = None
        self._is_blocked = is_blocked
        self.shape = shape
        self.default_coord = default_coord
        self.block_until = block_until
        self.shape_scope = shape_scope if shape_scope else DEFAULT_SHAPE_SCOPE

    @classmethod
    def get_from_token(cls, token, valid_until):
        """
        Create an empty user
        Must be overridden
        """
        assert False

    def is_blocked(self, datetime_utc):
        """
        Return True if user is blocked else False
        """
        return self._is_blocked


@contextmanager
def user_set(app, fake_user_type, user_name):
    """
    set the test user doing the request
    """

    def handler(sender, **kwargs):
        g.user = fake_user_type.get_from_token(user_name, valid_until=None)
        g.can_connect_to_database = True

    with appcontext_pushed.connected_to(handler, app):
        yield


def test_timestamp_to_datetime():
    # timestamp > MAX_INT
    assert timestamp_to_datetime(18446744071562142720) is None

    with app.app_context():
        g.timezone = pytz.utc
        # test valid date
        assert timestamp_to_datetime(1493296245) == datetime.datetime(2017, 4, 27, 12, 30, 45, tzinfo=pytz.UTC)

        g.timezone = None
        # test valid date but no timezone
        assert timestamp_to_datetime(1493296245) is None


def test_make_named_tuple():
    Bob = make_namedtuple('Bob', 'a', 'b', c=2, d=14)
    b = Bob(b=14, a=12)
    assert b.a == 12
    assert b.b == 14
    assert b.c == 2
    assert b.d == 14
    b = Bob(14, 12)  # non named argument also works
    assert b.a == 14
    assert b.b == 12
    assert b.c == 2
    assert b.d == 14
    b = Bob(12, b=14, d=123)
    assert b.a == 12
    assert b.b == 14
    assert b.c == 2
    assert b.d == 123
    with pytest.raises(TypeError):
        Bob(a=12)
    with pytest.raises(TypeError):
        Bob()


def test_walk_dict():
    bob = {
        'tutu': 1,
        'tata': [1, 2],
        'toto': {'bob': 12, 'bobette': 13, 'nested_bob': {'bob': 3}},
        'tete': ('tuple1', ['ltuple1', 'ltuple2']),
        'titi': [{'a': 1}, {'b': 1}],
    }
    result = io.StringIO()

    def my_first_stopper_visitor(name, val):
        result.write("{}={}\n".format(name, val))
        if val == 'ltuple1':
            return True

    walk_dict(bob, my_first_stopper_visitor)
    expected_nodes = ["tete", "tuple1"]
    assert all(node in result.getvalue() for node in expected_nodes)

    def my_second_stopper_visitor(name, val):
        result.write("{}={}\n".format(name, val))
        if val == 3:
            return True

    walk_dict(bob, my_second_stopper_visitor)
    expected_nodes = ["toto", "bob", "bobette", "nested_bob"]
    assert all(node in result.getvalue() for node in expected_nodes)


def compare_list_of_dicts(sorting_key, first_list, second_list):
    first_list.sort(key=itemgetter(sorting_key))
    second_list.sort(key=itemgetter(sorting_key))
    assert len(first_list) == len(second_list)
    return all(x == y for x, y in (zip(first_list, second_list)))


def test_get_pt_object_from_json_invalid_json():
    with pytest.raises(InvalidArguments) as error:
        get_pt_object_from_json("bob", object())
    assert error.value.data["message"] == "Invalid arguments dict_pt_object"


def test_get_pt_object_from_json_invalid_embedded_type():
    pt_object_json = {"embedded_type": "bob", "id": "8.98312e-05;8.98312e-05"}
    with pytest.raises(InvalidArguments) as error:
        get_pt_object_from_json(pt_object_json, object())
    assert error.value.data["message"] == "Invalid arguments embedded_type"


def test_get_pt_object_from_json():
    pt_object_json = {
        "embedded_type": "poi",
        "id": "poi:to",
        "name": "Jardin (City)",
        "poi": {
            "poi_type": {"id": "poi_type:jardin", "name": "Jardin"},
            "name": "Jardin",
            "children": [
                {
                    "name": "Jardin: Porte 3",
                    "coord": {"lat": "0.0007186505", "lon": "0.0018864605"},
                    "type": "poi",
                    "id": "poi:to:porte3",
                },
                {
                    "name": "Jardin: Porte 4",
                    "coord": {"lat": "0.0007186508", "lon": "0.0018864608"},
                    "type": "poi",
                    "id": "poi:to:porte4",
                },
            ],
            "coord": {"lat": "0.00071865", "lon": "0.00188646"},
            "label": "Jardin (City)",
            "id": "poi:to",
        },
    }
    pt_object = get_pt_object_from_json(pt_object_json, object())
    assert pt_object
    assert pt_object.name == "Jardin (City)"
    assert pt_object.uri == "poi:to"
    assert len(pt_object.poi.children) == 2


def test_portable_min():
    assert portable_min([]) is None
    assert portable_min((j for j in [])) is None


def test_read_best_boarding_positions():
    import shortuuid

    file_name = 'best_boarding_positions_test_{}.csv'.format(shortuuid.uuid())

    with open(file_name, 'w+') as file:
        writer = csv.writer(file)
        field = ["from_id", "to_id", "positionnement_navitia"]

        writer.writerow(field)
        # the boarding test should be case-insensitive and only one 'front' should be present in the map
        writer.writerow(["a", "b", "Front"])
        writer.writerow(["a", "b", "FRONT"])

        writer.writerow(["a", "b", "BACK"])
        writer.writerow(["a", "b", "middle"])
        writer.writerow(["a", "b", "WTH???"])

        writer.writerow(["b", "a", "BACK"])
        writer.writerow(["b", "a", "front"])

    d = read_best_boarding_positions(file_name)
    if os.path.exists(file_name):
        os.remove(file_name)

    assert d[make_origin_destination_key("a", "b")] == {
        response_pb2.FRONT,
        response_pb2.BACK,
        response_pb2.MIDDLE,
    }
    assert d[make_origin_destination_key("b", "a")] == {response_pb2.FRONT, response_pb2.BACK}


def test_read_origin_destination_data():
    import shortuuid

    file_name = 'od_allowed_ids_test_{}.csv'.format(shortuuid.uuid())

    with open(file_name, 'w+') as file:
        writer = csv.writer(file)
        field = ['origin', 'destination', 'allowed_id']

        writer.writerow(field)
        # the od allowed ids test should be case-insensitive
        writer.writerow(["sa:1", "sa:10", "rer:1"])
        writer.writerow(["sa:1", "sa:10", "bus:1"])
        writer.writerow(["sa:1", "sa:10", "metro:1"])

        # Repeated allowed_ids for a same key should not raise an error
        writer.writerow(["sa:1", "sa:11", "train:2"])
        writer.writerow(["sa:1", "sa:11", "tram:2"])
        writer.writerow(["sa:1", "sa:11", "tram:2"])

    d, _, _ = read_origin_destination_data(file_name)
    if os.path.exists(file_name):
        os.remove(file_name)

    assert d[make_origin_destination_key("sa:1", "sa:10")] == {"rer:1", "bus:1", "metro:1"}
    assert d[make_origin_destination_key("sa:1", "sa:11")] == {"train:2", "tram:2"}


def test_get_first_or_last_pt_section_journey_without_sections():
    response = response_pb2.Response()
    pb_j = response.journeys.add()
    section = get_first_or_last_pt_section(pb_j)
    assert not section


def test_get_first_or_last_pt_section_journey_only_street_network_section():
    response = response_pb2.Response()
    pb_j = response.journeys.add()
    pb_s = pb_j.sections.add()
    pb_s.type = response_pb2.STREET_NETWORK
    section = get_first_or_last_pt_section(pb_j)
    assert not section


def test_get_first_or_last_pt_section_journey_only_street_network_section():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    response = response_pb2.Response()
    pb_j = response.journeys.add()
    for index, stype in enumerate(
        [
            response_pb2.STREET_NETWORK,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.TRANSFER,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.STREET_NETWORK,
        ]
    ):
        pb_s = pb_j.sections.add()
        pb_s.type = stype
        pb_s.duration = index
    first_section = get_first_or_last_pt_section(pb_j)
    last_section = get_first_or_last_pt_section(pb_j, False)
    assert first_section.duration == 1
    assert last_section.duration == 3
