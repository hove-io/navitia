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
from flask import Flask, request
from flask.ext.restful import Resource, fields, reqparse, abort
from flask.ext.restful.inputs import boolean
from flask.globals import g
from jormungandr import i_manager, timezone, global_autocomplete, bss_provider_manager, authentication
from jormungandr.interfaces.v1.fields import disruption_marshaller
from jormungandr.interfaces.v1.make_links import add_id_links
from jormungandr.interfaces.v1.fields import place, NonNullList, NonNullNested, PbField, pagination,\
                                             error, feed_publisher, Lit, ListLit, beta_endpoint
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument, default_count_arg_type, date_time_format
from copy import deepcopy
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.exceptions import TechnicalError
from functools import wraps
from flask_restful import marshal, marshal_with
import datetime
from jormungandr.parking_space_availability.bss.stands_manager import ManageStands
import ujson as json


#global marshal
def delete_prefix(value, prefix):
    if value and value.startswith(prefix):
        return value[len(prefix):]
    return value

def create_admin_field(geocoding):
    """
    This field is needed to respect the geocodejson-spec
    https://github.com/geocoders/geocodejson-spec/tree/master/draft#feature-object
    """
    if not geocoding:
        return None
    admin_list = geocoding.get('admin', {})
    response = []
    for level, name in admin_list.items():
        response.append({
            "insee": None,
            "name": name,
            "level": int(level.replace('level', '')),
            "coord": {"lat": None, "lon": None},
            "label": None,
            "id": None,
            "zip_code": None
        })
    return response

class AddressId(fields.Raw):
    def output(self, key, obj):
        if not obj:
            return None
        geocoding = obj.get('properties', {}).get('geocoding', {})
        return delete_prefix(geocoding.get('id'), "addr:")

def create_administrative_regions_field(geocoding):
    if not geocoding:
        return None
    administrative_regions = geocoding.get('administrative_regions', {})
    response = []
    for admin in administrative_regions:
        coord = admin.get('coord', {})
        lat = coord.get('lat') if coord else None
        lon = coord.get('lon') if coord else None
        response.append({
            "insee": admin.get('insee'),
            "name": admin.get('label'),
            "level": int(admin.get('level')),
            "coord": {
                "lat": lat,
                "lon": lon
            },
            "label": admin.get('label'),
            "id": admin.get('id'),
            "zip_code": admin.get('zip_code')
        })
    return response


class AdministrativeRegionField(fields.Raw):
    """
    This field is needed to respect Navitia's spec for the sake of compatibility
    """
    def output(self, key, obj):
        if not obj:
            return None
        geocoding = obj.get('properties', {}).get('geocoding', {})
        return create_administrative_regions_field(geocoding) or create_admin_field(geocoding)


class AddressField(fields.Raw):
    def output(self, key, obj):
        if not obj:
            return None

        coordinates = obj.get('geometry', {}).get('coordinates', [])
        if len(coordinates) == 2:
            lon = coordinates[0]
            lat = coordinates[1]
        else:
            lon = None
            lat = None

        geocoding = obj.get('properties', {}).get('geocoding', {})

        return {
            "id": delete_prefix(geocoding.get('id'), "addr:"),
            "coord": {
                "lon": lon,
                "lat": lat,
            },
            "house_number": geocoding.get('housenumber') or '0',
            "label": geocoding.get('label'),
            "name": geocoding.get('name'),
            "administrative_regions":
                create_administrative_regions_field(geocoding) or create_admin_field(geocoding) ,
        }

geocode_admin = {
    "embedded_type": Lit("administrative_region"),
    "quality": Lit("0"),
    "id": fields.String(attribute='properties.geocoding.id'),
    "name": fields.String(attribute='properties.geocoding.name'),
    "administrative_regions": AdministrativeRegionField()
}


geocode_addr = {
    "embedded_type": Lit("address"),
    "quality": Lit("0"),
    "id": AddressId,
    "name": fields.String(attribute='properties.geocoding.label'),
    "address": AddressField()
}

geocode_poi = {
    "embedded_type": Lit("poi"),
    "quality": Lit("0"),
    "id": fields.String(attribute='properties.geocoding.id'),
    "name": fields.String(attribute='properties.geocoding.label'),
    "poi": AddressField()
}

class GeocodejsonFeature(fields.Raw):
    def format(self, place):
        type_ = place.get('properties', {}).get('geocoding', {}).get('type')

        if type_ == 'city':
            return marshal(place, geocode_admin)
        elif type_ in ('street', 'house'):
            return marshal(place, geocode_addr)
        elif type_ == 'poi':
            return marshal(place, geocode_poi)

        return place

geocodejson = {
    "places": fields.List(GeocodejsonFeature, attribute='features'),
    "warnings": ListLit([fields.Nested(beta_endpoint)]),
}


#instance marshal
places = {
    "places": NonNullList(NonNullNested(place)),
    "error": PbField(error, attribute='error'),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
    "feed_publishers": fields.List(NonNullNested(feed_publisher))
}


