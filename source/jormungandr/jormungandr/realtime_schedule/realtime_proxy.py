# coding=utf-8

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
from abc import abstractmethod, ABCMeta
from jormungandr.utils import timestamp_to_datetime, record_external_failure
from jormungandr import new_relic
import six

class RealtimeProxyError(RuntimeError):
    pass


class RealtimeProxy(six.with_metaclass(ABCMeta, object)):
    """
    abstract class managing calls to external service providing real-time next passages
    """

    @abstractmethod
    def _get_next_passage_for_route_point(self, route_point, count, from_dt, current_dt):
        """
        method that actually calls the external service to get the next passage for a given route_point
        """
        pass

    def _filter_passages(self, passages, count, from_dt):
        """
        after getting the next passages from the proxy, we might want to filter some

        by default we filter:
        * we keep at most 'count' items
        * we don't want to display datetime after from_dt
        """
        if not passages:
            return passages

        if from_dt:
            # we need to convert from_dt (which is a timestamp) to a datetime
            from_dt = timestamp_to_datetime(from_dt)
            passages = [p for p in passages if p.datetime >= from_dt]
            if not passages:
                # if there was some passages and everything was filtered,
                # we return None to keep the base schedule
                return None

        if count:
            del passages[count:]

        return passages

    def next_passage_for_route_point(self, route_point, count=None, from_dt=None, current_dt=None):
        """
        Main method for the proxy

        returns the next realtime passages
        """

        try:
            next_passages = self._get_next_passage_for_route_point(route_point, count, from_dt, current_dt)

            filtered_passage = self._filter_passages(next_passages, count, from_dt)

            self.record_call('ok')

            return filtered_passage
        except RealtimeProxyError as e:
            self.record_call('failure', reason=str(e))
            return None

    @abstractmethod
    def status(self):
        """
        return a status for the API
        """
        pass

    def record_external_failure(self, message):
        record_external_failure(message, 'realtime', self.rt_system_id)

    def record_internal_failure(self, message):
        params = {'realtime_system_id': repr(self.rt_system_id), 'message': message}
        new_relic.record_custom_event('realtime_internal_failure', params)

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'realtime_system_id': repr(self.rt_system_id), 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('realtime_status', params)
