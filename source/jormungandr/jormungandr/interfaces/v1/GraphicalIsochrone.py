# coding=utf-8

#  Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from flask_restful import abort
from jormungandr import i_manager
from jormungandr.timezone import set_request_timezone
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.utils import date_to_timestamp
from jormungandr.interfaces.v1.journey_common import JourneyCommon
from jormungandr.interfaces.v1.serializer.api import GraphicalIsrochoneSerializer
from jormungandr.interfaces.v1.decorators import get_serializer
from navitiacommon.parser_args_type import UnsignedInteger
import six


class GraphicalIsochrone(JourneyCommon):
    def __init__(self):
        super(GraphicalIsochrone, self).__init__(output_type_serializer=GraphicalIsrochoneSerializer)
        parser_get = self.parsers["get"]
        parser_get.add_argument(
            "min_duration", type=UnsignedInteger(), default=0, help="Minimum travel duration"
        )
        parser_get.add_argument(
            "boundary_duration[]",
            type=UnsignedInteger(),
            action="append",
            help="To provide multiple duration parameters",
        )
        parser_get.add_argument(
            "_override_scenario",
            type=six.text_type,
            hidden=True,
            help="debug param to specify a custom scenario",
        )

    @get_serializer(serpy=GraphicalIsrochoneSerializer)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):

        args = self.parsers['get'].parse_args()
        self.region = i_manager.get_region(region, lon, lat)
        args.update(self.parse_args(region, uri))

        # We set default modes for fallback modes.
        # The reason why we cannot put default values in parser_get.add_argument() is that, if we do so,
        # fallback modes will always have a value, and traveler_type will never override fallback modes.
        args['origin_mode'] = args.get('origin_mode') or ['walking']
        args['destination_mode'] = args['destination_mode'] or ['walking']

        if not (args['destination'] or args['origin']):
            abort(400, message="you should provide a 'from' or a 'to' argument")
        if not args['max_duration'] and not args["boundary_duration[]"]:
            abort(400, message="you should provide a 'boundary_duration[]' or a 'max_duration' argument")
        if args["boundary_duration[]"] and len(args["boundary_duration[]"]) > 10:
            abort(400, message="you cannot provide more than 10 'boundary_duration[]'")
        if args['destination'] and args['origin']:
            abort(400, message="you cannot provide a 'from' and a 'to' argument")
        if 'ridesharing' in args['origin_mode'] or 'ridesharing' in args['destination_mode']:
            abort(400, message='ridesharing isn\'t available on isochrone')

        set_request_timezone(self.region)
        original_datetime = args['original_datetime']
        if original_datetime:
            new_datetime = self.convert_to_utc(original_datetime)
        args['datetime'] = date_to_timestamp(new_datetime)

        response = i_manager.dispatch(args, "graphical_isochrones", self.region)

        return response

    def options(self, **kwargs):
        return self.api_description(**kwargs)
