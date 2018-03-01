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

import jormungandr
from jormungandr.autocomplete.abstract_autocomplete import AbstractAutocomplete
from jormungandr.utils import get_lon_lat as get_lon_lat_from_id, get_house_number
import requests
from jormungandr.exceptions import TechnicalError, UnknownObject
from flask.ext.restful import marshal, fields
from jormungandr.interfaces.v1.fields import Lit, ListLit, beta_endpoint, feed_publisher_bano, feed_publisher_osm
import re


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


class CoordId(fields.Raw):
    def output(self, key, obj):
        if not obj:
            return None
        lon, lat = get_lon_lat(obj)
        return '{};{}'.format(lon, lat)


def format_zip_code(zip_codes):
    if all(zip_code == "" for zip_code in zip_codes):
        return None
    elif len(zip_codes) == 1:
        return zip_codes[0]
    else:
        return '{}-{}'.format(min(zip_codes), max(zip_codes))


def create_administrative_regions_field(geocoding):
    if not geocoding:
        return None
    administrative_regions = geocoding.get('administrative_regions', {})
    response = []
    for admin in administrative_regions:
        coord = admin.get('coord', {})
        lat = str(coord.get('lat')) if coord and coord.get('lat') else None
        lon = str(coord.get('lon')) if coord and coord.get('lon') else None
        zip_codes = admin.get('zip_codes', [])
        response.append({
            "insee": admin.get('insee'),
            "name": admin.get('label'),
            "level":
                int(admin.get('level')) if admin.get('level') else None,
            "coord": {
                "lat": lat,
                "lon": lon
            },
            "label": admin.get('label'),
            "id": admin.get('id'),
            "zip_code": format_zip_code(zip_codes)
        })
    return response


def create_modes_field(modes):
    if not modes:
        return []
    return [{"id": mode.get('id'), "name": mode.get('name')} for mode in modes]


def get_lon_lat(obj):
    if not obj or not obj.get('geometry') or not obj.get('geometry').get('coordinates'):
        return None, None

    coordinates = obj.get('geometry', {}).get('coordinates', [])
    if len(coordinates) == 2:
        lon = str(coordinates[0])
        lat = str(coordinates[1])
    else:
        lon = None
        lat = None
    return lon, lat


def create_address_field(geocoding):
    if not geocoding:
        return None
    coord = geocoding.get('coord', {})
    lat = str(coord.get('lat')) if coord and coord.get('lat') else None
    lon = str(coord.get('lon')) if coord and coord.get('lon') else None

    return {
        "id": geocoding.get('id'),
        "label": geocoding.get('label'),
        "name": geocoding.get('name'),
        "coord": {
            "lat": lat,
            "lon": lon
        },
        "house_number": get_house_number(geocoding.get('housenumber'))
    }


class AdministrativeRegionField(fields.Raw):
    """
    This field is needed to respect Navitia's spec for the sake of compatibility
    """
    def output(self, key, obj):
        if not obj:
            return None

        lon, lat = get_lon_lat(obj)
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return {
            "insee": geocoding.get('citycode') or geocoding.get('city_code'),
            "level":
                int(geocoding.get('level')) if geocoding.get('level') else None,
            "name": geocoding.get('name'),
            "label": geocoding.get('label'),
            "id": geocoding.get('id'),
            "coord": {
                "lat": lat,
                "lon": lon
            },
            "zip_code": geocoding.get('postcode'),
            "administrative_regions":
                create_administrative_regions_field(geocoding) or create_admin_field(geocoding) ,
        }


class AddressField(fields.Raw):
    def output(self, key, obj):
        if not obj:
            return None

        lon, lat = get_lon_lat(obj)
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return {
            "id": '{};{}'.format(lon, lat),
            "coord": {
                "lon": lon,
                "lat": lat,
            },
            "house_number": get_house_number(geocoding.get('housenumber')),
            "label": geocoding.get('label'),
            "name": geocoding.get('name'),
            "administrative_regions":
                create_administrative_regions_field(geocoding) or create_admin_field(geocoding) ,
        }


