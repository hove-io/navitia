# coding=utf-8
from flask import Flask
from flask.ext.restful import Resource, fields
from instance_manager import NavitiaManager
from protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from interfaces.parsers import depth_argument
from interfaces.argument import ArgumentDoc


class TimeTables(Resource):
    def __init__(self):
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("from_datetime", type=str, required=True,
               description=" The date from which you want the times")
        self.parsers["get"].add_argument("duration", type=int, default=86400,
               description="""Maximum duration between the datetime and the
                              last retrieved stop time""")
        self.parsers["get"].add_argument("nb_stoptimes", type=int, default=100,
               description="Maximum number of stop times")
        self.parsers["get"].add_argument("depth", type=depth_argument, default=1,
               description="Maximal depth of the returned objects")
        self.parsers["get"].add_argument("interface_version", type=depth_argument,
                                 default=0, hidden=True)

    def get(self, region):
        args = self.parsers["get"].parse_args()
        response = NavitiaManager().dispatch(args, region, self.api)
        return protobuf_to_dict(response, use_enum_labels=True), 200

class NextDepartures(TimeTables):
    """Retrieves the next departures"""
    def __init__(self):
        super(NextDepartures, self).__init__()
        self.parsers["get"].add_argument("filter", "", type=str,
                description="Filter to have the times you want")
        self.api = "next_departures"

class NextArrivals(TimeTables):
    """Retrieves the next arrivals"""
    def __init__(self):
        TimeTables.__init__(self)
        self.parsers["get"].add_argument("filter", "", type=str,
                description="Filter to have the times you want")
        self.api = "next_arrivals"

class DepartureBoards(TimeTables):
    """Compute departure boards"""
    def __init__(self):
        TimeTables.__init__(self)
        self.parsers["get"].add_argument("filter", "", type=str,
                description="Filter to have the times you want")
        self.api = "departure_boards"

class RouteSchedules(TimeTables):
    """Compute route schedules"""
    def __init__(self):
        super(RouteSchedules, self).__init__()
        self.parsers["get"].add_argument("filter", "", type=str,
                description="Filter to have the times you want")
        self.api = "route_schedules"

class StopsSchedules(TimeTables):
    """Compute stop schedules"""
    def __init__(self):
        super(StopsSchedules, self).__init__()
        self.parsers["get"].add_argument("departure_filter", "", type=str,
                required=True,
                description="The filter of your departure point")
        self.parsers["get"].add_argument("arrival_filter", "", type=str,
                required=True,
                description="The filter of your departure point")
        self.api = "stops_schedules"
