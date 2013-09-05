from flask import Flask
from flask.ext.restful import Resource, fields
from instance_manager import NavitiaManager
from protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from interfaces.parsers import depth_argument

class PlacesNearby(Resource):
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("uri", type=str, required=True)
        self.parser.add_argument("type[]", type=str,
                                 action="append", default=["stop_area", "stop_point",
                                                           "address", "poi"])
        self.parser.add_argument("distance", type=int, default=500)
        self.parser.add_argument("count", type=int, default=10)
        self.parser.add_argument("depth", type=depth_argument, default=1)
        self.parser.add_argument("start_page", type=int, default=0)
    def get(self, region):
        args = self.parser.parse_args()
        response = NavitiaManager().dispatch(args, region, "places_nearby")
        return protobuf_to_dict(response), 200
