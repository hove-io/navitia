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
import re
from flask import Request
import uuid

from werkzeug.routing import Rule


class ReverseProxied(object):
    """
    Wrap the application in this middleware and configure the
    front-end server to add these headers, to let you quietly bind
    this to an HTTP scheme that is different than what is used locally.

    In nginx:
    location /myprefix {
        proxy_set_header X-Scheme $scheme;
    }

    :param app: the WSGI application
    """

    def __init__(self, app):
        self.app = app
        self.re = re.compile('^https?$')

    def __call__(self, environ, start_response):
        scheme = environ.get('HTTP_X_SCHEME', '')
        if scheme and self.re.match(scheme):
            environ['wsgi.url_scheme'] = scheme
        return self.app(environ, start_response)


class NavitiaRequest(Request):
    """
    override the request of flask to add an id on all request
    """

    def __init__(self, *args, **kwargs):
        super(Request, self).__init__(*args, **kwargs)
        self.id = str(uuid.uuid4())


class NavitiaRule(Rule):
    """
    custom class to add a 'hide' variable to hide a rule in swagger
    """

    def __init__(self, name, **kwargs):
        routes_to_hide = kwargs.pop('hide_routes', [])
        name_no_version = name.replace('/v1', '')
        self.hide = kwargs.pop('hide', False) or name_no_version in routes_to_hide

        super(NavitiaRule, self).__init__(name, **kwargs)
