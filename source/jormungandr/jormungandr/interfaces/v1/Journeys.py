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
import logging, json, hashlib, datetime
from flask import request, g
from flask_restful import abort
from jormungandr import i_manager, app, fallback_modes
from jormungandr.interfaces.parsers import default_count_arg_type
from jormungandr.interfaces.v1.ResourceUri import complete_links
from functools import wraps
from jormungandr.timezone import set_request_timezone
from jormungandr.interfaces.v1.make_links import (
    create_external_link,
    create_internal_link,
    make_external_service_link,
)
from jormungandr.interfaces.v1.errors import ManageError
from collections import defaultdict
from navitiacommon import response_pb2
from jormungandr.utils import (
    date_to_timestamp,
    dt_to_str,
    has_invalid_reponse_code,
    journeys_absent,
    local_str_date_to_utc,
    UTC_DATETIME_FORMAT,
    COVERAGE_ANY_BETA,
)
from jormungandr.interfaces.v1.serializer import api
from jormungandr.interfaces.v1.decorators import get_serializer
from navitiacommon import default_values
from navitiacommon import type_pb2
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.interfaces.v1.journey_common import JourneyCommon, compute_possible_region
from jormungandr.parking_space_availability.parking_places_manager import ManageParkingPlaces
import six
from navitiacommon.parser_args_type import (
    BooleanType,
    OptionValue,
    UnsignedInteger,
    PositiveInteger,
    DepthArgument,
    PositiveFloat,
)
from jormungandr.interfaces.common import add_poi_infos_types, handle_poi_infos
from jormungandr.fallback_modes import FallbackModes
from copy import deepcopy
from jormungandr.travelers_profile import TravelerProfile


f_datetime = "%Y%m%dT%H%M%S"


class add_debug_info(object):
    """
    display info stored in g for the debug

    must be called after the transformation from protobuff to dict
    """

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)

            response = objects[0]

            def get_debug():
                if not 'debug' in response:
                    response['debug'] = {}
                return response['debug']

            if hasattr(g, 'errors_by_region'):
                get_debug()['errors_by_region'] = {}
                for region, er in g.errors_by_region.items():
                    get_debug()['errors_by_region'][region] = er.message

            if hasattr(g, 'regions_called'):
                get_debug()['regions_called'] = g.regions_called

            return objects

        return wrapper


