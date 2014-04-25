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
from jormungandr.interfaces.parsers import depth_argument
from jormungandr.authentification import authentification_required


class Ptref(Resource):
    parsers = {}
    method_decorators = [authentification_required]

    def __init__(self):
        super(Ptref, self).__init__()
        self.resource_type = "Unknown"
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                                         description=
                                         "The page where want to start")
        self.parsers["get"].add_argument("count", type=int, default=25,
                                         description=
                                         "The number of objects you want on the page")
        self.parsers["get"].add_argument("filter", type=str, default="",
                                         description="The filter parameter")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description=
                                         "The depth of object")
        self.parsers["get"].add_argument("forbidden_uris[]", type=unicode,
                                         action="append",
                                         description="Uri to forbid")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        response = i_manager.dispatch(args, self.resource_type,
                                      instance_name=region)
        return protobuf_to_dict(response, use_enum_labels=True), 200


class StopAreas(Ptref):

    """ Retrieves all the stop areas of a region """

    def __init__(self):
        super(StopAreas, self).__init__()
        self.resource_type = "stop_areas"


class StopPoints(Ptref):

    """ Retrieves all the stop points of a region """

    def __init__(self):
        super(StopPoints, self).__init__()
        self.resource_type = "stop_points"


class Lines(Ptref):

    """ Retrieves all the lines of a region """

    def __init__(self):
        super(Lines, self).__init__()
        self.resource_type = "lines"


class Routes(Ptref):

    """ Retrieves all the routes of a region """

    def __init__(self):
        super(Routes, self).__init__()
        self.resource_type = "routes"


class PhysicalModes(Ptref):

    """ Retrieves all the physical modes of a region """

    def __init__(self):
        super(PhysicalModes, self).__init__()
        self.resource_type = "physical_modes"


class CommercialModes(Ptref):

    """ Retrieves all the commercial modes of a region """

    def __init__(self):
        super(CommercialModes, self).__init__()
        self.resource_type = "commercial_modes"


class Connections(Ptref):

    """ Retrieves all the connections of a region """

    def __init__(self):
        super(Connections, self).__init__()
        self.resource_type = "connections"


class JourneyPatternPoints(Ptref):

    """ Retrieves all the journey pattern points of a region """

    def __init__(self):
        super(JourneyPatternPoints, self).__init__()
        self.resource_type = "journey_pattern_points"


class JourneyPatterns(Ptref):

    """ Retrieves all the journey patterns of a region """

    def __init__(self):
        super(JourneyPatterns, self).__init__()
        self.resource_type = "journey_patterns"


class Companies(Ptref):

    """ Retrieves all the companies of a region """

    def __init__(self):
        super(Companies, self).__init__()
        self.resource_type = "companies"


class VehicleJourneys(Ptref):

    """ Retrieves all the vehicle journeys of a region """

    def __init__(self):
        super(VehicleJourneys, self).__init__()
        self.resource_type = "vehicle_journeys"


class Pois(Ptref):

    """ Retrieves all the pois of a region """

    def __init__(self):
        super(Pois, self).__init__()
        self.resource_type = "pois"


class PoiTypes(Ptref):

    """ Retrieves all the poi types of a region """

    def __init__(self):
        super(PoiTypes, self).__init__()
        self.resource_type = "poi_types"


class Networks(Ptref):

    """ Retrieves all the networks of a region """

    def __init__(self):
        super(Networks, self).__init__()
        self.resource_type = "networks"
