# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

from flask.ext.restful import fields, marshal_with, reqparse, abort
from jormungandr.parking_space_availability.bss.stands_manager import ManageStands
from jormungandr import i_manager
from jormungandr.interfaces.v1.converters_collection_type import collections_to_resource_type
from jormungandr.interfaces.v1.fields import stop_point, stop_area, route, line, line_group, \
    physical_mode, commercial_mode, company, network, pagination,\
    journey_pattern_point, poi, poi_type,\
    journey_pattern, trip, connection, error, PbField, contributor, dataset
from jormungandr.interfaces.v1.VehicleJourney import vehicle_journey
from collections import OrderedDict
from jormungandr.interfaces.v1.ResourceUri import ResourceUri, protect
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument, date_time_format, default_count_arg_type
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.interfaces.v1.Coord import Coord
from jormungandr.interfaces.v1.fields import disruption_marshaller, feed_publisher, NonNullList, NonNullNested
from jormungandr.timezone import set_request_timezone
from flask.ext.restful.inputs import boolean
from jormungandr.interfaces.parsers import option_value
from jormungandr.interfaces.common import odt_levels
from jormungandr.utils import date_to_timestamp
from jormungandr.resources_utc import ResourceUtc
from datetime import datetime
from flask import g
from jormungandr.interfaces.v1.decorators import get_serializer


class Uri(ResourceUri, ResourceUtc):

    def __init__(self, is_collection, collection, *args, **kwargs):
        kwargs['authentication'] = False
        ResourceUri.__init__(self, *args, **kwargs)
        ResourceUtc.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser = self.parsers["get"]
        parser.add_argument("start_page", type=int, default=0,
                            description="The page where you want to start")
        parser.add_argument("count", type=default_count_arg_type, default=25,
                            description="Number of objects you want on a page")
        parser.add_argument("depth", type=depth_argument,
                            default=1,
                            description="The depth of your object")
        parser.add_argument("forbidden_id[]", type=unicode,
                            description="DEPRECATED, replaced by forbidden_uris[]",
                            dest="__temporary_forbidden_id[]",
                            default=[],
                            action="append")
        parser.add_argument("forbidden_uris[]", type=unicode,
                            description="forbidden uris",
                            dest="forbidden_uris[]",
                            default=[],
                            action="append")
        parser.add_argument("external_code", type=unicode,
                            description="An external code to query")
        parser.add_argument("headsign", type=unicode,
                            description="filter vehicle journeys on headsign")
        parser.add_argument("show_codes", type=boolean, default=False,
                            description="show more identification codes")
        parser.add_argument("odt_level", type=option_value(odt_levels),
                                         default="all",
                                         description="odt level")
        parser.add_argument("_current_datetime", type=date_time_format, default=datetime.utcnow(),
                                description="The datetime used to consider the state of the pt object"
                                            " Default is the current date and it is used for debug."
                                            " Note: it will mainly change the disruptions that concern the object"
                                            " The timezone should be specified in the format,"
                                            " else we consider it as UTC")
        parser.add_argument("distance", type=int, default=200,
                                description="Distance range of the query. Used only if a coord is in the query")
        parser.add_argument("since", type=date_time_format,
                            description="filters objects not valid before this date")
        parser.add_argument("until", type=date_time_format,
                            description="filters objects not valid after this date")
        parser.add_argument("disable_geojson", type=boolean, default=False,
                            description="remove geojson from the response")

        if is_collection:
            parser.add_argument("filter", type=unicode, default="",
                                description="The filter parameter")
        self.collection = collection
        self.method_decorators.insert(0, ManageError())

    def get(self, region=None, lon=None, lat=None, uri=None, id=None):
        collection = self.collection

        args = self.parsers["get"].parse_args()

        # handle headsign
        if args.get("headsign"):
            f = u"vehicle_journey.has_headsign({})".format(protect(args["headsign"]))
            if args.get("filter"):
                args["filter"] += " and " + f
            else:
                args["filter"] = f

        if args['disable_geojson']:
            g.disable_geojson = True

        # for retrocompatibility purpose
        for forbid_id in args['__temporary_forbidden_id[]']:
            args['forbidden_uris[]'].append(forbid_id)

        if "odt_level" in args and args["odt_level"] != "all" and "lines" not in collection:
            abort(404, message="bad request: odt_level filter can only be applied to lines")

        if region is None and lat is None and lon is None:
            if "external_code" in args and args["external_code"]:
                type_ = collections_to_resource_type[collection]
                for instance in i_manager.get_regions():
                    res = i_manager.instances[instance].has_external_code(type_, args["external_code"])
                    if res:
                        region = instance
                        id = res
                        break
                if not region:
                    abort(404, message="Unable to find an object for the uri %s"
                          % args["external_code"])
            else:
                abort(503, message="Not implemented yet")

        self.region = i_manager.get_region(region, lon, lat)

        #we store the region in the 'g' object, which is local to a request
        set_request_timezone(self.region)

        # change dt to utc
        if args['since']:
            args['_original_since'] = args['since']
            args['since'] = date_to_timestamp(self.convert_to_utc(args['since']))
        if args['until']:
            args['_original_until'] = args['until']
            args['until'] = date_to_timestamp(self.convert_to_utc(args['until']))

        if not self.region:
            return {"error": "No region"}, 404
        if uri:
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            if collection is None:
                collection = uris[-1] if len(uris) % 2 != 0 else uris[-2]
            args["filter"] = self.get_filter(uris, args)
        if collection and id:
            f = u'{o}.uri={v}'.format(o=collections_to_resource_type[collection], v=protect(id))
            if args.get("filter"):
                args["filter"] += " and " + f
            else:
                args["filter"] = f

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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
    return VehicleJourneys


