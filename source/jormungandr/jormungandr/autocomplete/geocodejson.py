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
import logging

from jormungandr.autocomplete.abstract_autocomplete import (
    AbstractAutocomplete,
    AutocompleteUnavailable,
    AutocompleteError,
)
from jormungandr.utils import get_lon_lat as get_lon_lat_from_id, get_house_number
import requests
import pybreaker
from jormungandr import app
from jormungandr.exceptions import UnknownObject
from navitiacommon.constants import DEFAULT_SHAPE_SCOPE


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
        response.append(
            {
                "insee": None,
                "name": name,
                "level": int(level.replace('level', '')),
                "coord": {"lat": None, "lon": None},
                "label": None,
                "id": None,
                "zip_code": None,
            }
        )
    return response


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
        response.append(
            {
                "insee": admin.get('insee'),
                "name": admin.get('name'),
                "level": int(admin.get('level')) if admin.get('level') else None,
                "coord": {"lat": lat, "lon": lon},
                "label": admin.get('label'),
                "id": admin.get('id'),
                "zip_code": format_zip_code(zip_codes),
            }
        )
    return response


def create_modes_field(modes):
    if not modes:
        return []
    return [{"id": mode.get('id'), "name": mode.get('name')} for mode in modes]


def create_comments_field(modes):
    if not modes:
        return []
    # To be compatible, type = 'standard'
    return [{"type": 'standard', "value": mode.get('name')} for mode in modes]


def create_codes_field(codes):
    if not codes:
        return []
    # The code type value 'navitia1' replaced by 'external_code'
    for code in codes:
        if code.get('name') == 'navitia1':
            code['name'] = 'external_code'
    return [{"type": code.get('name'), "value": code.get('value')} for code in codes]


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


