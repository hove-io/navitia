# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from flask import Flask, request
from flask.ext.restful import Resource, fields
from jormungandr import i_manager
from jormungandr.interfaces.parsers import true_false, option_value
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.find_extrem_datetimes import extremes
from flask.ext.restful import reqparse
from flask.ext.restful.types import boolean
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument
from jormungandr.authentification import authentification_required
from jormungandr.utils import date_to_timestamp, ResourceUtc
from datetime import datetime, timedelta

f_datetime = "%Y%m%dT%H%M%S"


class FindAndFormatJourneys():

    def find_and_transform_datetimes(self, response):
        if not 'journeys' in response or len(response['journeys']) == 0:
            return response
        for i_journey in range(0, len(response['journeys'])-1):
            journey = response['journeys'][i_journey]
            if "arrival_date_time" in journey:
                journey['arrival_date_time'] = self.format(journey['arrival_date_time'])
            if "departure_date_time" in journey:
                journey['departure_date_time'] = self.format(journey['departure_date_time'])
            if "requested_date_time" in journey:
                journey['requested_date_time'] = self.format(journey['requested_date_time'])
            if not "sections" in journey:
                continue
            for i_section in range(0, len(journey['sections'])-1):
                section = journey['sections'][i_section]
                if "end_date_time" in section:
                    section['end_date_time'] = self.format(section['end_date_time'])
                if "begin_date_time" in section:
                    section['begin_date_time'] = self.format(section['begin_date_time'])
                    if not "stop_date_times" in section:
                        continue
                    for i_st in range(0, len(section['stop_date_times'])-1):
                        st = section['stop_date_times'][i_st]
                        if "arrival_date_time" in journey:
                            st['arrival_date_time'] = self.format(st['arrival_date_time'])
                        if "departure_date_time" in st:
                            st['departure_date_time'] = self.format(st['departure_date_time'])
        return response


class Journeys(Resource, ResourceUtc, FindAndFormatJourneys):

    """ Compute journeys"""
    parsers = {}
    method_decorators = [authentification_required]

    def __init__(self, *args, **kwargs):
        ResourceUtc.__init__(self)
        types = ["all", "rapid"]
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("origin", type=str, required=True,
                                description="Origin's uri of your journey")
        parser_get.add_argument("destination", type=str, required=True,
                                description=
                                "Destination's uri of your journey")
        parser_get.add_argument("datetime", type=str, required=True,
                                description=
                                "Datetime's uri of your journey")
        parser_get.add_argument("clockwise", type=true_false, default=True,
                                description=
                                "Whether you want your journey to start"\
                                "after datetime or to end before datetime")
        parser_get.add_argument("max_duration", type=int, default=36000,
                                description="Maximum duration of your journey")
        parser_get.add_argument("max_transfers", type=int, default=10,
                                description=
                                "Maximum transfers you want in "\
                                "your journey")
        parser_get.add_argument("origin_mode",
                                type=option_value(["walking", "car", "bike",
                                                   "bss"]),
                                action="append", default=['walking'],
                                description=
                                "The list of modes you want at the "\
                                "beggining of your journey")
        parser_get.add_argument("destination_mode",
                                type=option_value(["walking", "car", "bike",
                                                   "bss"]),
                                default=['walking'],
                                action="append",
                                description=
                                "The list of modes you want at the"\
                                " end of your journey")
        parser_get.add_argument("max_duration_to_pt", type=int, default=10*60,
                                description=
                                "maximal duration of non public "\
                                "transport in second")
        parser_get.add_argument("walking_speed", type=float, default=1.68,
                                description=
                                "Walking speed in meter/second")
        parser_get.add_argument("bike_speed", type=float, default=8.8,
                                description="Bike speed in meter/second")
        parser_get.add_argument("bss_speed", type=float, default=8.8,
                                description="Bike rental speed in "\
                                "meter/second")
        parser_get.add_argument("car_speed", type=float, default=16.8,
                                description=
                                "Car speed in meter/second")
        parser_get.add_argument("forbidden_uris[]", type=str, action="append",
                                description="Uri you want to forbid")
        parser_get.add_argument("type", type=option_value(types),
                                default="all")
        parser_get.add_argument("wheelchair", type=boolean, default=False)
        parser_get.add_argument("disruption_active", type=boolean, default=False)
        parser_get.add_argument("count", type=int)
        parser_get.add_argument("debug", type=boolean, default=False,
                                hidden=True)
        self.region = None
    
    def get(self, region=None):
        args = self.parsers["get"].parse_args()

        #default value for compatibility with v1
        args["min_nb_journeys"] = None
        args["max_nb_journeys"] = None
        args["show_codes"] = False
        if region is None:
            region = i_manager.key_of_id(args["origin"])
        self.region = region
        original_datetime = datetime.strptime(args['datetime'], f_datetime)
        new_datetime = self.convert_to_utc(original_datetime)
        args['original_datetime'] = date_to_timestamp(original_datetime)  # we save the original datetime for debuging purpose
        args['datetime'] = date_to_timestamp(new_datetime)
        
        response = i_manager.dispatch(args, "journeys", instance_name=region)
        if response.journeys:
            (before, after) = extremes(response, request)
            if before and after:
                response.prev = self.format(before)
                response.next = self.format(after)

        return self.find_and_transform_datetimes(protobuf_to_dict(response, use_enum_labels=True)), 200



class Isochrone(Resource, ResourceUtc, FindAndFormatJourneys):

    """ Compute isochrones """
    method_decorators = [authentification_required]

    def __init__(self):
        ResourceUtc.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("origin", type=str, required=True)
        parser_get.add_argument("datetime", type=str, required=True)
        parser_get.add_argument("clockwise", type=true_false, default=True)
        parser_get.add_argument("max_duration", type=int, default=3600)
        parser_get.add_argument("max_transfers", type=int, default=10)
        parser_get.add_argument("origin_mode",
                                type=option_value(["walking", "car", "bike",
                                                   "bss"]),
                                action="append", default=["walking"])
        parser_get.add_argument("max_duration_to_pt", type=int, default=10*60,
                                description="maximal duration of non public \
                                transport in second")
        parser_get.add_argument("walking_speed", type=float, default=1.68)
        parser_get.add_argument("bike_speed", type=float, default=8.8)
        parser_get.add_argument("bss_speed", type=float, default=8.8)
        parser_get.add_argument("car_speed", type=float, default=16.8)
        parser_get.add_argument("forbidden_uris[]", type=str, action="append")
        parser_get.add_argument("wheelchair", type=boolean, default=False)
        parser_get.add_argument("disruption_active", type=boolean, default=False)
        parser_get.add_argument("debug", type=boolean, default=False,
                                hidden=True)

    def get(self, region=None):
        args = self.parsers["get"].parse_args()
        if region is None:
            region = i_manager.key_of_id(args["origin"])
        self.region = region
        original_datetime = datetime.strptime(args['datetime'], f_datetime)
        new_datetime = self.convert_to_utc(original_datetime)
        args['original_datetime'] = date_to_timestamp(original_datetime)  # we save the original datetime for debuging purpose
        args['datetime'] = date_to_timestamp(new_datetime)
        #default value for compatibility with v1
        args["min_nb_journeys"] = None
        args["max_nb_journeys"] = None
        args["show_codes"] = False
        response = i_manager.dispatch(args, "isochrone", instance_name=self.region)
        if response.journeys:
            (before, after) = extremes(response, request)
            if before and after:
                response.prev = self.format(before)
                response.next = self.format(after)
        return self.find_and_transform_datetimes(protobuf_to_dict(response, use_enum_labels=True)), 200
