#coding=utf-8
from flask import url_for, redirect
from flask.ext.restful import fields, marshal_with, reqparse, abort
from jormungandr.instance_manager import InstanceManager
from converters_collection_type import collections_to_resource_type
from fields import stop_point, stop_area, route, line, physical_mode,\
                   commercial_mode, company, network, pagination,\
                   journey_pattern_point, NonNullList, poi, poi_type,\
                   journey_pattern, vehicle_journey, connection, error, PbField
from collections import OrderedDict
from ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument
from errors import ManageError
from Coord import Coord


class Uri(ResourceUri):
    parsers = {}
    def __init__(self, is_collection, collection, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                description="The page where you want to start")
        self.parsers["get"].add_argument("count", type=int, default=25,
                description="The number of objects you want on the page")
        self.parsers["get"].add_argument("depth", type=depth_argument, default=1,
                description="The depth of your object")
        self.parsers["get"].add_argument("forbidden_id[]", type=unicode,
                description="id to forbid", dest="forbidden_uris[]")
        if is_collection:
            self.parsers["get"].add_argument("filter", type=str, default = "",
                    description="The filter parameter")
        self.collection = collection
        self.method_decorators.insert(0, ManageError())


    def get(self, region=None, lon=None, lat=None, uri=None, id=None):
        collection = self.collection
        self.region = InstanceManager().get_region(region, lon, lat)
        if not self.region:
            return {"error" : "No region"}, 404
        args = self.parsers["get"].parse_args()
        if(collection != None and id != None):
            args["filter"] = collections_to_resource_type[collection]+".uri="
            if collection != 'pois':
                args["filter"] += id
            else:
                args["filter"] += id.split(":")[-1]
        elif(uri):
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            if collection is None:
                collection = uris[-1] if len(uris)%2!=0 else uris[-2]
            args["filter"] = self.get_filter(uris)
        response = InstanceManager().dispatch(args, self.region, collection)
        return response

def journey_pattern_points(is_collection):
    class JourneyPatternPoints(Uri):
        """ Retrieves journey pattern points"""
        def __init__(self):
            Uri.__init__(self, is_collection, "journey_pattern_points")
            self.collections = [
                ("journey_pattern_points",
                  NonNullList(fields.Nested(journey_pattern_point,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return JourneyPatternPoints

def commercial_modes(is_collection):
    class CommercialModes(Uri):
        """ Retrieves commercial modes"""
        def __init__(self):
            Uri.__init__(self, is_collection, "commercial_modes")
            self.collections = [
                ("commercial_modes",
                  NonNullList(fields.Nested(commercial_mode,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return CommercialModes


def journey_patterns(is_collection):
    class JourneyPatterns(Uri):
        """ Retrieves journey patterns"""
        def __init__(self):
            Uri.__init__(self, is_collection, "journey_patterns")
            self.collections = [
                ("journey_patterns",
                  NonNullList(fields.Nested(journey_pattern,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return JourneyPatterns


def vehicle_journeys(is_collection):
    class VehicleJourneys(Uri):
        """ Retrieves vehicle journeys"""
        def __init__(self):
            Uri.__init__(self, is_collection, "vehicle_journeys")
            self.collections = [
                ("vehicle_journeys",
                  NonNullList(fields.Nested(vehicle_journey,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return VehicleJourneys


def physical_modes(is_collection):
    class PhysicalModes(Uri):
        """ Retrieves physical modes"""
        def __init__(self):
            Uri.__init__(self, is_collection, "physical_modes")
            self.collections = [
                ("physical_modes",
                  NonNullList(fields.Nested(physical_mode,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return PhysicalModes


def stop_points(is_collection):
    class StopPoints(Uri):
        """ Retrieves stop points """
        def __init__(self, *args, **kwargs):
            Uri.__init__(self, is_collection, "stop_points", *args, **kwargs)
            self.collections = [
                ("stop_points",
                    NonNullList(fields.Nested(stop_point, display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return StopPoints


def stop_areas(is_collection):
    class StopAreas(Uri):
        """ Retrieves stop areas """
        def __init__(self):
            Uri.__init__(self, is_collection, "stop_areas")
            self.collections = [
                ("stop_areas",
                  NonNullList(fields.Nested(stop_area,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return StopAreas


def connections(is_collection):
    class Connections(Uri):
        """ Retrieves connections"""
        def __init__(self):
            Uri.__init__(self, is_collection, "connections")
            self.collections = [
                ("connections",
                  NonNullList(fields.Nested(connection,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return Connections

def companies(is_collection):
    class Companies(Uri):
        """ Retrieves companies"""
        def __init__(self):
            Uri.__init__(self, is_collection, "companies")
            self.collections = [
                ("companies",
                  NonNullList(fields.Nested(company,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return Companies


def poi_types(is_collection):
    class PoiTypes(Uri):
        """ Retrieves poi types"""
        def __init__(self):
            Uri.__init__(self, is_collection, "poi_types")
            self.collections = [
                ("poi_types",
                  NonNullList(fields.Nested(poi_type,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return PoiTypes



def routes(is_collection):
    class Routes(Uri):
        """ Retrieves routes"""
        def __init__(self):
            Uri.__init__(self, is_collection, "routes")
            self.collections = [
                ("routes",
                  NonNullList(fields.Nested(route,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                  ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return Routes


def lines(is_collection):
    class Lines(Uri):
        """ Retrieves lines"""
        def __init__(self):
            Uri.__init__(self, is_collection, "lines")
            self.collections = [
                ("lines",
                  NonNullList(fields.Nested(line,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                  ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return Lines


def pois(is_collection):
    class Pois(Uri):
        """ Retrieves pois"""
        def __init__(self):
            Uri.__init__(self, is_collection, "pois")
            self.collections = [
                ("pois",
                  NonNullList(fields.Nested(poi,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                  ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return Pois


def networks(is_collection):
    class Networks(Uri):
        """ Retrieves networks"""
        def __init__(self):
            Uri.__init__(self, is_collection, "networks")
            self.collections = [
                ("networks",
                  NonNullList(fields.Nested(network,
                                display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
                  ]
            self.method_decorators.insert(1, marshal_with(OrderedDict(self.collections),
                                                    display_null=False))
    return Networks



def addresses(is_collection):
    class Addresses(Coord):
        """ Not implemented yet"""
        def __init__(self, *args, **kwargs):
            Coord.__init__(self, *args, **kwargs)

    return Addresses



def coords(is_collection):
    class Coords(Coord):
        """ Not implemented yet"""
        def __init__(self, *args, **kwargs):
            Uri.__init__(self, is_collection, "coords")
        def get(self, *args, **kwargs):
            abort(500, message="Not implemented yet")
    return Coords


def Redirect(*args, **kwargs):
    id = kwargs["id"]
    collection = kwargs["collection"]
    region = InstanceManager().key_of_id(id)
    if not region:
        region = "{region.id}"
    url = url_for("v1.uri", region=region, collection=collection, id=id)
    return redirect(url, 303)
