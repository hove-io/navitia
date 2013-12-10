# coding=utf-8
from flask import Flask
from flask.ext.restful import Resource, fields
from jormungandr.instance_manager import InstanceManager
from .protobuf_to_dict import protobuf_to_dict
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
                                         description="The page where you\
                                         want to start")
        self.parsers["get"].add_argument("count", type=int, default=25,
                                         description="The number of objects\
                                         you want on the page")
        self.parsers["get"].add_argument("filter", type=str, default="",
                                         description="The filter parameter")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="The depth of your\
                                         object")
        self.parsers["get"].add_argument("forbidden_uris[]", type=unicode,
                description="uri to forbid")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        response = InstanceManager().dispatch(args, region, self.resource_type)
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
