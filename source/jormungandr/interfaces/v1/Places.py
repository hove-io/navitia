from flask import Flask, request
from flask.ext.restful import Resource, fields, marshal_with, reqparse
from instance_manager import NavitiaManager
from make_links import add_id_links
from fields import place
from ResourceUri import ResourceUri, add_address_region
from make_links import add_id_links

places = { "places" : fields.List(fields.Nested(place))}

class Places(ResourceUri):
    def __init__(self):
        super(Places, self).__init__(self)
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("q", type=str, required=True)
        self.parser.add_argument("type[]", type=str, action="append",
                                 default=["stop_area", "stop_point",
                                          "address", "poi"])
        self.parser.add_argument("count", type=int,  default=10)
        self.parser.add_argument("admin_uri[]", type=str, action="append")
        self.parser.add_argument("depth", type=int, default=1)

    @marshal_with(places)
    def get(self, region=None, lon=None, lat=None):
        self.region = NavitiaManager().get_region(region, lon, lat)
        args = self.parser.parse_args()
        response = NavitiaManager().dispatch(args, self.region, "places")
        return response, 200

places_nearby = { "places_nearby" : fields.List(fields.Nested(place))}

class PlacesNearby(ResourceUri):
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("uri", type=str)
        self.parser.add_argument("type[]", type=str,
                                 action="append", default=["stop_area", "stop_point",
                                                           "address", "poi"])
        self.parser.add_argument("distance", type=int, default=500)
        self.parser.add_argument("count", type=int, default=10)
        self.parser.add_argument("depth", type=int, default=1)
        self.parser.add_argument("start_page", type=int, default=0)

    @marshal_with(places_nearby)
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = NavitiaManager().get_region(region, lon, lat)
        args = self.parser.parse_args()
        if uri:
            if uri[-1] == '/':
                uri = uri[:-1]
            uris = uri.split("/")
            args["uri"] = uris[-1]
        response = NavitiaManager().dispatch(args, self.region, "places_nearby")
        return response, 200

