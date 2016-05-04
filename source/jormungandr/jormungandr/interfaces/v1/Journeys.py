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
from functools import cmp_to_key
import logging
from flask import request, g
from flask.ext.restful import fields, reqparse, marshal_with, abort
from flask.ext.restful.inputs import boolean
from jormungandr import i_manager, app
from jormungandr.exceptions import RegionNotFound
from jormungandr.instance_manager import instances_comparator
from jormungandr.interfaces.v1.fields import disruption_marshaller, Links
from jormungandr.interfaces.v1.fields import display_informations_vj, error, place,\
    PbField, stop_date_time, enum_type, NonNullList, NonNullNested,\
    SectionGeoJson, Co2Emission, PbEnum, feed_publisher

from jormungandr.interfaces.parsers import option_value, date_time_format, default_count_arg_type, date_time_format
from jormungandr.interfaces.v1.ResourceUri import ResourceUri, complete_links
from functools import wraps
from jormungandr.interfaces.v1.fields import DateTime
from jormungandr.timezone import set_request_timezone
from jormungandr.interfaces.v1.make_links import create_external_link, create_internal_link
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument, float_gt_0
from operator import itemgetter
from datetime import datetime, timedelta
from collections import defaultdict
from navitiacommon import type_pb2, response_pb2
from jormungandr.utils import date_to_timestamp
from jormungandr.resources_utc import ResourceUtc
from copy import deepcopy
from jormungandr.travelers_profile import TravelerProfile
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.interfaces.v1.Calendars import calendar
from navitiacommon.default_traveler_profile_params import acceptable_traveler_types
from navitiacommon import default_values

f_datetime = "%Y%m%dT%H%M%S"
class SectionLinks(fields.Raw):

    def output(self, key, obj):
        links = None
        try:
            if obj.HasField(b"uris"):
                links = obj.uris.ListFields()
        except ValueError:
            return None
        response = []
        if links:
            for type_, value in links:
                response.append({"type": type_.name, "id": value})

        if obj.HasField(b'pt_display_informations'):
            for value in obj.pt_display_informations.notes:
                response.append({"type": 'notes', "id": value.uri, 'value': value.note})
        return response


class FareLinks(fields.Raw):

    def output(self, key, obj):
        ticket_ids = []
        try:
            for t_id in obj.ticket_id:
                ticket_ids.append(t_id)
        except ValueError:
            return None
        response = []
        for value in ticket_ids:
            response.append(create_internal_link(_type="ticket", rel="tickets",
                                                 id=value))
        return response


class TicketLinks(fields.Raw):

    def output(self, key, obj):
        section_ids = []
        try:
            for s_id in obj.section_id:
                section_ids.append(s_id)
        except ValueError:
            return None
        response = []
        for value in section_ids:
            response.append({"type": "section", "rel": "sections",
                             "internal": True, "templated": False,
                             "id": value})
        return response


class section_type(enum_type):

    def if_on_demand_stop_time(self, stop):
        properties = stop.properties
        descriptor = properties.DESCRIPTOR
        enum = descriptor.enum_types_by_name["AdditionalInformation"]
        for v in properties.additional_informations:
            if enum.values_by_number[v].name == 'on_demand_transport':
                return True
        return False

    def output(self, key, obj):
        try:
            if obj.stop_date_times:
                first_stop = obj.stop_date_times[0]
                last_stop = obj.stop_date_times[-1]
                if self.if_on_demand_stop_time(first_stop):
                    return 'on_demand_transport'
                elif self.if_on_demand_stop_time(last_stop):
                    return 'on_demand_transport'
                return 'public_transport'
        except ValueError:
            pass
        return super(section_type, self).output("type", obj)


class section_place(PbField):

    def output(self, key, obj):
        enum_t = obj.DESCRIPTOR.fields_by_name['type'].enum_type.values_by_name
        if obj.type == enum_t['WAITING'].number:
            return None
        else:
            return super(PbField, self).output(key, obj)


