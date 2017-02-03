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
    def __init__(self, **kwargs):
        self.external_api = kwargs.get('host')
        self.timeout = kwargs.get('timeout', 10)

    def make_params(self, request, instance):
        params = {
            "q": request["q"],
            "limit": request["count"]
        }

        if request.get("from"):
            params["lon"], params["lat"] = self.get_coords(request["from"])

        if instance:
            params["pt_dataset"] = instance.name

        return params

    def get(self, request, instance, shape=None):
        if not self.external_api:
            raise TechnicalError('global autocomplete not configured')

        params = self.make_params(request, instance)

        try:
            if shape:
                raw_response = requests.post(self.external_api, timeout=self.timeout, json=shape, params=params)
            else:
                raw_response = requests.get(self.external_api, timeout=self.timeout, params=params)

        except requests.Timeout:
            logging.getLogger(__name__).error('autocomplete request timeout')
            raise TechnicalError('external autocomplete service timeout')
        except:
            logging.getLogger(__name__).exception('error in autocomplete request')
            raise TechnicalError('impossible to access external autocomplete service')

        bragi_response = raw_response.json()
        from flask.ext.restful import marshal
        from jormungandr.interfaces.v1.Places import geocodejson

        return marshal(bragi_response, geocodejson)

    def geo_status(self, instance):
        raise NotImplementedError

    def get_coords(self, param):
        """
        Get coordinates (longitude, latitude).
        For moment we consider that the param can only be a coordinate.
        """
        return param.split(";")
