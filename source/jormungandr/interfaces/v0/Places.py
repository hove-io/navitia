# coding=utf-8
from flask import Flask
from flask.ext.restful import Resource, fields, abort
from instance_manager import InstanceManager
from protobuf_to_dict import protobuf_to_dict
from flask.ext.restful import reqparse
from interfaces.parsers import depth_argument
from interfaces.argument import ArgumentDoc
from authentification import authentification_required

class Places(Resource):
    """Retreives places"""
    method_decorators = [authentification_required]
    def __init__(self):
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                description="The data to search")
        self.parsers["get"].add_argument("type[]", type=str, action="append",
                                 default=["stop_area","address",
                                          "poi", "administrative_region"],
                description="The type of data to search")
        self.parsers["get"].add_argument("count", type=int,  default=10,
                description="The maximum number of places returned")
        self.parsers["get"].add_argument("search_type", type=int, default=0,
                description="Type of search: firstletter or typeerror")
        self.parsers["get"].add_argument("admin_uri[]", type=str, action="append",
                description="""If filled, will restrained the search within the
                    ²           given admin uris""")
        self.parsers["get"].add_argument("depth", type=depth_argument, default=1,
                description="The depth of the objects")

    def get(self, region):
        args = self.parsers["get"].parse_args()
        if len(args['q']) == 0:
            abort(400, message="Search word absent")
        response = InstanceManager().dispatch(args, region, "places")
        return protobuf_to_dict(response, use_enum_labels=True), 200
