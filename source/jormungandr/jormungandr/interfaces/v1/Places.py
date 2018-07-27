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
from flask_restful import fields, abort
from flask.globals import g

from jormungandr.authentication import get_all_available_instances
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.serializer.api import PlacesSerializer, PlacesNearbySerializer
from navitiacommon import parser_args_type
from jormungandr import i_manager, timezone, global_autocomplete, authentication
from jormungandr.interfaces.v1.fields import disruption_marshaller
from jormungandr.interfaces.v1.fields import place, NonNullList, NonNullNested, PbField, pagination, \
    error, feed_publisher
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.parsers import depth_argument, default_count_arg_type, DateTimeFormat
from copy import deepcopy
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.exceptions import TechnicalError, InvalidArguments
from datetime import datetime
from jormungandr.parking_space_availability.parking_places_manager import ManageParkingPlaces
import ujson as json
from jormungandr.scenarios.utils import places_type
from navitiacommon.parser_args_type import TypeSchema, CoordFormat, CustomSchemaType, BooleanType, \
    OptionValue
from jormungandr.interfaces.common import add_poi_infos_types, handle_poi_infos
import six


places = {
    "places": NonNullList(NonNullNested(place)),
    "error": PbField(error, attribute='error'),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
    "feed_publishers": fields.List(NonNullNested(feed_publisher))
}


class geojson_argument(CustomSchemaType):
    def __call__(self, value):
        decoded = json.loads(value)
        if not decoded:
            raise ValueError('invalid shape')

        return parser_args_type.geojson_argument(decoded)

    def schema(self):
        return TypeSchema(type=str)  # TODO a better description of the geojson


class Places(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, authentication=False, output_type_serializer=PlacesSerializer,
                             *args, **kwargs)
        self.parsers["get"].add_argument("q", type=six.text_type, required=True,
                                         help="The data to search")
        self.parsers["get"].add_argument("type[]", type=OptionValue(list(places_type.keys())),
                                         action="append",
                                         default=["stop_area", "address", "poi", "administrative_region"],
                                         help="The type of data to search")
        self.parsers["get"].add_argument("count", type=default_count_arg_type, default=10,
                                         help="The maximum number of places returned")
        self.parsers["get"].add_argument("search_type", type=int, default=0, hidden=True,
                                         help="Type of search: firstletter or type error")
        self.parsers["get"].add_argument("admin_uri[]", type=six.text_type,
                                         action="append",
                                         help="If filled, will restrain the search within the "
                                              "given admin uris")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         help="The depth of objects")
        self.parsers["get"].add_argument("_current_datetime", type=DateTimeFormat(),
                                         schema_metadata={'default': 'now'}, hidden=True,
                                         default=datetime.utcnow(),
                                         help='The datetime considered as "now". Used for debug, default is '
                                              'the moment of the request. It will mainly change the output '
                                              'of the disruptions.')
        self.parsers['get'].add_argument("disable_geojson", type=BooleanType(), default=False,
                                         help="remove geojson from the response")

        self.parsers['get'].add_argument("from", type=CoordFormat(nullable=True),
                                         help="Coordinates longitude;latitude used to prioritize "
                                              "the objects around this coordinate")
        self.parsers['get'].add_argument("_autocomplete", type=six.text_type, hidden=True,
                                         help="name of the autocomplete service, used under the hood")
        self.parsers['get'].add_argument('shape', type=geojson_argument(),
                                         help='Geographical shape to limit the search.')

    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        self._register_interpreted_parameters(args)
        if len(args['q']) == 0:
            abort(400, message="Search word absent")

        if args['disable_geojson']:
            g.disable_geojson = True

        user = authentication.get_user(token=authentication.get_token(), abort_if_no_token=False)

        if args['shape'] is None and user and user.shape:
            args['shape'] = json.loads(user.shape)

        if user and user.default_coord:
            if args['from'] is None:
                args['from'] = CoordFormat()(user.default_coord)
        else:
            if args['from'] == '':
                raise InvalidArguments("if 'from' is provided it cannot be null")

        # If a region or coords are asked, we do the search according
        # to the region, else, we do a word wide search

        if any([region, lon, lat]):
            self.region = i_manager.get_region(region, lon, lat)
            timezone.set_request_timezone(self.region)
            response = i_manager.dispatch(args, "places", instance_name=self.region)
        else:
            available_instances = get_all_available_instances(user)
            autocomplete = global_autocomplete.get('bragi')
            if not autocomplete:
                raise TechnicalError('world wide autocompletion service not available')
            response = autocomplete.get(args, instances=available_instances)
        return response, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)