class PoiField(fields.Raw):
    def output(self, key, obj):
        if not obj:
            return None

        lon, lat = get_lon_lat(obj)
        geocoding = obj.get('properties', {}).get('geocoding', {})
        poi_types = geocoding.get('poi_types', [])

        # TODO add address, properties attributes
        res = {
            "id": geocoding.get('id'),
            "coord": {
                "lon": lon,
                "lat": lat,
            },
            "label": geocoding.get('label'),
            "name": geocoding.get('name'),
            "administrative_regions":
                create_administrative_regions_field(geocoding) or create_admin_field(geocoding),
            "properties": {p.get('key'): p.get('value') for p in geocoding.get("properties", [])},
            "address": create_address_field(geocoding.get("address"))
        }
        if isinstance(poi_types, list) and poi_types:
            res['poi_type'] = poi_types[0]
        return res


class StopAreaField(fields.Raw):
    def output(self, key, obj):
        if not obj:
            return None

        lon, lat = get_lon_lat(obj)
        geocoding = obj.get('properties', {}).get('geocoding', {})

        # TODO add codes
        resp = {
            "id": geocoding.get('id'),
            "coord": {
                "lon": lon,
                "lat": lat,
            },
            "label": geocoding.get('label'),
            "name": geocoding.get('name'),
            "administrative_regions":
                create_administrative_regions_field(geocoding) or create_admin_field(geocoding),
            "timezone": geocoding.get('timezone')
        }

        c_modes = geocoding.get('commercial_modes', [])
        if c_modes:
            resp['commercial_modes'] = create_modes_field(c_modes)

        p_modes = geocoding.get('physical_modes', [])
        if p_modes:
            resp['physical_modes'] = create_modes_field(p_modes)
        return resp

geocode_admin = {
    "embedded_type": Lit("administrative_region"),
    "quality": Lit(0),
    "id": fields.String(attribute='properties.geocoding.id'),
    "name": fields.String(attribute='properties.geocoding.name'),
    "administrative_region": AdministrativeRegionField()
}


geocode_addr = {
    "embedded_type": Lit("address"),
    "quality": Lit(0),
    "id": CoordId,
    "name": fields.String(attribute='properties.geocoding.label'),
    "address": AddressField()
}

geocode_poi = {
    "embedded_type": Lit("poi"),
    "quality": Lit(0),
    "id": fields.String(attribute='properties.geocoding.id'),
    "name": fields.String(attribute='properties.geocoding.label'),
    "poi": PoiField()
}

