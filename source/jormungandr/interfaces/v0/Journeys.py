# coding=utf-8
from flask import Flask, request
from flask.ext.restful import Resource, fields
from instance_manager import NavitiaManager
from interfaces.parsers import true_false, option_value
from protobuf_to_dict import protobuf_to_dict
from find_extrem_datetimes import extremes
from flask.ext.restful import reqparse
from interfaces.argument import ArgumentDoc
from interfaces.parsers import depth_argument

class Journeys(Resource):
    """ Compute journeys"""
    parsers = {}
    def __init__(self, *args, **kwargs):
        types = ["all", "rapid"]
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("origin", type=str, required=True,
                description="Origin's uri of your journey")
        parser_get.add_argument("destination", type=str, required=True,
                description="Destination's uri of your journey")
        parser_get.add_argument("datetime", type=str, required=True,
                description="Datetime's uri of your journey")
        parser_get.add_argument("clockwise", type=true_false, default=True,
                description="""Whether you want your journey to start after
                               datetime or to end before datetime""")
        parser_get.add_argument("max_duration", type=int, default=36000,
                description="Maximum duration of your journey")
        parser_get.add_argument("max_transfers", type=int, default=10,
                description="Maximum transfers you want in your journey")
        parser_get.add_argument("origin_mode",
                                type=option_value(["walking", "car", "bike", "br"]),
                                action="append", default="walking",
                description="""The list of modes you want at the beggining of
                               your journey""")
        parser_get.add_argument("destination_mode",
                                type=option_value(["walking", "car", "bike", "br"]),
                                default="walking",
                description="""The list of modes you want at the end of
                               your journey""")
        parser_get.add_argument("walking_speed", type=float, default=1.68,
                description="Walking speed in meter/second")
        parser_get.add_argument("walking_distance", type=int, default=1000,
                description="Maximum walking distance")
        parser_get.add_argument("bike_speed", type=float, default=8.8,
                description="Bike speed in meter/second")
        parser_get.add_argument("bike_distance", type=int, default=5000,
                description="Maximum bike distance")
        parser_get.add_argument("br_speed", type=float, default=8.8,
                description="Bike rental speed in meter/second")
        parser_get.add_argument("br_distance", type=int, default=5000,
                description="Maximum distance for bike rental mode")
        parser_get.add_argument("car_speed", type=float, default=16.8,
                description="Car speed in meter/second")
        parser_get.add_argument("car_distance", type=int, default=15000,
                description="Maximum car distance")
        parser_get.add_argument("forbidden_uris[]", type=str, action="append",
                description="Uri you want to forbid")
        parser_get.add_argument("type", type=option_value(types), default="all")
        parser_get.add_argument("count", type=int)

    def get(self, region):
        args = self.parsers["get"].parse_args()
        response = NavitiaManager().dispatch(args, region, "journeys")
        if response.journeys:
            (before, after) = extremes(response, request)
            if before and after:
                response.prev = before
                response.next = after
        return protobuf_to_dict(response, use_enum_labels=True), 200

class Isochrone(Resource):
    """ Compute isochrones """
    def __init__(self):
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser()
        parser_get = self.parsers["get"]
        parser_get.add_argument("origin", type=str, required=True)
        parser_get.add_argument("datetime", type=str, required=True)
        parser_get.add_argument("clockwise", type=true_false, default=True)
        parser_get.add_argument("max_duration", type=int, default=3600)
        parser_get.add_argument("max_transfers", type=int, default=10)
        parser_get.add_argument("origin_mode",
                      type=option_value(["walking", "car", "bike", "br"]),
                      action="append", default="walking")
        parser_get.add_argument("walking_speed", type=float, default=1.68)
        parser_get.add_argument("walking_distance", type=int, default=1000)
        parser_get.add_argument("bike_speed", type=float, default=8.8)
        parser_get.add_argument("bike_distance", type=int, default=5000)
        parser_get.add_argument("br_speed", type=float, default=8.8)
        parser_get.add_argument("br_distance", type=int, default=5000)
        parser_get.add_argument("car_speed", type=float, default=16.8)
        parser_get.add_argument("car_distance", type=int, default=15000)
        parser_get.add_argument("forbidden_uris[]", type=str, action="append")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        response = NavitiaManager().dispatch(args, region, "isochrone")
        if response.journeys:
            (before, after) = extremes(response, request)
            if before and after:
                response.prev = before
                response.next = after
        return protobuf_to_dict(response, use_enum_labels=True), 200
