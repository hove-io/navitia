from flask import Flask
from flask.ext.restful import Resource, fields
from instance_manager import NavitiaManager
from protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from interfaces.parsers import depth_argument

class Places(Resource):
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("q", type=str, required=True)
        self.parser.add_argument("type[]", type=str, action="append",
                                 default=["stop_area", "stop_point",
                                          "address", "poi"])
        self.parser.add_argument("count", type=int,  default=10)
        self.parser.add_argument("admin_uri[]", type=str, action="append")
        self.parser.add_argument("depth", type=depth_argument, default=1)

    def get(self, region):
        args = self.parser.parse_args()
        response = NavitiaManager().dispatch(args, region, "places")
        return protobuf_to_dict(response), 200
