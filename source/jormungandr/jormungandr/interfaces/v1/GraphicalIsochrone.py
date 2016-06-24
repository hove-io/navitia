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
from jormungandr.interfaces.v1.fields import error,\
    PbField, NonNullList, NonNullNested,\
    feed_publisher, Links, MultiPolyGeoJson, place, NonNullString
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
from jormungandr.interfaces.v1.journey_common import JourneyCommon, dt_represents, compute_possible_region


graphical_isochrone = {
    "geojson": MultiPolyGeoJson(),
    "max_duration": fields.Integer(),
    "min_duration": fields.Integer(),
    'from': PbField(place, attribute='origin'),
    "to": PbField(place, attribute="destination"),
}


graphical_isochrones = {
    "isochrones": NonNullList(NonNullNested(graphical_isochrone), attribute="graphical_isochrones"),
    "error": PbField(error, attribute='error'),
    "feed_publishers": fields.List(NonNullNested(feed_publisher)),
    "links": fields.List(Links()),
}


class GraphicalIsochrone(JourneyCommon):

    def __init__(self):
        super(GraphicalIsochrone, self).__init__()
        parser_get = self.parsers["get"]
        parser_get.add_argument("min_duration", type=unsigned_integer, default=0)

    @marshal_with(graphical_isochrones)
    @ManageError()
    def get(self, region=None, uri=None):

        args = self.parsers['get'].parse_args()
        self.region = i_manager.get_region(region)
        args.update(self.parse_args(region, uri))

        if not (args['destination'] or args['origin']):
            abort(400, message="you should provide a 'from' or a 'to' argument")
        if not args['max_duration']:
            abort(400, message="you should provide a 'max_duration' argument")
        if args['destination'] and args['origin']:
            abort(400, message="you cannot provide a 'from' and a 'to' argument")

        set_request_timezone(self.region)
        original_datetime = args['original_datetime']
        new_datetime = self.convert_to_utc(original_datetime)
        args['datetime'] = date_to_timestamp(new_datetime)

        response = i_manager.dispatch(args, "graphical_isochrones", self.region)

        return response
