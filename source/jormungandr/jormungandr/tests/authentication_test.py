# encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
from jormungandr import app
from jormungandr.authentication import get_token
import base64
import pytest
from werkzeug.exceptions import Unauthorized

def get_token_direct_test():
    with app.test_request_context('/', headers={'Authorization': 'mykey'}):
        assert get_token() == 'mykey'

def get_token_basic_auth_test():
    key = base64.encodestring('mykey:').strip()
    with app.test_request_context('/', headers={'Authorization': 'BASIC {}'.format(key)}):
        assert get_token() == 'mykey'

def get_token_no_token_test():
    with app.test_request_context('/'):
        assert get_token() == None

def get_token_basic_auth_unicode_test():
    """
    base64 doesn't seems to like unicode, it doesn't matter since our token are always in ascii (uuid).
    But we want a clean error, not a 500
    """
    key = base64.encodestring(b'maclé:').strip()
    with app.test_request_context('/', headers={'Authorization': 'BASIC {}'.format(key)}):
        with pytest.raises(Unauthorized):
            get_token()

def get_token_url_test():
    with app.test_request_context('/', query_string='key=mykey'):
        assert get_token() == 'mykey'
