# coding=utf-8

# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
from jormungandr.autocomplete.abstract_autocomplete import AbstractAutocomplete
import elasticsearch
from elasticsearch.connection.http_urllib3 import ConnectionError
from flask.ext.restful import fields, marshal_with
from flask_restful import marshal


class Lit(fields.Raw):
    def __init__(self, val):
        self.val = val

    def output(self, key, obj):
        return self.val

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


class Elasticsearch(AbstractAutocomplete):

    def __init__(self, **kwargs):
        self.es = elasticsearch.Elasticsearch(kwargs.get('hosts'),
                                              http_auth=(kwargs.get('user'),
                                              kwargs.get('password')),
                                              use_ssl=kwargs.get('use_ssl'))

    @marshal_with(ww_places)
    def get(self, request, instance):
        q = request['q']
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
            res = self.es.search(index="munin", size=request['count'], body=query)
            return res['hits']
        except ConnectionError:
            raise TechnicalError("world wide autocompletion service not available")
