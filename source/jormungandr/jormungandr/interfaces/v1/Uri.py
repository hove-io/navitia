# coding=utf-8
from flask import url_for, redirect
from flask.ext.restful import fields, marshal_with, reqparse, abort
from jormungandr import i_manager
from converters_collection_type import collections_to_resource_type
from fields import stop_point, stop_area, route, line, physical_mode,\
    commercial_mode, company, network, pagination,\
    journey_pattern_point, NonNullList, poi, poi_type,\
    journey_pattern, vehicle_journey, connection, error, calendar, PbField
from collections import OrderedDict
from ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument
from errors import ManageError
from Coord import Coord
from navitiacommon.models import PtObject


class Uri(ResourceUri):
    parsers = {}

    def __init__(self, is_collection, collection, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser = self.parsers["get"]
        parser.add_argument("start_page", type=int, default=0,
                            description="The page where you want to start")
        parser.add_argument("count", type=int, default=25,
                            description="Number of objects you want on a page")
        parser.add_argument("depth", type=depth_argument,
                            default=1,
                            description="The depth of your object")
        parser.add_argument("forbidden_id[]", type=unicode,
                            description="forbidden ids",
                            dest="forbidden_uris[]",
                            action="append")
        parser.add_argument("external_code", type=unicode,
                            description="An external code to query")
        if is_collection:
            parser.add_argument("filter", type=str, default="",
                                description="The filter parameter")
        self.collection = collection
        self.method_decorators.insert(0, ManageError())

    def get(self, region=None, lon=None, lat=None, uri=None, id=None):
        collection = self.collection
        args = self.parsers["get"].parse_args()
        if region is None and lat is None and lon is None:
            if "external_code" in args and args["external_code"]:
                res = PtObject.get_from_external_code(args["external_code"])
                if res:
                    id = res.uri
                    instances = res.instances()
                    if len(instances) > 0:
                        region = res.instances()[0].name
                else:
                    abort(404, message="Unable to find an object for the uri %s"
                          % args["external_code"])
            else:
                abort(503, message="Not implemented yet")
        self.region = i_manager.get_region(region, lon, lat)
        if not self.region:
            return {"error": "No region"}, 404



        if(collection and id):
            args["filter"] = collections_to_resource_type[collection] + ".uri="
            if collection != 'pois':
                args["filter"] += id
            else:
                args["filter"] += id.split(":")[-1]
        elif(uri):
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            if collection is None:
                collection = uris[-1] if len(uris) % 2 != 0 else uris[-2]
            args["filter"] = self.get_filter(uris)
        #else:
        #    abort(503, message="Not implemented")
        response = i_manager.dispatch(args, collection,
                                      instance_name=self.region)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
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
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
    return Networks


def calendars(is_collection):
    class Calendars(Uri):

        """ Retrieves calendars"""

        def __init__(self):
            Uri.__init__(self, is_collection, "calendars")
            self.collections = [
                ("calendars", NonNullList(fields.Nested(calendar,
                                           display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error))
            ]
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
    return Calendars


def addresses(is_collection):
    class Addresses(Coord):

        """ Not implemented yet"""
        pass

    return Addresses


def coords(is_collection):
    class Coords(Coord):

        """ Not implemented yet"""
        pass
    return Coords


def Redirect(*args, **kwargs):
    id = kwargs["id"]
    collection = kwargs["collection"]
    region = i_manager.key_of_id(id)
    if not region:
        region = "{region.id}"
    url = url_for("v1.uri", region=region, collection=collection, id=id)
    return redirect(url, 303)
