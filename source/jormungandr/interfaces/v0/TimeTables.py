from flask import Flask
from flask.ext.restful import Resource, fields
from instance_manager import NavitiaManager
from protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from interfaces.parsers import depth_argument
class TimeTables(Resource):
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("from_datetime", type=str, required=True)
        self.parser.add_argument("duration", type=int, default=3600)
        self.parser.add_argument("nb_stoptimes", type=int, default=100)
        self.parser.add_argument("depth", type=depth_argument, default=1)
        self.parser.add_argument("interface_version", type=depth_argument,
                                 default=0)

    def get(self, region):
        args = self.parser.parse_args()
        response = NavitiaManager().dispatch(args, region, self.api)
        return protobuf_to_dict(response), 200

class NextDepartures(TimeTables):
    def __init__(self):
        super(NextDepartures, self).__init__()
        self.parser.add_argument("filter", "", type=str)
        self.api = "next_departures"

class NextArrivals(TimeTables):
    def __init__(self):
        super(NextArrivals, self).__init__()
        self.parser.add_argument("filter", "", type=str)
        self.api = "next_arrivals"

class DepartureBoards(TimeTables):
    def __init__(self):
        super(DepartureBoards, self).__init__()
        self.parser.add_argument("filter", "", type=str)
        self.api = "departure_boards"

class RouteSchedules(TimeTables):
    def __init__(self):
        super(RouteSchedules, self).__init__()
        self.parser.add_argument("filter", "", type=str)
        self.api = "route_schedules"

class StopsSchedules(TimeTables):
    def __init__(self):
        super(StopsSchedules, self).__init__()
        self.parser.add_argument("departure_filter", "", type=str, required=True)
        self.parser.add_argument("arrival_filter", "", type=str, required=True)
        self.api = "stops_schedules"
