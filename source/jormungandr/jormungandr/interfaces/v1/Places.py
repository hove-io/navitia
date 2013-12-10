# coding=utf-8
from flask import Flask, request
from flask.ext.restful import Resource, fields, marshal_with, reqparse, abort
from jormungandr.instance_manager import InstanceManager
from make_links import add_id_links
from fields import place, NonNullList, NonNullNested, PbField, pagination
from ResourceUri import ResourceUri
from make_links import add_id_links
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument
from copy import deepcopy


places = {"places": NonNullList(NonNullNested(place))}


class Places(ResourceUri):
    parsers = {}

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                                         description="The data to search")
        self.parsers["get"].add_argument("type[]", type=str, action="append",
                                         default=["stop_area", "address",
                                                  "poi",
                                                  "administrative_region"],
                                         description="The type of data to
                                         search")
        self.parsers["get"].add_argument("count", type=int, default=10,
                                         description="The maximum number of\
                                         places returned")
        self.parsers["get"].add_argument("search_type", type=int, default=0,
                                         description="Type of search:\
                                         firstletter or type error")
        self.parsers["get"].add_argument("admin_uri[]", type=str,
                                         action="append",
                                         description="If filled, will\
                                         restrained the search within the\
                                         given admin uris")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="The depth of objects")

    @marshal_with(places)
    def get(self, region=None, lon=None, lat=None):
        self.region = InstanceManager().get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()
        if len(args['q']) == 0:
            abort(400, message="Search word absent")
        response = InstanceManager().dispatch(args, self.region, "places")
        return response, 200


class PlaceUri(ResourceUri):

    @marshal_with(places)
    def get(self, id, region=None, lon=None, lat=None):
        self.region = InstanceManager().get_region(region, lon, lat)
        args = {}
        if id.count(";") == 1:
            lon, lat = id.split(";")
            try:
                args["uri"] = "coord:" + str(float(lon)) + ":"
                args["uri"] += str(float(lat))
            except ValueError:
                pass
        elif id[:3] == "poi":
            args["uri"] = id.split(":")[-1]
        elif id[:5] == "admin":
            args["uri"] = "admin:" + id.split(":")[-1]
        if not "uri" in args.keys():
            args["uri"] = id
        response = InstanceManager().dispatch(args, self.region, "place_uri")
        return response, 200

place_nearby = deepcopy(place)
place_nearby["distance"] = fields.Float()
places_nearby = {"places_nearby": NonNullList(NonNullNested(place_nearby)),
                 "pagination": PbField(pagination)}


class PlacesNearby(ResourceUri):
    parsers = {}

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        self.parsers["get"].add_argument("type[]", type=str,
                                         action="append",
                                         default=["stop_area", "stop_point",
                                                  "poi"],
                                         description="Type of the objects to\
                                         return")
        self.parsers["get"].add_argument("filter", type=str, default="",
                                         description="Filter your objects")
        self.parsers["get"].add_argument("distance", type=int, default=500,
                                         description="Distance range of the\
                                         query")
        self.parsers["get"].add_argument("count", type=int, default=10,
                                         description="Elements per page")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="Maximum depth on\
                                         objects")
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                                         description="The page number of the\
                                         ptref result")

    @marshal_with(places_nearby)
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = InstanceManager().get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()
        if uri:
            if uri[-1] == '/':
                uri = uri[:-1]
            uris = uri.split("/")
            if len(uris) > 1:
                id = uris[-1]
                if id.count(";") == 1:
                    lon, lat = id.split(";")
                    try:
                        args["uri"] = "coord:" + str(float(lon)) + ":"
                        args["uri"] += str(float(lat))
                    except ValueError:
                        pass
                elif id[:3] == "poi":
                    args["uri"] = id.split(":")[-1]
                elif id[:5] == "admin":
                    args["uri"] = "admin:" + id.split(":")[-1]
                else:
                    args["uri"] = id
            else:
                abort(404)
        else:
            abort(404)
        args["filter"] = args["filter"].replace(".id", ".uri")
        response = InstanceManager().dispatch(args, self.region,
                                              "places_nearby")
        return response, 200
