# coding=utf-8
from flask import Flask, request, url_for
from flask.ext.restful import fields, reqparse, marshal_with
from flask.ext.restful.types import boolean
from jormungandr import i_manager
from jormungandr.interfaces.v1.Uri import journey_pattern_points
from jormungandr.protobuf_to_dict import protobuf_to_dict
from fields import stop_point, stop_area, line, physical_mode,\
    commercial_mode, company, network, pagination, place,\
    PbField, stop_date_time, enum_type, NonNullList, NonNullNested,\
    display_informations_vj, additional_informations_vj, error,\
    generic_message

from jormungandr.interfaces.parsers import option_value
#from exceptions import RegionNotFound
from ResourceUri import ResourceUri, add_notes, update_journeys_status
import datetime
from functools import wraps
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
        length = 0
        enum = obj.DESCRIPTOR.fields_by_name['type'].enum_type.values_by_name
        if obj.type == enum['STREET_NETWORK'].number:
            try:
                if obj.HasField("street_network"):
                    coords = obj.street_network.coordinates
                else:
                    return None
            except ValueError:
                return None
        elif obj.type == enum['PUBLIC_TRANSPORT'].number:
            coords = [sdt.stop_point.coord for sdt in obj.stop_date_times]
        elif obj.type == enum['TRANSFER'].number:
            coords.append(obj.origin.stop_point.coord)
            coords.append(obj.destination.stop_point.coord)
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

    def output(self, key, obj):
        try:
            if obj.HasField("pt_display_informations"):
                infos = obj.pt_display_informations
                if infos.HasField("vehicle_journey_type"):
                    infos_desc = infos.DESCRIPTOR
                    fields = infos_desc.fields_by_name['vehicle_journey_type']
                    odt_enum = fields.enum_type
                    regular = odt_enum.values_by_name['regular'].number
                    if infos.vehicle_journey_type != regular:
                        return "on_demand_transport"
        except ValueError:
            pass
        return super(section_type, self).output(key, obj)


class section_place(PbField):

    def output(self, key, obj):
        enum_t = obj.DESCRIPTOR.fields_by_name['type'].enum_type.values_by_name
        if obj.type == enum_t['WAITING'].number:
            return None
        else:
            return super(PbField, self).output(key, obj)
section = {
    "type": section_type(attribute="type"),
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
    "departure_date_time": fields.String(attribute="begin_date_time"),
    "arrival_date_time": fields.String(attribute="end_date_time"),
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
    'departure_date_time': fields.String(),
    'arrival_date_time': fields.String(),
    'requested_date_time': fields.String(),
    'sections': NonNullList(NonNullNested(section)),
    'from': PbField(place, attribute='origin'),
    'to': PbField(place, attribute='destination'),
    'type': fields.String(),
    'fare': NonNullNested(fare),
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

                args = request.args.copy()
                args["datetime"] = datetime_before.strftime(f_datetime)
                args["datetime_represents"] = "arrival"
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

                args = request.args.copy()
                args["datetime"] = datetime_first.strftime(f_datetime)
                args["datetime_represents"] = "departure"
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
        try:
            list_journeys = [journey for journey in resp['journeys']
                             if 'arrival_date_time' in journey.keys() and
                             journey['arrival_date_time'] != '']
            asap_journey = min(list_journeys,
                               key=itemgetter('arrival_date_time'))
        except:
            return (None, None)
        if asap_journey['arrival_date_time'] \
                and asap_journey['departure_date_time']:
            minute = timedelta(minutes=1)
            s_departure = asap_journey['departure_date_time']
            f_departure = datetime.strptime(s_departure, f_datetime)
            datetime_after = f_departure + minute
            f_arrival = datetime.strptime(s_departure, f_datetime)
            datetime_before = f_arrival - minute

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
            departure_date = datetime.strptime(departure, f_datetime).date()
            midnight = datetime.strptime('0000', '%H%M').time()
            datetime_first = datetime.combine(departure_date, midnight)
            arrival = asap_max['arrival_date_time']
            arrival_date = datetime.strptime(arrival, f_datetime).date()
            almost_midnight = datetime.strptime('2359', '%H%M').time()
            datetime_last = datetime.combine(arrival_date, almost_midnight)

        return (datetime_first, datetime_last)

#add the link between a section and the ticket needed for that section
class add_fare_links(object):

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            objects = f(*args, **kwargs)
            print "add fare_links"
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



class Journeys(ResourceUri):

    def __init__(self):
        ResourceUri.__init__(self)
        modes = ["walking", "car", "bike", "bss"]
        types = ["all", "rapid"]
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
        parser_get.add_argument("first_section_mode",
                                type=option_value(modes),
                                default=["walking", "bike", "car"],
                                dest="origin_mode", action="append")
        parser_get.add_argument("last_section_mode",
                                type=option_value(modes),
                                default=["walking", "bike", "car"],
                                dest="destination_mode", action="append")
        parser_get.add_argument("max_duration_to_pt", type=int, default=10*60,
                                description="maximal duration of non public \
                                transport in second")
        parser_get.add_argument("walking_speed", type=float, default=1.68)
        parser_get.add_argument("bike_speed", type=float, default=8.8)
        parser_get.add_argument("bss_speed", type=float, default=8.8,)
        parser_get.add_argument("car_speed", type=float, default=16.8)
        parser_get.add_argument("forbidden_uris[]", type=str, action="append")
        parser_get.add_argument("count", type=int)
        parser_get.add_argument("type", type=option_value(types),
                                default="all")
# a supprimer
        parser_get.add_argument("max_duration", type=int, default=36000)
        parser_get.add_argument("wheelchair", type=boolean, default=False)
        parser_get.add_argument("debug", type=boolean, default=False,
                                hidden=True)
        self.method_decorators.append(add_notes(self))
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
        if region or (lon and lat):
            self.region = i_manager.get_region(region, lon, lat)
            if uri:
                objects = uri.split('/')
                if objects and len(objects) % 2 == 0:
                    args['origin'] = self.transform_id(objects[-1])
                else:
                    return {'error': 'Unable to compute journeys from this '
                            'object'}, 503
        else:
            if args['origin']:
                self.region = i_manager.key_of_id(args['origin'])
                args['origin'] = self.transform_id(args['origin'])
            elif args['destination']:
                self.region = i_manager.key_of_id(args['destination'])
            if args['destination']:
                args['destination'] = self.transform_id(args['destination'])
            # else:
            #    raise RegionNotFound("")
        if not args['datetime']:
            args['datetime'] = datetime.now().strftime('%Y%m%dT1337')
        api = None
        if args['destination']:
            api = 'journeys'
        else:
            api = 'isochrone'

        response = i_manager.dispatch(args, self.region, api)
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
