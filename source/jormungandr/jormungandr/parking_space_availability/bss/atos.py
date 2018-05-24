# encoding: utf-8
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
from contextlib import contextmanager
import logging
import pybreaker
import zeep

from jormungandr import cache, app
from jormungandr.parking_space_availability import AbstractParkingPlacesProvider
from jormungandr.parking_space_availability.bss.stands import Stands
from jormungandr.ptref import FeedPublisher

DEFAULT_ATOS_FEED_PUBLISHER = None


class AtosProvider(AbstractParkingPlacesProvider):

    def __init__(self, id_ao, network, url, operators={'keolis'}, timeout=5,
                 feed_publisher=DEFAULT_ATOS_FEED_PUBLISHER, **kwargs):
        self.id_ao = id_ao
        self.network = network.lower()
        self.WS_URL = url
        self.operators = [o.lower() for o in operators]
        self.timeout = timeout
        self._client = None
        self.breaker = pybreaker.CircuitBreaker(fail_max=kwargs.get('fail_max', 5), reset_timeout=kwargs.get('reset_timeout', 120))
        self._feed_publisher = FeedPublisher(**feed_publisher) if feed_publisher else None

    def __repr__(self):
        return self.WS_URL + str(self.id_ao)

    def support_poi(self, poi):
        properties = poi.get('properties', {})
        return properties.get('operator', '').lower() in self.operators and \
               properties.get('network', '').lower() == self.network

    def get_informations(self, poi):
        logging.debug('building stands')
        try:
            all_stands = self.breaker.call(self._get_all_stands)
            ref = poi.get('properties', {}).get('ref')
            if ref:
                stands = all_stands.get(ref.lstrip('0'))
                return stands
        except:
            logging.getLogger(__name__).exception('transport error during call to %s bss provider', self.id_ao)
        return None

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_ATOS', 30))
    def _get_all_stands(self):
        with self._get_client() as client:
            all_stands = client.service.getSummaryInformationTerminals(self.id_ao)
            return {stands.libelle: Stands(stands.nbPlacesDispo, stands.nbVelosDispo) for stands in all_stands}

    @contextmanager
    def _get_client(self):
        try:
            transport = zeep.Transport(timeout=self.timeout, operation_timeout=self.timeout)
            client = zeep.Client(self.WS_URL, transport=transport)
            yield client
        finally:
            transport.session.close()

    def _get_client(self):
        if not self._client:
            transport = zeep.Transport(timeout=self.timeout, operation_timeout=self.timeout)
            self._client = zeep.Client(self.WS_URL, transport=transport)
        return self._client

    def status(self):
        return {'network': self.network, 'operators': self.operators, 'id_ao': self.id_ao}

    def feed_publisher(self):
        return self._feed_publisher
