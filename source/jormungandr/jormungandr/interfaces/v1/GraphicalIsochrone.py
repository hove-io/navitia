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
from flask.ext.restful import fields, reqparse, marshal_with, abort
from jormungandr import i_manager
from jormungandr.interfaces.v1.fields import Links, MultiPolyGeoJson
from jormungandr.interfaces.v1.fields import error,\
    PbField, NonNullList, NonNullNested,\
    feed_publisher
from jormungandr.interfaces.parsers import date_time_format
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.timezone import set_request_timezone
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.interfaces.argument import ArgumentDoc
from datetime import datetime
from jormungandr.utils import date_to_timestamp
from jormungandr.resources_utc import ResourceUtc
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.interfaces.parsers import option_value
from jormungandr.interfaces.parsers import float_gt_0
from jormungandr.interfaces.v1.Journeys import dt_represents
from jormungandr.interfaces.parsers import unsigned_integer


graphical_isochrone = {
    "geojson": MultiPolyGeoJson(),
}


graphical_isochrones = {
    "isochrones": NonNullList(NonNullNested(graphical_isochrone), attribute="graphical_isochrones"),
    "error": PbField(error, attribute='error'),
    "feed_publishers": fields.List(NonNullNested(feed_publisher)),
    "links": fields.List(Links()),
}


class GraphicalIsochrone(ResourceUri, ResourceUtc):

    def __init__(self):
        ResourceUri.__init__(self, authentication=False)
        ResourceUtc.__init__(self)

        modes = ["walking", "car", "bike", "bss"]
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]

        parser_get.add_argument("from", type=unicode, dest="origin")
        parser_get.add_argument("first_section_mode[]",
                                type=option_value(modes),
                                dest="origin_mode", action="append")
        parser_get.add_argument("last_section_mode[]",
                                type=option_value(modes),
                                dest="destination_mode", action="append")
        parser_get.add_argument("to", type=unicode, dest="destination")
        parser_get.add_argument("datetime", type=date_time_format)
        parser_get.add_argument("max_duration", type=unsigned_integer)
        parser_get.add_argument("min_duration", type=unsigned_integer, default=0)
        parser_get.add_argument("forbidden_uris[]", type=unicode, action="append")
        parser_get.add_argument("max_transfers", type=int, default=42)
        parser_get.add_argument("_current_datetime", type=date_time_format, default=datetime.utcnow(),
                                description="The datetime used to consider the state of the pt object"
                                            " Default is the current date and it is used for debug."
                                            " Note: it will mainly change the disruptions that concern "
                                            "the object The timezone should be specified in the format,"
                                            " else we consider it as UTC")
        parser_get.add_argument("max_walking_duration_to_pt", type=int,
                                description="maximal duration of walking on public transport in second")
        parser_get.add_argument("max_bike_duration_to_pt", type=int,
                                description="maximal duration of bike on public transport in second")
        parser_get.add_argument("max_bss_duration_to_pt", type=int,
                                description="maximal duration of bss on public transport in second")
        parser_get.add_argument("max_car_duration_to_pt", type=int,
                                description="maximal duration of car on public transport in second")
        parser_get.add_argument("walking_speed", type=float_gt_0)
        parser_get.add_argument("bike_speed", type=float_gt_0)
        parser_get.add_argument("bss_speed", type=float_gt_0)
        parser_get.add_argument("car_speed", type=float_gt_0)

    @marshal_with(graphical_isochrones)
    @ManageError()
    def get(self, region=None):
        args = self.parsers['get'].parse_args()

        if args.get('origin_mode') is None:
            args['origin_mode'] = ['walking']
        if args.get('destination_mode') is None:
            args['destination_mode'] = ['walking']

        self.region = i_manager.get_region(region)
        if args['origin']:
            args['origin'] = transform_id(args['origin'])
        if args['destination']:
            args['destination'] = transform_id(args['destination'])
        if not (args['destination'] or args['origin']):
            abort(400, message="you should provide a 'from' or a 'to' argument")
        if not args['max_duration']:
            abort(400, message="you should provide a 'max_duration' argument")
        if not args['datetime']:
            args['datetime'] = args['_current_datetime']

        set_request_timezone(self.region)
        args['original_datetime'] = args['datetime']
        original_datetime = args['original_datetime']
        new_datetime = self.convert_to_utc(original_datetime)
        args['datetime'] = date_to_timestamp(new_datetime)

        response = i_manager.dispatch(args, "graphical_isochrones", instance_name=region)

        return response
