# coding=utf-8

#  Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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

from navitiacommon.parser_args_type import BooleanType
from jormungandr import i_manager, timezone
from jormungandr.interfaces.parsers import DateTimeFormat, default_count_arg_type
from jormungandr.interfaces.v1.decorators import get_obj_serializer
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.v1.serializer import api
from jormungandr.resources_utils import ResourceUtc
from jormungandr.utils import date_to_timestamp

from flask.globals import g
from datetime import datetime
import six


class LineReports(ResourceUri, ResourceUtc):
    def __init__(self):
        ResourceUri.__init__(self, output_type_serializer=api.LineReportsSerializer)
        ResourceUtc.__init__(self)
        parser_get = self.parsers["get"]
        parser_get.add_argument("depth", type=int, default=1, help="The depth of your object")
        parser_get.add_argument("count", type=default_count_arg_type, default=25,
                                help="Number of objects per page")
        parser_get.add_argument("start_page", type=int, default=0,
                                help="The current page")
        parser_get.add_argument("_current_datetime", type=DateTimeFormat(),
                                schema_metadata={'default': 'now'}, hidden=True,
                                default=datetime.utcnow(),
                                help='The datetime considered as "now". Used for debug, default is '
                                     'the moment of the request. It will mainly change the output '
                                     'of the disruptions.')
        parser_get.add_argument("forbidden_uris[]", type=six.text_type,
                                help="forbidden uris",
                                dest="forbidden_uris[]",
                                default=[],
                                action="append",
                                schema_metadata={'format': 'pt-object'})
        parser_get.add_argument("disable_geojson", type=BooleanType(), default=False,
                                help="remove geojson from the response")
        parser_get.add_argument("since", type=DateTimeFormat(),
                                help="use disruptions valid after this date")
        parser_get.add_argument("until", type=DateTimeFormat(),
                                help="use disruptions valid before this date")

        #self.collection = 'line_reports'
        #self.collections = api.LineReportsSerializer
        self.get_decorators.insert(0, ManageError())
        self.get_decorators.insert(1, get_obj_serializer(self))

    def options(self, **kwargs):
        return self.api_description(**kwargs)

    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()

        if args['disable_geojson']:
            g.disable_geojson = True

        if uri:
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            args["filter"] = self.get_filter(uris, args)
        else:
            args["filter"] = ""

        if args['since']:
            args['since'] = date_to_timestamp(self.convert_to_utc(args['since']))
        if args['until']:
            args['until'] = date_to_timestamp(self.convert_to_utc(args['until']))

        response = i_manager.dispatch(args, "line_reports", instance_name=self.region)

        return response
