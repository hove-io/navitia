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

from flask import current_app
from jormungandr.interfaces.v1.serializer import serialize_with
from jormungandr.interfaces.v1.serializer import api
from flask.ext.restful import marshal_with
from collections import OrderedDict

map_serializer = {
   "networks": api.NetworksSerializer,
   "lines": api.LinesSerializer,
   "commercial_modes": api.CommercialModesSerializer,
   "physical_modes": api.PhysicalModesSerializer,
   "stop_points": api.StopPointsSerializer,
   "stop_areas": api.StopAreasSerializer,
   "routes": api.RoutesSerializer,
   "line_groups": api.LineGroupsSerializer,
   "lines": api.LinesSerializer,
   "disruptions": api.DisruptionsSerializer,
}


def get_serializer(collection, collections, display_null=False):
    if current_app.config.get('USE_SERPY', False):
        return serialize_with(map_serializer.get(collection))
    else:
        return marshal_with(OrderedDict(collections), display_null=display_null)