class add_journey_href(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if has_invalid_reponse_code(objects) or journeys_absent(objects):
                return objects

            # TODO: This is a temporary hack to resolve Error introduced during migration to Python3
            # Instead of using wrongly parsed request.args we use original args to get existing allowed_id[]
            raw_args = args[0].parsers['get'].parse_args()
            for journey in objects[0]['journeys']:
                args = dict(request.args)
                allowed_ids = {
                    o['stop_point']['id']
                    for s in journey.get('sections', [])
                    if 'from' in s
                    for o in (s['from'], s['to'])
                    if 'stop_point' in o
                }

                if 'region' in kwargs:
                    args['region'] = kwargs['region']
                if "sections" not in journey:  # this mean it's an isochrone...
                    if 'to' not in args:
                        args['to'] = journey['to']['id']
                    if 'from' not in args:
                        args['from'] = journey['from']['id']
                    args['rel'] = 'journeys'
                    journey['links'] = [create_external_link('v1.journeys', **args)]
                elif allowed_ids and 'public_transport' in (s['type'] for s in journey['sections']):
                    # exactly one first_section_mode
                    if any(s['type'].startswith('bss') for s in journey['sections'][:2]):
                        args['first_section_mode[]'] = 'bss'
                    else:
                        args['first_section_mode[]'] = journey['sections'][0].get('mode', 'walking')

                    # exactly one last_section_mode
                    if any(s['type'].startswith('bss') for s in journey['sections'][-2:]):
                        args['last_section_mode[]'] = 'bss'
                    else:
                        args['last_section_mode[]'] = journey['sections'][-1].get('mode', 'walking')

                    args['min_nb_transfers'] = journey['nb_transfers']
                    args['direct_path'] = 'only' if 'non_pt' in journey['tags'] else 'none'
                    args['min_nb_journeys'] = 5
                    args['is_journey_schedules'] = True
                    param_values = raw_args.get('allowed_id[]') or []
                    allowed_ids.update(param_values)
                    args['allowed_id[]'] = list(allowed_ids)
                    args['_type'] = 'journeys'

                    # Delete arguments that are contradictory to the 'same_journey_schedules' concept
                    if '_final_line_filter' in args:
                        del args['_final_line_filter']
                    if '_no_shared_section' in args:
                        del args['_no_shared_section']

                    # Add datetime depending on datetime_represents parameter
                    if 'datetime_represents' not in args:
                        args['datetime'] = journey['departure_date_time']
                    else:
                        args['datetime'] = (
                            journey['departure_date_time']
                            if 'departure' in args.get('datetime_represents')
                            else journey['arrival_date_time']
                        )

                    # Here we create two links same_journey_schedules and this_journey
                    args['rel'] = 'same_journey_schedules'
                    same_journey_schedules_link = create_external_link('v1.journeys', **args)
                    args['rel'] = 'this_journey'
                    args['min_nb_journeys'] = 1
                    args['count'] = 1
                    this_journey_link = create_external_link('v1.journeys', **args)
                    journey['links'] = [same_journey_schedules_link, this_journey_link]
            return objects

        return wrapper


# add the link between a section and the ticket needed for that section
class add_fare_links(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if has_invalid_reponse_code(objects) or journeys_absent(objects):
                return objects

            if 'tickets' not in objects[0]:
                return objects

            ticket_by_section = defaultdict(list)
            for t in objects[0]['tickets']:
                if "links" in t:
                    for s in t['links']:
                        ticket_by_section[s['id']].append(t['id'])

            for j in objects[0]['journeys']:
                if "sections" not in j:
                    continue
                for s in j['sections']:

                    # them we add the link to the different tickets needed
                    for ticket_needed in ticket_by_section[s["id"]]:
                        s['links'].append(create_internal_link(_type="ticket", rel="tickets", id=ticket_needed))
                    if "ridesharing_journeys" not in s:
                        continue
                    for rsj in s['ridesharing_journeys']:
                        if "sections" not in rsj:
                            continue
                        for rss in rsj['sections']:
                            # them we add the link to the different ridesharing-tickets needed
                            for rs_ticket_needed in ticket_by_section[rss["id"]]:
                                rss['links'].append(
                                    create_internal_link(_type="ticket", rel="tickets", id=rs_ticket_needed)
                                )

            return objects

        return wrapper


# Add TAD deep links in each section of type on_demand_transport if network contains app_code in codes
class add_tad_links(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if has_invalid_reponse_code(objects) or journeys_absent(objects):
                return objects

            for j in objects[0]['journeys']:
                if "sections" not in j:
                    continue
                for s in j['sections']:
                    # For a section with type = on_demand_transport
                    if s.get('type') == 'on_demand_transport':
                        # get network uri from the link
                        network_id = next(
                            (link['id'] for link in s.get('links', []) if link['type'] == "network"), None
                        )
                        if not network_id:
                            continue

                        region = kwargs.get('region')
                        if region is None:
                            continue

                        # Get the Network details and verify if it contains codes with type = "app_code"
                        instance = i_manager.instances.get(region)
                        network_details = instance.ptref.get_objs(
                            type_pb2.NETWORK, 'network.uri={}'.format(network_id)
                        )
                        network_dict = protobuf_to_dict(next(network_details))
                        app_value = next(
                            (
                                code['value']
                                for code in network_dict.get('codes', [])
                                if code.get('type') == "app_code"
                            ),
                            None,
                        )
                        if not app_value:
                            continue

                        # Prepare parameters for the deeplink of external service
                        from_embedded_type = s.get('from').get('embedded_type')
                        to_embedded_type = s.get('to').get('embedded_type')
                        from_coord = s.get('from').get(from_embedded_type).get('coord')
                        to_coord = s.get('to').get(to_embedded_type).get('coord')
                        args = dict()
                        date_utc = local_str_date_to_utc(s.get('departure_date_time'), instance.timezone)
                        args['departure_latitude'] = from_coord.get('lat')
                        args['departure_longitude'] = from_coord.get('lon')
                        args['destination_latitude'] = to_coord.get('lat')
                        args['destination_longitude'] = to_coord.get('lon')
                        args['requested_departure_time'] = dt_to_str(date_utc, _format=UTC_DATETIME_FORMAT)
                        url = "{}://home?".format(app_value)
                        tad_link = make_external_service_link(
                            url=url, rel="tad_dynamic_link", _type="tad_dynamic_link", **args
                        )
                        s['links'].append(tad_link)

            return objects

        return wrapper


class rig_journey(object):
    """
    decorator to rig journeys in order to put back the requested origin/destination in the journeys
    those origin/destination can be changed internally by some scenarios
    (querying external autocomplete service)
    """

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if has_invalid_reponse_code(objects):
                return objects

            if not hasattr(g, 'origin_detail') or not hasattr(g, 'destination_detail'):
                return objects

            response = objects[0]
            for j in response.get('journeys', []):
                if 'sections' not in j:
                    continue
                logging.debug(
                    'for journey changing origin: {old_o} to {new_o}'
                    ', destination to {old_d} to {new_d}'.format(
                        old_o=j.get('sections', [{}])[0].get('from').get('id'),
                        new_o=(g.origin_detail or {}).get('id'),
                        old_d=j.get('sections', [{}])[-1].get('to').get('id'),
                        new_d=(g.destination_detail or {}).get('id'),
                    )
                )
                if g.origin_detail:
                    j['sections'][0]['from'] = g.origin_detail
                if g.destination_detail:
                    j['sections'][-1]['to'] = g.destination_detail

            return objects

        return wrapper


class Journeys(JourneyCommon):
    def __init__(self):
        # journeys must have a custom authentication process

        super(Journeys, self).__init__(output_type_serializer=api.JourneysSerializer)

        parser_get = self.parsers["get"]

        parser_get.add_argument("count", type=default_count_arg_type, help='Fixed number of different journeys')
        parser_get.add_argument("_min_journeys_calls", type=int, hidden=True)
        parser_get.add_argument("_final_line_filter", type=BooleanType(), hidden=True)
        parser_get.add_argument(
            "is_journey_schedules",
            type=BooleanType(),
            default=False,
            help="True when '/journeys' is called to compute"
            "the same journey schedules and "
            "it'll override some specific parameters",
        )
        parser_get.add_argument(
            "min_nb_journeys",
            type=UnsignedInteger(),
            help='Minimum number of different suggested journeys, must be >= 0',
        )
        parser_get.add_argument(
            "max_nb_journeys",
            type=PositiveInteger(),
            help='Maximum number of different suggested journeys, must be > 0',
        )
        parser_get.add_argument("_max_extra_second_pass", type=int, dest="max_extra_second_pass", hidden=True)

        parser_get.add_argument(
            "debug",
            type=BooleanType(),
            default=False,
            hidden=True,
            help='Activate debug mode.\nNo journeys are filtered in this mode.',
        )
        parser_get.add_argument(
            "_filter_odt_journeys",
            type=BooleanType(),
            hidden=True,
            help='Filter journeys that uses On Demand Transport if they arrive later/depart earlier than a pure public transport journey.',
        )
        parser_get.add_argument(
            "show_codes",
            type=BooleanType(),
            default=True,
            hidden=True,
            deprecated=True,
            help="DEPRECATED (always True), show more identification codes",
        )
        parser_get.add_argument(
            "_override_scenario",
            type=six.text_type,
            hidden=True,
            help="debug param to specify a custom scenario",
        )
        parser_get.add_argument(
            "_street_network", type=six.text_type, hidden=True, help="choose the streetnetwork component"
        )
        parser_get.add_argument("_walking_transfer_penalty", hidden=True, type=int)
        parser_get.add_argument("_arrival_transfer_penalty", hidden=True, type=int)
        parser_get.add_argument("_max_successive_physical_mode", hidden=True, type=int)
        parser_get.add_argument("_max_additional_connections", hidden=True, type=int)
        parser_get.add_argument("_night_bus_filter_base_factor", hidden=True, type=int)
        parser_get.add_argument("_night_bus_filter_max_factor", hidden=True, type=float)
        parser_get.add_argument("_min_car", hidden=True, type=int)
        parser_get.add_argument("_min_bike", hidden=True, type=int)
        parser_get.add_argument("_min_taxi", hidden=True, type=int)
        parser_get.add_argument("_min_ridesharing", hidden=True, type=int)
        parser_get.add_argument(
            "bss_stands",
            type=BooleanType(),
            default=False,
            deprecated=True,
            help="DEPRECATED, Use add_poi_infos[]=bss_stands",
        )
        parser_get.add_argument(
            "add_poi_infos[]",
            type=OptionValue(add_poi_infos_types),
            default=[],
            dest="add_poi_infos",
            action="append",
            help="Show more information about the poi if it's available, for instance, show "
            "BSS/car park availability in the pois(BSS/car park) of response",
        )
        parser_get.add_argument(
            "_no_shared_section",
            type=BooleanType(),
            default=False,
            hidden=True,
            dest="no_shared_section",
            help="Shared section journeys aren't returned as a separate journey",
        )
        parser_get.add_argument(
            "timeframe_duration",
            type=int,
            help="Minimum timeframe to search journeys.\n"
            "For example 'timeframe_duration=3600' will search for all "
            "interesting journeys departing within the next hour.\n"
            "Nota 1: Navitia can return journeys after that timeframe as it's "
            "actually a minimum.\n"
            "Nota 2: 'max_nb_journeys' parameter has priority over "
            "'timeframe_duration' parameter.",
        )
        parser_get.add_argument(
            "_max_nb_crowfly_by_walking",
            type=int,
            hidden=True,
            help="limit nb of stop points accesible by walking crowfly, "
            "used especially in distributed scenario",
        )
        parser_get.add_argument(
            "_max_nb_crowfly_by_car",
            type=int,
            hidden=True,
            help="limit nb of stop points accesible by car crowfly, " "used especially in distributed scenario",
        )
        parser_get.add_argument(
            "_max_nb_crowfly_by_car_nor_park",
            type=int,
            hidden=True,
            help="limit nb of stop points accesible by car no park crowfly, used especially in distributed scenario",
        )
        parser_get.add_argument(
            "_max_nb_crowfly_by_taxi",
            type=int,
            hidden=True,
            help="limit nb of stop points accesible by taxi crowfly, " "used especially in distributed scenario",
        )
        parser_get.add_argument(
            "_max_nb_crowfly_by_bike",
            type=int,
            hidden=True,
            help="limit nb of stop points accesible by bike crowfly, " "used especially in distributed scenario",
        )
        parser_get.add_argument(
            "_max_nb_crowfly_by_bss",
            type=int,
            hidden=True,
            help="limit nb of stop points accesible by bss crowfly, " "used especially in distributed scenario",
        )
        parser_get.add_argument(
            "_car_park_duration",
            type=int,
            default=default_values.car_park_duration,
            hidden=True,
            help="how long it takes to park the car, " "used especially in distributed scenario",
        )
        parser_get.add_argument(
            "_here_realtime_traffic",
            type=BooleanType(),
            default=True,
            hidden=True,
            help="Here, Active or not the realtime traffic information (True/False)",
        )
        parser_get.add_argument(
            "_handimap_language",
            type=OptionValue(
                [
                    'english',
                    'french',
                ]
            ),
            hidden=True,
            help='Handimap, select a specific language for guidance instruction.\n'
            'list available:\n'
            '- english = english\n'
            '- french = french\n',
        )
        parser_get.add_argument(
            "_here_language",
            type=OptionValue(
                [
                    'afrikaans',
                    'arabic',
                    'chinese',
                    'dutch',
                    'english',
                    'french',
                    'german',
                    'hebrew',
                    'hindi',
                    'italian',
                    'japanese',
                    'nepali',
                    'portuguese',
                    'russian',
                    'spanish',
                ]
            ),
            hidden=True,
            help='Here, select a specific language for guidance instruction.\n'
            'list available:\n'
            '- afrikaans = af\n'
            '- arabic = ar-sa\n'
            '- chinese = zh-cn\n'
            '- dutch = nl-nl\n'
            '- english = en-gb\n'
            '- french = fr-fr\n'
            '- german = de-de\n'
            '- hebrew = he\n'
            '- hindi = hi\n'
            '- italian = it-it\n'
            '- japanese = ja-jp\n'
            '- nepali = ne-np\n'
            '- portuguese = pt-pt\n'
            '- russian = ru-ru\n'
            '- spanish = es-es\n',
        )
        parser_get.add_argument(
            "_here_matrix_type",
            type=six.text_type,
            hidden=True,
            help="Here, street network matrix type (simple_matrix/multi_direct_path)",
        )
        parser_get.add_argument(
            "_here_max_matrix_points",
            type=int,
            default=default_values.here_max_matrix_points,
            hidden=True,
            help="Here, Max number of matrix points for the street network computation (limited to 100)",
        )
        parser_get.add_argument(
            '_here_exclusion_area[]',
            type=six.text_type,
            case_sensitive=False,
            hidden=True,
            action='append',
            dest='_here_exclusion_area[]',
            help='Give 2 coords for an exclusion box. The format is like that:\n'
            'Coord_1!Coord_2 with Coord=lat;lon\n'
            ' - exemple : _here_exclusion_area[]=2.40553;48.84866!2.41453;48.85677\n'
            ' - This is a list, you can add to the maximun 20 _here_exclusion_area[]\n',
        )
        parser_get.add_argument(
            "_asgard_language",
            type=OptionValue(
                [
                    'bulgarian',
                    'catalan',
                    'czech',
                    'danish',
                    'german',
                    'greek',
                    'english_gb',
                    'english_pirate',
                    'english_us',
                    'spanish',
                    'estonian',
                    'finnish',
                    'french',
                    'hindi',
                    'hungarian',
                    'italian',
                    'japanese',
                    'bokmal',
                    'dutch',
                    'polish',
                    'portuguese_br',
                    'portuguese_pt',
                    'romanian',
                    'russian',
                    'slovak',
                    'slovenian',
                    'swedish',
                    'turkish',
                    'ukrainian',
                ]
            ),
            hidden=True,
            help='Select a specific language for Asgard guidance instruction.\n'
            'list available:\n'
            '- bulgarian = bg-BG\n'
            '- catalan = ca-ES\n'
            '- czech = cs-CZ\n'
            '- danish = da-DK\n'
            '- german = de-DE\n'
            '- greek = el-GR\n'
            '- english_gb = en-GB\n'
            '- english_pirate = en-US-x-pirate\n'
            '- english_us = en-US\n'
            '- spanish = es-ES\n'
            '- estonian = et-EE\n'
            '- finnish = fi-FI\n'
            '- french = fr-FR\n'
            '- hindi = hi-IN\n'
            '- hungarian = hu-HU\n'
            '- italian = it-IT\n'
            '- japanese = ja-JP\n'
            '- bokmal = nb-NO\n'
            '- dutch = nl-NL\n'
            '- polish = pl-PL\n'
            '- portuguese_br = pt-BR\n'
            '- portuguese_pt = pt-PT\n'
            '- romanian = ro-RO\n'
            '- russian = ru-RU\n'
            '- slovak = sk-SK\n'
            '- slovenian = sl-SI\n'
            '- swedish = sv-SE\n'
            '- turkish = tr-TR\n'
            '- ukrainian = uk-UA\n',
        )
        parser_get.add_argument(
            "equipment_details",
            default=True,
            type=BooleanType(),
            help="enhance response with accessibility equipement details",
        )

        parser_get.add_argument(
            "_enable_instructions",
            type=BooleanType(),
            default=True,
            hidden=True,
            help="Enable/Disable the narrative instructions for street network sections",
        )

        for mode in FallbackModes.modes_str():
            parser_get.add_argument(
                "max_{}_direct_path_duration".format(mode),
                type=int,
                help="limit duration of direct path in {}, used ONLY in distributed scenario".format(mode),
            )

        parser_get.add_argument("depth", type=DepthArgument(), default=1, help="The depth of your object")
        args = self.parsers["get"].parse_args()

        self.get_decorators.append(complete_links(self))

        if handle_poi_infos(args["add_poi_infos"], args["bss_stands"]):
            self.get_decorators.insert(1, ManageParkingPlaces(self, 'journeys'))

        parser_get.add_argument(
            "max_waiting_duration",
            type=PositiveInteger(),
            help='A journey containing a waiting section with a duration greater or equal to  max_waiting_duration '
            'will be discarded. Units : seconds. Must be > 0. Default value : 4h',
        )

        parser_get.add_argument(
            "bss_rent_duration",
            type=int,
            hidden=True,
            help="Only used in bss mode, how long it takes to rent a bike from bike share station",
        )

        parser_get.add_argument(
            "bss_rent_penalty",
            type=int,
            hidden=True,
            help="Only used in bss mode, how much the maneuver is penalized in the search algorithm",
        )

        parser_get.add_argument(
            "bss_return_duration",
            type=int,
            hidden=True,
            help="Only used in bss mode, how long it takes to return a bike to bike share station",
        )

        parser_get.add_argument(
            "bss_return_penalty",
            type=int,
            hidden=True,
            help="Only used in bss mode, how much the maneuver is penalized in the search algorithm",
        )

        parser_get.add_argument(
            "_transfer_path",
            type=BooleanType(),
            hidden=True,
            help="Compute pathways using the street_network engine for transfers between surface physical modes",
        )

        parser_get.add_argument(
            "_asgard_max_walking_duration_coeff",
            type=PositiveFloat(),
            default=1.12,
            hidden=True,
            help="used to adjust the search range in Asgard when computing matrix",
        )
        parser_get.add_argument(
            "_asgard_max_bike_duration_coeff",
            type=PositiveFloat(),
            default=2.8,
            hidden=True,
            help="used to adjust the search range in Asgard when computing matrix",
        )
        parser_get.add_argument(
            "_asgard_max_bss_duration_coeff",
            type=PositiveFloat(),
            default=0.46,
            hidden=True,
            help="used to adjust the search range in Asgard when computing matrix",
        )
        parser_get.add_argument(
            "_asgard_max_car_duration_coeff",
            type=PositiveFloat(),
            default=1,
            hidden=True,
            help="used to adjust the search range in Asgard when computing matrix",
        )

    @add_tad_links()
    @add_debug_info()
    @add_fare_links()
    @add_journey_href()
    @rig_journey()
    @get_serializer(serpy=api.JourneysSerializer)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        # We remove the region any-beta if present. This is a temporary hack and should be removed later
        if region == COVERAGE_ANY_BETA:
            region = None

        args = self.parsers['get'].parse_args()
        possible_regions = compute_possible_region(region, args)
        logging.getLogger(__name__).debug("Possible regions for the request : {}".format(possible_regions))
        args.update(self.parse_args(region, uri))

        # count override min_nb_journey or max_nb_journey
        if 'count' in args and args['count']:
            args['min_nb_journeys'] = args['count']
            args['max_nb_journeys'] = args['count']

        if (
            args['min_nb_journeys']
            and args['max_nb_journeys']
            and args['max_nb_journeys'] < args['min_nb_journeys']
        ):
            abort(400, message='max_nb_journeyes must be >= min_nb_journeys')

        if args.get('timeframe_duration'):
            args['timeframe_duration'] = min(args['timeframe_duration'], default_values.max_duration)

        if not (args['destination'] or args['origin']):
            abort(400, message="you should at least provide either a 'from' or a 'to' argument")

        if args['destination'] and args['origin']:
            api = 'journeys'
        else:
            api = 'isochrone'

        if api == 'journeys' and args['origin'] == args['destination']:
            abort(400, message="your origin and destination points are the same")

        if api == 'isochrone':
            # we have custom default values for isochrone because they are very resource expensive
            if args.get('max_duration') is None:
                args['max_duration'] = app.config['ISOCHRONE_DEFAULT_VALUE']
            if 'ridesharing' in (args['origin_mode'] or []) or 'ridesharing' in (args['destination_mode'] or []):
                abort(400, message='ridesharing isn\'t available on isochrone')

        def _set_specific_params(mod):
            if args.get('max_duration') is None:
                args['max_duration'] = mod.max_duration
            if args.get('_arrival_transfer_penalty') is None:
                args['_arrival_transfer_penalty'] = mod.arrival_transfer_penalty
            if args.get('_walking_transfer_penalty') is None:
                args['_walking_transfer_penalty'] = mod.walking_transfer_penalty
            if args.get('_night_bus_filter_base_factor') is None:
                args['_night_bus_filter_base_factor'] = mod.night_bus_filter_base_factor
            if args.get('_night_bus_filter_max_factor') is None:
                args['_night_bus_filter_max_factor'] = mod.night_bus_filter_max_factor
            if args.get('_max_additional_connections') is None:
                args['_max_additional_connections'] = mod.max_additional_connections
            if args.get('min_nb_journeys') is None:
                args['min_nb_journeys'] = mod.min_nb_journeys
            if args.get('max_nb_journeys') is None:
                args['max_nb_journeys'] = mod.max_nb_journeys
            if args.get('_min_journeys_calls') is None:
                args['_min_journeys_calls'] = mod.min_journeys_calls
            if args.get('_max_successive_physical_mode') is None:
                args['_max_successive_physical_mode'] = mod.max_successive_physical_mode
            if args.get('_final_line_filter') is None:
                args['_final_line_filter'] = mod.final_line_filter
            if args.get('max_extra_second_pass') is None:
                args['max_extra_second_pass'] = mod.max_extra_second_pass
            if args.get('additional_time_after_first_section_taxi') is None:
                args['additional_time_after_first_section_taxi'] = mod.additional_time_after_first_section_taxi
            if args.get('additional_time_before_last_section_taxi') is None:
                args['additional_time_before_last_section_taxi'] = mod.additional_time_before_last_section_taxi

            if args.get('_stop_points_nearby_duration') is None:
                args['_stop_points_nearby_duration'] = mod.stop_points_nearby_duration

            # we create a new arg for internal usage, only used by distributed scenario
            args['max_nb_crowfly_by_mode'] = mod.max_nb_crowfly_by_mode  # it's a dict of str vs int
            for mode in fallback_modes.all_fallback_modes:
                nb_crowfly = args.get('_max_nb_crowfly_by_{}'.format(mode))
                if nb_crowfly is not None:
                    args['max_nb_crowfly_by_mode'][mode] = nb_crowfly

            # activated only for distributed
            for mode in FallbackModes.modes_str():
                tmp = 'max_{}_direct_path_duration'.format(mode)
                if args.get(tmp) is None:
                    args[tmp] = getattr(mod, tmp)

            for mode in FallbackModes.modes_str():
                tmp = 'max_{}_direct_path_distance'.format(mode)
                if args.get(tmp) is None:
                    args[tmp] = getattr(mod, tmp)

            if args.get('_transfer_path') is None:
                args['_transfer_path'] = mod.transfer_path

            if args.get('_access_points') is None:
                args['_access_points'] = mod.access_points

            if args.get('_poi_access_points') is None:
                args['_poi_access_points'] = mod.poi_access_points

            if args.get('_pt_planner') is None:
                args['_pt_planner'] = mod.default_pt_planner

            if args.get('_asgard_language') is None:
                args['_asgard_language'] = mod.asgard_language

            if args.get('bss_rent_duration') is None:
                args['bss_rent_duration'] = mod.bss_rent_duration

            if args.get('bss_rent_penalty') is None:
                args['bss_rent_penalty'] = mod.bss_rent_penalty

            if args.get('bss_return_duration') is None:
                args['bss_return_duration'] = mod.bss_return_duration

            if args.get('bss_return_penalty') is None:
                args['bss_return_penalty'] = mod.bss_return_penalty

            if args.get('_filter_odt_journeys') is None:
                args['_filter_odt_journeys'] = mod.filter_odt_journeys

        # When computing 'same_journey_schedules'(is_journey_schedules=True), some parameters need to be overridden
        # because they are contradictory to the request
        if args.get("is_journey_schedules"):
            # '_final_line_filter' (defined in db) removes journeys with the same lines sequence
            args["_final_line_filter"] = False
            # 'no_shared_section' removes journeys with a section that have the same origin and destination stop points
            args["no_shared_section"] = False

        if args['debug']:
            g.debug = True

        # Add the interpreted parameters to the stats
        self._register_interpreted_parameters(args)

        logger = logging.getLogger(__name__)

        # generate an id that :
        #  - depends on the coverage and differents arguments of the request (from, to, datetime, etc.)
        #  - does not depends on the order of the arguments in the request (only on their values)
        #  - does not depend on the _override_scenario argument
        #  - if  _override_scenario is present, its type is added at the end of the id in
        #    order to identify identical requests made with the different scenarios
        #  - we add the current_datetime of the request so as to differentiate
        #    the same request made at distinct moments
        def generate_request_id():
            if "_override_scenario" in args:
                scenario = str(args["_override_scenario"])
            else:
                scenario = "new_default"

            json_hash = request.id

            now = dt_to_str(datetime.datetime.utcnow())

            result = "journeys_{}_{}#{}#".format(json_hash, now, scenario)

            logger.info("Generating id : {} for request : {}".format(result, request.url))

            return result

        request_id = generate_request_id()
        args["request_id"] = request_id

        # If there are several possible regions to query:
        # copy base request arguments before setting region specific parameters
        if len(possible_regions) > 1:
            base_args = deepcopy(args)

        # Store the different errors
        responses = {}
        for r in possible_regions:
            self.region = r
            if args.get('traveler_type'):
                traveler_profile = TravelerProfile.make_traveler_profile(region, args['traveler_type'])
                traveler_profile.override_params(args)

            # We set default modes for fallback modes.
            # The reason why we cannot put default values in parser_get.add_argument() is that, if we do so,
            # fallback modes will always have a value, and traveler_type will never override fallback modes.
            args['origin_mode'] = args.get('origin_mode') or ['walking']
            args['destination_mode'] = args['destination_mode'] or ['walking']

            _set_specific_params(i_manager.instances[r])
            set_request_timezone(self.region)
            logging.getLogger(__name__).debug("Querying region : {}".format(r))

            # Save the original datetime for debuging purpose
            original_datetime = args['original_datetime']
            if original_datetime:
                new_datetime = self.convert_to_utc(original_datetime)
            args['datetime'] = date_to_timestamp(new_datetime)

            scenario_name = i_manager.get_instance_scenario_name(self.region, args.get('_override_scenario'))

            if scenario_name == "new_default" and (
                "taxi" in args["origin_mode"] or "taxi" in args["destination_mode"]
            ):
                abort(400, message="taxi is not available with new_default scenario")

            response = i_manager.dispatch(args, api, instance_name=self.region)

            # Store the region in the 'g' object, which is local to a request
            if args['debug']:
                instance = i_manager.instances.get(self.region)
                # In debug we store all queried region
                if not hasattr(g, 'regions_called'):
                    g.regions_called = []
                region = {"name": instance.name, "scenario": instance._scenario_name}
                g.regions_called.append(region)

            # If journeys list is empty and error field not exist, we create
            # the error message field
            if not response.journeys and not response.HasField(str('error')):
                logging.getLogger(__name__).debug(
                    "impossible to find journeys for the region {}," " insert error field in response ".format(r)
                )
                response.error.id = response_pb2.Error.no_solution
                response.error.message = "no solution found for this journey"
                response.response_type = response_pb2.NO_SOLUTION

            if response.HasField(str('error')) and len(possible_regions) > 1:

                if args['debug']:
                    # In debug we store all errors
                    if not hasattr(g, 'errors_by_region'):
                        g.errors_by_region = {}
                    g.errors_by_region[r] = response.error

                logging.getLogger(__name__).debug(
                    "impossible to find journeys for the region {},"
                    " we'll try the next possible region ".format(r)
                )
                responses[r] = response
                # Before requesting the next region, reset args before region specific settings
                args = base_args
                continue

            # If every journeys found doesn't use PT, request the next possible region
            non_pt_types = ("non_pt_walk", "non_pt_bike", "non_pt_bss", "car")
            if all(j.type in non_pt_types for j in response.journeys) or all(
                "non_pt" in j.tags for j in response.journeys
            ):
                responses[r] = response
                if len(possible_regions) > 1:
                    # Before requesting the next region, reset args before region specific settings
                    args = base_args
                continue

            if args['equipment_details']:
                # Manage equipments in stop points from the journeys sections
                instance = i_manager.instances.get(self.region)
                return instance.equipment_provider_manager.manage_equipments_for_journeys(response)

            return response

        for response in responses.values():
            if not response.HasField(str("error")):
                return response

        # if no response have been found for all the possible regions, we have a problem
        # if all response had the same error we give it, else we give a generic 'no solution' error
        first_response = list(responses.values())[0]
        if all(r.error.id == first_response.error.id for r in responses.values()):
            return first_response

        resp = response_pb2.Response()
        er = resp.error
        er.id = response_pb2.Error.no_solution
        er.message = "No journey found"

        return resp

    def options(self, **kwargs):
        return self.api_description(**kwargs)
