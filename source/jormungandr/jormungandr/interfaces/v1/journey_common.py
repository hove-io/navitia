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
from jormungandr.interfaces.parsers import DateTimeFormat
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from datetime import datetime
from jormungandr.resources_utils import ResourceUtc
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.interfaces.parsers import float_gt_0, UnsignedInteger
from flask.ext.restful import reqparse, abort
import logging
from jormungandr.exceptions import RegionNotFound
from functools import cmp_to_key
from jormungandr.instance_manager import instances_comparator
from jormungandr.travelers_profile import TravelerProfile
from navitiacommon.default_traveler_profile_params import acceptable_traveler_types
import pytz
import six
from navitiacommon.parser_args_type import CustomSchemaType, TypeSchema, BooleanType, OptionValue, \
    DescribedOptionValue


class DatetimeRepresents(CustomSchemaType):
    def __call__(self, value, name):
        if value == "arrival":
            return False
        if value == "departure":
            return True
        raise ValueError("Unable to parse {}".format(name))

    def schema(self):
        return TypeSchema(type=str, metadata={'enum': ['arrival', 'departure'], 'default': 'departure'})


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
    def __init__(self, output_type_serializer):
        ResourceUri.__init__(self, authentication=False, output_type_serializer=output_type_serializer)
        ResourceUtc.__init__(self)

        modes = ["walking", "car", "bike", "bss", "ridesharing"]
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
        data_freshnesses = {
            'base_schedule': 'Use theoric schedule information',
            'realtime': 'Use all realtime information',
            'adapted_schedule': 'Use of adapted schedule information (like strike adjusting, etc.). '
                                'Prefer `realtime` for traveler information as it will also contain '
                                'adapted information schedule.'
        }
        parser_get = self.parsers["get"]

        parser_get.add_argument("from", type=six.text_type, dest="origin",
                                help='The id of the departure of your journey. '
                                     'If not provided an isochrone is computed.',
                                schema_metadata={'format': 'place'})
        parser_get.add_argument("to", type=six.text_type, dest="destination",
                                help='The id of the arrival of your journey. '
                                     'If not provided an isochrone is computed.',
                                schema_metadata={'format': 'place'})
        parser_get.add_argument("datetime", type=DateTimeFormat(),
                                help='Date and time to go/arrive (see `datetime_represents`).\n'
                                     'Note: the datetime must be in the coverage’s publication period.')
        parser_get.add_argument("datetime_represents", dest="clockwise",
                                type=DatetimeRepresents(), default=True,
                                help="Determine how datetime is handled.\n\n"
                                     "Possible values:\n"
                                     " * 'departure' - Compute journeys starting after datetime\n"
                                     " * 'arrival' - Compute journeys arriving before datetime")
        parser_get.add_argument("max_transfers", type=int, default=42, hidden=True, deprecated=True)
        parser_get.add_argument("max_nb_transfers", type=int, dest="max_transfers",
                                help='Maximum number of transfers in each journey')
        parser_get.add_argument("min_nb_transfers", type=int, default=0,
                                help='Minimum number of transfers in each journey')
        parser_get.add_argument("first_section_mode[]", type=OptionValue(modes),
                                dest="origin_mode", action="append",
                                help='Force the first section mode '
                                     'if the first section is not a public transport one.\n'
                                     '`bss` stands for bike sharing system.\n'
                                     'Note 1: It’s an array, you can give multiple modes.\n'
                                     'Note 2: Choosing `bss` implicitly allows the walking mode since '
                                     'you might have to walk to the bss station.\n'
                                     'Note 3: The parameter is inclusive, not exclusive, '
                                     'so if you want to forbid a mode, you need to add all the other modes. '
                                     'Eg: If you never want to use a car, you need: '
                                     '`first_section_mode[]=walking&first_section_mode[]=bss&'
                                     'first_section_mode[]=bike&last_section_mode[]=walking&'
                                     'last_section_mode[]=bss&last_section_mode[]=bike`')
        parser_get.add_argument("last_section_mode[]", type=OptionValue(modes),
                                dest="destination_mode", action="append",
                                help='Same as first_section_mode but for the last section.')
        # for retrocompatibility purpose, we duplicate (without []):
        parser_get.add_argument("first_section_mode", hidden=True, deprecated=True,
                                type=OptionValue(modes), action="append")
        parser_get.add_argument("last_section_mode", hidden=True, deprecated=True,
                                type=OptionValue(modes), action="append")

        parser_get.add_argument("max_duration_to_pt", type=int,
                                help="Maximal duration of non public transport in second")
        parser_get.add_argument("max_walking_duration_to_pt", type=int,
                                help="Maximal duration of walking on public transport in second")
        parser_get.add_argument("max_bike_duration_to_pt", type=int,
                                help="Maximal duration of bike on public transport in second")
        parser_get.add_argument("max_bss_duration_to_pt", type=int,
                                help="Maximal duration of bss on public transport in second")
        parser_get.add_argument("max_car_duration_to_pt", type=int,
                                help="Maximal duration of car on public transport in second")
        parser_get.add_argument("max_ridesharing_duration_to_pt", type=int,
                                dest="max_car_no_park_duration_to_pt",
                                help="Maximal duration of ridesharing on public transport in second")
        parser_get.add_argument("walking_speed", type=float_gt_0,
                                help='Walking speed for the fallback sections.\n'
                                     'Speed unit must be in meter/second')
        parser_get.add_argument("bike_speed", type=float_gt_0,
                                help='Biking speed for the fallback sections.\n'
                                     'Speed unit must be in meter/second')
        parser_get.add_argument("bss_speed", type=float_gt_0,
                                help='Speed while using a bike from a bike sharing system for the '
                                     'fallback sections.\n'
                                     'Speed unit must be in meter/second')
        parser_get.add_argument("car_speed", type=float_gt_0,
                                help='Driving speed for the fallback sections.\n'
                                     'Speed unit must be in meter/second')
        parser_get.add_argument("ridesharing_speed", type=float_gt_0, dest="car_no_park_speed",
                                help='ridesharing speed for the fallback sections.\n'
                                     'Speed unit must be in meter/second')
        parser_get.add_argument("forbidden_uris[]", type=six.text_type, action="append",
                                help='If you want to avoid lines, modes, networks, etc.\n'
                                     'Note: the forbidden_uris[] concern only the public transport objects. '
                                     'You can’t for example forbid the use of the bike with them, '
                                     'you have to set the fallback modes for this '
                                     '(first_section_mode[] and last_section_mode[])',
                                schema_metadata={'format': 'pt-object'})
        parser_get.add_argument("allowed_id[]", type=six.text_type, action="append",
                                help='If you want to use only a small subset of '
                                     'the public transport objects in your solution.\n'
                                     'Note: The constraint intersects with forbidden_uris[]. '
                                     'For example, if you ask for '
                                     '`allowed_id[]=line:A&forbidden_uris[]=physical_mode:Bus`, '
                                     'only vehicles of the line A that are not buses will be used.',
                                schema_metadata={'format': 'pt-object'})
        parser_get.add_argument("type", type=DescribedOptionValue(types), default="all", deprecated=True,
                                help='DEPRECATED, desired type of journey.', hidden=True)
        parser_get.add_argument("disruption_active", type=BooleanType(), default=False, deprecated=True,
                                help='DEPRECATED, replaced by `data_freshness`.\n'
                                     'If true the algorithm takes the disruptions into account, '
                                     'and thus avoid disrupted public transport.\n'
                                     'Nota: `disruption_active=true` <=> `data_freshness=realtime`')
        # no default value for data_freshness because we need to maintain retrocomp with disruption_active
        parser_get.add_argument("data_freshness", type=DescribedOptionValue(data_freshnesses),
                                help="Define the freshness of data to use to compute journeys.\n"
                                     "When using the following parameter `&data_freshness=base_schedule` "
                                     "you can get disrupted journeys in the response. "
                                     "You can then display the disruption message to the traveler and "
                                     "make a `realtime` request to get a new undisrupted solution.")
        parser_get.add_argument("max_duration", type=UnsignedInteger(),
                                help='Maximum duration of journeys in secondes.\n'
                                     'Really useful when computing an isochrone.')
        parser_get.add_argument("wheelchair", type=BooleanType(), default=None,
                                help='If true the traveler is considered to be using a wheelchair, '
                                     'thus only accessible public transport are used.\n'
                                     'Be warned: many data are currently too faint to provide '
                                     'acceptable answers with this parameter on.')
        parser_get.add_argument("traveler_type", type=OptionValue(acceptable_traveler_types),
                                help='Define speeds and accessibility values for different kind of people.\n'
                                     'Each profile also automatically determines appropriate first and '
                                     'last section modes to the covered area.\n'
                                     'Note: this means that you might get car, bike, etc. fallback routes '
                                     'even if you set `forbidden_uris[]`!\n'
                                     'You can overload all parameters '
                                     '(especially speeds, distances, first and last modes) by setting '
                                     'all of them specifically.\n'
                                     'We advise that you don’t rely on the traveler_type’s fallback modes '
                                     '(`first_section_mode[]` and `last_section_mode[]`) '
                                     'and set them yourself.')
        parser_get.add_argument("_current_datetime", type=DateTimeFormat(),
                                schema_metadata={'default': 'now'}, hidden=True,
                                default=datetime.utcnow(),
                                help='The datetime considered as "now". Used for debug, default is '
                                     'the moment of the request. It will mainly change the output '
                                     'of the disruptions.')
        parser_get.add_argument("direct_path", type=OptionValue(['indifferent', 'only', 'none']),
                                default='indifferent',
                                help="Specify if direct path should be suggested")

        parser_get.add_argument("free_radius_from", type=int, default=0,
                                help="Radius length (in meters) around the coordinates of departure "
                                     "in which the stop points are considered free to go (crowfly=0)")
        parser_get.add_argument("free_radius_to", type=int, default=0,
                                help="Radius length (in meters) around the coordinates of arrival "
                                     "in which the stop points are considered free to go (crowfly=0)")

    def parse_args(self, region=None, uri=None):
        args = self.parsers['get'].parse_args()

        if args.get('max_duration_to_pt') is not None:
            # retrocompatibility: max_duration_to_pt override all individual value by mode
            args['max_walking_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bike_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bss_duration_to_pt'] = args['max_duration_to_pt']
            args['max_car_duration_to_pt'] = args['max_duration_to_pt']
            args['max_car_no_park_duration_to_pt'] = args['max_duration_to_pt']

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

        if args['datetime']:
            args['original_datetime'] = args['datetime']
        else:
            args['original_datetime'] = pytz.UTC.localize(args['_current_datetime'])


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
