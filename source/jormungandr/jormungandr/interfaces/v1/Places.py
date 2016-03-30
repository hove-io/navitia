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
from flask.ext.restful import Resource, fields, marshal_with, reqparse, abort
from flask.globals import g
from jormungandr import i_manager, timezone, autocomplete
from jormungandr.interfaces.v1.fields import disruption_marshaller
from jormungandr.interfaces.v1.make_links import add_id_links
from jormungandr.interfaces.v1.fields import place, NonNullList, NonNullNested, PbField, pagination, error, coord, feed_publisher
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument, default_count_arg_type, date_time_format
from copy import deepcopy
from jormungandr.interfaces.v1.transform_id import transform_id
from elasticsearch import Elasticsearch
from elasticsearch.connection.http_urllib3 import ConnectionError
from jormungandr.exceptions import TechnicalError
from functools import wraps
from flask_restful import marshal
import datetime


class Lit(fields.Raw):
    def __init__(self, val):
        self.val = val

    def output(self, key, obj):
        return self.val


places = {
    "places": NonNullList(NonNullNested(place)),
    "error": PbField(error, attribute='error'),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
    "feed_publishers": fields.List(NonNullNested(feed_publisher))
}

ww_admin = {
    "id": fields.String,
    #"insee": dict["id"][6:],
    #"coord": ??
    "level": fields.Integer,
    "name": fields.String,
    "label": fields.String(attribute="name"),
    "zip_code": fields.String,
}

ww_address = {
    "embeded_type": Lit("address"),
    "id": fields.String,
    "name": fields.String,
    "address": {
        "id": fields.String,
        "coord": {
            "lon": fields.Float(attribute="coord.lon"),
            "lat": fields.Float(attribute="coord.lat"),
        },
        "house_number": fields.String,
        "label": fields.String(attribute="name"),
        "name": fields.String(attribute="street.street_name"),
        "administrative_regions":
            fields.List(fields.Nested(ww_admin), attribute="street.administrative_region")
    }
}

ww_street = {
    "embeded_type": Lit("address"),
    #"id": id,
    "name": fields.String,
    "address": {
        #"id": id,
        #"coord": source["coord"],
        #"house_number": 0,
        "label": fields.String(attribute="name"),
        "name": fields.String(attribute="street_name"),
        "administrative_regions":
            fields.List(fields.Nested(ww_admin), attribute="administrative_region")
    }
}

class WWPlace(fields.Raw):
    def format(self, place):
        source = place["_source"]
        if place["_type"] == "addr":
            source["id"] = "{lat};{lon}".format(**source["coord"])
            return marshal(source, ww_address)
        if place["_type"] == "street":
            return marshal(source, ww_street)
        if place["_type"] == "admin":
            return {
                "embeded_type": "administrative_region",
                "id": source["id"],
                "name": source["name"],
                "administrative_region": marshal(source, ww_admin)
            }
        return place


ww_places = {
    "places": fields.List(WWPlace, attribute='hits')
}


class WorldWidePlaces(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, authentication=False, *args, **kwargs)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                                         description="The data to search")
        self.parsers["get"].add_argument("count", type=default_count_arg_type, default=10,
                                         description="The maximum number of\
                                         places returned")

    def get(self):
        args = self.parsers["get"].parse_args()
        self._register_interpreted_parameters(args)
        if len(args['q']) == 0:
            abort(400, message="Search word absent")
        return self._search_world_wide(args)

    @marshal_with(ww_places)
    def _search_world_wide(self, args):
        q = args['q']
        query = {
            "query": {
                "filtered": {
                    "query": {
                        "bool": {
                            "should": [
                                {
                                    "term": {
                                        "_type": {
                                            "value": "addr",
                                            "boost": 1000
                                        }
                                    }
                                },
                                {
                                    "match": {
                                        "name.prefix": {
                                            "query": q,
                                            "boost": 100
                                        }
                                    }
                                },
                                {
                                    "match": {
                                        "name.ngram": {
                                            "query": q,
                                            "boost": 1
                                        }
                                    }
                                },
                                {
                                    "function_score": {
                                        "query": { "match_all": { } },
                                        "field_value_factor": {
                                            "field": "weight",
                                            "modifier": "log1p",
                                            "factor": 1
                                        },
                                        "boost_mode": "multiply",
                                        "boost": 30
                                    }
                                }
                            ]
                        }
                    },
                    "filter": {
                        "bool": {
                            "should": [
                                { "missing": { "field": "house_number" } },
                                {
                                    "query": {
                                        "match": { "house_number": q }
                                    }
                                }
                            ],
                            "must": [
                                {
                                    "query": {
                                        "match": {
                                            "name.ngram": {
                                                "query": q,
                                                "minimum_should_match": "50%"
                                            }
                                        }
                                    }
                                }
                            ]
                        }
                    }
                }
            }
        }
        try:
            es = Elasticsearch(autocomplete["args"]["hosts"],
                               http_auth=(autocomplete["args"]['user'],
                                          autocomplete["args"]['password']),
                               use_ssl=autocomplete["args"]['use_ssl'])
            res = es.search(index="munin", size=args['count'], body=query)
            return res['hits'], 200
        except ConnectionError:
            raise TechnicalError("world wide autocompletion service not available")


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

    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        self._register_interpreted_parameters(args)
        if len(args['q']) == 0:
            abort(400, message="Search word absent")

        # If a region or coords are asked, we do the search according
        # to the region, else, we do a word wide search

        if any([region, lon, lat]):
            return self._search_region(args, region, lon, lat)

    @marshal_with(places)
    def _search_region(self, args, region=None, lon=None, lat=None):
        self.region = i_manager.get_region(region, lon, lat)
        response = i_manager.dispatch(args, "places", instance_name=self.region)
        return response, 200


class PlaceUri(ResourceUri):

    @marshal_with(places)
    def get(self, id, region=None, lon=None, lat=None):
        self.region = i_manager.get_region(region, lon, lat)
        args = {
            "uri": transform_id(id),
            "_current_datetime": datetime.datetime.utcnow()}
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
        parser_get = self.parsers["get"]
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

        self.parsers["get"].add_argument("_current_datetime", type=date_time_format, default=datetime.datetime.utcnow(),
                                         description="The datetime used to consider the state of the pt object"
                                                     " Default is the current date and it is used for debug."
                                                     " Note: it will mainly change the disruptions that concern "
                                                     "the object The timezone should be specified in the format,"
                                                     " else we consider it as UTC")

    @marshal_with(places_nearby)
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()
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
