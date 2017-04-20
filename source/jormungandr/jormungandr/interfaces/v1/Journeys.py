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
import logging
from flask import request, g
from flask.ext.restful import fields, marshal_with, abort
from flask.ext.restful.inputs import boolean
from jormungandr import i_manager, app
from jormungandr.interfaces.v1.fields import disruption_marshaller, Links
from jormungandr.interfaces.v1.fields import display_informations_vj, error, place,\
    PbField, stop_date_time, enum_type, NonNullList, NonNullNested,\
    SectionGeoJson, PbEnum, feed_publisher, Durations

from jormungandr.interfaces.parsers import default_count_arg_type
from jormungandr.interfaces.v1.ResourceUri import complete_links
from functools import wraps
from jormungandr.interfaces.v1.fields import DateTime, Integer
from jormungandr.timezone import set_request_timezone
from jormungandr.interfaces.v1.make_links import create_external_link, create_internal_link
from jormungandr.interfaces.v1.errors import ManageError
from collections import defaultdict
from navitiacommon import response_pb2
from jormungandr.utils import date_to_timestamp
from jormungandr.interfaces.v1.Calendars import calendar
from navitiacommon import default_values
from jormungandr.interfaces.v1.journey_common import JourneyCommon, compute_possible_region
from jormungandr.parking_space_availability.bss.stands_manager import ManageStands


f_datetime = "%Y%m%dT%H%M%S"
class SectionLinks(fields.Raw):

    def output(self, key, obj):
        links = None
        try:
            if obj.HasField(str("uris")):
                links = obj.uris.ListFields()
        except ValueError:
            return None
        response = []
        if links:
            for type_, value in links:
                response.append({"type": type_.name, "id": value})

        if obj.HasField(str('pt_display_informations')):
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
    "duration": Integer(),
    "from": section_place(place, attribute="origin"),
    "to": section_place(place, attribute="destination"),
    "links": SectionLinks(attribute="uris"),
    "display_informations": PbField(display_informations_vj,
                                    attribute='pt_display_informations'),
    "additional_informations": NonNullList(PbEnum(response_pb2.SectionAdditionalInformationType)),
    "geojson": SectionGeoJson(),
    "path": NonNullList(NonNullNested({"length": Integer(),
                                       "name": fields.String(),
                                       "duration": Integer(),
                                       "direction": fields.Integer()}),
                        attribute="street_network.path_items"),
    "transfer_type": enum_type(),
    "stop_date_times": NonNullList(NonNullNested(stop_date_time)),
    "departure_date_time": DateTime(attribute="begin_date_time"),
    "base_departure_date_time": DateTime(attribute="base_begin_date_time"),
    "arrival_date_time": DateTime(attribute="end_date_time"),
    "base_arrival_date_time": DateTime(attribute="base_end_date_time"),
    "co2_emission": NonNullNested({
        'value': fields.Raw,
        'unit': fields.String
        }),
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
    "co2_emission": NonNullNested({
        'value': fields.Raw,
        'unit': fields.String
        }),
    "durations": Durations(),
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

context = {
    'car_direct_path': {
        'co2_emission': NonNullNested({
            'value': fields.Raw,
            'unit': fields.String
        }, attribute="car_co2_emission")
    }
}

journeys = {
    "journeys": NonNullList(NonNullNested(journey)),
    "error": PbField(error, attribute='error'),
    "tickets": fields.List(NonNullNested(ticket)),
    "disruptions": fields.List(NonNullNested(disruption_marshaller), attribute="impacts"),
    "feed_publishers": fields.List(NonNullNested(feed_publisher)),
    "links": fields.List(Links()),
    "context": context,
}


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
                args = dict(request.args)
                ids = {o['stop_point']['stop_area']['id']
                       for s in journey.get('sections', []) if 'from' in s
                       for o in (s['from'], s['to']) if 'stop_point' in o}
                if 'region' in kwargs:
                    args['region'] = kwargs['region']
                if "sections" not in journey:#this mean it's an isochrone...
                    if 'to' not in args:
                        args['to'] = journey['to']['id']
                    if 'from' not in args:
                        args['from'] = journey['from']['id']
                    journey['links'] = [create_external_link('v1.journeys', rel='journeys', **args)]
                elif ids and 'public_transport' in (s['type'] for s in journey['sections']):
                    args['min_nb_journeys'] = 5
                    ids.update(args.get('allowed_id[]', []))
                    args['allowed_id[]'] = list(ids)
                    journey['links'] = [
                        create_external_link('v1.journeys', rel='same_journey_schedules', _type='journeys', **args)
                    ]
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


