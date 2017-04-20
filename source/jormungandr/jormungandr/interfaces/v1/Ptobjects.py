# coding=utf-8

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

from __future__ import absolute_import, print_function, unicode_literals, division
from flask.ext.restful import fields, marshal_with, reqparse, abort
from flask.ext.restful.inputs import boolean
from flask.globals import g
from jormungandr import i_manager, timezone
from jormungandr.interfaces.v1.fields import disruption_marshaller
from jormungandr.interfaces.v1.fields import NonNullList, NonNullNested, PbField, error, pt_object, feed_publisher
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument, option_value, default_count_arg_type, date_time_format
import datetime

pt_objects = {
    "pt_objects": NonNullList(NonNullNested(pt_object), attribute='places'),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
    "error": PbField(error, attribute='error'),
    "feed_publishers": fields.List(NonNullNested(feed_publisher))
}

pt_object_type_values = ["network", "commercial_mode", "line", "line_group", "route", "stop_area"]


class Ptobjects(ResourceUri):

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                                         description="The data to search")
        self.parsers["get"].add_argument("type[]", type=option_value(pt_object_type_values),
                                         action="append",default=pt_object_type_values,
                                         description="The type of data to\
                                         search")
        self.parsers["get"].add_argument("count", type=default_count_arg_type, default=10,
                                         description="The maximum number of\
                                         ptobjects returned")
        self.parsers["get"].add_argument("search_type", type=int, default=0,
                                         description="Type of search:\
                                         firstletter or type error")
        self.parsers["get"].add_argument("admin_uri[]", type=unicode,
                                         action="append",
                                         description="If filled, will\
                                         restrained the search within the\
                                         given admin uris")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="The depth of objects")
        self.parsers["get"].add_argument("_current_datetime", type=date_time_format, default=datetime.datetime.utcnow(),
                                         description="The datetime used to consider the state of the pt object"
                                                     " Default is the current date and it is used for debug."
                                                     " Note: it will mainly change the disruptions that concern "
                                                     "the object The timezone should be specified in the format,"
                                                     " else we consider it as UTC")
        self.parsers['get'].add_argument("disable_geojson", type=boolean, default=False,
                            description="remove geojson from the response")

    @marshal_with(pt_objects)
    def get(self, region=None, lon=None, lat=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()
        if args['disable_geojson']:
            g.disable_geojson = True
        self._register_interpreted_parameters(args)
        if len(args['q']) == 0:
            abort(400, message="Search word absent")
        response = i_manager.dispatch(args, "pt_objects",
                                      instance_name=self.region)
        return response, 200

