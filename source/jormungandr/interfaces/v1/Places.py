#coding=utf-8
from flask import Flask, request
from flask.ext.restful import Resource, fields, marshal_with, reqparse
from instance_manager import NavitiaManager
from make_links import add_id_links
from fields import place, NonNullList, NonNullNested
from fields import place, NonNullList, NonNullNested, PbField, pagination
from ResourceUri import ResourceUri
from make_links import add_id_links
from interfaces.argument import ArgumentDoc
from interfaces.parsers import depth_argument
from copy import deepcopy

places = { "places" : NonNullList(NonNullNested(place))}

class Places(ResourceUri):
    parsers = {}
    def __init__(self):
        super(Places, self).__init__()
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                description="The data to search")
        self.parsers["get"].add_argument("type[]", type=str, action="append",
                                 default=["stop_area","address",
                                          "poi", "administrative_region"],
                description="The type of data to search")
        self.parsers["get"].add_argument("count", type=int,  default=10,
                description="The maximum number of places returned")
        self.parsers["get"].add_argument("admin_uri[]", type=str, action="append",
                description="""If filled, will restrained the search within the
                               given admin uris""")
        self.parsers["get"].add_argument("depth", type=depth_argument, default=1,
                description="The depth of the objects")

    @marshal_with(places)
    def get(self, region=None, lon=None, lat=None):
        self.region = NavitiaManager().get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()
        response = NavitiaManager().dispatch(args, self.region, "places")
        return response, 200

places_nearby = { "places_nearby" : NonNullList(NonNullNested(place))}
places_nearby = { "places_nearby" : NonNullList(NonNullNested(place_nearby)),
                  "pagination" : PbField(pagination)}

class PlacesNearby(ResourceUri):
    parsers = {}
    def __init__(self):
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        self.parsers["get"].add_argument("uri", type=str, required=True,
                description="""uri arround which you want to look for objects.
                                Not all objects make sense (e.g. a mode). """)
        self.parsers["get"].add_argument("type[]", type=str,
                                 action="append", default=["stop_area", "stop_point",
                                                           "address", "poi"],
                description="Type of the objects to return")
        self.parsers["get"].add_argument("distance", type=int, default=500,
                description="Distance range of the query")
        self.parsers["get"].add_argument("count", type=int, default=10,
                description="Number of elements per page")
        self.parsers["get"].add_argument("depth", type=depth_argument, default=1,
                description="Maximum depth on objects")
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                description="The page number of the ptref result")

    @marshal_with(places_nearby)
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = NavitiaManager().get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()
        if uri:
            if uri[-1] == '/':
                uri = uri[:-1]
            uris = uri.split("/")
            if len(uris) > 1:
                args["uri"] = uris[-1]
            else:
                coord = uri.split(";")
                if len(coord) == 2:
                    try:
                        lon = str(float(coord[0]))
                        lat = str(float(coord[1]))
                        args["uri"] = "coord:"+lon+":"+lat
                    except ValueError:
                        pass
                else:
                    abort(500, "Not implemented yet")
        response = NavitiaManager().dispatch(args, self.region, "places_nearby")
        return response, 200

