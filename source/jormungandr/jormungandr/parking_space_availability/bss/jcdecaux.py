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
from jormungandr.parking_space_availability.bss.bss_provider import BssProvider
from jormungandr.parking_space_availability.bss.stands import Stands
from jormungandr import cache, app
from urllib2 import URLError, HTTPError, urlopen
import logging
import json


class JcdecauxProvider(BssProvider):

    WS_URL_TEMPLATE = 'https://api.jcdecaux.com/vls/v1/stations/{}?contract={}&apiKey={}'
    OPERATOR = 'JCDecaux'

    def __init__(self, network, contract, api_key):
        self.network = network
        self.contract = contract
        self.api_key = api_key

    def support_poi(self, poi):
        properties = poi.get('properties', {})
        return properties.get('operator') == self.OPERATOR and \
               properties.get('network') == self.network

    def _call_webservice(self, station_id):
        try:
            response = urlopen(self.WS_URL_TEMPLATE.format(station_id, self.contract, self.api_key))
            return json.load(response)
        except (URLError, HTTPError, ValueError, KeyError, TypeError) as e:
            logging.getLogger(__name__).info(str(e))

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_JCDECAUX', 5))
    def get_informations(self, poi):
        ref = poi.get('properties', {}).get('ref')
        data = self._call_webservice(ref)
        if data and 'available_bike_stands' in data and 'available_bikes' in data:
            return Stands(data['available_bike_stands'], data['available_bikes'])
