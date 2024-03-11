# coding=utf-8

#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from jormungandr import i_manager, fallback_modes, partner_services
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from datetime import datetime
from jormungandr.resources_utils import ResourceUtc
from jormungandr.interfaces.v1.transform_id import transform_id
from flask_restful import abort
import logging
from jormungandr.exceptions import RegionNotFound
from functools import cmp_to_key
from jormungandr.instance_manager import instances_comparator
from navitiacommon.default_traveler_profile_params import acceptable_traveler_types
import pytz
import six
from jormungandr.fallback_modes import FallbackModes
from navitiacommon.parser_args_type import (
    CustomSchemaType,
    TypeSchema,
    BooleanType,
    OptionValue,
    DescribedOptionValue,
    UnsignedInteger,
    DateTimeFormat,
    IntervalValue,
    SpeedRange,
    FloatRange,
    KeyValueType,
)
from navitiacommon import type_pb2

BICYCLE_TYPES = [t for t, _ in type_pb2.BicycleType.items()]


class DatetimeRepresents(CustomSchemaType):
    def __call__(self, value, name):
        if value == "arrival":
            return False
        if value == "departure":
            return True
        raise ValueError("Unable to parse {}".format(name))

    def schema(self):
        return TypeSchema(type=str, metadata={'enum': ['arrival', 'departure'], 'default': 'departure'})


def sort_regions(regions_list):
    """
    Sort the regions according to the instances_comparator (priority > is_free=False > is_free=True)
    :param regions_list: possible regions for the journeys request
    :return: sorted list according to the comparator
    """
    return sorted(regions_list, key=cmp_to_key(instances_comparator))


def compute_regions(args):
    """
    Method computing the possible regions on which the journey can be queried
    The complexity comes from the fact that the regions in Jormungandr can overlap.

    Rules are :
    - Fetch the different regions that can be used for 'origin' and 'destination'
    - Intersect both lists and sort it

    :return: Kraken instance keys
    """
    from_regions = set()
    to_regions = set()
    if args['origin']:
        from_regions = set(i_manager.get_instances(object_id=args['origin']))
        # Note: if get_regions does not find any region, it raises a RegionNotFoundException

    if args['destination']:
        to_regions = set(i_manager.get_instances(object_id=args['destination']))

    if not from_regions:
        # we didn't get any origin, the region is in the destination's list
        possible_regions = to_regions
    elif not to_regions:
        # we didn't get any origin, the region is in the destination's list
        possible_regions = from_regions
    else:
        # we need the intersection set
        possible_regions = from_regions.intersection(to_regions)
        logging.getLogger(__name__).debug(
            "orig region = {o}, dest region = {d} => set = {p}".format(
                o=from_regions, d=to_regions, p=possible_regions
            )
        )

    if not possible_regions:
        raise RegionNotFound(
            custom_msg="cannot find a region with {o} and {d} in the same time".format(
                o=args['origin'], d=args['destination']
            )
        )

    sorted_regions = list(possible_regions)

    regions = sort_regions(sorted_regions)

    return [r.name for r in regions]


def compute_possible_region(region, args):
    """
    :return:    If region is set in the query, return it
                If not, return a list of possible regions for the query
    """
    if region:
        return [i_manager.get_region(region)]
    else:
        # TODO: how to handle lon/lat ? don't we have to override args['origin'] ?
        return compute_regions(args)


