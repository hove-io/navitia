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

from __future__ import absolute_import
from flask import current_app
import flask_restful
from flask_restful import reqparse
import six


class ArgumentDoc(reqparse.Argument):
    def __init__(
        self,
        name,
        default=None,
        dest=None,
        required=False,
        ignore=False,
        type=six.text_type,
        location=('values',),
        choices=(),
        action='store',
        help=None,
        operators=('=',),
        case_sensitive=True,
        hidden=False,
        deprecated=False,
        schema_type=None,
        schema_metadata={},
    ):
        super(ArgumentDoc, self).__init__(
            name,
            default,
            dest,
            required,
            ignore,
            type,
            location,
            choices,
            action,
            help,
            operators,
            case_sensitive,
        )
        self.hidden = hidden
        self.schema_type = schema_type
        self.schema_metadata = schema_metadata
        self.deprecated = deprecated  # not used for now, but usefull "comment" (to be used in swagger v3.0)

    def handle_validation_error(self, error, bundle_errors):
        """
        Override method to output message from exception and from argument's help
        """
        error_str = six.text_type(error)
        error_msg = u'parameter "{}" invalid: '.format(self.name)
        if error_str:
            error_msg += error_str
        if self.help:
            if error_msg:
                error_msg += u'\n'
            error_msg += u'{} description: {}'.format(self.name, self.help)
        msg = {self.name: error_msg}

        if current_app.config.get("BUNDLE_ERRORS", False) or bundle_errors:
            return error, msg
        flask_restful.abort(400, message=msg)