class JourneyDebugInfo(fields.Raw):
    def output(self, key, obj):
        if not hasattr(g, 'debug') or not g.debug:
            return None

        debug = {
            'streetnetwork_duration': obj.sn_dur,
            'transfer_duration': obj.transfer_dur,
            'min_waiting_duration': obj.min_waiting_dur,
            'nb_vj_extentions': obj.nb_vj_extentions,
            'nb_sections': obj.nb_sections,
        }
        if hasattr(obj, 'internal_id'):
            debug['internal_id'] = obj.internal_id

        return debug


section = {
    "type": section_type(),
    "id": fields.String(),
    "mode": enum_type(attribute="street_network.mode"),
    "duration": fields.Integer(),
    "from": section_place(place, attribute="origin"),
    "to": section_place(place, attribute="destination"),
    "links": SectionLinks(attribute="uris"),
    "display_informations": PbField(display_informations_vj,
                                    attribute='pt_display_informations'),
    "additional_informations": NonNullList(PbEnum(response_pb2.SectionAdditionalInformationType)),
    "geojson": SectionGeoJson(),
    "path": NonNullList(NonNullNested({"length": fields.Integer(),
                                       "name": fields.String(),
                                       "duration": fields.Integer(),
                                       "direction": fields.Integer()}),
                        attribute="street_network.path_items"),
    "transfer_type": enum_type(),
    "stop_date_times": NonNullList(NonNullNested(stop_date_time)),
    "departure_date_time": DateTime(attribute="begin_date_time"),
    "base_departure_date_time": DateTime(attribute="base_begin_date_time"),
    "arrival_date_time": DateTime(attribute="end_date_time"),
    "base_arrival_date_time": DateTime(attribute="base_end_date_time"),
    "co2_emission": Co2Emission(),
}

cost = {
    'value': fields.String(),
    'currency': fields.String(),
}

fare = {
    'total': NonNullNested(cost),
    'found': fields.Boolean(),
    'links': FareLinks(attribute="ticket_id")
}

journey = {
    'duration': fields.Integer(),
    'nb_transfers': fields.Integer(),
    'departure_date_time': DateTime(),
    'arrival_date_time': DateTime(),
    'requested_date_time': DateTime(),
    'sections': NonNullList(NonNullNested(section)),
    'from': PbField(place, attribute='origin'),
    'to': PbField(place, attribute='destination'),
    'type': fields.String(),
    'fare': NonNullNested(fare),
    'tags': fields.List(fields.String),
    "status": fields.String(attribute="most_serious_disruption_effect"),
    "calendars": NonNullList(NonNullNested(calendar)),
    "co2_emission": Co2Emission(),
    "debug": JourneyDebugInfo()
}

ticket = {
    "id": fields.String(),
    "name": fields.String(),
    "comment": fields.String(),
    "found": fields.Boolean(),
    "cost": NonNullNested(cost),
    "links": TicketLinks(attribute="section_id")
}


journeys = {
    "journeys": NonNullList(NonNullNested(journey)),
    "error": PbField(error, attribute='error'),
    "tickets": fields.List(NonNullNested(ticket)),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
    "feed_publishers": fields.List(NonNullNested(feed_publisher)),
    "links": fields.List(Links()),
}


def dt_represents(value):
    if value == "arrival":
        return False
    elif value == "departure":
        return True
    else:
        raise ValueError("Unable to parse datetime_represents")



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
            if objects[1] != 200 or 'journeys' not in objects[0]:
                return objects
            for journey in objects[0]['journeys']:
                if "sections" not in journey:#this mean it's an isochrone...
                    args = dict(request.args)
                    if 'to' not in args:
                        args['to'] = journey['to']['id']
                    if 'from' not in args:
                        args['from'] = journey['from']['id']
                    if 'region' in kwargs:
                        args['region'] = kwargs['region']
                    journey['links'] = [create_external_link('v1.journeys', rel='journeys', **args)]
            return objects
        return wrapper


#add the link between a section and the ticket needed for that section
class add_fare_links(object):

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            if "journeys" not in objects[0]:
                return objects
            ticket_by_section = defaultdict(list)
            if 'tickets' not in objects[0]:
                return objects

            for t in objects[0]['tickets']:
                if "links" in t:
                    for s in t['links']:
                        ticket_by_section[s['id']].append(t['id'])

            for j in objects[0]['journeys']:
                if "sections" not in j:
                    continue
                for s in j['sections']:

                    #them we add the link to the different tickets needed
                    for ticket_needed in ticket_by_section[s["id"]]:
                        s['links'].append(create_internal_link(_type="ticket",
                                                               rel="tickets",
                                                               id=ticket_needed))

            return objects
        return wrapper


