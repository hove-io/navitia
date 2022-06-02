# coding=utf-8

# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from flask_restful import abort
from flask.globals import g
import flask
from jormungandr.authentication import get_all_available_instances
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.serializer.api import PlacesSerializer, PlacesNearbySerializer
from jormungandr import i_manager, timezone, global_autocomplete, authentication, app
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.parsers import default_count_arg_type, places_count_arg_type
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.exceptions import TechnicalError, InvalidArguments
from datetime import datetime
from jormungandr.parking_space_availability.parking_places_manager import ManageParkingPlaces
import ujson as json
from jormungandr.scenarios.utils import places_type
from navitiacommon import parser_args_type
from navitiacommon.parser_args_type import (
    TypeSchema,
    CoordFormat,
    CustomSchemaType,
    BooleanType,
    OptionValue,
    DateTimeFormat,
    DepthArgument,
    IntRange,
)
from navitiacommon.constants import ENUM_SHAPE_SCOPE, DEFAULT_SHAPE_SCOPE
from jormungandr.interfaces.common import add_poi_infos_types, handle_poi_infos
import six
from jormungandr.instance import Instance
from typing import Optional, Dict


class geojson_argument(CustomSchemaType):
    def __call__(self, value):
        decoded = json.loads(value)
        if not decoded:
            raise ValueError('invalid shape')

        return parser_args_type.geojson_argument(decoded)

    def schema(self):
        return TypeSchema(type=str)  # TODO a better description of the geojson


def build_instance_shape(instance):
    # type: (Instance) -> Optional[Dict]
    if instance and instance.geojson:
        return {"type": "Feature", "properties": {}, "geometry": instance.geojson}
    return None


class Places(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(
            self, authentication=False, output_type_serializer=PlacesSerializer, *args, **kwargs
        )
        self.parsers["get"].add_argument("q", type=six.text_type, required=True, help="The data to search")
        self.parsers["get"].add_argument(
            "type[]",
            type=OptionValue(list(places_type)),
            action="append",
            default=["stop_area", "address", "poi", "administrative_region"],
            help="The type of data to search",
        )
        self.parsers["get"].add_argument(
            "count", type=places_count_arg_type, default=10, help="The maximum number of places returned"
        )
        self.parsers["get"].add_argument(
            "search_type", type=int, default=0, hidden=True, help="Type of search: firstletter or type error"
        )
        self.parsers["get"].add_argument(
            "_main_stop_area_weight_factor",
            type=float,
            default=1.0,
            hidden=True,
            help="multiplicator for the weight of main stop area",
        )
        self.parsers["get"].add_argument(
            "admin_uri[]",
            type=six.text_type,
            action="append",
            help="If filled, will restrain the search within the " "given admin uris",
        )
        self.parsers["get"].add_argument("depth", type=DepthArgument(), default=1, help="The depth of objects")
        self.parsers["get"].add_argument(
            "_current_datetime",
            type=DateTimeFormat(),
            schema_metadata={'default': 'now'},
            hidden=True,
            default=datetime.utcnow(),
            help='The datetime considered as "now". Used for debug, default is '
            'the moment of the request. It will mainly change the output '
            'of the disruptions.',
        )
        self.parsers['get'].add_argument(
            "disable_geojson", type=BooleanType(), default=False, help="remove geojson from the response"
        )

        self.parsers['get'].add_argument(
            "from",
            type=CoordFormat(nullable=True),
            help="Coordinates longitude;latitude used to prioritize " "the objects around this coordinate",
        )
        self.parsers['get'].add_argument(
            "_autocomplete",
            type=six.text_type,
            hidden=True,
            choices=['kraken', 'bragi', 'bragi7'],
            help="name of the autocomplete service, used under the hood",
        )
        self.parsers['get'].add_argument(
            'shape', type=geojson_argument(), help='Geographical shape to limit the search.'
        )

        self.parsers["get"].add_argument(
            "shape_scope[]",
            type=OptionValue(ENUM_SHAPE_SCOPE),
            action="append",
            help="The scope shape on data to search",
        )
        self.parsers['get'].add_argument(
            "places_proximity_radius",
            type=IntRange(100, 1000000),
            help="Radius used to prioritize " "the objects around coordinate from",
        )

    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        self._register_interpreted_parameters(args)
        size_q = len(args['q'])
        if size_q == 0:
            abort(400, message="Search word absent")

        if size_q > 1024:
            abort(413, message="Number of characters allowed for the search is 1024")

        if args['disable_geojson']:
            g.disable_geojson = True

        user = authentication.get_user(token=authentication.get_token(), abort_if_no_token=False)

        if args['shape'] is None and user and user.shape:
            args['shape'] = json.loads(user.shape)

        if not args.get("shape_scope[]") and user:
            args.update({"shape_scope[]": user.shape_scope})

        if user and user.default_coord:
            if args['from'] is None:
                args['from'] = CoordFormat()(user.default_coord)
        else:
            if args['from'] == '':
                raise InvalidArguments("if 'from' is provided it cannot be null")

        # If a region or coords are asked, we do the search according
        # to the region, else, we do a word wide search
        args["request_id"] = args.get('request_id', flask.request.id)
        if any([region, lon, lat]):
            self.region = i_manager.get_region(region, lon, lat)

            # when autocompletion is done on a specific coverage we want to filter on its shape
            if not args['shape']:
                instance = i_manager.instances.get(self.region)
                args['shape'] = build_instance_shape(instance)
            timezone.set_request_timezone(self.region)
            response = i_manager.dispatch(args, "places", instance_name=self.region)
        else:
            available_instances = get_all_available_instances(user, exclude_backend='kraken')

            # If no instance available most probably due to database error
            if (not user) and (not available_instances):
                raise TechnicalError('world wide autocompletion service not available temporarily')

            # If parameter '_autocomplete' is absent use 'bragi' as default value
            if args["_autocomplete"] is None:
                args["_autocomplete"] = app.config.get('DEFAULT_AUTOCOMPLETE_BACKEND', 'bragi')
            autocomplete = global_autocomplete.get(args["_autocomplete"])
            if not autocomplete:
                raise TechnicalError('world wide autocompletion service not available')
            response = autocomplete.get(args, instances=available_instances)
        return response, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)


