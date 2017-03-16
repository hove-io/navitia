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
import requests
from jormungandr.exceptions import TechnicalError


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

    def call_bragi(self, url, method, **kwargs):
        try:
            return method(url, **kwargs)
        except requests.Timeout:
            logging.getLogger(__name__).error('autocomplete request timeout')
            raise TechnicalError('external autocomplete service timeout')
        except:
            logging.getLogger(__name__).exception('error in autocomplete request')
            raise TechnicalError('impossible to access external autocomplete service')

    @staticmethod
    def response_marshaler(response_bragi):
        json_response = response_bragi.json()
        from flask.ext.restful import marshal
        from jormungandr.interfaces.v1.Places import geocodejson

        return marshal(json_response, geocodejson)

    def make_url(self, end_point, uri=None):

        if end_point not in ['autocomplete', 'features']:
            raise TechnicalError('Unknown endpoint')

        if not self.host:
            raise TechnicalError('global autocomplete not configured')

        url = "{host}/{end_point}".format(host=self.host, end_point=end_point)
        if uri:
            url = '{url}/{uri}'.format(url=url, uri=uri)
        return url

    def basic_params(self, instance):
        params = {}
        if instance:
            params["pt_dataset"] = instance.name
        return params

    def make_params(self, request, instance):
        params = self.basic_params(instance)
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

    def get(self, request, instance):

        params = self.make_params(request, instance)

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

    def get_coords(self, param):
        """
        Get coordinates (longitude, latitude).
        For moment we consider that the param can only be a coordinate.
        """
        return param.split(";")

    def get_uri(self, uri, request, instance):

        params = self.basic_params(instance)

        url = self.make_url('features', uri)
        raw_response = self.call_bragi(url, requests.get, timeout=self.timeout, params=params)

        return self.response_marshaler(raw_response)
