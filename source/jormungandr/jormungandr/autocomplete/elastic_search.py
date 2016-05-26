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
import logging
from jormungandr.autocomplete.abstract_autocomplete import AbstractAutocomplete
import elasticsearch
from elasticsearch.connection.http_urllib3 import ConnectionError
from flask.ext.restful import fields, marshal_with
from flask_restful import marshal
import requests
from jormungandr.exceptions import TechnicalError


class Lit(fields.Raw):
    def __init__(self, val):
        self.val = val

    def output(self, key, obj):
        return self.val

ww_admin = {
    "id": fields.String,
    # "insee": dict["id"][6:],
    # "coord": ??
    "level": fields.Integer,
    "name": fields.String,
    "label": fields.String(attribute="name"),
    "zip_code": fields.String(attribute="postcode")
}

ww_address = {
    "embedded_type": Lit("address"),
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
    "embedded_type": Lit("address"),
    # "id": id,
    "name": fields.String,
    "address": {
        # "id": id,
        # "coord": source["coord"],
        # "house_number": 0,
        "label": fields.String(attribute="name"),
        "name": fields.String(attribute="street_name"),
        "administrative_regions":
            fields.List(fields.Nested(ww_admin), attribute="administrative_region")
    }
}


class WWPlace(fields.Raw):
    def format(self, place):
        source = place["_source"]
        admin = source.get('administrative_region')
        if place["_type"] == "addr":
            source["id"] = "{lat};{lon}".format(**source["coord"])
            if not isinstance(admin, list):
                source['administrative_region'] = [admin] if admin else []
            return marshal(source, ww_address)
        if place["_type"] == "street":
            if not isinstance(admin, list):
                source['administrative_region'] = [admin] if admin else []
            return marshal(source, ww_street)
        if place["_type"] == "admin":
            return {
                "embedded_type": "administrative_region",
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
                                        "query": {"match_all": {}},
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
                                {"missing": {"field": "house_number"}},
                                {
                                    "query": {
                                        "match": {"house_number": q}
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


def create_admin_field(geocoding):
        if not geocoding:
            return None
        admin_list = geocoding.get('admin', {})
        response = []
        for level, name in admin_list.iteritems():
            response.append({
                "insee": geocoding.get('TODO'),
                "name": name,
                "level": int(level.replace('level', '')),
                "coord": {
                    "lat": geocoding.get('TODO'),
                    "lon": geocoding.get('TODO')
                },
                "label": name,
                "id": geocoding.get('TODO'),
                "zip_code": geocoding.get('TODO')
            })
        return response


class AdminField(fields.Raw):
    def output(self, key, obj):
        if not obj:
            return None
        geocoding = obj.get('properties', {}).get('geocoding', {})
        return create_admin_field(geocoding)


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

        housenumber = geocoding.get('housenumber')
        return {
            "id": geocoding.get('id'),
            "coord": {
                "lon": lon,
                "lat": lat,
            },
            "house_number": geocoding.get('housenumber') or '0',
            "label": geocoding.get('name'),
            "name": geocoding.get('name'),
            "administrative_regions": create_admin_field(geocoding),
        }

geocode_admin = {
    "embedded_type": Lit("administrative_region"),
    "quality": Lit("0"),
    "id": fields.String(attribute='properties.geocoding.id'),
    "name": fields.String(attribute='properties.geocoding.name'),
    "administrative_region": AdminField()
}

geocode_addr = {
    "embedded_type": Lit("address"),
    "quality": Lit("0"),
    "id": fields.String(attribute='properties.geocoding.id'),
    "name": fields.String(attribute='properties.geocoding.name'),
    "address": AddressField()
}


class GeocodejsonFeature(fields.Raw):
    def format(self, place):
        type_ = place.get('properties', {}).get('geocoding', {}).get('type')

        if type_ == 'city':
            return marshal(place, geocode_admin)
        elif type_ in ('street', 'house'):
            return marshal(place, geocode_addr)

        return place

geocodejson = {
    "places": fields.List(GeocodejsonFeature, attribute='Autocomplete.features')
}


class GeocodeJson(AbstractAutocomplete):
    """
    Autocomplete with an external service returning geocodejson
    (https://github.com/geocoders/geocodejson-spec/)

    """
    def __init__(self, **kwargs):
        self.external_api = kwargs.get('host')
        self.timeout = kwargs.get('timeout', 10)

    @marshal_with(geocodejson)
    def get(self, request, instance):
        if not self.external_api:
            raise TechnicalError('global autocomplete not configured')

        q = request['q']
        url = '{endpoint}?q={q}'.format(endpoint=self.external_api, q=q)
        try:
            raw_response = requests.get(url, timeout=self.timeout)
        except requests.Timeout:
            logging.getLogger(__name__).error('autocomplete request timeout')
            raise TechnicalError('external autocomplete service timeout')
        except:
            logging.getLogger(__name__).exception('error in autocomplete request')
            raise TechnicalError('impossible to access external autocomplete service')

        return raw_response.json()


