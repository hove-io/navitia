# coding=utf-8
from flask import Flask
from flask.ext.restful import Resource, fields
from jormungandr.instance_manager import InstanceManager
from .protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from jormungandr.interfaces.parsers import depth_argument
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.authentification import authentification_required


class PlacesNearby(Resource):

    """Retreives places nearby a point"""
    method_decorators = [authentification_required]

    def __init__(self):
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("uri", type=str, required=True,
                                         description="""uri arround which you\
                                         want to look for objects.
                                Not all objects make sense (e.g. a mode). """)
        self.parsers["get"].add_argument("type[]", type=str,
                                         action="append",
                                         default=["stop_area", "stop_point",
                                                  "address", "poi"],
                                         description="Type of the objects to\
                                         return")
        self.parsers["get"].add_argument("distance", type=int, default=500,
                                         description="Distance range of the\
                                         query")
        self.parsers["get"].add_argument("filter", type=str, default="",
                                         description="Filter places")
        self.parsers["get"].add_argument("count", type=int, default=10,
                                         description="Number of elements per\
                                         page")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="Maximum depth on\
                                         objects")
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                                         description="The page number of the\
                                         ptref result")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        response = InstanceManager().dispatch(args, region, "places_nearby")
        return protobuf_to_dict(response, use_enum_labels=True), 200
