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

from flask import Flask, request, url_for
from flask.ext.restful import fields, reqparse, marshal_with, abort
from flask.ext.restful.types import boolean
from jormungandr import i_manager
from jormungandr.exceptions import RegionNotFound
from jormungandr.instance_manager import choose_best_instance
from jormungandr import authentification
from jormungandr.protobuf_to_dict import protobuf_to_dict
from fields import stop_point, stop_area, line, physical_mode, \
    commercial_mode, company, network, pagination, place,\
    PbField, stop_date_time, enum_type, NonNullList, NonNullNested,\
    display_informations_vj, additional_informations_vj, error,\
    generic_message

from jormungandr.interfaces.parsers import option_value
#from exceptions import RegionNotFound
from ResourceUri import ResourceUri, complete_links, update_journeys_status
from functools import wraps
from fields import DateTime
from make_links import add_id_links, clean_links
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
            response.append({"type": "ticket", "rel": "tickets",
                             "internal": True, "templated": False,
                             "id": value})
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


class GeoJson(fields.Raw):

    def __init__(self, **kwargs):
        super(GeoJson, self).__init__(**kwargs)

    def output(self, key, obj):
        coords = []
        if obj.type == response_pb2.STREET_NETWORK:
            try:
                if obj.HasField("street_network"):
                    coords = obj.street_network.coordinates
                else:
                    return None
            except ValueError:
                return None
        elif obj.type == response_pb2.PUBLIC_TRANSPORT:
            coords = [sdt.stop_point.coord for sdt in obj.stop_date_times]
        elif obj.type == response_pb2.TRANSFER:
            coords.append(obj.origin.stop_point.coord)
            coords.append(obj.destination.stop_point.coord)
        elif obj.type == response_pb2.CROW_FLY:
            for place in [obj.origin, obj.destination]:
                type_ = place.embedded_type
                if type_ == type_pb2.STOP_POINT:
                    coords.append(place.stop_point.coord)
                elif type_ == type_pb2.STOP_AREA:
                    coords.append(place.stop_area.coord)
                elif type_ == type_pb2.POI:
                    coords.append(place.poi.coord)
                elif type_ == type_pb2.ADDRESS:
                    coords.append(place.address.coord)
                elif type_ == type_pb2.ADMINISTRATIVE_REGION:
                    coords.append(place.administrative_region.coord)
        else:
            return None

        response = {
            "type": "LineString",
            "coordinates": [],
            "properties": [{
                "length": 0 if not obj.HasField("length") else obj.length
            }]
        }
        for coord in coords:
            response["coordinates"].append([coord.lon, coord.lat])
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
    "additional_informations": additional_informations_vj(),
    "geojson": GeoJson(),
    "path": NonNullList(NonNullNested({"length": fields.Integer(),
                                       "name": fields.String(),
                                       "duration": fields.Integer(),
                                       "direction": fields.Integer()}),
                        attribute="street_network.path_items"),
    "transfer_type": enum_type(),
    "stop_date_times": NonNullList(NonNullNested(stop_date_time)),
    "departure_date_time": DateTime(attribute="begin_date_time",
                                    timezone="origin.stop_area.timezone"),
    "arrival_date_time": DateTime(attribute="end_date_time",
                                  timezone="destination.stop_area.timezone"),
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
    'departure_date_time': DateTime(timezone='origin.stop_area.timezone'),
    'arrival_date_time': DateTime(timezone='destination.stop_area.timezone'),
    'requested_date_time': fields.String(), #TODO datetime ?
    'sections': NonNullList(NonNullNested(section)),
    'from': PbField(place, attribute='origin'),
    'to': PbField(place, attribute='destination'),
    'type': fields.String(),
    'fare': NonNullNested(fare),
    'tags': fields.List(fields.String),
    "status": enum_type(attribute="message_status")
}

