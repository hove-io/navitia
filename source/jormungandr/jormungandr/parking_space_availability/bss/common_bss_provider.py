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
from jormungandr import new_relic
from jormungandr.parking_space_availability import AbstractParkingPlacesProvider
from abc import abstractmethod
from jormungandr.parking_space_availability.bss.stands import Stands, StandsStatus
import six


class BssProxyError(RuntimeError):
    pass


class CommonBssProvider(AbstractParkingPlacesProvider):
    @abstractmethod
    def _get_informations(self, poi):
        pass

    def get_informations(self, poi):
        try:
            stand = self._get_informations(poi)
            self.record_call('ok')
            return stand

        except BssProxyError as e:
            self.record_call('failure', reason=str(e))
            return Stands(0, 0, StandsStatus.unavailable)

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'bss_system_id': six.text_type(self.network), 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('bss_status', params)