class Places(ResourceUri):

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, authentication=False, *args, **kwargs)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                                         description="The data to search")
        self.parsers["get"].add_argument("type[]", type=unicode, action="append",
                                         default=["stop_area", "address",
                                                  "poi",
                                                  "administrative_region"],
                                         description="The type of data to\
                                         search")
        self.parsers["get"].add_argument("count", type=default_count_arg_type, default=10,
                                         description="The maximum number of\
                                         places returned")
        self.parsers["get"].add_argument("search_type", type=int, default=0,
                                         description="Type of search:\
                                         firstletter or type error")
        self.parsers["get"].add_argument("admin_uri[]", type=unicode,
                                         action="append",
                                         description="If filled, will\
                                         restrained the search within the\
                                         given admin uris")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="The depth of objects")
        self.parsers["get"].add_argument("_current_datetime", type=date_time_format, default=datetime.datetime.utcnow(),
                                         description="The datetime used to consider the state of the pt object"
                                                     " Default is the current date and it is used for debug."
                                                     " Note: it will mainly change the disruptions that concern "
                                                     "the object The timezone should be specified in the format,"
                                                     " else we consider it as UTC")
        self.parsers['get'].add_argument("disable_geojson", type=boolean, default=False,
                            description="remove geojson from the response")

    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        self._register_interpreted_parameters(args)
        if len(args['q']) == 0:
            abort(400, message="Search word absent")

        if args['disable_geojson']:
            g.disable_geojson = True

        # If a region or coords are asked, we do the search according
        # to the region, else, we do a word wide search

        if any([region, lon, lat]):
            instance = i_manager.get_region(region, lon, lat)
            timezone.set_request_timezone(region)
            response = i_manager.dispatch(args, "places", instance_name=instance)
        else:
            if global_autocomplete:
                user = authentication.get_user(token=authentication.get_token(), abort_if_no_token=False)
                shape = None
                if user and user.shape :
                    shape = json.loads(user.shape)
                bragi_response = global_autocomplete.get(args, None, shape=shape)
                response = marshal(bragi_response, geocodejson)
            else:
                raise TechnicalError('world wide autocompletion service not available')
        return response, 200


class PlaceUri(ResourceUri):

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("bss_stands", type=boolean, default=True,
                                         description="Show bss stands availability")
        self.parsers['get'].add_argument("disable_geojson", type=boolean, default=False,
                            description="remove geojson from the response")
        args = self.parsers["get"].parse_args()
        if args["bss_stands"]:
            self.method_decorators.insert(1, ManageStands(self, 'places'))
        if args['disable_geojson']:
            g.disable_geojson = True

    @marshal_with(places)
    def get(self, id, region=None, lon=None, lat=None):
        self.region = i_manager.get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()
        args.update({
            "uri": transform_id(id),
            "_current_datetime": datetime.datetime.utcnow()})
        response = i_manager.dispatch(args, "place_uri",
                                      instance_name=self.region)
        return response, 200

place_nearby = deepcopy(place)
place_nearby["distance"] = fields.String()
places_nearby = {
    "places_nearby": NonNullList(NonNullNested(place_nearby)),
    "error": PbField(error, attribute='error'),
    "pagination": PbField(pagination),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
}

places_types = {'stop_areas', 'stop_points', 'pois',
                'addresses', 'coords', 'places', 'coord'}  # add admins when possible


class PlacesNearby(ResourceUri):

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("type[]", type=unicode,
                                         action="append",
                                         default=["stop_area", "stop_point",
                                                  "poi"],
                                         description="Type of the objects to\
                                         return")
        self.parsers["get"].add_argument("filter", type=unicode, default="",
                                         description="Filter your objects")
        self.parsers["get"].add_argument("distance", type=int, default=500,
                                         description="Distance range of the\
                                         query")
        self.parsers["get"].add_argument("count", type=default_count_arg_type, default=10,
                                         description="Elements per page")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="Maximum depth on\
                                         objects")
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                                         description="The page number of the\
                                         ptref result")
        self.parsers["get"].add_argument("bss_stands", type=boolean, default=True,
                                         description="Show bss stands availability")

        self.parsers["get"].add_argument("_current_datetime", type=date_time_format, default=datetime.datetime.utcnow(),
                                         description="The datetime used to consider the state of the pt object"
                                                     " Default is the current date and it is used for debug."
                                                     " Note: it will mainly change the disruptions that concern "
                                                     "the object The timezone should be specified in the format,"
                                                     " else we consider it as UTC")
        self.parsers['get'].add_argument("disable_geojson", type=boolean, default=False,
                            description="remove geojson from the response")
        args = self.parsers["get"].parse_args()
        if args["bss_stands"]:
            self.method_decorators.insert(1, ManageStands(self, 'places_nearby'))

    @marshal_with(places_nearby)
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