class JourneyCommon(ResourceUri, ResourceUtc):
    def __init__(self, output_type_serializer):
        ResourceUri.__init__(self, authentication=False, output_type_serializer=output_type_serializer)
        ResourceUtc.__init__(self)

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
            'adapted information schedule.',
        }
        parser_get = self.parsers["get"]

        parser_get.add_argument(
            "from",
            type=six.text_type,
            dest="origin",
            help='The id of the departure of your journey. If not provided an isochrone is computed.',
            schema_metadata={'format': 'place'},
        )
        parser_get.add_argument(
            "to",
            type=six.text_type,
            dest="destination",
            help='The id of the arrival of your journey. If not provided an isochrone is computed.',
            schema_metadata={'format': 'place'},
        )
        parser_get.add_argument(
            "datetime",
            type=DateTimeFormat(),
            help='Date and time to go/arrive (see `datetime_represents`).\n'
            'Note: the datetime must be in the coverage’s publication period.',
        )
        parser_get.add_argument(
            "datetime_represents",
            dest="clockwise",
            type=DatetimeRepresents(),
            default=True,
            help="Determine how datetime is handled.\n\n"
            "Possible values:\n"
            " * 'departure' - Compute journeys starting after datetime\n"
            " * 'arrival' - Compute journeys arriving before datetime",
        )
        parser_get.add_argument("max_transfers", type=int, default=42, hidden=True, deprecated=True)
        parser_get.add_argument(
            "max_nb_transfers",
            type=int,
            dest="max_transfers",
            help='Maximum number of transfers in each journey',
        )
        parser_get.add_argument(
            "min_nb_transfers", type=int, default=0, help='Minimum number of transfers in each journey'
        )
        parser_get.add_argument(
            "first_section_mode[]",
            type=OptionValue(fallback_modes.all_fallback_modes),
            dest="origin_mode",
            action="append",
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
            'last_section_mode[]=bss&last_section_mode[]=bike`',
        )
        parser_get.add_argument(
            "last_section_mode[]",
            type=OptionValue(fallback_modes.all_fallback_modes),
            dest="destination_mode",
            action="append",
            help='Same as first_section_mode but for the last section.',
        )
        # for retrocompatibility purpose, we duplicate (without []):
        parser_get.add_argument(
            "first_section_mode",
            hidden=True,
            deprecated=True,
            type=OptionValue(fallback_modes.all_fallback_modes),
            action="append",
        )
        parser_get.add_argument(
            "last_section_mode",
            hidden=True,
            deprecated=True,
            type=OptionValue(fallback_modes.all_fallback_modes),
            action="append",
        )

        parser_get.add_argument(
            "max_duration_to_pt",
            type=int,
            help="Maximum allowed duration to reach the public transport (same limit used before and "
            "after public transport).\n"
            "Use this to limit the walking/biking part.\n"
            "Unit is seconds",
        )
        parser_get.add_argument(
            "max_walking_duration_to_pt",
            type=int,
            help="Maximal duration of walking on public transport in second",
        )
        parser_get.add_argument(
            "max_bike_duration_to_pt", type=int, help="Maximal duration of bike on public transport in second"
        )
        parser_get.add_argument(
            "max_bss_duration_to_pt", type=int, help="Maximal duration of bss on public transport in second"
        )
        parser_get.add_argument(
            "max_car_duration_to_pt", type=int, help="Maximal duration of car on public transport in second"
        )
        parser_get.add_argument(
            "max_ridesharing_duration_to_pt",
            type=int,
            dest="max_ridesharing_duration_to_pt",
            help="Maximal duration of ridesharing on public transport in second",
        )
        parser_get.add_argument(
            "max_car_no_park_duration_to_pt",
            type=int,
            help="Maximal duration of car no park on public transport in second",
        )
        parser_get.add_argument(
            "max_taxi_duration_to_pt",
            type=int,
            dest="max_taxi_duration_to_pt",
            help="Maximal duration of taxi on public transport in second, only available in distributed scenario",
        )
        for mode in FallbackModes.modes_str():
            parser_get.add_argument(
                "max_{}_direct_path_distance".format(mode),
                type=int,
                hidden=True,
                help="limit distance of direct path in {}, used ONLY in distributed scenario".format(mode),
            )
        parser_get.add_argument(
            "walking_speed",
            type=SpeedRange(),
            help='Walking speed for the fallback sections.\nSpeed unit must be in meter/second',
        )
        parser_get.add_argument(
            "bike_speed",
            type=SpeedRange(),
            help='Biking speed for the fallback sections.\nSpeed unit must be in meter/second',
        )
        parser_get.add_argument(
            "bss_speed",
            type=SpeedRange(),
            help='Speed while using a bike from a bike sharing system for the '
            'fallback sections.\n'
            'Speed unit must be in meter/second',
        )
        parser_get.add_argument(
            "car_speed",
            type=SpeedRange(),
            help='Driving speed for the fallback sections.\nSpeed unit must be in meter/second',
        )
        parser_get.add_argument(
            "ridesharing_speed",
            type=SpeedRange(),
            dest="ridesharing_speed",
            help='ridesharing speed for the fallback sections.\nSpeed unit must be in meter/second',
        )
        parser_get.add_argument(
            "car_no_park_speed",
            type=SpeedRange(),
            help='Driving speed without car park for the fallback sections.\n'
            'Speed unit must be in meter/second',
        )
        parser_get.add_argument(
            "taxi_speed",
            type=SpeedRange(),
            help='taxi speed speed for the fallback sections.\nSpeed unit must be in meter/second',
        )
        parser_get.add_argument(
            "forbidden_uris[]",
            type=six.text_type,
            action="append",
            help='If you want to avoid lines, modes, networks, etc.\n'
            'Note: the forbidden_uris[] concern only the public transport objects. '
            'You can’t for example forbid the use of the bike with them, '
            'you have to set the fallback modes for this '
            '(first_section_mode[] and last_section_mode[])',
            schema_metadata={'format': 'pt-object'},
        )
        parser_get.add_argument(
            "allowed_id[]",
            type=six.text_type,
            action="append",
            help='If you want to use only a small subset of '
            'the public transport objects in your solution.\n'
            'Note: The constraint intersects with forbidden_uris[]. '
            'For example, if you ask for '
            '`allowed_id[]=line:A&forbidden_uris[]=physical_mode:Bus`, '
            'only vehicles of the line A that are not buses will be used.',
            schema_metadata={'format': 'pt-object'},
        )
        parser_get.add_argument(
            "type",
            type=DescribedOptionValue(types),
            default="all",
            deprecated=True,
            help='DEPRECATED, desired type of journey.',
            hidden=True,
        )
        parser_get.add_argument(
            "disruption_active",
            type=BooleanType(),
            default=False,
            deprecated=True,
            help='DEPRECATED, replaced by `data_freshness`.\n'
            'If true the algorithm takes the disruptions into account, '
            'and thus avoid disrupted public transport.\n'
            'Nota: `disruption_active=true` <=> `data_freshness=realtime`',
        )
        # no default value for data_freshness because we need to maintain retrocomp with disruption_active
        parser_get.add_argument(
            "data_freshness",
            type=DescribedOptionValue(data_freshnesses),
            help="Define the freshness of data to use to compute journeys.\n"
            "When using the following parameter `&data_freshness=base_schedule` "
            "you can get disrupted journeys in the response. "
            "You can then display the disruption message to the traveler and "
            "make a `realtime` request to get a new undisrupted solution.",
        )
        parser_get.add_argument(
            "max_duration",
            type=UnsignedInteger(),
            help='Maximum duration of journeys in seconds (from `datetime` parameter).\n'
            'More usefull when computing an isochrone (only `from` or `to` is provided).\n'
            'On a classic journey (from-to), it will mostly speedup Navitia: You may have journeys a bit '
            'longer than that value (you would have to filter them).',
        )
        parser_get.add_argument(
            "wheelchair",
            type=BooleanType(),
            default=None,
            help='If true the traveler is considered to be using a wheelchair, '
            'thus only accessible public transport are used.\n'
            'Be warned: many data are currently too faint to provide '
            'acceptable answers with this parameter on.',
        )
        parser_get.add_argument(
            "traveler_type",
            type=OptionValue(acceptable_traveler_types),
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
            'and set them yourself.',
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
        parser_get.add_argument(
            "direct_path",
            type=OptionValue(['indifferent', 'only', 'none', 'only_with_alternatives']),
            default='indifferent',
            help="Specify if direct path should be suggested",
        )

        parser_get.add_argument(
            "free_radius_from",
            type=int,
            default=0,
            help="Radius length (in meters) around the coordinates of departure "
            "in which the stop points are considered free to go (crowfly=0)",
        )
        parser_get.add_argument(
            "free_radius_to",
            type=int,
            default=0,
            help="Radius length (in meters) around the coordinates of arrival "
            "in which the stop points are considered free to go (crowfly=0)",
        )
        parser_get.add_argument(
            "direct_path_mode[]",
            type=OptionValue(fallback_modes.all_fallback_modes),
            default=[],
            dest="direct_path_mode",
            action="append",
            help="Force the direct-path modes."
            "If this list is not empty, we only compute direct_path for modes in this list"
            "And filter all the direct_paths of modes in first_section_mode[]",
        )
        parser_get.add_argument(
            "_stop_points_nearby_duration",
            type=int,
            hidden=True,
            help="define the duration to reach stop points by crow fly",
        )
        parser_get.add_argument(
            "partner_services[]",
            type=OptionValue(partner_services.all_partner_services),
            dest="partner_services",
            action="append",
            help='Expose only the partner type into the response.',
        )
        parser_get.add_argument(
            "_asynchronous_ridesharing",
            type=BooleanType(),
            hidden=True,
            help="active the asynchronous mode for the ridesharing services",
        )
        parser_get.add_argument(
            "_access_points",
            type=BooleanType(),
            hidden=True,
            help="use/disuse the entrance/exit in journeys computation",
        )
        parser_get.add_argument(
            "_poi_access_points",
            type=BooleanType(),
            hidden=True,
            help="use/disuse the poi entrance/exit in journeys computation",
        )

        parser_get.add_argument(
            "additional_time_after_first_section_taxi",
            type=int,
            help="the additional time added to the taxi section, right after riding the taxi but before hopping on the public transit",
        )
        parser_get.add_argument(
            "additional_time_before_last_section_taxi",
            type=int,
            help="the additional time added to the taxi section, right before riding the taxi but after hopping off the public transit",
        )
        parser_get.add_argument(
            "_pt_planner",
            type=OptionValue(['kraken', 'loki']),
            hidden=True,
            help="choose which pt engine to compute the pt journey",
        )
        parser_get.add_argument(
            "criteria",
            type=OptionValue(
                [
                    'classic',
                    'robustness',
                    'occupancy',
                    'arrival_stop_attractivity',
                    'departure_stop_attractivity',
                ]
            ),
            help="choose the criteria used to compute pt journeys, feature in beta ",
        )
        parser_get.add_argument(
            "_keep_olympics_journeys",
            type=BooleanType(),
            hidden=True,
            default=None,
            help="do not delete journeys tagged for olympics",
        )
        parser_get.add_argument(
            "_olympics_sites_virtual_fallback[]",
            type=KeyValueType(),
            action="append",
            hidden=True,
            help="virtual fallback duration for olympics sites. The format should be stop_point_id,duration",
        )

        parser_get.add_argument(
            "_olympics_sites_attractivities[]",
            type=KeyValueType(min_value=0, max_value=255),
            action="append",
            hidden=True,
            help=(
                "attractivities for olympics sites. The format should be stop_point_id,attractivity,"
                "the value of attractivity must be in the rage of (0, 255)"
            ),
        )
        parser_get.add_argument(
            "_show_natural_opg_journeys",
            type=BooleanType(),
            hidden=True,
            default=None,
            help="Show natural opg journeys",
        )
        parser_get.add_argument(
            "_deactivate_opg_scenario",
            type=BooleanType(),
            hidden=True,
            default=False,
            help="Deactivate opg scenario",
        )
        # Advanced parameters for valhalla bike
        parser_get.add_argument(
            "bike_use_roads",
            type=FloatRange(0, 1),
            hidden=True,
            default=0.5,
            help="only available for Asgard: A cyclist's propensity to use roads alongside other vehicles.",
        )
        parser_get.add_argument(
            "bike_use_hills",
            type=FloatRange(0, 1),
            hidden=True,
            default=0.5,
            help="only available for Asgard: A cyclist's desire to tackle hills in their routes.",
        )
        parser_get.add_argument(
            "bike_use_ferry",
            type=FloatRange(0, 1),
            hidden=True,
            default=0.5,
            help="only available for Asgard: This value indicates the willingness to take ferries.",
        )
        parser_get.add_argument(
            "bike_avoid_bad_surfaces",
            type=FloatRange(0, 1),
            hidden=True,
            default=0.25,
            help="only available for Asgard: This value is meant to represent how much a cyclist wants to avoid roads with poor surfaces relative to the bicycle type being used.",
        )
        parser_get.add_argument(
            "bike_shortest",
            type=BooleanType(),
            hidden=True,
            default=False,
            help="only available for Asgard: Changes the metric to quasi-shortest, i.e. purely distance-based costing.",
        )
        parser_get.add_argument(
            "bicycle_type",
            type=OptionValue(BICYCLE_TYPES),
            hidden=True,
            default='hybrid',
            help="only available for Asgard: The type of bicycle.",
        )
        parser_get.add_argument(
            "bike_use_living_streets",
            type=FloatRange(0, 1),
            hidden=True,
            default=0.5,
            help="only available for Asgard: This value indicates the willingness to take living streets.",
        )
        parser_get.add_argument(
            "bike_maneuver_penalty",
            type=float,
            hidden=True,
            default=5,
            help="only available for Asgard: A penalty applied when transitioning between roads that do not have consistent naming–in other words, no road names in common. This penalty can be used to create simpler routes that tend to have fewer maneuvers or narrative guidance instructions.",
        )
        parser_get.add_argument(
            "bike_service_penalty",
            type=float,
            hidden=True,
            default=0,
            help="only available for Asgard: A penalty applied for transition to generic service road. ",
        )
        parser_get.add_argument(
            "bike_service_factor",
            type=float,
            hidden=True,
            default=1,
            help="only available for Asgard: A factor that modifies (multiplies) the cost when generic service roads are encountered. ",
        )
        parser_get.add_argument(
            "bike_country_crossing_cost",
            type=float,
            hidden=True,
            default=600,
            help="only available for Asgard: A cost applied when encountering an international border. This cost is added to the estimated and elapsed times. ",
        )
        parser_get.add_argument(
            "bike_country_crossing_penalty",
            type=float,
            hidden=True,
            default=0,
            help="only available for Asgard: A penalty applied for a country crossing. ",
        )

        # Advanced parameters for valhalla walking
        parser_get.add_argument(
            "walking_walkway_factor",
            type=float,
            hidden=True,
            default=1.0,
            help="only available for Asgard: "
            "A factor that modifies (multiplies) the cost when encountering roads classified as footway.",
        )

        parser_get.add_argument(
            "walking_sidewalk_factor",
            type=float,
            hidden=True,
            default=1.0,
            help="only available for Asgard: "
            "A factor that modifies (multiplies) the cost when encountering roads with dedicated sidewalks.",
        )

        parser_get.add_argument(
            "walking_alley_factor",
            type=float,
            hidden=True,
            default=2.0,
            help="only available for Asgard: "
            "A factor that modifies (multiplies) the cost when alleys are encountered.",
        )

        parser_get.add_argument(
            "walking_driveway_factor",
            type=float,
            hidden=True,
            default=5.0,
            help="only available for Asgard: "
            "A factor that modifies (multiplies) the cost when encountering a driveway, "
            "which is often a private, service road.",
        )

        parser_get.add_argument(
            "walking_step_penalty",
            type=float,
            hidden=True,
            default=30.0,
            help="only available for Asgard: "
            "A penalty in seconds added to each transition onto a path with steps or stairs.",
        )

        parser_get.add_argument(
            "walking_use_ferry",
            type=IntervalValue(type=float, min_value=0, max_value=1),
            hidden=True,
            default=0.5,
            help="only available for Asgard: "
            "This value indicates the willingness to take ferries. This is range of values between 0 and 1. "
            "Values near 0 attempt to avoid ferries and values near 1 will favor ferries.",
        )

        parser_get.add_argument(
            "walking_use_living_streets",
            type=IntervalValue(type=float, min_value=0, max_value=1),
            hidden=True,
            default=0.6,
            help="only available for Asgard: "
            "This value indicates the willingness to take living streets.It is a range of values between 0 and 1. "
            "Values near 0 attempt to avoid living streets and values near 1 will favor living streets. ",
        )

        parser_get.add_argument(
            "walking_use_tracks",
            type=IntervalValue(type=float, min_value=0, max_value=1),
            hidden=True,
            default=0.5,
            help="only available for Asgard: "
            "This value indicates the willingness to take track roads. This is a range of values between 0 and 1. "
            "Values near 0 attempt to avoid tracks and values near 1 will favor tracks a little bit.",
        )

        parser_get.add_argument(
            "walking_use_hills",
            type=IntervalValue(type=float, min_value=0, max_value=1),
            hidden=True,
            default=0.5,
            help="only available for Asgard: "
            "This is a range of values from 0 to 1, where 0 attempts to avoid hills and steep grades even if it "
            "means a longer (time and distance) path, while 1 indicates the pedestrian does not fear hills and "
            "steeper grades.",
        )

        parser_get.add_argument(
            "walking_service_penalty",
            type=float,
            hidden=True,
            default=0,
            help="only available for Asgard: A penalty applied for transition to generic service road.",
        )

        parser_get.add_argument(
            "walking_service_factor",
            type=float,
            hidden=True,
            default=1,
            help="only available for Asgard: "
            "A factor that modifies (multiplies) the cost when generic service roads are encountered.",
        )

        parser_get.add_argument(
            "walking_max_hiking_difficulty",
            type=IntervalValue(type=int, min_value=0, max_value=6),
            hidden=True,
            default=1,
            help="only available for Asgard: "
            "This value indicates the maximum difficulty of hiking trails that is allowed. "
            "Values between 0 and 6 are allowed.",
        )

        parser_get.add_argument(
            "walking_shortest",
            type=BooleanType(),
            hidden=True,
            default=False,
            help="only available for Asgard: "
            "Changes the metric to quasi-shortest, i.e. purely distance-based costing.",
        )

        parser_get.add_argument(
            "walking_ignore_oneways",
            type=BooleanType(),
            hidden=True,
            default=True,
            help="only available for Asgard: " "Ignore when encountering roads that are oneway.",
        )

        parser_get.add_argument(
            "_use_excluded_zones",
            type=BooleanType(),
            hidden=True,
            help="only available for Asgard so far: take into account excluded zones pre-defined in Asgard, "
            "Warning: this feature may be performance impacting.",
        )

    def parse_args(self, region=None, uri=None):
        args = self.parsers['get'].parse_args()

        if args.get('max_duration_to_pt') is not None:
            # retrocompatibility: max_duration_to_pt override all individual value by mode
            args['max_walking_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bike_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bss_duration_to_pt'] = args['max_duration_to_pt']
            args['max_car_duration_to_pt'] = args['max_duration_to_pt']
            args['max_car_no_park_duration_to_pt'] = args['max_duration_to_pt']
            args['max_ridesharing_duration_to_pt'] = args['max_duration_to_pt']
            args['max_taxi_duration_to_pt'] = args['max_duration_to_pt']

        if args['data_freshness'] is None:
            # retrocompatibilty handling
            args['data_freshness'] = 'realtime' if args['disruption_active'] is True else 'base_schedule'

        # TODO : Change protobuff to be cleaner
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
                    abort(503, message="Unable to compute journeys " "from this object")

        # we transform the origin/destination url to add information
        if args['origin']:
            args['origin'] = transform_id(args['origin'])
        if args['destination']:
            args['destination'] = transform_id(args['destination'])

        if args['datetime']:
            args['original_datetime'] = args['datetime']
        else:
            args['original_datetime'] = pytz.UTC.localize(args['_current_datetime'])

        return args
