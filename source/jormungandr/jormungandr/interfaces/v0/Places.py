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
from flask.ext.restful import Resource, fields, abort
from jormungandr import i_manager
from jormungandr.protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from jormungandr.interfaces.parsers import depth_argument
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.authentification import authentification_required


class Places(Resource):

    """Retreives places"""
    method_decorators = [authentification_required]

    def __init__(self):
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                                         description="The data to search")
        self.parsers["get"].add_argument("type[]", type=str, action="append",
                                         default=["stop_area", "address",
                                                  "poi",
                                                  "administrative_region"],
                                         description=
                                         "The type of data to search")
        self.parsers["get"].add_argument("count", type=int, default=10,
                                         description=
                                         "The maximum number of places returned")
        self.parsers["get"].add_argument("search_type", type=int, default=0,
                                         description=
                                         "Type of search: firstletter or typeerror")
        self.parsers["get"].add_argument("admin_uri[]", type=str,
                                         action="append",
                                         description=
                                         "Restraine the search to the given admin uris")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description=
                                         "The depth of the objects")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        if len(args['q']) == 0:
            abort(400, message="Search word absent")
        response = i_manager.dispatch(args, "places", instance_name=region)
        return protobuf_to_dict(response, use_enum_labels=True), 200
