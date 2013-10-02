#coding=utf-8
from flask import Flask, request, url_for
from flask.ext.restful import fields, reqparse, marshal_with
from instance_manager import NavitiaManager, RegionNotFound
from protobuf_to_dict import protobuf_to_dict
from fields import stop_point, stop_area, route, line, physical_mode,\
                   commercial_mode, company, network, pagination, place,\
                   PbField, stop_date_time, enum_type, NonNullList, NonNullNested,\
                   display_informations_vj,additional_informations_vj,error

from interfaces.parsers import option_value
from ResourceUri import ResourceUri, add_notes
import datetime
from functools import wraps
from make_links import add_id_links, clean_links
from errors import ManageError
from interfaces.argument import ArgumentDoc
from interfaces.parsers import depth_argument
from operator import itemgetter
from datetime import datetime, timedelta
import sys
from copy import copy
from datetime import datetime

class SectionLinks(fields.Raw):
    def __init__(self, **kwargs):
        super(SectionLinks, self).__init__(**kwargs)

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
                    length = obj.street_network.length
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
                "type" : "LineString",
                "coordinates" : [],
                "properties" : [{
                    "length" : length
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
                    odt_enum = infos_desc.fields_by_name['vehicle_journey_type'].enum_type
                    if infos.vehicle_journey_type != odt_enum.values_by_name['regular'].number:
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
    "mode" : enum_type(attribute="street_network.mode"),
    "duration": fields.Integer(),
    "from" : section_place(place, attribute="origin"),
    "to": section_place(place, attribute="destination"),
    "links" : SectionLinks(attribute="uris"),
    "display_informations" : display_informations_vj(),
    "additional_informations" : additional_informations_vj(),
    "geojson" : GeoJson(),
    "path" : NonNullList(NonNullNested({"length":fields.Integer(),
                                        "name":fields.String()}),
                         attribute="street_network.path_items"),
    "transfer_type" : enum_type(),
    "stop_date_times" : NonNullList(NonNullNested(stop_date_time)),
    "departure_date_time" : fields.String(attribute="begin_date_time"),
    "arrival_date_time" : fields.String(attribute="end_date_time")
}


journey = {
    'duration' : fields.Integer(),
    'nb_transfers' : fields.Integer(),
    'departure_date_time' : fields.String(),
    'arrival_date_time' : fields.String(),
    'requested_date_time' : fields.String(),
    'sections' : NonNullList(NonNullNested(section)),
    'from' : PbField(place, attribute='origin'),
    'to' : PbField(place, attribute='destination'),
    'type' : fields.String()
}

journeys = {
    "journeys" : NonNullList(NonNullNested(journey)),
    "error": PbField(error,attribute='error')
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
                                "type" : "journeys",
                                "href" : url_for("v1.journeys", **kwargs),
                                "templated" : False
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
            datetime_after, datetime_before = self.extremes(objects[0])
            if not datetime_before is None and not datetime_after is None:
                if not "links" in objects[0]:
                    objects[0]["links"] = []

                args = request.args.copy()
                args["datetime"] = datetime_before.strftime("%Y%m%dT%H%M%S")
                args["datetime_represents"] = "arrival"
                objects[0]["links"].append({
                    "href" : url_for("v1.journeys", **args),
                    "templated" : False,
                    "type" : "prev"
                    })
                args["datetime"] = datetime_after.strftime("%Y%m%dT%H%M%S")
                args["datetime_represents"] = "departure"
                objects[0]["links"].append({
                    "href" : url_for("v1.journeys", **args),
                    "templated" : False,
                    "type" : "next"
                    })
            return objects
        return wrapper

    def extremes(self, resp):
        datetime_before = None
        datetime_after = None
        try:
            list_journeys = [journey for journey in resp['journeys']\
                                if 'arrival_date_time' in journey.keys() and\
                                    journey['arrival_date_time'] != '']
            asap_journey = min(list_journeys, key=itemgetter('arrival_date_time'))
        except:
            return (None, None)
        if asap_journey['arrival_date_time'] and ['asap_journey.departure_date_time']:
            minute = timedelta(minutes = 1)
            datetime_after = datetime.strptime(asap_journey['departure_date_time'], "%Y%m%dT%H%M%S") + minute
            datetime_before = datetime.strptime(asap_journey['arrival_date_time'], "%Y%m%dT%H%M%S") - minute

        return (datetime_before, datetime_after)

class Journeys(ResourceUri):
    def __init__(self):
        modes = ["walking", "car", "bike", "br"]
        types = ["all", "rapid"]
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
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
                           default="walking",
                           dest="origin_mode")
        parser_get.add_argument("last_section_mode",
                           type=option_value(modes),
                           default="walking",
                           dest="destination_mode")
        parser_get.add_argument("walking_speed", type=float, default=1.68)
        parser_get.add_argument("walking_distance", type=int, default=1000)
        parser_get.add_argument("bike_speed", type=float, default=8.8)
        parser_get.add_argument("bike_distance", type=int, default=5000)
        parser_get.add_argument("br_speed", type=float, default=8.8,)
        parser_get.add_argument("br_distance", type=int, default=5000)
        parser_get.add_argument("car_speed", type=float, default=16.8)
        parser_get.add_argument("car_distance", type=int, default=15000)
        parser_get.add_argument("forbidden_uris[]", type=str, action="append")
        parser_get.add_argument("count", type=int)
        parser_get.add_argument("type", type=option_value(types), default="all")
#a supprimer
        parser_get.add_argument("max_duration", type=int, default=36000)
        self.method_decorators.append(add_notes(self))


    @clean_links()
    @add_id_links()
    @add_journey_pagination()
    @add_journey_href()
    @marshal_with(journeys)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        args = self.parsers["get"].parse_args()
        #TODO : Changer le protobuff pour que ce soit propre
        args["destination_mode"] = "vls" if args["destination_mode"] == "br" else args["destination_mode"]
        args["origin_mode"] = "vls" if args["origin_mode"] == "br" else args["origin_mode"]
        if not region is None or (not lon is None and not lat is None):
            self.region = NavitiaManager().get_region(region, lon, lat)
            if uri:
                objects = uri.split("/")
                if objects and len(objects) % 2 == 0:
                    args["origin"] = objects[-1]
                else:
                    return {"error" : "Unable to compute journeys from this \
                                       object"}, 503
        else:
            if "origin" in args.keys():
                self.region = NavitiaManager().key_of_id(args["origin"])
                args["origin"] = self.transform_id(args["origin"])
            elif "destination" in args.keys():
                self.region = NavitiaManager().key_of_id(args["destination"])
            if "destination" in args.keys():
                args["destination"] = self.transform_id(args["destination"])
            else:
                raise RegionNotFound("")
        if not args["datetime"]:
            args["datetime"] = datetime.now().strftime("%Y%m%dT1337")
        api = None
        if "destination" in args.keys() and args["destination"]:
            api = "journeys"
        else:
            api = "isochrone"
        response = NavitiaManager().dispatch(args, self.region, api)
        return response

    def transform_id(self, id):
        splitted_coord = id.split(";")
        splitted_address = id.split(":")
        if len(splitted_coord) == 2:
            return "coord:"+id.replace(";", ":")
        if len(splitted_address) >=3 and splitted_address[0] == 'address':
            del splitted_address[1]
            return ':'.join(splitted_address)
        return id
