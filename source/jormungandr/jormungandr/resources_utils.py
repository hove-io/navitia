# coding=utf-8

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

from __future__ import absolute_import, print_function, unicode_literals, division
from collections import defaultdict

from flask_restful import reqparse, Resource

import jormungandr
from jormungandr import i_manager
import logging
import pytz
from jormungandr.exceptions import RegionNotFound, UnableToParse
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.v1.serializer.jsonschema.serializer import SwaggerOptionPathSerializer
from jormungandr.interfaces.v1.swagger_schema import make_schema
from datetime import datetime
from navitiacommon.parser_args_type import BooleanType


class ResourceUtc(object):
    def __init__(self):
        self._tz = None

    def tz(self):
        if not self._tz:
            instance = i_manager.instances.get(self.region, None)

            if not instance:
                raise RegionNotFound(self.region)

            tz_name = instance.timezone  # TODO store directly the tz?

            if not tz_name:
                logging.Logger(__name__).warning("unknown timezone for region {}".format(self.region))
                return None
            self._tz = (pytz.timezone(tz_name),)
        return self._tz[0]

    def convert_to_utc(self, original_datetime):
        """
        convert the original_datetime in the args to UTC

        for that we need to 'guess' the timezone wanted by the user

        For the moment We only use the default instance timezone.

        It won't obviously work for multi timezone instances, we'll have to do
        something smarter.

        We'll have to consider either the departure or the arrival of the journey
        (depending on the `clockwise` param)
        and fetch the tz of this point.
        we'll have to store the tz for stop area and the coord for admin, poi, ...
        """
        if self.tz() is None:
            return original_datetime

        if original_datetime.tzinfo is not None:
            localized_dt = original_datetime
        else:
            # if we don't have a timezone in the datetime, we consider it a local time from the coverage's tz
            localized_dt = self.tz().normalize(self.tz().localize(original_datetime))

        try:
            utctime = localized_dt.astimezone(pytz.utc)
        except ValueError as e:
            raise UnableToParse("Unable to parse datetime, " + e.message)

        return utctime

    def format(self, value):
        dt = datetime.utcfromtimestamp(value)

        if self.tz() is not None:
            dt = pytz.utc.localize(dt)
            dt = dt.astimezone(self.tz())
            return dt.strftime("%Y%m%dT%H%M%S")
        return None  # for the moment I prefer not to display anything instead of something wrong


class DocumentedResource(Resource):
    def __init__(self, output_type_serializer=None):
        super(Resource, self).__init__()
        self.parsers = defaultdict(lambda: reqparse.RequestParser(argument_class=ArgumentDoc))
        self.output_type_serializer = output_type_serializer
        self.parsers["options"].add_argument(
            'schema', help='dump the swagger schema of the API', type=BooleanType(), default=False
        )

    def api_description(self, **kwargs):
        """
        return the API description

        Note: when the migration to swagger is completed, we can rename this function 'options'
        but for the moment we don't want all DocumentedResource to have an 'options' method
        """
        args = self.parsers["options"].parse_args()
        options_response = jormungandr.app.make_default_options_response()
        if not args['schema']:
            return options_response
        from flask import request

        schema = make_schema(resource=self, rule=request.url_rule)
        return SwaggerOptionPathSerializer(schema).data, 200, {'Allow': options_response.allow}
