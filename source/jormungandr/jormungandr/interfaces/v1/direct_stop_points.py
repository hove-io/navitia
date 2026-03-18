# Copyright (c) 2001-2026, Hove and/or its affiliates. All rights reserved.
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

from source.jormungandr.jormungandr.interfaces.v1 import ResourceUri
from source.jormungandr.jormungandr.interfaces.v1.serializer.direct_stop_points import DirectStopPointsSerializer


class DirectStopPoints(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, output_type_serializer=DirectStopPointsSerializer, *args, **kwargs)
        parser_get = self.parsers["get"]
        parser_get.add_argument(
            "stop_point_id",
            type=str,
            required=True,
            help="Id of the stop point from where we want to acess others stop points",
        )
        parser_get.add_argument(
            "line_id", type=str, required=True, help="Id of the line from where we want to access stop points"
        )

    def options(self, **kwargs):
        return self.api_description(**kwargs)

    def get(self, region=None, lon=None, lat=None, uri=None):
        pass
