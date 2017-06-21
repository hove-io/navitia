# coding=utf-8

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
from flask.ext.restful import marshal_with, reqparse
from jormungandr import i_manager
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import default_count_arg_type, date_time_format
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.interfaces.v1.fields import fields, enum_type, NonNullList,\
    NonNullNested, NonNullProtobufNested, PbField, error, pagination, NonNullString
import datetime
import six


week_pattern = {
    "monday": fields.Boolean(),
    "tuesday": fields.Boolean(),
    "wednesday": fields.Boolean(),
    "thursday": fields.Boolean(),
    "friday": fields.Boolean(),
    "saturday": fields.Boolean(),
    "sunday": fields.Boolean(),
}

calendar_period = {
    "begin": fields.String(),
    "end": fields.String(),
}

calendar_exception = {
    "datetime": fields.String(attribute="date"),
    "type": enum_type(),
}
validity_pattern = {
    'beginning_date': fields.String(),
    'days': fields.String(),
}
calendar = {
    "id": NonNullString(attribute="uri"),
    "name": NonNullString(),
    "week_pattern": NonNullNested(week_pattern),
    "active_periods": NonNullList(NonNullNested(calendar_period)),
    "exceptions": NonNullList(NonNullNested(calendar_exception)),
    "validity_pattern": NonNullProtobufNested(validity_pattern)
}

calendars = {
    "calendars": NonNullList(NonNullNested(calendar)),
    "error": PbField(error, attribute='error'),
    "pagination": NonNullNested(pagination)
}


class Calendars(ResourceUri):
    def __init__(self):
        ResourceUri.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("depth", type=int, default=1)
        parser_get.add_argument("count", type=default_count_arg_type, default=10,
                                description="Number of calendars per page")
        parser_get.add_argument("start_page", type=int, default=0,
                                description="The current page")
        parser_get.add_argument("start_date", type=six.text_type, default="",
                                description="Start date")
        parser_get.add_argument("end_date", type=six.text_type, default="",
                                description="End date")
        parser_get.add_argument("forbidden_id[]", type=six.text_type,
                                description="DEPRECATED, replaced by forbidden_uris[]",
                                dest="__temporary_forbidden_id[]",
                                default=[],
                                action='append')
        parser_get.add_argument("forbidden_uris[]", type=six.text_type,
                                description="forbidden uris",
                                dest="forbidden_uris[]",
                                default=[],
                                action='append')
        parser_get.add_argument("distance", type=int, default=200,
                                description="Distance range of the query. Used only if a coord is in the query")

        self.parsers["get"].add_argument("_current_datetime", type=date_time_format, default=datetime.datetime.utcnow(),
                                         description="The datetime used to consider the state of the pt object"
                                                     " Default is the current date and it is used for debug."
                                                     " Note: it will mainly change the disruptions that concern "
                                                     "the object The timezone should be specified in the format,"
                                                     " else we consider it as UTC")

    @marshal_with(calendars)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None, id=None):
        self.region = i_manager.get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()

        # for retrocompatibility purpose
        for forbid_id in args['__temporary_forbidden_id[]']:
            args['forbidden_uris[]'].append(forbid_id)

        if id:
            args["filter"] = "calendar.uri=" + id
        elif uri:
            # Calendars of line
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            args["filter"] = self.get_filter(uris, args)
        else:
            args["filter"] = ""

        self._register_interpreted_parameters(args)
        response = i_manager.dispatch(args, "calendars",
                                      instance_name=self.region)
        return response
