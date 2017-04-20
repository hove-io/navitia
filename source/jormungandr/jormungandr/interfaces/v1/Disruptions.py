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
from flask.ext.restful import marshal_with, reqparse, fields
from flask.ext.restful.inputs import boolean
from flask.globals import g
from jormungandr import i_manager, timezone
from jormungandr.interfaces.v1.fields import PbField, error, network, line,\
    NonNullList, NonNullNested, pagination, stop_area
from jormungandr.interfaces.v1.VehicleJourney import vehicle_journey
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import date_time_format, default_count_arg_type
from jormungandr.interfaces.v1.errors import ManageError
from datetime import datetime
from jormungandr.interfaces.v1.fields import disruption_marshaller

disruption = {
    "network": PbField(network, attribute='network'),
    "lines": NonNullList(NonNullNested(line)),
    "stop_areas": NonNullList(NonNullNested(stop_area)),
    "vehicle_journeys": NonNullList(NonNullNested(vehicle_journey))
}

traffic = {
    "traffic_reports": NonNullList(NonNullNested(disruption)),
    "error": PbField(error, attribute='error'),
    "pagination": NonNullNested(pagination),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
}


class TrafficReport(ResourceUri):
    def __init__(self):
        ResourceUri.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("depth", type=int, default=1)
        parser_get.add_argument("count", type=default_count_arg_type, default=10,
                                description="Number of disruptions per page")
        parser_get.add_argument("start_page", type=int, default=0,
                                description="The current page")
        parser_get.add_argument("_current_datetime", type=date_time_format, default=datetime.utcnow(),
                                description="The datetime we want to publish the disruptions from."
                                            " Default is the current date and it is mainly used for debug.")
        parser_get.add_argument("forbidden_id[]", type=unicode,
                                description="DEPRECATED, replaced by forbidden_uris[]",
                                dest="__temporary_forbidden_id[]",
                                default=[],
                                action="append")
        parser_get.add_argument("forbidden_uris[]", type=unicode,
                                description="forbidden uris",
                                dest="forbidden_uris[]",
                                default=[],
                                action="append")
        parser_get.add_argument("distance", type=int, default=200,
                                description="Distance range of the query. Used only if a coord is in the query")
        parser_get.add_argument("disable_geojson", type=boolean, default=False,
                            description="remove geojson from the response")
        self.collection = 'traffic_reports'

    @marshal_with(traffic)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()

        if args['disable_geojson']:
            g.disable_geojson = True

        # for retrocompatibility purpose
        for forbid_id in args['__temporary_forbidden_id[]']:
            args['forbidden_uris[]'].append(forbid_id)

        if uri:
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            args["filter"] = self.get_filter(uris, args)
        else:
            args["filter"] = ""

        response = i_manager.dispatch(args, "traffic_reports", instance_name=self.region)

        return response
