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

from flask import Flask, request
from flask.ext.restful import Resource, fields, marshal_with, reqparse, abort
from flask.globals import g
from jormungandr import i_manager, timezone
from jormungandr.interfaces.v1.fields import use_old_disruptions_if_needed, DisruptionsField
from make_links import add_id_links
from fields import place, NonNullList, NonNullNested, PbField, pagination, error
from ResourceUri import ResourceUri
from make_links import add_id_links
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument
from copy import deepcopy
from jormungandr.interfaces.v1.transform_id import transform_id
from elasticsearch import Elasticsearch
from elasticsearch.connection.http_urllib3 import ConnectionError
from jormungandr.exceptions import TechnicalError
from functools import wraps


places = {
    "places": NonNullList(NonNullNested(place)),
    "error": PbField(error, attribute='error'),
    "disruptions": DisruptionsField,
}


def marshal_es_admin(dict):
    return {
        "id": dict["id"],
        "insee": dict["id"][6:],
        #"coord": ??
        "level": dict["level"],
        "name": dict["name"],
        "label": dict["name"],
        "zip_code": dict["zip_code"],
    }

def marshal_es_place(place):
    source = place["_source"]
    if place["_type"] == "addr":
        id = "{lat};{lon}".format(**source["coord"])
        return {
            "embeded_type": "address",
            "id": id,
            "name": source["name"],
            "address": {
                "id": id,
                "coord": source["coord"],
                "house_number": source["house_number"],
                "label": source["name"],
                "name": source["street"]["street_name"],
                "administrative_regions": [
                    marshal_es_admin(source["street"]["administrative_region"])
                ]
            }
        }
    if place["_type"] == "street":
        return {
            "embeded_type": "address",
            #"id": id,
            "name": source["name"],
            "address": {
                #"id": id,
                #"coord": source["coord"],
                #"house_number": 0,
                "label": source["name"],
                "name": source["street_name"],
                "administrative_regions": [
                    marshal_es_admin(source["administrative_region"])
                ]
            }
        }
    if place["_type"] == "admin":
        return {
            "embeded_type": "administrative_region",
            "id": source["id"],
            "name": source["name"],
            "administrative_region": marshal_es_admin(source)
        }
    return place

class marshal_es:
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            resp, code = f(*args, **kwargs)
            return {
                "places": map(marshal_es_place, resp),
                "disruptions": []
            }, code
        return wrapper


class Places(ResourceUri):
    parsers = {}

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, authentication=False, *args, **kwargs)
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        self.parsers["get"].add_argument("q", type=unicode, required=True,
                                         description="The data to search")
        self.parsers["get"].add_argument("type[]", type=str, action="append",
                                         default=["stop_area", "address",
                                                  "poi",
                                                  "administrative_region"],
                                         description="The type of data to\
                                         search")
        self.parsers["get"].add_argument("count", type=int, default=10,
                                         description="The maximum number of\
                                         places returned")
        self.parsers["get"].add_argument("search_type", type=int, default=0,
                                         description="Type of search:\
                                         firstletter or type error")
        self.parsers["get"].add_argument("admin_uri[]", type=str,
                                         action="append",
                                         description="If filled, will\
                                         restrained the search within the\
                                         given admin uris")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="The depth of objects")
        self.parsers["get"].add_argument("_use_old_disruptions", type=bool,
                                description="temporary boolean to use the old disruption interface. "
                                            "Will be deleted soon, just needed for synchronization with the front end",
                                default=False)

    @use_old_disruptions_if_needed()
    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        self._register_interpreted_parameters(args)
        g.use_old_disruptions = args['_use_old_disruptions']
        if len(args['q']) == 0:
            abort(400, message="Search word absent")

        if not any([region, lon, lat]):
            return self.get_ww(args)
        else:
            return self.get_region(args, region, lon, lat)

    @marshal_with(places)
    def get_region(self, args, region=None, lon=None, lat=None):
        self.region = i_manager.get_region(region, lon, lat)
        response = i_manager.dispatch(args, "places", instance_name=self.region)
        return response, 200

    @marshal_es()
    def get_ww(self, args):
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
            # TODO: get params?
            es = Elasticsearch()
            res = es.search(index="munin", size=args['count'], body=query)
            return res['hits']['hits'], 200
        except ConnectionError:
            raise TechnicalError("world wide autocompletion service not available")


class PlaceUri(ResourceUri):

    @use_old_disruptions_if_needed()
    @marshal_with(places)
    def get(self, id, region=None, lon=None, lat=None):
        self.region = i_manager.get_region(region, lon, lat)
        args = {"uri": transform_id(id)}
        response = i_manager.dispatch(args, "place_uri",
                                      instance_name=self.region)
        return response, 200

place_nearby = deepcopy(place)
place_nearby["distance"] = fields.Float()
places_nearby = {
    "places_nearby": NonNullList(NonNullNested(place_nearby)),
    "error": PbField(error, attribute='error'),
    "pagination": PbField(pagination),
    "disruptions": DisruptionsField,
}


class PlacesNearby(ResourceUri):
    parsers = {}

    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, *args, **kwargs)
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        self.parsers["get"].add_argument("type[]", type=str,
                                         action="append",
                                         default=["stop_area", "stop_point",
                                                  "poi"],
                                         description="Type of the objects to\
                                         return")
        self.parsers["get"].add_argument("filter", type=str, default="",
                                         description="Filter your objects")
        self.parsers["get"].add_argument("distance", type=int, default=500,
                                         description="Distance range of the\
                                         query")
        self.parsers["get"].add_argument("count", type=int, default=10,
                                         description="Elements per page")
        self.parsers["get"].add_argument("depth", type=depth_argument,
                                         default=1,
                                         description="Maximum depth on\
                                         objects")
        self.parsers["get"].add_argument("start_page", type=int, default=0,
                                         description="The page number of the\
                                         ptref result")

    @use_old_disruptions_if_needed()
    @marshal_with(places_nearby)
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()
        if uri:
            if uri[-1] == '/':
                uri = uri[:-1]
            uris = uri.split("/")
            if len(uris) > 1:
                args["uri"] = transform_id(uris[-1])
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
