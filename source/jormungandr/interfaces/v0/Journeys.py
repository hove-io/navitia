from flask import Flask, request
from flask.ext.restful import Resource, fields
from instance_manager import NavitiaManager
from interfaces.parsers import true_false, option_value
from protobuf_to_dict import protobuf_to_dict
from find_extrem_datetimes import extremes
from flask.ext.restful import reqparse

class Journeys(Resource):
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("origin", type=str, required=True)
        self.parser.add_argument("destination", type=str, required=True)
        self.parser.add_argument("datetime", type=str, required=True)
        self.parser.add_argument("clockwise", type=true_false, default=True)
        self.parser.add_argument("max_duration", type=int, default=36000)
        self.parser.add_argument("max_transfers", type=int, default=10)
        self.parser.add_argument("origin_mode",
                                 type=option_value(["walking", "car", "bike", "br"]),
                                 action="append", default="walking")
        self.parser.add_argument("destination_mode",
                                 type=option_value(["walking", "car", "bike", "br"]),
                                  default="walking")
        self.parser.add_argument("walking_speed", type=float, default=1.68)
        self.parser.add_argument("walking_distance", type=int, default=1000)
        self.parser.add_argument("bike_speed", type=float, default=8.8)
        self.parser.add_argument("bike_distance", type=int, default=5000)
        self.parser.add_argument("br_speed", type=float, default=8.8)
        self.parser.add_argument("br_distance", type=int, default=5000)
        self.parser.add_argument("car_speed", type=float, default=16.8)
        self.parser.add_argument("car_distance", type=int, default=15000)
        self.parser.add_argument("forbidden_uris[]", type=str, action="append")

    def get(self, region):
        args = self.parser.parse_args()
        response = NavitiaManager().dispatch(args, region, "journeys")
        if response.journeys:
            (before, after) = extremes(response, request)
            if before and after:
                response.prev = before
                response.next = after
            print response.journeys[0].sections[0].origin
        return protobuf_to_dict(response), 200

class Isochrone(Resource):
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("origin", type=str, required=True)
        self.parser.add_argument("datetime", type=str, required=True)
        self.parser.add_argument("clockwise", type=true_false, default=True)
        self.parser.add_argument("max_duration", type=int, default=3600)
        self.parser.add_argument("max_transfers", type=int, default=10)
        self.parser.add_argument("origin_mode",
                                 type=option_value(["walking", "car", "bike", "br"]),
                                 action="append", default="walking")
        self.parser.add_argument("walking_speed", type=float, default=1.68)
        self.parser.add_argument("walking_distance", type=int, default=1000)
        self.parser.add_argument("bike_speed", type=float, default=8.8)
        self.parser.add_argument("bike_distance", type=int, default=5000)
        self.parser.add_argument("br_speed", type=float, default=8.8)
        self.parser.add_argument("br_distance", type=int, default=5000)
        self.parser.add_argument("car_speed", type=float, default=16.8)
        self.parser.add_argument("car_distance", type=int, default=15000)
        self.parser.add_argument("forbidden_uris[]", type=str, action="append")

    def get(self, region):
        args = self.parser.parse_args()
        response = NavitiaManager().dispatch(args, region, "isochrone")
        if response.journeys:
            (before, after) = extremes(response, request)
            if before and after:
                response.prev = before
                response.next = after
        return protobuf_to_dict(response), 200
