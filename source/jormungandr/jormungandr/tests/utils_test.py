# coding=utf-8
# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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

from contextlib import contextmanager
from flask import appcontext_pushed, g


class MockResponse(object):
    """
    small class to mock an http response
    """
    def __init__(self, data, status_code, url, *args, **kwargs):
        self.data = data
        self.status_code = status_code
        self.url = url

    def json(self):
        return self.data

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
        if kwargs.get('params'):
            from urllib import urlencode
            url += "?{}".format(urlencode(kwargs.get('params'), doseq=True))

        return MockResponse(self.responses[url][0], self.responses[url][1], url)


class FakeUser:
    """
    We create a user independent from a database
    """
    def __init__(self, name, id,
                 have_access_to_free_instances=True, is_super_user=False, is_blocked=False, shape=None):
        """
        We just need a fake user, we don't really care about its identity
        """
        self.id = id
        self.login = name
        self.have_access_to_free_instances = have_access_to_free_instances
        self.is_super_user = is_super_user
        self.end_point_id = None
        self._is_blocked = is_blocked
        self.shape = shape

    @classmethod
    def get_from_token(cls, token):
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
        g.user = fake_user_type.get_from_token(user_name)
    with appcontext_pushed.connected_to(handler, app):
        yield
