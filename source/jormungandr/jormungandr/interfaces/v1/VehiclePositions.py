# coding=utf-8

#  Copyright (c) 2001-2019, Hove and/or its affiliates. All rights reserved.
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

from navitiacommon.parser_args_type import DepthArgument, DateTimeFormat
from jormungandr import i_manager, timezone
from jormungandr.interfaces.parsers import default_count_arg_type
from jormungandr.interfaces.v1.decorators import get_obj_serializer
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.v1.serializer import api
from jormungandr.resources_utils import ResourceUtc
from jormungandr.interfaces.common import split_uri

import six
from datetime import datetime


class VehiclePositions(ResourceUri, ResourceUtc):
    def __init__(self):
        ResourceUri.__init__(self, output_type_serializer=api.VehiclePositionsSerializer)
        ResourceUtc.__init__(self)
        parser_get = self.parsers["get"]
        parser_get.add_argument("depth", type=DepthArgument(), default=1, help="The depth of your object")
        parser_get.add_argument(
            "count", type=default_count_arg_type, default=25, help="Number of objects per page"
        )
        parser_get.add_argument("filter", type=six.text_type, default="", help="Filter your objects")
        parser_get.add_argument("start_page", type=int, default=0, help="The current page")
        parser_get.add_argument(
            "forbidden_uris[]",
            type=six.text_type,
            help="forbidden uris",
            dest="forbidden_uris[]",
            default=[],
            action="append",
            schema_metadata={'format': 'pt-object'},
        )
        parser_get.add_argument(
            "_current_datetime",
            type=DateTimeFormat(),
            schema_metadata={'default': 'now'},
            hidden=True,
            default=datetime.utcnow(),
            help='The datetime considered as "now". Used for debug, default is '
            'the moment of the request. It will mainly change the output '
            'of the disruptions.',
        )

        self.collection = 'vehicle_positions'
        self.get_decorators.insert(0, ManageError())
        self.get_decorators.insert(1, get_obj_serializer(self))

    def options(self, **kwargs):
        return self.api_description(**kwargs)

    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()
        instance = i_manager.instances.get(self.region)

        args["filter"] = self.get_filter(split_uri(uri), args)
        response = i_manager.dispatch(args, "vehicle_positions", instance_name=self.region)
        instance.external_service_provider_manager.manage_vehicle_positions(
            instance, response.vehicle_positions, **args
        )
        return response
        #
