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
from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr import i_manager
from jormungandr.interfaces.parsers import date_time_format
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from datetime import datetime
from jormungandr.resources_utc import ResourceUtc
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.interfaces.parsers import option_value
from jormungandr.interfaces.parsers import float_gt_0
from jormungandr.interfaces.parsers import unsigned_integer
from flask.ext.restful.inputs import boolean
from flask.ext.restful import reqparse, abort
import logging
from jormungandr.exceptions import RegionNotFound
from functools import cmp_to_key
from jormungandr.instance_manager import instances_comparator
from jormungandr.travelers_profile import TravelerProfile
from navitiacommon.default_traveler_profile_params import acceptable_traveler_types


def dt_represents(value):
    if value == "arrival":
        return False
    elif value == "departure":
        return True
    else:
        raise ValueError("Unable to parse datetime_represents")


def compute_regions(args):
    """
    method computing the region the journey has to be computed on
    The complexity comes from the fact that the regions in jormungandr can overlap.

    return the kraken instance keys

    rules are easy:
    we fetch the different regions the user can use for 'origin' and 'destination'
    we do the intersection and sort the list
    """
    from_regions = set()
    to_regions = set()
    if args['origin']:
        from_regions = set(i_manager.get_instances(object_id=args['origin']))
        #Note: if get_regions does not find any region, it raises a RegionNotFoundException

    if args['destination']:
        to_regions = set(i_manager.get_instances(object_id=args['destination']))

    if not from_regions:
        #we didn't get any origin, the region is in the destination's list
        possible_regions = to_regions
    elif not to_regions:
        #we didn't get any origin, the region is in the destination's list
        possible_regions = from_regions
    else:
        #we need the intersection set
        possible_regions = from_regions.intersection(to_regions)
        logging.getLogger(__name__).debug("orig region = {o}, dest region = {d} => set = {p}".
                 format(o=from_regions, d=to_regions, p=possible_regions))

    if not possible_regions:
        raise RegionNotFound(custom_msg="cannot find a region with {o} and {d} in the same time"
                             .format(o=args['origin'], d=args['destination']))

    sorted_regions = list(possible_regions)

    regions = sorted(sorted_regions, key=cmp_to_key(instances_comparator))

    return [r.name for r in regions]


def compute_possible_region(region, args):

    if region:
        region_obj = i_manager.get_region(region)

    if not region:
        # TODO how to handle lon/lat ? don't we have to override args['origin'] ?
        possible_regions = compute_regions(args)
    else:
        possible_regions = [region_obj]

    return possible_regions