geocode_stop_area = {
    "embedded_type": Lit("stop_area"),
    "quality": Lit(0),
    "id": fields.String(attribute='properties.geocoding.id'),
    "name": fields.String(attribute='properties.geocoding.label'),
    "stop_area": StopAreaField()
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
        elif type_ == 'public_transport:stop_area':
            return marshal(place, geocode_stop_area)

        logging.getLogger(__name__).error('Place not serialized (unknown type): {}'.format(place))
        return None

geocodejson = {
    "places": fields.List(GeocodejsonFeature, attribute='features'),
    "warnings": ListLit([fields.Nested(beta_endpoint)]),
    "feed_publishers": ListLit([fields.Nested(feed_publisher_bano),
                                fields.Nested(feed_publisher_osm)])
}


class GeocodeJson(AbstractAutocomplete):
    """
    Autocomplete with an external service returning geocodejson
    (https://github.com/geocoders/geocodejson-spec/)

    """
    # the geocodejson types
    TYPE_STOP_AREA = "public_transport:stop_area"
    TYPE_CITY = "city"
    TYPE_POI = "poi"
    TYPE_HOUSE = "house"
    TYPE_STREET = "street"

    def __init__(self, **kwargs):
        self.host = kwargs.get('host')
        self.timeout = kwargs.get('timeout', 10)

    @staticmethod
    def call_bragi(url, method, **kwargs):
        try:
            return method(url, **kwargs)
        except requests.Timeout:
            logging.getLogger(__name__).error('autocomplete request timeout')
            raise TechnicalError('external autocomplete service timeout')
        except:
            logging.getLogger(__name__).exception('error in autocomplete request')
            raise TechnicalError('impossible to access external autocomplete service')

    @classmethod
    def _check_response(cls, response, uri):
        if response is None:
            raise TechnicalError('impossible to access autocomplete service')
        if response.status_code == 404:
            raise UnknownObject(uri)
        if response.status_code != 200:
            raise TechnicalError('error in autocomplete request')

    @classmethod
    def response_marshaler(cls, response_bragi, uri=None):
        cls._check_response(response_bragi, uri)
        json_response = response_bragi.json()

        if jormungandr.USE_SERPY:
            from jormungandr.interfaces.v1.serializer.geocode_json import GeocodePlacesSerializer
            return GeocodePlacesSerializer(json_response).data
        else:
            from flask.ext.restful import marshal
            m = marshal(json_response, geocodejson)
            # Removing places that are not marshalled (None)
            if 'places' in m and isinstance(m['places'], list):
                m['places'] = [p for p in m['places'] if p is not None]
            return m

    def make_url(self, end_point, uri=None):

        if end_point not in ['autocomplete', 'features', 'reverse']:
            raise TechnicalError('Unknown endpoint')

        if not self.host:
            raise TechnicalError('global autocomplete not configured')

        url = "{host}/{end_point}".format(host=self.host, end_point=end_point)
        if uri:
            url = '{url}/{uri}'.format(url=url, uri=uri)
        return url

    def basic_params(self, instances):
        if not instances:
            return {}
        return {'pt_dataset': [i.name for i in instances]}

    def make_params(self, request, instances):
        params = self.basic_params(instances)
        params.update({
            "q": request["q"],
            "limit": request["count"]
        })
        if request.get("type[]"):
            types = []
            map_type = {
                "administrative_region": [self.TYPE_CITY],
                "address": [self.TYPE_STREET, self.TYPE_HOUSE],
                "stop_area": [self.TYPE_STOP_AREA],
                "poi": [self.TYPE_POI]
            }
            for type in request.get("type[]"):
                if type == 'stop_point':
                    logging.getLogger(__name__).debug('stop_point is not handled by bragi')
                    continue

                types.extend(map_type[type])

            params["type[]"] = types

        if request.get("from"):
            params["lon"], params["lat"] = self.get_coords(request["from"])
        return params

    def get(self, request, instances):
        params = self.make_params(request, instances)

        shape = request.get('shape', None)

        url = self.make_url('autocomplete')
        kwargs = {"params": params, "timeout": self.timeout}
        method = requests.get
        if shape:
            kwargs["json"] = {"shape": shape}
            method = requests.post

        raw_response = self.call_bragi(url, method, **kwargs)

        return self.response_marshaler(raw_response)

    def geo_status(self, instance):
        raise NotImplementedError

    @staticmethod
    def get_coords(param):
        """
        Get coordinates (longitude, latitude).
        For moment we consider that the param can only be a coordinate.
        """
        return param.split(";")

    def get_by_uri(self, uri, instances=None, current_datetime=None):

        params = self.basic_params(instances)
        lon, lat = get_lon_lat_from_id(uri)

        if lon is not None and lat is not None:
            url = self.make_url('reverse')
            params['lon'] = lon
            params['lat'] = lat
        else:
            url = self.make_url('features', uri)

        raw_response = self.call_bragi(url, requests.get, timeout=self.timeout, params=params)
        return self.response_marshaler(raw_response, uri)

    def status(self):
        return {'class': self.__class__.__name__, 'timeout': self.timeout}
