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

from flask import Flask
from flask.ext.restful import Resource, fields
from jormungandr import i_manager
from jormungandr.protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from jormungandr.interfaces.parsers import depth_argument
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.authentification import authentification_required


class TimeTables(Resource):
    method_decorators = [authentification_required]

    def __init__(self):
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("from_datetime", type=str,
                                required=True,
                                description=" The date from which you\
                                want the times")
        parser_get.add_argument("duration", type=int, default=86400,
                                description=
                                "Maximum duration between "\
                                "the datetime and the "\
                                "last retrieved stop time")
        parser_get.add_argument("nb_stoptimes", type=int, default=100,
                                description=
                                "Maximum number of stop times",
                                dest="max_date_times")
        parser_get.add_argument("depth", type=depth_argument, default=1,
                                description=
                                "Maximal depth of the returned objects")
        parser_get.add_argument("forbidden_uris[]", type=unicode,
                                action="append",
                                description="Uri to forbid")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        args["interface_version"] = 0
        response = i_manager.dispatch(args, self.api, instance_name=region)
        return protobuf_to_dict(response, use_enum_labels=True), 200


class NextDepartures(TimeTables):

    """Retrieves the next departures"""

    def __init__(self):
        super(NextDepartures, self).__init__()
        self.parsers["get"].add_argument("filter", "", type=str,
                                         description="Filter to have the times\
                                         you want")
        self.api = "next_departures"


class NextArrivals(TimeTables):

    """Retrieves the next arrivals"""

    def __init__(self):
        TimeTables.__init__(self)
        self.parsers["get"].add_argument("filter", "", type=str,
                                         description="Filter to have the times\
                                         you want")
        self.api = "next_arrivals"


class DepartureBoards(TimeTables):

    """Compute departure boards"""

    def __init__(self):
        TimeTables.__init__(self)
        self.parsers["get"].add_argument("filter", "", type=str,
                                         description="Filter to have the times\
                                         you want")
        self.api = "departure_boards"


class RouteSchedules(TimeTables):

    """Compute route schedules"""

    def __init__(self):
        super(RouteSchedules, self).__init__()
        self.parsers["get"].add_argument("filter", "", type=str,
                                         description=
                                         "Filter to have the times you want")
        self.api = "route_schedules"


class StopsSchedules(TimeTables):

    """Compute stop schedules"""

    def __init__(self):
        super(StopsSchedules, self).__init__()
        self.parsers["get"].add_argument("departure_filter", "", type=str,
                                         required=True,
                                         description="The filter of your "\
                                         "departure point")
        self.parsers["get"].add_argument("arrival_filter", "", type=str,
                                         required=True,
                                         description="The filter of your "\
                                         "departure point")
        self.api = "stops_schedules"
