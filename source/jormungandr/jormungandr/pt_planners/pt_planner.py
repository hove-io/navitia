# Copyright (c) 2001-2022, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import, print_function, unicode_literals, division

# Using abc.ABCMeta in a way it is compatible both with Python 2.7 and Python 3.x
# http://stackoverflow.com/a/38668373/1614576
from abc import abstractmethod, ABCMeta
import six


class AbstractPtPlanner(six.with_metaclass(ABCMeta, object)):  # type: ignore
    @abstractmethod
    def journeys(self, origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt, request_id):
        pass

    @abstractmethod
    def graphical_isochrones(
        self, origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
    ):
        pass

    @abstractmethod
    def stop_points_nearby(self, place, distance, access_points=False):
        """
        place: the center of the circle
        access_points: should I return the access points attached to the stop_points?

        Retrieve stop_points around the give place with distance.
        """
        pass

    # to be continued...