def trips(is_collection):
    class Trips(Uri):

        """ Retrieves trips"""

        def __init__(self):
            Uri.__init__(self, is_collection, "trips")
            self.collections = [
                ("trips",
                 NonNullList(fields.Nested(trip,
                                           display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
    return Trips


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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False))),
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            collections = marshal_with(OrderedDict(self.collections), display_null=False)
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
    return Routes


def line_groups(is_collection):
    class LineGroups(Uri):
        """ Retrieves line_groups"""

        def __init__(self):
            Uri.__init__(self, is_collection, "line_groups")
            self.collections = [
                ("line_groups",
                 NonNullList(fields.Nested(line_group,
                                           display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
    return LineGroups


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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))

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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
            self.parsers["get"].add_argument("bss_stands", type=boolean, default=True,
                                             description="Show bss stands availability")
            args = self.parsers["get"].parse_args()
            if args["bss_stands"]:
                self.method_decorators.insert(2, ManageStands(self, 'pois'))

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
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
    return Networks


def disruptions(is_collection):

    class Disruptions(Uri):

        def __init__(self):
            Uri.__init__(self, is_collection, "disruptions")
            self.collections = [
                ("pagination", PbField(pagination)),
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            self.method_decorators.insert(1, get_serializer(self.collection))
            self.parsers["get"].add_argument("original_id", type=unicode,
                            description="original uri of the object you"
                                    "want to query")
    return Disruptions


def contributors(is_collection):
    class Contributors(Uri):

        """ Retrieves contributors"""

        def __init__(self):
            Uri.__init__(self, is_collection, "contributors")
            self.collections = [
                ("contributors",
                 NonNullList(fields.Nested(contributor,
                                           display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
    return Contributors


def datasets(is_collection):
    class Datasets(Uri):

        """ Retrieves datasets"""

        def __init__(self):
            Uri.__init__(self, is_collection, "datasets")
            self.collections = [
                ("datasets",
                 NonNullList(fields.Nested(dataset,
                                           display_null=False))),
                ("pagination", PbField(pagination)),
                ("error", PbField(error)),
                ("disruptions", fields.List(NonNullNested(disruption_marshaller), attribute="impacts")),
                ("feed_publishers", NonNullList(fields.Nested(feed_publisher,
                                           display_null=False)))
            ]
            collections = marshal_with(OrderedDict(self.collections),
                                       display_null=False)
            self.method_decorators.insert(1, collections)
    return Datasets


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

coord = coords