class rig_journey(object):
    """
    decorator to rig journeys in order to put back the requested origin/destination in the journeys
    those origin/destination can be changed internaly by some scenarios (querying external autocomple service)
    """
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            response, status, _ = objects
            if status != 200:
                return objects

            if not hasattr(g, 'origin_detail') or not hasattr(g, 'destination_detail'):
                return objects

            for j in response.get('journeys', []):
                if not 'sections' in j:
                    continue
                logging.debug('for journey changing origin: {old_o} to {new_o}'
                              ', destination to {old_d} to {new_d}'
                              .format(old_o=j.get('sections', [{}])[0].get('from').get('id'),
                                      new_o=(g.origin_detail or {}).get('id'),
                                      old_d=j.get('sections', [{}])[-1].get('to').get('id'),
                                      new_d=(g.destination_detail or {}).get('id')))
                if g.origin_detail:
                    j['sections'][0]['from'] = g.origin_detail
                if g.destination_detail:
                    j['sections'][-1]['to'] = g.destination_detail

            return objects
        return wrapper


class Journeys(JourneyCommon):

    def __init__(self):
        # journeys must have a custom authentication process
        super(Journeys, self).__init__()

        parser_get = self.parsers["get"]

        parser_get.add_argument("count", type=default_count_arg_type)
        parser_get.add_argument("_min_journeys_calls", type=int)
        parser_get.add_argument("_final_line_filter", type=boolean)
        parser_get.add_argument("min_nb_journeys", type=int)
        parser_get.add_argument("max_nb_journeys", type=int)
        parser_get.add_argument("_max_extra_second_pass", type=int, dest="max_extra_second_pass")

        parser_get.add_argument("debug", type=boolean, default=False,
                                hidden=True)
        parser_get.add_argument("show_codes", type=boolean, default=False,
                            description="show more identification codes")
        parser_get.add_argument("_override_scenario", type=unicode, description="debug param to specify a custom scenario")

        parser_get.add_argument("_walking_transfer_penalty", type=int)
        parser_get.add_argument("_max_successive_physical_mode", type=int)
        parser_get.add_argument("_max_additional_connections", type=int)
        parser_get.add_argument("_night_bus_filter_base_factor", type=int)
        parser_get.add_argument("_night_bus_filter_max_factor", type=float)
        parser_get.add_argument("_min_car", type=int)
        parser_get.add_argument("_min_bike", type=int)
        parser_get.add_argument("bss_stands", type=boolean, default=False, description="Show bss stands availability")

        self.method_decorators.append(complete_links(self))

        if parser_get.parse_args().get("bss_stands"):
            self.method_decorators.insert(1, ManageStands(self, 'journeys'))

    @add_debug_info()
    @add_fare_links()
    @add_journey_href()
    @rig_journey()
    @marshal_with(journeys)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        args = self.parsers['get'].parse_args()
        possible_regions = compute_possible_region(region, args)
        args.update(self.parse_args(region, uri))


        #count override min_nb_journey or max_nb_journey
        if 'count' in args and args['count']:
            args['min_nb_journeys'] = args['count']
            args['max_nb_journeys'] = args['count']

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
            if args.get('_max_additional_connections') is None:
                args['_max_additional_connections'] = mod.max_additional_connections

        if region:
            _set_specific_params(i_manager.instances[region])
        else:
            _set_specific_params(default_values)

        if not (args['destination'] or args['origin']):
            abort(400, message="you should at least provide either a 'from' or a 'to' argument")

        if args['debug']:
            g.debug = True

        #we add the interpreted parameters to the stats
        self._register_interpreted_parameters(args)
        logging.getLogger(__name__).debug("We are about to ask journeys on regions : {}".format(possible_regions))

        #we want to store the different errors
        responses = {}
        for r in possible_regions:
            self.region = r

            set_request_timezone(self.region)

            #we store the region in the 'g' object, which is local to a request
            if args['debug']:
                # In debug we store all queried region
                if not hasattr(g, 'regions_called'):
                    g.regions_called = []
                g.regions_called.append(r)

            # we save the original datetime for debuging purpose
            original_datetime = args['original_datetime']
            if original_datetime:
                new_datetime = self.convert_to_utc(original_datetime)
            else:
                new_datetime = args['_current_datetime']
            args['datetime'] = date_to_timestamp(new_datetime)

            response = i_manager.dispatch(args, api, instance_name=self.region)

            if response.HasField(str('error')) \
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

            non_pt_types = ("non_pt_walk", "non_pt_bike", "non_pt_bss", "car")
            if all(j.type in non_pt_types for j in response.journeys) or \
               all("non_pt" in j.tags for j in response.journeys):
                responses[r] = response
                continue

            return response

        for response in responses.values():
            if not response.HasField(str("error")):
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
