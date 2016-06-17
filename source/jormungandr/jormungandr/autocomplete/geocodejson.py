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
from functools import wraps


class Lit(fields.Raw):
    def __init__(self, val):
        self.val = val

    def output(self, key, obj):
        return self.val


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
    for level, name in admin_list.iteritems():
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
        response.append({
            "insee": admin.get('insee'),
            "name": admin.get('name'),
            "level": int(admin.get('level')),
            "coord": {
                "lat": admin.get('coord', {}).get('lat'),
                "lon": admin.get('coord', {}).get('lon')
            },
            "label": admin.get('name'),
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
            "label": geocoding.get('name'),
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
    "places": fields.List(GeocodejsonFeature, attribute='features')
}


class delete_attribute_autocomplete():
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            return objects.get('Autocomplete') or objects
        return wrapper

class GeocodeJson(AbstractAutocomplete):
    """
    Autocomplete with an external service returning geocodejson
    (https://github.com/geocoders/geocodejson-spec/)

    """
    def __init__(self, **kwargs):
        self.external_api = kwargs.get('host')
        self.timeout = kwargs.get('timeout', 10)

    @marshal_with(geocodejson)
    # TODO: To be deleted when bragi is modified
    @delete_attribute_autocomplete()
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