class PlaceUri(ResourceUri):

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, authentication=False, output_type_serializer=PlacesSerializer, *args, **kwargs)
        self.parsers["get"].add_argument("bss_stands", type=BooleanType(), default=False,
                                         help="Deprecated - Use add_poi_infos[]=bss_stands")
        self.parsers["get"].add_argument("add_poi_infos[]", type=OptionValue(add_poi_infos_types),
                                         default=['bss_stands', 'car_park'],
                                         dest="add_poi_infos", action="append",
                                         help="Show more information about the poi if it's available, for instance, "
                                              "show BSS/car park availability in the pois(BSS/car park) of the response")
        self.parsers['get'].add_argument("disable_geojson", type=BooleanType(), default=False,
                                         help="remove geojson from the response")
        args = self.parsers["get"].parse_args()

        if handle_poi_infos(args["add_poi_infos"], args["bss_stands"]):
            self.get_decorators.insert(1, ManageParkingPlaces(self, 'places'))

        if args['disable_geojson']:
            g.disable_geojson = True

        self.parsers['get'].add_argument("_autocomplete", type=six.text_type, hidden=True,
                                         help="name of the autocomplete service, used under the hood")

    def get(self, id, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        args.update({
            "uri": transform_id(id),
            "_current_datetime": datetime.utcnow()})
        if any([region, lon, lat]):
            self.region = i_manager.get_region(region, lon, lat)
            timezone.set_request_timezone(self.region)
            response = i_manager.dispatch(args, "place_uri", instance_name=self.region)
        else:
            user = authentication.get_user(token=authentication.get_token(), abort_if_no_token=False)
            available_instances = get_all_available_instances(user)
            autocomplete = global_autocomplete.get('bragi')
            if not autocomplete:
                raise TechnicalError('world wide autocompletion service not available')
            response = autocomplete.get_by_uri(args["uri"], instances=available_instances)

        return response, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)


place_nearby = deepcopy(place)
place_nearby["distance"] = fields.String()
places_nearby = {
    "places_nearby": NonNullList(NonNullNested(place_nearby)),
    "error": PbField(error, attribute='error'),
    "pagination": PbField(pagination),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
    "feed_publishers": fields.List(NonNullNested(feed_publisher))
}

places_types = {'stop_areas', 'stop_points', 'pois',
                'addresses', 'coords', 'places', 'coord'}  # add admins when possible


class PlacesNearby(ResourceUri):

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, output_type_serializer=PlacesNearbySerializer, *args, **kwargs)
        self.parsers["get"].add_argument("type[]", type=OptionValue(list(places_type.keys())),
                                         action="append",
                                         default=["stop_area", "stop_point", "poi"],
                                         help="Type of the objects to return")
        self.parsers["get"].add_argument("filter", type=six.text_type, default="",
                                         help="Filter your objects")
        self.parsers["get"].add_argument("distance", type=int, default=500,
                                         help="Distance range of the query in meters")
        self.parsers["get"].add_argument("count", type=default_count_arg_type, default=10,
                                         help="Elements per page")
        self.parsers["get"].add_argument("depth", type=depth_argument, default=1,
                                         help="Maximum depth on objects")
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                                         help="The page number of the ptref result")
        self.parsers["get"].add_argument("bss_stands", type=BooleanType(), default=False,
                                         help="Deprecated - Use add_poi_infos[]=bss_stands")
        self.parsers["get"].add_argument("add_poi_infos[]", type=OptionValue(add_poi_infos_types),
                                         default=['bss_stands', 'car_park'],
                                         dest="add_poi_infos", action="append",
                                         help="Show more information about the poi if it's available, for instance, "
                                              "show BSS/car park availability in the pois(BSS/car park) of the response")
        self.parsers["get"].add_argument("_current_datetime", type=DateTimeFormat(),
                                         schema_metadata={'default': 'now'}, hidden=True,
                                         default=datetime.utcnow(),
                                         help='The datetime considered as "now". Used for debug, default is '
                                              'the moment of the request. It will mainly change the output '
                                              'of the disruptions.')
        self.parsers['get'].add_argument("disable_geojson", type=BooleanType(), default=False,
                                         help="remove geojson from the response")
        args = self.parsers["get"].parse_args()
        if handle_poi_infos(args["add_poi_infos"], args["bss_stands"]):
            self.get_decorators.insert(1, ManageParkingPlaces(self, 'places_nearby'))

    @get_serializer(serpy=PlacesNearbySerializer, marshall=places_nearby)
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()
        if args['disable_geojson']:
            g.disable_geojson = True
        if uri:
            if uri[-1] == '/':
                uri = uri[:-1]
            uris = uri.split("/")
            if len(uris) >= 2:
                args["uri"] = transform_id(uris[-1])
                # for coherence we check the type of the object
                obj_type = uris[-2]
                if obj_type not in places_types:
                    abort(404, message='places_nearby api not available for {}'.format(obj_type))
            else:
                abort(404)
        elif lon and lat:
            # check if lon and lat can be converted to float
            float(lon)
            float(lat)
            args["uri"] = "coord:{}:{}".format(lon, lat)
        else:
            abort(404)
        args["filter"] = args["filter"].replace(".id", ".uri")
        self._register_interpreted_parameters(args)
        response = i_manager.dispatch(args, "places_nearby",
                                      instance_name=self.region)
        return response, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)
