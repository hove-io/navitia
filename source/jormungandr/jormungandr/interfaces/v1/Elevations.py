# Copyright (c) 2001-2025, Hove and/or its affiliates. All rights reserved.
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
from jormungandr import i_manager
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.serializer import api


class Elevations(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, links=False, *args, **kwargs)
        self.parsers['get'].add_argument(
            "polyline",
            type=str,
            required=True,
            help="Encoded polyline, with 6 digits precision",
        )

    @get_serializer(serpy=api.ElevationsDictSerializer)
    def get(self, region=None):

        args = self.parsers["get"].parse_args()
        response = i_manager.dispatch(args, "elevations", instance_name=region)

        return {"elevations": response, "polyline": args["polyline"]}, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)