class JourneyCommon(ResourceUri, ResourceUtc) :
    def __init__(self):
        ResourceUri.__init__(self, authentication=False)
        ResourceUtc.__init__(self)

        modes = ["walking", "car", "bike", "bss"]
        types = {
            "all": "All types",
            "best": "The best journey",
            "rapid": "A good trade off between duration, changes and constraint respect",
            'no_train': "Journey without train",
            'comfort': "A journey with less changes and walking",
            'car': "A journey with car to get to the public transport",
            'less_fallback_walk': "A journey with less walking",
            'less_fallback_bike': "A journey with less biking",
            'less_fallback_bss': "A journey with less bss",
            'fastest': "A journey with minimum duration",
            'non_pt_walk': "A journey without public transport, only walking",
            'non_pt_bike': "A journey without public transport, only biking",
            'non_pt_bss': "A journey without public transport, only bike sharing",
        }
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]

        parser_get.add_argument("from", type=unicode, dest="origin")
        parser_get.add_argument("to", type=unicode, dest="destination")
        parser_get.add_argument("datetime", type=date_time_format)
        parser_get.add_argument("datetime_represents", dest="clockwise",
                                type=dt_represents, default=True)
        parser_get.add_argument("max_transfers", type=int, default=42)
        parser_get.add_argument("max_nb_transfers", type=int, dest="max_transfers")
        parser_get.add_argument("first_section_mode[]",
                                type=option_value(modes),
                                dest="origin_mode", action="append")
        parser_get.add_argument("last_section_mode[]",
                                type=option_value(modes),
                                dest="destination_mode", action="append")
        parser_get.add_argument("max_duration_to_pt", type=int,
                                description="maximal duration of non public transport in second")

        parser_get.add_argument("max_walking_duration_to_pt", type=int,
                                description="maximal duration of walking on public transport in second")

        parser_get.add_argument("max_bike_duration_to_pt", type=int,
                                description="maximal duration of bike on public transport in second")
        parser_get.add_argument("max_bss_duration_to_pt", type=int,
                                description="maximal duration of bss on public transport in second")
        parser_get.add_argument("max_car_duration_to_pt", type=int,
                                description="maximal duration of car on public transport in second")

        parser_get.add_argument("walking_speed", type=float_gt_0)
        parser_get.add_argument("bike_speed", type=float_gt_0)
        parser_get.add_argument("bss_speed", type=float_gt_0)
        parser_get.add_argument("car_speed", type=float_gt_0)
        parser_get.add_argument("forbidden_uris[]", type=unicode, action="append")
        parser_get.add_argument("allowed_id[]", type=unicode, action="append")
        parser_get.add_argument("type", type=option_value(types),
                                default="all")
        parser_get.add_argument("disruption_active", type=boolean, default=False)  # for retrocomp
        # no default value for data_freshness because we need to maintain retrocomp with disruption_active
        parser_get.add_argument("data_freshness",
                                type=option_value(['base_schedule', 'adapted_schedule', 'realtime']))
        parser_get.add_argument("max_duration", type=unsigned_integer)
        parser_get.add_argument("wheelchair", type=boolean, default=None)
        # for retrocompatibility purpose, we duplicate (without []):
        parser_get.add_argument("first_section_mode",
                                type=option_value(modes), action="append")
        parser_get.add_argument("last_section_mode",
                                type=option_value(modes), action="append")
        parser_get.add_argument("traveler_type", type=option_value(acceptable_traveler_types))
        parser_get.add_argument("_current_datetime", type=date_time_format, default=datetime.utcnow(),
                                description="The datetime used to consider the state of the pt object"
                                            " Default is the current date and it is used for debug."
                                            " Note: it will mainly change the disruptions that concern "
                                            "the object The timezone should be specified in the format,"
                                            " else we consider it as UTC")

    def parse_args(self, region=None, uri=None):
        args = self.parsers['get'].parse_args()

        if args.get('max_duration_to_pt') is not None:
            # retrocompatibility: max_duration_to_pt override all individual value by mode
            args['max_walking_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bike_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bss_duration_to_pt'] = args['max_duration_to_pt']
            args['max_car_duration_to_pt'] = args['max_duration_to_pt']

        if args['data_freshness'] is None:
            # retrocompatibilty handling
            args['data_freshness'] = \
                'adapted_schedule' if args['disruption_active'] is True else 'base_schedule'

        # TODO : Changer le protobuff pour que ce soit propre
        if args['destination_mode'] == 'vls':
            args['destination_mode'] = 'bss'
        if args['origin_mode'] == 'vls':
            args['origin_mode'] = 'bss'

        # for last and first section mode retrocompatibility
        if 'first_section_mode' in args and args['first_section_mode']:
            args['origin_mode'] = args['first_section_mode']
        if 'last_section_mode' in args and args['last_section_mode']:
            args['destination_mode'] = args['last_section_mode']

        if region:
            if uri:
                objects = uri.split('/')
                if objects and len(objects) % 2 == 0:
                    args['origin'] = objects[-1]
                else:
                    abort(503, message="Unable to compute journeys "
                                           "from this object")

        #we transform the origin/destination url to add information
        if args['origin']:
            args['origin'] = transform_id(args['origin'])
        if args['destination']:
            args['destination'] = transform_id(args['destination'])

        args['original_datetime'] = args['datetime']

        if args.get('traveler_type'):
            traveler_profile = TravelerProfile.make_traveler_profile(region, args['traveler_type'])
            traveler_profile.override_params(args)

        # We set default modes for fallback modes.
        # The reason why we cannot put default values in parser_get.add_argument() is that, if we do so,
        # fallback modes will always have a value, and traveler_type will never override fallback modes.
        if args.get('origin_mode') is None:
            args['origin_mode'] = ['walking']
        if args.get('destination_mode') is None:
            args['destination_mode'] = ['walking']

        return args
