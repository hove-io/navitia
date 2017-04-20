
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
from jormungandr.interfaces.v1.serializer import pt
from jormungandr.interfaces.v1.serializer.fields import ErrorSerializer, FeedPublisherSerializer, PaginationSerializer
import serpy


class PTReferentialSerializer(serpy.Serializer):
    pagination = PaginationSerializer(attr='pagination', display_none=True, required=True)
    error = ErrorSerializer(display_none=False)
    feed_publishers = FeedPublisherSerializer(many=True, display_none=False)
    disruptions = pt.DisruptionSerializer(attr='impacts', many=True)


class LinesSerializer(PTReferentialSerializer):
    lines = pt.LineSerializer(many=True)


class DisruptionsSerializer(PTReferentialSerializer):
    #we already have a disruptions fields by default
    pass


class CommercialModesSerializer(PTReferentialSerializer):
    commercial_modes = pt.CommercialModeSerializer(many=True)


class PhysicalModesSerializer(PTReferentialSerializer):
    physical_modes = pt.PhysicalModeSerializer(many=True)


class StopPointsSerializer(PTReferentialSerializer):
    stop_points = pt.StopPointSerializer(many=True)


class StopAreasSerializer(PTReferentialSerializer):
    stop_areas = pt.StopAreaSerializer(many=True)


class RoutesSerializer(PTReferentialSerializer):
    routes = pt.RouteSerializer(many=True)


class LineGroupsSerializer(PTReferentialSerializer):
    line_groups = pt.LineGroupSerializer(many=True)


class NetworksSerializer(PTReferentialSerializer):
    networks = pt.NetworkSerializer(many=True)


class PlacesSerializer(PTReferentialSerializer):
    places = pt.PlaceSerializer(many=True)