class PlaceUri(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(
            self, authentication=False, output_type_serializer=PlacesSerializer, *args, **kwargs
        )
        self.parsers["get"].add_argument(
            "bss_stands",
            type=BooleanType(),
            default=False,
            deprecated=True,
            help="DEPRECATED, Use add_poi_infos[]=bss_stands",
        )
        self.parsers["get"].add_argument(
            "add_poi_infos[]",
            type=OptionValue(add_poi_infos_types),
            default=['bss_stands', 'car_park'],
            dest="add_poi_infos",
            action="append",
            help="Show more information about the poi if it's available, for instance, "
            "show BSS/car park availability in the pois(BSS/car park) of the response",
        )
        self.parsers['get'].add_argument(
            "disable_geojson", type=BooleanType(), default=False, help="remove geojson from the response"
        )
        self.parsers['get'].add_argument(
            "disable_disruption", type=BooleanType(), default=False, help="remove disruptions from the response"
        )
        args = self.parsers["get"].parse_args()

        if handle_poi_infos(args["add_poi_infos"], args["bss_stands"]):
            self.get_decorators.insert(1, ManageParkingPlaces(self, 'places'))

        if args['disable_geojson']:
            g.disable_geojson = True

        self.parsers['get'].add_argument(
            "_autocomplete",
            type=six.text_type,
            hidden=True,
            choices=['kraken', 'bragi', 'bragi7'],
            help="name of the autocomplete service, used under the hood",
        )

    def get(self, id, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        args.update({"uri": transform_id(id), "_current_datetime": datetime.utcnow()})
        request_id = "places_{}".format(flask.request.id)
        args["request_id"] = request_id

        if any([region, lon, lat]):
            self.region = i_manager.get_region(region, lon, lat)
            timezone.set_request_timezone(self.region)
            response = i_manager.dispatch(args, "place_uri", instance_name=self.region)
        else:
            user = authentication.get_user(token=authentication.get_token(), abort_if_no_token=False)
            available_instances = get_all_available_instances(user)

            # If no instance available most probably due to database error
            if (not user) and (not available_instances):
                raise TechnicalError('world wide autocompletion service not available temporarily')

            # If parameter '_autocomplete' is absent use 'bragi' as default value
            if args["_autocomplete"] is None:
                args["_autocomplete"] = 'bragi'
            autocomplete = global_autocomplete.get(args["_autocomplete"])
            if not autocomplete:
                raise TechnicalError('world wide autocompletion service not available')
            response = autocomplete.get_by_uri(args["uri"], request_id=request_id, instances=available_instances)

        return response, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)


places_types = {
    'stop_areas',
    'stop_points',
    'pois',
    'addresses',
    'coords',
    'places',
    'coord',
}  # add admins when possible


class PlacesNearby(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, output_type_serializer=PlacesNearbySerializer, *args, **kwargs)
        parser_get = self.parsers["get"]
        parser_get.add_argument(
            "type[]",
            type=OptionValue(list(places_type.keys())),
            action="append",
            default=["stop_area", "stop_point", "poi"],
            help="Type of the objects to return",
        )
        parser_get.add_argument("filter", type=six.text_type, default="", help="Filter your objects")
        parser_get.add_argument("distance", type=int, default=500, help="Distance range of the query in meters")
        parser_get.add_argument("count", type=default_count_arg_type, default=10, help="Elements per page")
        parser_get.add_argument("depth", type=DepthArgument(), default=1, help="Maximum depth on objects")
        parser_get.add_argument("start_page", type=int, default=0, help="The page number of the ptref result")
        parser_get.add_argument(
            "bss_stands",
            type=BooleanType(),
            default=False,
            deprecated=True,
            help="DEPRECATED, Use add_poi_infos[]=bss_stands",
        )
        parser_get.add_argument(
            "add_poi_infos[]",
            type=OptionValue(add_poi_infos_types),
            default=['bss_stands', 'car_park'],
            dest="add_poi_infos",
            action="append",
            help="Show more information about the poi if it's available, for instance, "
            "show BSS/car park availability in the pois(BSS/car park) of the response",
        )
        parser_get.add_argument(
            "_current_datetime",
            type=DateTimeFormat(),
            schema_metadata={'default': 'now'},
            hidden=True,
            default=datetime.utcnow(),
            help='The datetime considered as "now". Used for debug, default is '
            'the moment of the request. It will mainly change the output '
            'of the disruptions.',
        )
        parser_get.add_argument(
            "disable_geojson", type=BooleanType(), default=False, help="remove geojson from the response"
        )
        parser_get.add_argument(
            "disable_disruption", type=BooleanType(), default=False, help="remove disruptions from the response"
        )

        args = parser_get.parse_args()
        if handle_poi_infos(args["add_poi_infos"], args["bss_stands"]):
            self.get_decorators.insert(1, ManageParkingPlaces(self, 'places_nearby'))

    @get_serializer(serpy=PlacesNearbySerializer)
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
        response = i_manager.dispatch(args, "places_nearby", instance_name=self.region)
        return response, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)
