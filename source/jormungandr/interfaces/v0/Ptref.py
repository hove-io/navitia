from flask import Flask
from flask.ext.restful import Resource, fields
from instance_manager import NavitiaManager
from protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from interfaces.parsers import depth_argument

class Ptref(Resource):
    def __init__(self):
        self.resource_type = "Unknown"
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("start_page", type=int, default=0)
        self.parser.add_argument("count", type=int, default=25)
        self.parser.add_argument("filter", type=str, default = "")
        self.parser.add_argument("depth", type=depth_argument, default=1)

    def get(self, region):
        args = self.parser.parse_args()
        response = NavitiaManager().dispatch(args, region, self.resource_type)
        return protobuf_to_dict(response), 200

class StopAreas(Ptref):
    def __init__(self):
        super(StopAreas, self).__init__()
        self.resource_type = "stop_areas"

class StopPoints(Ptref):
    def __init__(self):
        super(StopPoints, self).__init__()
        self.resource_type = "stop_points"

class Lines(Ptref):
    def __init__(self):
        super(Lines, self).__init__()
        self.resource_type = "lines"

class Routes(Ptref):
    def __init__(self):
        super(Routes, self).__init__()
        self.resource_type = "routes"

class PhysicalModes(Ptref):
    def __init__(self):
        super(PhysicalModes, self).__init__()
        self.resource_type = "physical_modes"

class CommercialModes(Ptref):
    def __init__(self):
        super(CommercialModes, self).__init__()
        self.resource_type = "commercial_modes"

class Connections(Ptref):
    def __init__(self):
        super(Connections, self).__init__()
        self.resource_type = "connections"

class JourneyPatternPoints(Ptref):
    def __init__(self):
        super(JourneyPatternPoints, self).__init__()
        self.resource_type = "journey_pattern_points"

class JourneyPatterns(Ptref):
    def __init__(self):
        super(JourneyPatterns, self).__init__()
        self.resource_type = "journey_patterns"

class Companies(Ptref):
    def __init__(self):
        super(Companies, self).__init__()
        self.resource_type = "companies"

class VehicleJourneys(Ptref):
    def __init__(self):
        super(VehicleJourneys, self).__init__()
        self.resource_type = "vehicle_journeys"

class Pois(Ptref):
    def __init__(self):
        super(Pois, self).__init__()
        self.resource_type = "pois"

class PoiTypes(Ptref):
    def __init__(self):
        super(PoiTypes, self).__init__()
        self.resource_type = "poi_types"
