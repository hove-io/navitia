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


class RealtimeProxy(object):
    """
    abstract class managing calls to external service providing real-time next passages
    """
    __metaclass__ = ABCMeta

    @abstractmethod
    def _get_next_passage_for_route_point(self, route_point, items_per_schedule, from_dt):
        """
        method that actually calls the external service to get the next passage for a given route_point
        """
        pass

    def _filter_passages(self, passages, items_per_schedule, from_dt):
        """
        after getting the next passages from the proxy, we might want to filter some

        by default we filter:
        * we keep at most 'items_per_schedule' items
        * we don't want to display datetime after from_dt
        """
        if from_dt:
            passages = filter(lambda p: p.datetime < from_dt, passages)

        if items_per_schedule:
            del passages[items_per_schedule:]

        return passages

    def next_passage_for_route_point(self, route_point, items_per_schedule=None, from_dt=None):
        """
        Main method for the proxy

        returns the next realtime passages
        """
        next_passages = self._get_next_passage_for_route_point(route_point, items_per_schedule, from_dt)

        filtered_passage = self._filter_passages(next_passages, items_per_schedule, from_dt)

        return filtered_passage

    @abstractmethod
    def status(self):
        """
        return a status for the API
        """
        pass
