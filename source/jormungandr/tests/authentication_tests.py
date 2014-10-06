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
import logging

from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *
from contextlib import contextmanager
from flask import appcontext_pushed, g
from jormungandr import app
import logging


authorizations = {1: { "main_routing_test": {'ALL' : True},
    "departure_board_test" : {'ALL' : False}}}
class FakeUser(Object):
    """
    We create a user independent from a database
    """
    def __init__(self, id):
        """
        We just need a fake user, we don't really care about its identity
        """
        self.id = id
        self.login = str(id)

    @classmethod
    def get_from_token(cls, token):
        """
        Create an empty user
        """
        return FakeUser(token)

    def has_access(self, instance_name, api_name):
        """
        This is made to avoid using of database
        """
        return authorizations[self.id][api_name][instance_name]

@contextmanager
def user_set(app, user_name):
    """
    add user
    """

    def handler(sender, **kwargs):
        g.user = FakeUser.get_from_token(user_name)
    with appcontext_pushed.connected_to(handler, app):
        yield

@dataset(["main_routing_test", "departure_board_test"])
class TestAuthentication(AbstractTestFixture):

    def setUp(self):
        app.config['PUBLIC'] = False
        self.app = app.test_client()

    def test_status_code(self):
        requests_status_codes = [
        ('/v1/coverage/main_routing_test', 200),
        ('/v1/coverage/departure_board_test', 403),
        ('/v1/journeys?from=stopA&to=stopB&datetime=20120614T080000', 200),
        ('/v1/journeys?from=stopA&to=stop2&datetime=20120614T080000', 403),
        ('/v1/journeys?from=stop1&to=stop2&datetime=20120614T080000', 403)
        ]

        with user_set(app, 1):
            for request, status_code in requests_status_codes:
                assert(self.app.get(request).status_code == status_code)