ticket = {
    "id": fields.String(),
    "name": fields.String(),
    "found": fields.Boolean(),
    "cost": NonNullNested(cost),
    "links": TicketLinks(attribute="section_id")
}
journeys = {
    "journeys": NonNullList(NonNullNested(journey)),
    "error": PbField(error, attribute='error'),
    "tickets": NonNullList(NonNullNested(ticket))
}


def dt_represents(value):
    if value == "arrival":
        return False
    elif value == "departure":
        return True
    else:
        raise ValueError("Unable to parse datetime_represents")


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
            kwargs["_external"] = True
            for journey in objects[0]['journeys']:
                if not "sections" in journey.keys():
                    kwargs["datetime"] = journey["requested_date_time"]
                    kwargs["to"] = journey["to"]["id"]
                    journey['links'] = [
                        {
                            "type": "journeys",
                            "href": url_for("v1.journeys", **kwargs),
                            "templated": False
                        }
                    ]
            return objects
        return wrapper


class add_journey_pagination(object):
    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            if objects[1] != 200:
                return objects
            datetime_before, datetime_after = self.extremes(objects[0])
            if not datetime_before is None and not datetime_after is None:
                if not "links" in objects[0]:
                    objects[0]["links"] = []

                args = dict()
                for item in request.args.iteritems():
                    args[item[0]] = item[1]
                args["datetime"] = datetime_before.strftime(f_datetime)
                args["datetime_represents"] = "arrival"
                if "region" in kwargs:
                    args["region"] = kwargs["region"]
                objects[0]["links"].append({
                    "href": url_for("v1.journeys", _external=True, **args),
                    "templated": False,
                    "type": "prev"
                })
                args["datetime"] = datetime_after.strftime(f_datetime)
                args["datetime_represents"] = "departure"
                objects[0]["links"].append({
                    "href": url_for("v1.journeys", _external=True, **args),
                    "templated": False,
                    "type": "next"
                })

            datetime_first, datetime_last = self.first_and_last(objects[0])
            if not datetime_first is None and not datetime_last is None:
                if not "links" in objects[0]:
                    objects[0]["links"] = []

                args = dict()
                for item in request.args.iteritems():
                    args[item[0]] = item[1]
                args["datetime"] = datetime_first.strftime(f_datetime)
                args["datetime_represents"] = "departure"
                if "region" in kwargs:
                    args["region"] = kwargs["region"]
                objects[0]["links"].append({
                    "href": url_for("v1.journeys", _external=True, **args),
                    "templated": False,
                    "type": "first"
                })
                args["datetime"] = datetime_last.strftime(f_datetime)
                args["datetime_represents"] = "arrival"
                objects[0]["links"].append({
                    "href": url_for("v1.journeys", _external=True, **args),
                    "templated": False,
                    "type": "last"
                })
            return objects
        return wrapper

    def extremes(self, resp):
        datetime_before = None
        datetime_after = None
        section_is_pt = lambda section: section['type'] == "public_transport"\
                           or section['type'] == "on_demand_transport"
        filter_journey = lambda journey: 'arrival_date_time' in journey and\
                             journey['arrival_date_time'] != '' and\
                             "sections" in journey and\
                             any(section_is_pt(section) for section in journey['sections'])
        try:
            list_journeys = filter(filter_journey, resp['journeys'])
            asap_journey = min(list_journeys,
                               key=itemgetter('arrival_date_time'))
        except:
            return (None, None)
        if asap_journey['arrival_date_time'] \
                and asap_journey['departure_date_time']:
            s_departure = asap_journey['departure_date_time']
            f_departure = datetime.strptime(s_departure, f_datetime)
            s_arrival = asap_journey['arrival_date_time']
            f_arrival = datetime.strptime(s_arrival, f_datetime)
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
                        s['links'].append({"type": "ticket",
                                           "rel": "tickets",
                                           "internal": True,
                                           "templated": False,
                                           "id": ticket_needed})

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
        from_regions = set(i_manager.key_of_id(args['origin'], only_one=False))
        #Note: if the key_of_id does not find any region, it raises a RegionNotFoundException

    if args['destination']:
        to_regions = set(i_manager.key_of_id(args['destination'], only_one=False))

    if not from_regions:
        #we didn't get any origin, the region is in the destination's list
        possible_regions = to_regions
    elif not to_regions:
        #we didn't get any origin, the region is in the destination's list
        possible_regions = from_regions
    else:
        #we need the intersection set
        possible_regions = from_regions.intersection(to_regions)

    logging.debug("orig region = {o}, dest region = {d} => set = {p}".
                 format(o=from_regions, d=to_regions, p=possible_regions))

    if not possible_regions:
        raise RegionNotFound(custom_msg="cannot find a region with {o} and {d} in the same time"
                             .format(o=args['origin'], d=args['destination']))

    sorted_regions = list(possible_regions)

    _region = choose_best_instance(sorted_regions)

    return _region


