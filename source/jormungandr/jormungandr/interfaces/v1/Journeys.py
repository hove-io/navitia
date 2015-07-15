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
import logging
from flask import Flask, request, g
from flask.ext.restful import fields, reqparse, marshal_with, abort
from flask.ext.restful.types import boolean
from jormungandr import i_manager
from jormungandr.exceptions import RegionNotFound
from jormungandr.instance_manager import instances_comparator
from jormungandr import authentication
from jormungandr.interfaces.v1.fields import DisruptionsField
from jormungandr.protobuf_to_dict import protobuf_to_dict
from fields import stop_point, stop_area, line, physical_mode, \
    commercial_mode, company, network, pagination, place,\
    PbField, stop_date_time, enum_type, NonNullList, NonNullNested,\
    display_informations_vj, error,\
    SectionGeoJson, Co2Emission, PbEnum, feed_publisher

from jormungandr.interfaces.parsers import option_value, date_time_format
#from exceptions import RegionNotFound
from ResourceUri import ResourceUri, complete_links
import datetime
from functools import wraps
from fields import DateTime
from jormungandr.timezone import set_request_timezone
from make_links import add_id_links, clean_links, create_external_link, create_internal_link
from errors import ManageError
from jormungandr.interfaces.argument import ArgumentDoc
from jormungandr.interfaces.parsers import depth_argument
from operator import itemgetter
from datetime import datetime, timedelta
import sys
from copy import copy
from datetime import datetime
from collections import defaultdict
from navitiacommon import type_pb2, response_pb2
from jormungandr.utils import date_to_timestamp, ResourceUtc
from copy import deepcopy
from jormungandr.travelers_profile import travelers_profile
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.interfaces.v1.Calendars import calendar


f_datetime = "%Y%m%dT%H%M%S"
class SectionLinks(fields.Raw):

    def output(self, key, obj):
        links = None
        try:
            if obj.HasField("uris"):
                links = obj.uris.ListFields()
        except ValueError:
            return None
        response = []
        if links:
            for type_, value in links:
                response.append({"type": type_.name, "id": value})

        if obj.HasField('pt_display_informations'):
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
    "arrival_date_time": DateTime(attribute="end_date_time"),
    "co2_emission": Co2Emission(),
}

