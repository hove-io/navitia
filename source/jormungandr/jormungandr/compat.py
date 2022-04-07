# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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

"""
This module is here for handling compatibility with other version of dependencies than the one we used at first
Ideally the code must be directly adapted, this module should use for ease transitioning.
"""

from __future__ import absolute_import, print_function, unicode_literals, division

from functools import wraps
from flask_restful import reqparse
from werkzeug.exceptions import BadRequest
import logging


def replace_parse_arg(func):
    @wraps(func)
    def _exec(*args, **kw):
        try:
            return func(*args, **kw)
        except BadRequest as e:
            if hasattr(e, "data") and isinstance(e.data, dict) and 'message' in e.data:
                e.data['message'] = list(e.data['message'].values())[0]
            raise e

    return _exec


def patch_reqparse():
    """
    New version of flask-restful return a dict in place of a str when args validation fail.
    This is a lot nicer but will break the interface, so in a first time we want to go back to the previous errors
    """
    logging.getLogger(__name__).info('monkey patching parse_args of RequestParser')
    reqparse.RequestParser.parse_args = replace_parse_arg(reqparse.RequestParser.parse_args)
