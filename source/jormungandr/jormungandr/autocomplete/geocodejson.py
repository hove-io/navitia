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
from functools import wraps


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

    # TODO: To be deleted when bragi is modified
    @delete_attribute_autocomplete()
    def get(self, request, instance, shape):
        if not self.external_api:
            raise TechnicalError('global autocomplete not configured')

        url = '{endpoint}?q={q}&limit={count}'.format(endpoint=self.external_api,
                                                      q=request['q'],
                                                      count=request['count'])
        try:
            if shape:
                raw_response = requests.post(url, timeout=self.timeout, json=shape)
            else:
                raw_response = requests.get(url, timeout=self.timeout)

        except requests.Timeout:
            logging.getLogger(__name__).error('autocomplete request timeout')
            raise TechnicalError('external autocomplete service timeout')
        except:
            logging.getLogger(__name__).exception('error in autocomplete request')
            raise TechnicalError('impossible to access external autocomplete service')

        return raw_response.json()

    def geo_status(self, instance):
        raise NotImplementedError


