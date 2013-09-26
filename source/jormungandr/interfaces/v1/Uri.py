from flask import Flask, url_for, redirect
from flask.ext.restful import Resource, fields, marshal_with, reqparse
from instance_manager import NavitiaManager
from converters_collection_type import collections_to_resource_type
from fields import stop_point, stop_area, route, line, physical_mode,\
                   commercial_mode, company, network, pagination,\
                   journey_pattern_point, NonNullList, poi, poi_type
from collections import OrderedDict
from ResourceUri import ResourceUri

collections = OrderedDict([
    ("pagination", fields.Nested(pagination)),
    ("stop_points", NonNullList(fields.Nested(stop_point, display_null=False))),
    ("stop_areas", NonNullList(fields.Nested(stop_area, display_null=False))),
    ("routes", NonNullList(fields.Nested(route, display_null=False))),
    ("lines", NonNullList(fields.Nested(line, display_null=False))),
    ("physical_modes", NonNullList(fields.Nested(physical_mode, display_null=False))),
    ("commercial_modes", NonNullList(fields.Nested(commercial_mode, display_null=False))),
    ("companies", NonNullList(fields.Nested(company, display_null=False))),
    ("networks", NonNullList(fields.Nested(network, display_null=False))),
    ("journey_pattern_points", NonNullList(fields.Nested(journey_pattern_point, display_null=False))),
    ("pois", NonNullList(fields.Nested(poi, display_null=False))),
    ("poi_types", NonNullList(fields.Nested(poi_type, display_null=False))),
])

class Uri(ResourceUri):

    def __init__(self):
        super(Uri, self).__init__(self)
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("start_page", type=int, default=0)
        self.parser.add_argument("count", type=int, default=25)
        self.parser.add_argument("depth", type=int, default=1)

    @marshal_with(collections, display_null=False)
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




