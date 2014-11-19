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

from flask.ext.restful import marshal_with, reqparse
from jormungandr import i_manager, timezone
from fields import PbField, error, network, line,\
    NonNullList, NonNullNested, pagination, stop_area
from ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.common import odt_levels
from jormungandr.interfaces.parsers import option_value
from errors import ManageError
from datetime import datetime

disruption = {
    "network": PbField(network, attribute='network'),
    "lines": NonNullList(NonNullNested(line)),
    "stop_areas": NonNullList(NonNullNested(stop_area))
}

disruptions = {
    "disruptions": NonNullList(NonNullNested(disruption)),
    "error": PbField(error, attribute='error'),
    "pagination": NonNullNested(pagination)
}


class Disruptions(ResourceUri):
    def __init__(self):
        ResourceUri.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("depth", type=int, default=1)
        parser_get.add_argument("count", type=int, default=10,
                                description="Number of disruptions per page")
        parser_get.add_argument("start_page", type=int, default=0,
                                description="The current page")
        parser_get.add_argument("period", type=int, default=365,
                                description="Period from datetime")
        parser_get.add_argument("datetime", type=str,
                                description="The datetime from which you want\
                                the disruption")
        parser_get.add_argument("forbidden_id[]", type=unicode,
                                description="forbidden ids",
                                dest="forbidden_uris[]",
                                action="append")

    @marshal_with(disruptions)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()

        if not args["datetime"]:
            args["datetime"] = datetime.now().strftime("%Y%m%dT1337")

        if(uri):
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            args["filter"] = self.get_filter(uris)
        else:
            args["filter"] = ""

        response = i_manager.dispatch(args, "disruptions",
                                      instance_name=self.region)
        return response