cost = {
    'value': fields.Float(),
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
    "disruptions": DisruptionsField,
    "feed_publishers": fields.List(NonNullNested(feed_publisher)),
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
                for region, er in g.errors_by_region.iteritems():
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
            if objects[1] != 200:
                return objects
            if not "journeys" in objects[0].keys():
                return objects
            if "region" in kwargs.keys():
                del kwargs["region"]
            if "uri" in kwargs.keys():
                kwargs["from"] = kwargs["uri"].split("/")[-1]
                del kwargs["uri"]
            if "lon" in kwargs.keys() and "lat" in kwargs.keys():
                if not "from" in kwargs.keys():
                    kwargs["from"] = kwargs["lon"] + ';' + kwargs["lat"]
                del kwargs["lon"]
                del kwargs["lat"]
            for journey in objects[0]['journeys']:
                if not "sections" in journey.keys():
                    kwargs["datetime"] = journey["requested_date_time"]
                    kwargs["to"] = journey["to"]["id"]
                    journey['links'] = [create_external_link("v1.journeys", rel="journeys", **kwargs)]
            return objects
        return wrapper


class add_journey_pagination(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            #self is the first parameter, so the resources
            scenario = g.scenario
            if scenario and hasattr(scenario, 'extremes') and callable(scenario.extremes):
                datetime_before, datetime_after = scenario.extremes(objects[0])
            else:
                datetime_before, datetime_after = self.extremes(objects[0])
            if datetime_before and datetime_after:
                if not "links" in objects[0]:
                    objects[0]["links"] = []

                args = dict(deepcopy(request.args))
                args["datetime"] = datetime_before.strftime(f_datetime)
                args["datetime_represents"] = "arrival"
                if "region" in kwargs:
                    args["region"] = kwargs["region"]
                # Note, it's not the right thing to do, the rel should be 'next' and
                # the type 'journey' but for compatibility reason we cannot change before the v2
                objects[0]["links"].append(create_external_link("v1.journeys",
                                                       rel='prev',
                                                       _type='prev',
                                                       **args))
                args["datetime"] = datetime_after.strftime(f_datetime)
                args["datetime_represents"] = "departure"
                objects[0]["links"].append(create_external_link("v1.journeys",
                                                       rel='next',
                                                       _type='next',
                                                       **args))

            datetime_first, datetime_last = self.first_and_last(objects[0])
            if datetime_first and datetime_last:
                if not "links" in objects[0]:
                    objects[0]["links"] = []

                args = dict(deepcopy(request.args))
                args["datetime"] = datetime_first.strftime(f_datetime)
                args["datetime_represents"] = "departure"
                if "region" in kwargs:
                    args["region"] = kwargs["region"]
                objects[0]["links"].append(create_external_link("v1.journeys",
                                                                rel='first',
                                                                _type='first',
                                                                **args))
                args["datetime"] = datetime_last.strftime(f_datetime)
                args["datetime_represents"] = "arrival"
                objects[0]["links"].append(create_external_link("v1.journeys",
                                                                rel='last',
                                                                _type='last',
                                                                **args))
            return objects
        return wrapper

    def extremes(self, resp):
        datetime_before = None
        datetime_after = None
        if 'journeys' not in resp:
            return (None, None)
        section_is_pt = lambda section: section['type'] == "public_transport"\
                           or section['type'] == "on_demand_transport"
        filter_journey = lambda journey: 'arrival_date_time' in journey and\
                             journey['arrival_date_time'] != '' and\
                             "sections" in journey and\
                             any(section_is_pt(section) for section in journey['sections'])
        list_journeys = filter(filter_journey, resp['journeys'])
        if not list_journeys:
            return (None, None)
        prev_journey = min(list_journeys, key=itemgetter('arrival_date_time'))
        next_journey = max(list_journeys, key=itemgetter('departure_date_time'))
        f_datetime = "%Y%m%dT%H%M%S"
        f_departure = datetime.strptime(next_journey['departure_date_time'], f_datetime)
        f_arrival = datetime.strptime(prev_journey['arrival_date_time'], f_datetime)
        datetime_after = f_departure + timedelta(minutes=1)
        datetime_before = f_arrival - timedelta(minutes=1)

        return (datetime_before, datetime_after)

    def first_and_last(self, resp):
        datetime_first = None
        datetime_last = None
        try:
            list_journeys = [journey for journey in resp['journeys']
                             if 'arrival_date_time' in journey.keys() and
                             journey['arrival_date_time'] != '' and
                             'departure_date_time' in journey.keys() and
                             journey['departure_date_time'] != '']
            asap_min = min(list_journeys,
                           key=itemgetter('departure_date_time'))
            asap_max = max(list_journeys,
                           key=itemgetter('arrival_date_time'))
        except:
            return (None, None)
        if asap_min['departure_date_time'] and asap_max['arrival_date_time']:
            departure = asap_min['departure_date_time']
            departure_date = datetime.strptime(departure, f_datetime)
            midnight = datetime.strptime('0000', '%H%M').time()
            datetime_first = datetime.combine(departure_date, midnight)
            arrival = asap_max['arrival_date_time']
            arrival_date = datetime.strptime(arrival, f_datetime)
            almost_midnight = datetime.strptime('2359', '%H%M').time()
            datetime_last = datetime.combine(arrival_date, almost_midnight)

        return (datetime_first, datetime_last)


#add the link between a section and the ticket needed for that section
class add_fare_links(object):

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            if not "journeys" in objects[0].keys():
                return objects
            ticket_by_section = defaultdict(list)
            if not 'tickets' in objects[0].keys():
                return objects

            for t in objects[0]['tickets']:
                if "links" in t.keys():
                    for s in t['links']:
                        ticket_by_section[s['id']].append(t['id'])

            for j in objects[0]['journeys']:
                if not "sections" in j.keys():
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

    regions = sorted(sorted_regions, cmp=instances_comparator)

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
                                default=["walking"],
                                dest="origin_mode", action="append")
        parser_get.add_argument("last_section_mode[]",
                                type=option_value(modes),
                                default=["walking"],
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

        parser_get.add_argument("walking_speed", type=float)
        parser_get.add_argument("bike_speed", type=float)
        parser_get.add_argument("bss_speed", type=float)
        parser_get.add_argument("car_speed", type=float)
        parser_get.add_argument("forbidden_uris[]", type=str, action="append")
        parser_get.add_argument("count", type=int)
        parser_get.add_argument("min_nb_journeys", type=int)
        parser_get.add_argument("max_nb_journeys", type=int)
        parser_get.add_argument("type", type=option_value(types),
                                default="all")
        parser_get.add_argument("disruption_active",
                                type=boolean, default=False)
# a supprimer
        parser_get.add_argument("max_duration", type=int, default=3600*24)
        parser_get.add_argument("wheelchair", type=boolean, default=False)
        parser_get.add_argument("debug", type=boolean, default=False,
                                hidden=True)
        # for retrocompatibility purpose, we duplicate (without []):
        parser_get.add_argument("first_section_mode",
                                type=option_value(modes), action="append")
        parser_get.add_argument("last_section_mode",
                                type=option_value(modes), action="append")
        parser_get.add_argument("show_codes", type=boolean, default=False,
                            description="show more identification codes")
        parser_get.add_argument("traveler_type", type=option_value(travelers_profile.keys()))
        parser_get.add_argument("_override_scenario", type=str, description="debug param to specify a custom scenario")

        self.method_decorators.append(complete_links(self))

        # manage post protocol (n-m calculation)
        self.parsers["post"] = deepcopy(parser_get)
        parser_post = self.parsers["post"]
        parser_post.add_argument("details", type=boolean, default=False, location="json")
        for index, elem in enumerate(parser_post.args):
            if elem.name in ["from", "to"]:
                parser_post.args[index].type = list
                parser_post.args[index].dest = elem.name
            parser_post.args[index].location = "json"

    @add_debug_info()
    @add_fare_links()
    @add_journey_pagination()
    @add_journey_href()
    @marshal_with(journeys)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        args = self.parsers['get'].parse_args()

        if args['traveler_type']:
            profile = travelers_profile[args['traveler_type']]
            profile.override_params(args)

        if args['max_duration_to_pt'] is not None:
            #retrocompatibility: max_duration_to_pt override all individual value by mode
            args['max_walking_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bike_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bss_duration_to_pt'] = args['max_duration_to_pt']
            args['max_car_duration_to_pt'] = args['max_duration_to_pt']

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

        if not (args['destination'] or args['origin']):
            abort(400, message="you should at least provide either a 'from' or a 'to' argument")

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

        if args['destination'] and args['origin']:
            api = 'journeys'
        else:
            api = 'isochrone'

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

            if response.HasField('error') \
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

        for response in responses.itervalues():
            if not response.HasField("error"):
                return response


        # if no response have been found for all the possible regions, we have a problem
        # if all response had the same error we give it, else we give a generic 'no solution' error
        first_response = responses.itervalues().next()
        if all(r.error.id == first_response.error.id for r in responses.values()):
            return first_response

        resp = response_pb2.Response()
        er = resp.error
        er.id = response_pb2.Error.no_solution
        er.message = "No journey found"

        return resp

    @add_journey_pagination()
    @add_journey_href()
    @marshal_with(journeys)
    @ManageError()
    def post(self, region=None, lon=None, lat=None, uri=None):
        args = self.parsers['post'].parse_args()
        if args['traveler_type']:
            profile = travelers_profile[args['traveler_type']]
            profile.override_params(args)

        #check that we have at least one departure and one arrival
        if len(args['from']) == 0:
            abort(400, message="from argument must contain at least one item")
        if len(args['to']) == 0:
            abort(400, message="to argument must contain at least one item")

        # TODO : Changer le protobuff pour que ce soit propre
        if args['destination_mode'] == 'vls':
            args['destination_mode'] = 'bss'
        if args['origin_mode'] == 'vls':
            args['origin_mode'] = 'bss'

        if args['max_duration_to_pt']:
            #retrocompatibility: max_duration_to_pt override all individual value by mode
            args['max_walking_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bike_duration_to_pt'] = args['max_duration_to_pt']
            args['max_bss_duration_to_pt'] = args['max_duration_to_pt']
            args['max_car_duration_to_pt'] = args['max_duration_to_pt']

        #count override min_nb_journey or max_nb_journey
        if 'count' in args and args['count']:
            args['min_nb_journeys'] = args['count']
            args['max_nb_journeys'] = args['count']

        if region:
            self.region = i_manager.get_region(region)
            set_request_timezone(self.region)

        if not region:
            #TODO how to handle lon/lat ? don't we have to override args['origin'] ?
            self.region = compute_regions(args)

        #store json data into 4 arrays
        args['origin'] = []
        args['origin_access_duration'] = []
        args['destination'] = []
        args['destination_access_duration'] = []
        for loop in [('from', 'origin', True), ('to', 'destination', False)]:
            for location in args[loop[0]]:
                if "access_duration" in location:
                    args[loop[1]+'_access_duration'].append(location["access_duration"])
                else:
                    args[loop[1]+'_access_duration'].append(0)
                stop_uri = location["uri"]
                stop_uri = transform_id(stop_uri)
                args[loop[1]].append(stop_uri)

        #default Date
        if not "datetime" in args or not args['datetime']:
            args['datetime'] = datetime.now()
            args['datetime'] = args['datetime'].replace(hour=13, minute=37)

        # we save the original datetime for debuging purpose
        args['original_datetime'] = args['datetime']
        original_datetime = args['original_datetime']

        #we add the interpreted parameters to the stats
        self._register_interpreted_parameters(args)

        new_datetime = self.convert_to_utc(original_datetime)
        args['datetime'] = date_to_timestamp(new_datetime)

        api = 'nm_journeys'

        response = i_manager.dispatch(args, api, instance_name=self.region)
        return response
