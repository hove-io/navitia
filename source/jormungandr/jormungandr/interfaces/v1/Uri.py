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

from flask_restful import abort

from jormungandr.parking_space_availability.parking_places_manager import ManageParkingPlaces
from jormungandr import i_manager
from jormungandr.interfaces.v1.converters_collection_type import collections_to_resource_type
from jormungandr.interfaces.v1.fields import get_collections
from jormungandr.interfaces.v1.ResourceUri import ResourceUri, protect
from jormungandr.interfaces.parsers import depth_argument, DateTimeFormat, default_count_arg_type
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.interfaces.v1.Coord import Coord
from jormungandr.timezone import set_request_timezone
from jormungandr.interfaces.common import odt_levels, add_poi_infos_types, handle_poi_infos
from jormungandr.utils import date_to_timestamp
from jormungandr.resources_utils import ResourceUtc
from datetime import datetime
from flask import g
from jormungandr.interfaces.v1.decorators import get_obj_serializer
from jormungandr.interfaces.v1.serializer import api
import six
from navitiacommon.parser_args_type import BooleanType, OptionValue


class Uri(ResourceUri, ResourceUtc):

    def __init__(self, is_collection, collection, *args, **kwargs):
        kwargs['authentication'] = False
        ResourceUri.__init__(self, *args, **kwargs)
        ResourceUtc.__init__(self)
        parser = self.parsers["get"]
        parser.add_argument("start_page", type=int, default=0,
                            help="The page where you want to start")
        parser.add_argument("count", type=default_count_arg_type, default=25,
                            help="Number of objects you want on a page")
        parser.add_argument("depth", type=depth_argument,
                            schema_type=int,
                            default=1,
                            help="The depth of your object")
        parser.add_argument("forbidden_id[]", type=six.text_type,
                            help="DEPRECATED, replaced by `forbidden_uris[]`",
                            dest="__temporary_forbidden_id[]",
                            default=[],
                            action="append", schema_metadata={'format': 'pt-object'})
        parser.add_argument("forbidden_uris[]", type=six.text_type,
                            help="forbidden uris",
                            dest="forbidden_uris[]",
                            default=[],
                            action="append", schema_metadata={'format': 'pt-object'})
        # for the top level collection apis (/v1/networks, /v1/lines, ...) the external_code is mandatory
        external_code_mandatory = '.external_codes' in self.endpoint
        parser.add_argument("external_code", type=six.text_type,
                            help="An external code to query", required=external_code_mandatory)
        parser.add_argument("headsign", type=six.text_type,
                            help="filter vehicle journeys on headsign")
        parser.add_argument("show_codes", type=BooleanType(), default=False,
                            help="show more identification codes")
        parser.add_argument("odt_level", type=OptionValue(odt_levels), default="all",
                            schema_type=str, schema_metadata={"enum": odt_levels},
                            help="odt level")
        parser.add_argument("_current_datetime", type=DateTimeFormat(),
                            schema_metadata={'default': 'now'}, hidden=True,
                            default=datetime.utcnow(),
                            help='The datetime considered as "now". Used for debug, default is '
                                 'the moment of the request. It will mainly change the output '
                                 'of the disruptions.')
        parser.add_argument("distance", type=int, default=200,
                            help="Distance range of the query. Used only if a coord is in the query")
        parser.add_argument("since", type=DateTimeFormat(),
                            help="filters objects not valid before this date")
        parser.add_argument("until", type=DateTimeFormat(),
                            help="filters objects not valid after this date")
        parser.add_argument("disable_geojson", type=BooleanType(), default=False,
                            help="remove geojson from the response")

        if is_collection:
            parser.add_argument("filter", type=six.text_type, default="",
                                help="The filter parameter")
        parser.add_argument("tags[]", type=six.text_type, action="append",
                            help="If filled, will restrained the search within the given disruption tags")
        self.collection = collection
        self.get_decorators.insert(0, ManageError())

    def get(self, region=None, lon=None, lat=None, uri=None, id=None):
        collection = self.collection

        args = self.parsers["get"].parse_args()

        # handle headsign
        if args.get("headsign"):
            f = u"vehicle_journey.has_headsign({})".format(protect(args["headsign"]))
            if args.get("filter"):
                args["filter"] = '({}) and {}'.format(args["filter"], f)
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
        uris = []
        if uri:
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            if collection is None:
                collection = uris[-1] if len(uris) % 2 != 0 else uris[-2]
        if uris or args.get('tags[]', []):
            args["filter"] = self.get_filter(uris, args)
        if collection and id:
            f = u'{o}.uri={v}'.format(o=collections_to_resource_type[collection], v=protect(id))
            if args.get("filter"):
                args["filter"] = '({}) and {}'.format(args["filter"], f)
            else:
                args["filter"] = f

        response = i_manager.dispatch(args, collection,
                                      instance_name=self.region)
        return response

    def options(self, **kwargs):
        return self.api_description(**kwargs)


