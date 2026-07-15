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

import logging
import six
from werkzeug.datastructures import MultiDict
from flask_restful import reqparse


def _source(self, request):
    """
    Reimplementation of ``flask_restful.reqparse.Argument.source`` that reads
    the JSON body with ``get_json(silent=True)``.

    Since Flask 2.1 / Werkzeug 2.1, accessing ``request.json`` (or
    ``request.get_json()``) on a request that does not carry an
    ``application/json`` content type raises a 415 error instead of returning
    ``None``. As the default argument location is ``('json', 'values')``, every
    plain ``GET`` would then fail with "415 Unsupported Media Type". Reading the
    body silently restores the previous, lenient behaviour.
    """
    if isinstance(self.location, six.string_types):
        if self.location in ('json', 'get_json'):
            value = request.get_json(silent=True)
        else:
            value = getattr(request, self.location, MultiDict())
            if callable(value):
                value = value()
        if value is not None:
            return value
    else:
        values = MultiDict()
        for l in self.location:
            if l in ('json', 'get_json'):
                value = request.get_json(silent=True)
            else:
                value = getattr(request, l, None)
                if callable(value):
                    value = value()
            if value is not None:
                values.update(value)
        return values

    return MultiDict()


def patch_reqparse_json_location():
    """
    Make flask-restful tolerant to requests without an ``application/json``
    content type again (see :func:`_source`).
    """
    logging.getLogger(__name__).info('monkey patching source of reqparse.Argument')
    reqparse.Argument.source = _source
