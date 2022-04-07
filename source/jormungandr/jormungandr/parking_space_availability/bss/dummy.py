# encoding: utf-8
# Copyright (c) 2001-2020, Hove and/or its affiliates. All rights reserved.
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
from jormungandr.ptref import FeedPublisher
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
from jormungandr.parking_space_availability.bss.common_bss_provider import CommonBssProvider

DUMMY_FEED_PUBLISHER = {'id': 'Dummy', 'name': 'Dummy', 'license': 'Dummy', 'url': 'Dummy'}


class DummyProvider(CommonBssProvider):
    """
    Dummy BSS Provider that is used for testing purpose

    WARNING:
        This provider ACCEPTS ALL bss stations that have a non empty ref in properties. In order not to override other
        bss providers that are already existent, you should put this class at the end of bss provider list.

    """

    def __init__(self, network, operators):
        super(DummyProvider, self).__init__()
        self.url = 'Dummy Url'
        self.network = network.lower() if network else None
        self.service_id = "Dummy Service"
        self.organization_id = "Dummy Organization"
        self.operators = [o.lower() for o in operators or []]
        self._feed_publisher = FeedPublisher(**DUMMY_FEED_PUBLISHER)

    def service_caller(self, method, url, headers, data=None, params=None):
        pass

    def support_poi(self, poi):
        if all([self.network is None, poi.get('properties', {}).get('ref') is None, not self.operators]):
            return True

        network = poi.get('properties', {}).get('network', '').lower()
        operator = poi.get('properties', {}).get('operator', '').lower()
        return network == self.network and operator in self.operators

    def status(self):
        return {'network': self.network, 'operators': self.operators}

    def feed_publisher(self):
        return self._feed_publisher

    def _get_informations(self, poi):
        ref = poi.get('properties', {}).get('ref')
        if not ref:
            return
        # generate dummy available stands number using a hash algo
        import hashlib

        m = hashlib.md5()
        for k, v in poi.get('properties', {}).items():
            m.update(v.encode('utf-8', errors='ignore'))
        s = int(bytearray(str(m.hexdigest()))[0])
        return Stands(int(s / 2), s, StandsStatus.closed if int(ref) % 2 else StandsStatus.open)