def journey_pattern_points(is_collection):
    class JourneyPatternPoints(Uri):

        """ Retrieves journey pattern points"""

        def __init__(self):
            Uri.__init__(self, is_collection, "journey_pattern_points",
                         output_type_serializer=api.JourneyPatternPointsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return JourneyPatternPoints


def commercial_modes(is_collection):
    class CommercialModes(Uri):

        """ Retrieves commercial modes"""

        def __init__(self):
            Uri.__init__(self, is_collection, "commercial_modes", output_type_serializer=api.CommercialModesSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return CommercialModes


def journey_patterns(is_collection):
    class JourneyPatterns(Uri):

        """ Retrieves journey patterns"""

        def __init__(self):
            Uri.__init__(self, is_collection, "journey_patterns", output_type_serializer=api.JourneyPatternsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return JourneyPatterns


def vehicle_journeys(is_collection):
    class VehicleJourneys(Uri):

        """ Retrieves vehicle journeys"""

        def __init__(self):
            Uri.__init__(self, is_collection, "vehicle_journeys", output_type_serializer=api.VehicleJourneysSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return VehicleJourneys


def trips(is_collection):
    class Trips(Uri):

        """ Retrieves trips"""

        def __init__(self):
            Uri.__init__(self, is_collection, "trips", output_type_serializer=api.TripsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return Trips


def physical_modes(is_collection):
    class PhysicalModes(Uri):

        """ Retrieves physical modes"""

        def __init__(self):
            Uri.__init__(self, is_collection, "physical_modes", output_type_serializer=api.PhysicalModesSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return PhysicalModes


def stop_points(is_collection):
    class StopPoints(Uri):

        """ Retrieves stop points """

        def __init__(self, *args, **kwargs):
            Uri.__init__(self, is_collection, "stop_points", output_type_serializer=api.StopPointsSerializer,
                         *args, **kwargs)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
    return StopPoints


def stop_areas(is_collection):
    class StopAreas(Uri):

        """ Retrieves stop areas """

        def __init__(self):
            Uri.__init__(self, is_collection, "stop_areas", output_type_serializer=api.StopAreasSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
    return StopAreas


def connections(is_collection):
    class Connections(Uri):

        """ Retrieves connections"""

        def __init__(self):
            Uri.__init__(self, is_collection, "connections", output_type_serializer=api.ConnectionsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return Connections


def companies(is_collection):
    class Companies(Uri):

        """ Retrieves companies"""

        def __init__(self):
            Uri.__init__(self, is_collection, "companies", output_type_serializer=api.CompaniesSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return Companies


def poi_types(is_collection):
    class PoiTypes(Uri):

        """ Retrieves poi types"""

        def __init__(self):
            Uri.__init__(self, is_collection, "poi_types", output_type_serializer=api.PoiTypesSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return PoiTypes


def routes(is_collection):
    class Routes(Uri):

        """ Retrieves routes"""

        def __init__(self):
            Uri.__init__(self, is_collection, "routes", output_type_serializer=api.RoutesSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
    return Routes


def line_groups(is_collection):
    class LineGroups(Uri):
        """ Retrieves line_groups"""

        def __init__(self):
            Uri.__init__(self, is_collection, "line_groups", output_type_serializer=api.LineGroupsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
    return LineGroups


def lines(is_collection):
    class Lines(Uri):

        """ Retrieves lines"""

        def __init__(self):
            Uri.__init__(self, is_collection, "lines", output_type_serializer=api.LinesSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))

            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
    return Lines


def pois(is_collection):
    class Pois(Uri):

        """ Retrieves pois"""

        def __init__(self):
            Uri.__init__(self, is_collection, "pois", output_type_serializer=api.PoisSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
            self.parsers["get"].add_argument("bss_stands", type=BooleanType(), default=False,
                                             help="Deprecated - Use add_poi_infos[]=bss_stands")
            self.parsers["get"].add_argument("add_poi_infos[]", type=OptionValue(add_poi_infos_types),
                                             default=['bss_stands', 'car_park'],
                                             dest="add_poi_infos", action="append",
                                             help="Show more information about the poi if it's available, for instance,"
                                                  " show BSS/car park availability in the pois(BSS/car park) of the "
                                                  "response")

            args = self.parsers["get"].parse_args()
            if handle_poi_infos(args["add_poi_infos"], args["bss_stands"]):
                self.get_decorators.insert(2, ManageParkingPlaces(self, 'pois'))

    return Pois


def networks(is_collection):
    class Networks(Uri):

        """ Retrieves networks"""

        def __init__(self):
            Uri.__init__(self, is_collection, "networks", output_type_serializer=api.NetworksSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
    return Networks


def disruptions(is_collection):

    class Disruptions(Uri):

        def __init__(self):
            Uri.__init__(self, is_collection, "disruptions", output_type_serializer=api.DisruptionsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
            self.parsers["get"].add_argument("original_id", type=six.text_type,
                                             help="original uri of the object you want to query")
    return Disruptions


def contributors(is_collection):
    class Contributors(Uri):

        """ Retrieves contributors"""

        def __init__(self):
            Uri.__init__(self, is_collection, "contributors", output_type_serializer=api.ContributorsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
    return Contributors


def datasets(is_collection):
    class Datasets(Uri):

        """ Retrieves datasets"""

        def __init__(self):
            Uri.__init__(self, is_collection, "datasets", output_type_serializer=api.DatasetsSerializer)
            self.collections = get_collections(self.collection)
            self.get_decorators.insert(1, get_obj_serializer(self))
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