class Journeys(ResourceUri):

    def __init__(self):
        # journeys must have a custom authentication process
        ResourceUri.__init__(self, authentication=False)
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
        parser_get.add_argument("from", type=str, dest="origin")
        parser_get.add_argument("to", type=str, dest="destination")
        parser_get.add_argument("datetime", type=str)
        parser_get.add_argument("datetime_represents", dest="clockwise",
                                type=dt_represents, default=True)
        parser_get.add_argument("max_nb_transfers", type=int, default=10,
                                dest="max_transfers")
        parser_get.add_argument("first_section_mode[]",
                                type=option_value(modes),
                                default=["walking"],
                                dest="origin_mode", action="append")
        parser_get.add_argument("last_section_mode[]",
                                type=option_value(modes),
                                default=["walking"],
                                dest="destination_mode", action="append")
        parser_get.add_argument("max_duration_to_pt", type=int, default=15*60,
                                description="maximal duration of non public \
                                transport in second")
        parser_get.add_argument("walking_speed", type=float, default=1.12)
        parser_get.add_argument("bike_speed", type=float, default=4.1)
        parser_get.add_argument("bss_speed", type=float, default=4.1,)
        parser_get.add_argument("car_speed", type=float, default=16.8)
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

        self.method_decorators.append(complete_links(self))
        self.method_decorators.append(update_journeys_status(self))

    @clean_links()
    @add_id_links()
    @add_fare_links()
    @add_journey_pagination()
    @add_journey_href()
    @marshal_with(journeys)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        args = self.parsers['get'].parse_args()
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
            #we check that the user can use this api
            authentification.authenticate(region, 'ALL', abort=True)
            if uri:
                objects = uri.split('/')
                if objects and len(objects) % 2 == 0:
                    args['origin'] = objects[-1]
                else:
                    abort(503, message="Unable to compute journeys "
                                       "from this object")

        if not args["origin"]:  #@vlara really ? I though we could do reverse isochrone ?
            #shoudl be in my opinion if not args["origin"] and not args["destination"]:
            abort(400, message="from argument is required")

        if not region:
            #TODO how to handle lon/lat ? don't we have to override args['origin'] ?
            self.region = compute_regions(args)

        #we transform the origin/destination url to add information
        if args['origin']:
            args['origin'] = self.transform_id(args['origin'])
        if args['destination']:
            args['destination'] = self.transform_id(args['destination'])

        if not args['datetime']:
            args['datetime'] = datetime.now().strftime('%Y%m%dT1337')

        api = None
        if args['destination']:
            api = 'journeys'
        else:
            api = 'isochrone'


        response = i_manager.dispatch(args, api, instance_name=self.region)
        return response

    def transform_id(self, id):
        splitted_coord = id.split(";")
        splitted_address = id.split(":")
        if len(splitted_coord) == 2:
            return "coord:" + id.replace(";", ":")
        if len(splitted_address) >= 3 and splitted_address[0] == 'address':
            del splitted_address[1]
            return ':'.join(splitted_address)
        if len(splitted_address) >= 3 and splitted_address[0] == 'admin':
            del splitted_address[1]
            return ':'.join(splitted_address)
        return id
