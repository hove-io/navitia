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


class PlacesNearby(Resource):

    """Retreives places nearby a point"""
    method_decorators = [authentification_required]

    def __init__(self):
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("uri", type=str, required=True,
                                         description=
                                         """uri around which you want to look for objects.
                                Not all objects make sense (e.g. a mode). """)
        self.parsers["get"].add_argument("type[]", type=str,
                                         action="append",
                                         default=["stop_area", "stop_point",
                                                  "address", "poi"],
                                         description=
                                         "Type of the objects to return")
        self.parsers["get"].add_argument("distance", type=int, default=500,
                                         description=
                                         "Distance range of the query")
        self.parsers["get"].add_argument("filter", type=str, default="",
                                         description="Filter places")
        self.parsers["get"].add_argument("count", type=int, default=10,
                                         description=
                                         "Number of elements per page")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description=
                                         "Maximum depth on objects")
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                                         description=
                                        "The page number of the ptref result")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        response = i_manager.dispatch(args, "places_nearby",
                                      instance_name=region)
        return protobuf_to_dict(response, use_enum_labels=True), 200