def compute_regions(args):
    """
    method computing the region the journey has to be computed on
    The complexity comes from the fact that the regions in jormungandr can overlap.

    return the kraken instance key

    rules are easy:
    we fetch the different regions the user can use for 'origin' and 'destination'
    we do the intersection and sort the list
    """
    _region = None
    possible_regions = set()
    from_regions = set()
    to_regions = set()
    if args['origin']:
        from_regions = set(i_manager.get_regions(object_id=args['origin']))
        #Note: if get_regions does not find any region, it raises a RegionNotFoundException

    if args['destination']:
        to_regions = set(i_manager.get_regions(object_id=args['destination']))

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

    return regions

class Journeys(ResourceUri, ResourceUtc):

    def __init__(self):
        # journeys must have a custom authentication process
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
        parser_get.add_argument("count", type=default_count_arg_type)
        parser_get.add_argument("_min_journeys_calls", type=int)
        parser_get.add_argument("_final_line_filter", type=boolean)
        parser_get.add_argument("min_nb_journeys", type=int)
        parser_get.add_argument("max_nb_journeys", type=int)
        parser_get.add_argument("_max_extra_second_pass", type=int, dest="max_extra_second_pass")
        parser_get.add_argument("type", type=option_value(types),
                                default="all")
        parser_get.add_argument("disruption_active", type=boolean, default=False)  # for retrocomp
        # no default value for data_freshness because we need to maintain retrocomp with disruption_active
        parser_get.add_argument("data_freshness",
                                type=option_value(['base_schedule', 'adapted_schedule', 'realtime']))

        parser_get.add_argument("max_duration", type=int)
        parser_get.add_argument("wheelchair", type=boolean, default=None)
        parser_get.add_argument("debug", type=boolean, default=False,
                                hidden=True)
        # for retrocompatibility purpose, we duplicate (without []):
        parser_get.add_argument("first_section_mode",
                                type=option_value(modes), action="append")
        parser_get.add_argument("last_section_mode",
                                type=option_value(modes), action="append")
        parser_get.add_argument("show_codes", type=boolean, default=False,
                            description="show more identification codes")
        parser_get.add_argument("traveler_type", type=option_value(acceptable_traveler_types))
        parser_get.add_argument("_override_scenario", type=unicode, description="debug param to specify a custom scenario")

        parser_get.add_argument("_walking_transfer_penalty", type=int)
        parser_get.add_argument("_night_bus_filter_base_factor", type=int)
        parser_get.add_argument("_night_bus_filter_max_factor", type=float)
        parser_get.add_argument("_min_car", type=int)
        parser_get.add_argument("_min_bike", type=int)
        parser_get.add_argument("_current_datetime", type=date_time_format, default=datetime.utcnow(),
                                description="The datetime used to consider the state of the pt object"
                                            " Default is the current date and it is used for debug."
                                            " Note: it will mainly change the disruptions that concern "
                                            "the object The timezone should be specified in the format,"
                                            " else we consider it as UTC")


        self.method_decorators.append(complete_links(self))


    @add_debug_info()
    @add_fare_links()
    @add_journey_href()
    @marshal_with(journeys)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        args = self.parsers['get'].parse_args()

        if args.get('traveler_type') is not None:
            traveler_profile = TravelerProfile.make_traveler_profile(region, args['traveler_type'])
            traveler_profile.override_params(args)

        # We set default modes for fallback modes.
        # The reason why we cannot put default values in parser_get.add_argument() is that, if we do so,
        # fallback modes will always have a value, and traveler_type will never override fallback modes.
        if args.get('origin_mode') is None:
            args['origin_mode'] = ['walking']
        if args.get('destination_mode') is None:
            args['destination_mode'] = ['walking']

        if args['max_duration_to_pt'] is not None:
            #retrocompatibility: max_duration_to_pt override all individual value by mode
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

        #count override min_nb_journey or max_nb_journey
        if 'count' in args and args['count']:
            args['min_nb_journeys'] = args['count']
            args['max_nb_journeys'] = args['count']

        # for last and first section mode retrocompatibility
        if 'first_section_mode' in args and args['first_section_mode']:
            args['origin_mode'] = args['first_section_mode']
        if 'last_section_mode' in args and args['last_section_mode']:
            args['destination_mode'] = args['last_section_mode']

        if region:
            self.region = i_manager.get_region(region)
            if uri:
                objects = uri.split('/')
                if objects and len(objects) % 2 == 0:
                    args['origin'] = objects[-1]
                else:
                    abort(503, message="Unable to compute journeys "
                                       "from this object")

        if args['destination'] and args['origin']:
            api = 'journeys'
        else:
            api = 'isochrone'

        if api == 'isochrone':
            # we have custom default values for isochrone because they are very resource expensive
            if args.get('max_duration') is None:
                args['max_duration'] = app.config['ISOCHRONE_DEFAULT_VALUE']

        def _set_specific_params(mod):
            if args.get('max_duration') is None:
                args['max_duration'] = mod.max_duration
            if args.get('_walking_transfer_penalty') is None:
                args['_walking_transfer_penalty'] = mod.walking_transfer_penalty
            if args.get('_night_bus_filter_base_factor') is None:
                args['_night_bus_filter_base_factor'] = mod.night_bus_filter_base_factor
            if args.get('_night_bus_filter_max_factor') is None:
                args['_night_bus_filter_max_factor'] = mod.night_bus_filter_max_factor

        if region:
            _set_specific_params(i_manager.instances[region])
        else:
            _set_specific_params(default_values)

        if not (args['destination'] or args['origin']):
            abort(400, message="you should at least provide either a 'from' or a 'to' argument")

        if args['debug']:
            g.debug = True

        #we transform the origin/destination url to add information
        if args['origin']:
            args['origin'] = transform_id(args['origin'])
        if args['destination']:
            args['destination'] = transform_id(args['destination'])

        if not args['datetime']:
            args['datetime'] = datetime.now()
            args['datetime'] = args['datetime'].replace(hour=13, minute=37)

        if not region:
            #TODO how to handle lon/lat ? don't we have to override args['origin'] ?
            possible_regions = compute_regions(args)
        else:
            possible_regions = [region]

        # we save the original datetime for debuging purpose
        args['original_datetime'] = args['datetime']
        #we add the interpreted parameters to the stats
        self._register_interpreted_parameters(args)

        logging.getLogger(__name__).debug("We are about to ask journeys on regions : {}" .format(possible_regions))
        #we want to store the different errors
        responses = {}
        for r in possible_regions:
            self.region = r

            #we store the region in the 'g' object, which is local to a request
            set_request_timezone(self.region)

            if args['debug']:
                # In debug we store all queried region
                if not hasattr(g, 'regions_called'):
                    g.regions_called = []
                g.regions_called.append(r)

            original_datetime = args['original_datetime']
            new_datetime = self.convert_to_utc(original_datetime)
            args['datetime'] = date_to_timestamp(new_datetime)

            response = i_manager.dispatch(args, api, instance_name=self.region)

            if response.HasField(b'error') \
                    and len(possible_regions) != 1:
                logging.getLogger(__name__).debug("impossible to find journeys for the region {},"
                                                 " we'll try the next possible region ".format(r))

                if args['debug']:
                    # In debug we store all errors
                    if not hasattr(g, 'errors_by_region'):
                        g.errors_by_region = {}
                    g.errors_by_region[r] = response.error

                responses[r] = response
                continue

            if all(map(lambda j: j.type in ("non_pt_walk", "non_pt_bike", "non_pt_bss", "car"), response.journeys)):
                responses[r] = response
                continue

            return response

        for response in responses.values():
            if not response.HasField(b"error"):
                return response


        # if no response have been found for all the possible regions, we have a problem
        # if all response had the same error we give it, else we give a generic 'no solution' error
        first_response = responses.values()[0]
        if all(r.error.id == first_response.error.id for r in responses.values()):
            return first_response

        resp = response_pb2.Response()
        er = resp.error
        er.id = response_pb2.Error.no_solution
        er.message = "No journey found"

        return resp
