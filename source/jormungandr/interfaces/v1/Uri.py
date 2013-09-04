from flask import Flask, url_for, redirect
from flask.ext.restful import Resource, fields, marshal_with, reqparse
from instance_manager import NavitiaManager
from converters_collection_type import collections_to_resource_type
from fields import stop_point, stop_area, route, line, physical_mode,\
                   commercial_mode, company, network, pagination,\
                   journey_pattern_point, NonNullList
from collections import OrderedDict
from ResourceUri import ResourceUri

collections = OrderedDict([
    ("pagination", fields.Nested(pagination)),
    ("stop_points", NonNullList(fields.Nested(stop_point))),
    ("stop_areas", NonNullList(fields.Nested(stop_area))),
    ("routes", NonNullList(fields.Nested(route))),
    ("lines", NonNullList(fields.Nested(line))),
    ("physical_modes", NonNullList(fields.Nested(physical_mode))),
    ("commercial_modes", NonNullList(fields.Nested(commercial_mode))),
    ("companies", NonNullList(fields.Nested(company))),
    ("networks", NonNullList(fields.Nested(network))),
    ("journey_pattern_points", NonNullList(fields.Nested(journey_pattern_point))),
])

class Uri(ResourceUri):

    def __init__(self):
        super(Uri, self).__init__(self)
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("start_page", type=int, default=0)
        self.parser.add_argument("count", type=int, default=25)
        self.parser.add_argument("depth", type=int, default=1)

    @marshal_with(collections, allow_null=False)
    def get(self, collection=None, region=None, lon=None, lat=None,
            uri=None, id=None):
        self.region = NavitiaManager().get_region(region, lon, lat)
        if not self.region:
            return {"error" : "No region"}, 404
        args = self.parser.parse_args()
        if(collection != None and id != None):
            args["filter"] = collections_to_resource_type[collection]+".uri="+id
        elif(uri):
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            if collection is None:
                collection = uris[-1] if len(uris)%2!=0 else uris[-2]
            args["filter"] = self.get_filter(uris)
        response = NavitiaManager().dispatch(args, self.region, collection)
        return response, 200


class Collection(Uri):
    def __init__(self):
        super(Collection, self).__init__()
        self.parser.add_argument("filter", type=str, default = "")

    def get(self, region=None, lon=None, lat=None, collection=None, uri=None):
        return super(Collection, self).get(region=region, lon=lon, lat=lat,
                                    collection=collection, uri=uri)

def Redirect(*args, **kwargs):
    id = kwargs["id"]
    collection = kwargs["collection"]
    region = NavitiaManager().key_of_id(id)
    if not region:
        region = "{region.id}"
    url = url_for("v1.uri", region=region, collection=collection, id=id)
    return redirect(url, 303)