def create_address_field(geocoding, poi_lat=None, poi_lon=None):
    if not geocoding:
        return None
    coord = geocoding.get('coord', {})
    lat = str(coord.get('lat')) if coord and coord.get('lat') else poi_lat
    lon = str(coord.get('lon')) if coord and coord.get('lon') else poi_lon
    address_id = '{lon};{lat}'.format(lon=lon, lat=lat)
    resp = {
        "id": address_id,
        "label": geocoding.get('label'),
        "name": geocoding.get('name'),
        "coord": {"lat": lat, "lon": lon},
        "house_number": get_house_number(geocoding.get('housenumber')),
    }

    admins = create_administrative_regions_field(geocoding) or create_admin_field(geocoding)
    if admins:
        resp['administrative_regions'] = admins
    return resp


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

    TYPE_LIST = [TYPE_STOP_AREA, TYPE_CITY, TYPE_POI, TYPE_HOUSE, TYPE_STREET]

    def __init__(self, **kwargs):
        self.host = kwargs.get('host')
        self.timeout = kwargs.get('timeout', 2)  # used for slow call, like geocoding
        # used for fast call like reverse geocoding and features
        self.fast_timeout = kwargs.get('fast_timeout', 0.2)
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_BRAGI_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_BRAGI_TIMEOUT_S'],
        )
        self.session = requests
        self.timeout_bragi_es = kwargs.get('timeout_bragi_es', self.timeout)
        self.fast_timeout_bragi_es = kwargs.get('fast_timeout_bragi_es', self.fast_timeout)
        self.fast_reverse_timeout_bragi_es = kwargs.get('fast_reverse_timeout_bragi_es', self.fast_timeout)
        self.fast_within_timeout_bragi_es = kwargs.get('fast_within_timeout_bragi_es', self.fast_timeout)

    def call_bragi(self, url, method, **kwargs):
        try:
            return self.breaker.call(method, url, **kwargs)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('external autocomplete service dead (error: {})'.format(e))
            raise GeocodeJsonUnavailable('circuit breaker open')
        except requests.Timeout:
            logging.getLogger(__name__).error('autocomplete request timeout')
            raise GeocodeJsonUnavailable('external autocomplete service timeout')
        except:
            logging.getLogger(__name__).exception('error in autocomplete request')
            raise GeocodeJsonUnavailable('impossible to access external autocomplete service')

    @classmethod
    def _check_response(cls, response, uri):
        if response is None:
            raise GeocodeJsonError('impossible to access autocomplete service')
        if response.status_code in (400, 404):
            logging.getLogger(__name__).error(
                'Autocomplete request failed with HTTP code {} fallback kraken'.format(response.status_code)
            )
            raise UnknownObject(uri)
        if response.status_code == 503:
            raise GeocodeJsonUnavailable('geocodejson responded with 503')
        if response.status_code != 200:
            error_msg = 'Autocomplete request failed with HTTP code {}'.format(response.status_code)
            if response.text:
                error_msg += ' ({})'.format(response.text)
            raise GeocodeJsonError(error_msg)

    @classmethod
    def _clean_response(cls, response, depth=1):
        def is_deleteable(_key, _value, _depth):
            if _depth > -1:
                return False
            else:
                if _key == 'administrative_regions':
                    return True
                elif isinstance(_value, dict) and _value.get('type') in cls.TYPE_LIST:
                    return True
                else:
                    return False

        def _clear_object(obj):
            if isinstance(obj, list):
                del obj[:]
            elif isinstance(obj, dict):
                obj.clear()

        def _manage_depth(_key, _value, _depth):

            if is_deleteable(_key, _value, _depth):
                _clear_object(_value)
            elif isinstance(_value, dict):
                for k, v in _value.items():
                    _manage_depth(k, v, _depth - 1)

        key = 'geocoding'
        for item in ["features", "zones"]:
            for feature in response.get(item, []):
                value = feature.get('properties', {}).get('geocoding')
                if not value:
                    continue
                _manage_depth(key, value, depth)
        return response

    @classmethod
    def response_marshaller(cls, response_bragi, uri=None, depth=1):
        cls._check_response(response_bragi, uri)
        try:
            json_response = response_bragi.json()
        except ValueError:
            logging.getLogger(__name__).error(
                "impossible to get json for response %s with body: %s",
                response_bragi.status_code,
                response_bragi.text,
            )
            raise
        # Clean dict objects depending on depth passed in request parameter.
        json_response = cls._clean_response(json_response, depth)
        from jormungandr.interfaces.v1.serializer.geocode_json import GeocodePlacesSerializer

        return GeocodePlacesSerializer(json_response).data

    def make_url(self, end_point, uri=None):

        if end_point not in {'autocomplete', 'features', 'reverse', 'multi-reverse'}:
            raise GeocodeJsonError('Unknown endpoint')

        if not self.host:
            raise GeocodeJsonError('global autocomplete not configured')

        url = "{host}/{end_point}".format(host=self.host, end_point=end_point)
        if uri:
            url = '{url}/{uri}'.format(url=url, uri=uri)
        return url

    def basic_params(self, instances):
        '''
        These are the parameters common to the three endpoints
        '''
        if not instances:
            return []
        params = [('pt_dataset[]', i.name) for i in instances]
        params.extend([('poi_dataset[]', i.poi_dataset) for i in instances if i.poi_dataset])
        return params

    def make_params(self, request, instances):
        '''
        These are the parameters used specifically for the autocomplete endpoint.
        '''
        params = self.basic_params(instances)

        params.extend([("q", request["q"]), ("limit", request["count"])])
        if request.get("type[]"):
            map_type = {
                "administrative_region": [self.TYPE_CITY],
                "address": [self.TYPE_STREET, self.TYPE_HOUSE],
                "stop_area": [self.TYPE_STOP_AREA],
                "poi": [self.TYPE_POI],
            }
            for type in request.get("type[]"):
                if type == 'stop_point':
                    logging.getLogger(__name__).debug('stop_point is not handled by bragi')
                    continue

                for t in map_type[type]:
                    params.append(("type[]", t))
        shape_scope = request.get("shape_scope[]")
        shape_scope = shape_scope if shape_scope else DEFAULT_SHAPE_SCOPE
        for ss in shape_scope:
            params.append(("shape_scope[]", ss))

        # if places_proximity_radius is not provided bragi will used default values
        if 'places_proximity_radius' in request and request['places_proximity_radius'] is not None:
            params.extend(
                [
                    ('proximity_scale', 6.5 * request['places_proximity_radius'] / 1000),
                    ('proximity_offset', request['places_proximity_radius'] / 1000),
                    ('proximity_decay', 0.4),
                ]
            )

        if request.get("from"):
            lon, lat = self.get_coords(request["from"])
            params.extend([('lon', lon), ('lat', lat)])
        if self.timeout_bragi_es:
            # bragi timeout is in ms
            params.append(("timeout", int(self.timeout_bragi_es * 1000)))
        params.append(("request_id", request["request_id"]))
        return params

    def get(self, request, instances):
        params = self.make_params(request, instances)

        shape = request.get('shape')

        url = self.make_url('autocomplete')
        kwargs = {"params": params, "timeout": self.timeout}
        method = self.session.get
        if shape:
            kwargs["json"] = {"shape": shape}
            method = self.session.post
        logging.getLogger(__name__).debug("call bragi with parameters %s", kwargs)

        raw_response = self.call_bragi(url, method, **kwargs)
        depth = request.get('depth', 1)

        return self.response_marshaller(raw_response, None, depth)

    def geo_status(self, instance):
        raise NotImplementedError

    @staticmethod
    def get_coords(param):
        """
        Get coordinates (longitude, latitude).
        For moment we consider that the param can only be a coordinate.
        """
        return param.split(";")

    def extend_params(self, params, api):
        if api == 'multi-reverse':
            params.append(("reverse_timeout", int(self.fast_reverse_timeout_bragi_es * 1000)))
            params.append(("within_timeout", int(self.fast_within_timeout_bragi_es * 1000)))
        else:
            params.append(("timeout", int(self.fast_timeout_bragi_es * 1000)))

    def get_url_params(self, uri, instances=None):
        lon, lat = get_lon_lat_from_id(uri)
        params = self.basic_params(instances)
        if lon is not None and lat is not None:
            params.extend([('lon', lon), ('lat', lat)])
            api_name = 'multi-reverse' if instances and instances[0].use_multi_reverse else 'reverse'
        else:
            api_name = 'features'
        self.extend_params(params, api_name)
        return (
            (self.make_url(api_name, uri), params)
            if api_name == 'features'
            else (self.make_url(api_name), params)
        )

    def get_by_uri(self, uri, request_id, instances=None, current_datetime=None, _add_poi_shape=False):
        url, params = self.get_url_params(
            uri,
            instances,
        )
        params.append(("request_id", request_id))
        if _add_poi_shape:
            params.append(("_add_poi_shape", "true"))
        raw_response = self.call_bragi(url, self.session.get, timeout=self.fast_timeout, params=params)
        return self.response_marshaller(raw_response, uri)

    def status(self):
        return {
            'class': self.__class__.__name__,
            'timeout': self.timeout,
            'fast_timeout': self.fast_timeout,
            "fast_timeout_bragi_es": self.fast_timeout_bragi_es,
            "timeout_bragi_es": self.timeout_bragi_es,
        }

    def is_handling_stop_points(self):
        return False


class GeocodeJsonError(AutocompleteError):
    pass


class GeocodeJsonUnavailable(AutocompleteUnavailable):
    pass
