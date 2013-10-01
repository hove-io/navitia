#coding=utf-8
from flask import Flask
from flask.ext.restful import Resource, fields, marshal_with, reqparse
from instance_manager import NavitiaManager
from converters_collection_type import collections_to_resource_type
from fields import stop_point, stop_area, route, line, physical_mode,\
                   commercial_mode, company, network, pagination, PbField,\
                   stop_date_time, enum_type, NonNullList, NonNullNested,\
                   additional_informations,  notes,notes_links,\
                   get_label,display_informations_vj,display_informations_route,\
                   additional_informations_vj, UrisToLinks,  error
from make_links import add_collection_links, add_id_links
from collections import OrderedDict
from ResourceUri import ResourceUri, add_notes
from datetime import datetime
from interfaces.argument import ArgumentDoc
from interfaces.parsers import depth_argument
from errors import ManageError

class Schedules(ResourceUri):
    parsers = {}
    def __init__(self, endpoint):
        super(Schedules, self).__init__()
        self.endpoint = endpoint
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("filter", type=str)
        parser_get.add_argument("from_datetime", type=str,
                description="The datetime from which you want the schedules")
        parser_get.add_argument("duration", type=int, default=3600*24,
                description="Maximum duration between datetime and the\
                             retrieved stop time")
        parser_get.add_argument("depth", type=int, default=2)
        parser_get.add_argument("count", type=int, default=10,
                description="Number of schedules per page")
        parser_get.add_argument("start_page", type=int, default=0,
                description="The current page")
        self.method_decorators.append(add_notes(self))


    def get(self, uri=None, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        args["nb_stoptimes"] = args["count"]
        if uri == None:
            first_filter = args["filter"].lower().split("and")[0].strip()
            filter_parts = first_filter.lower().split("=")
            if len(filter_parts) != 2:
                error = "Unable to parse filter {filter}"
                return {"error" : error.format(filter=args["filter"])},503
            else:
                self.region = NavitiaManager().key_of_id(filter_parts[1].strip())
        else:
            args["filter"] = self.get_filter(uri.split("/"))
            self.region = NavitiaManager().get_region(region, lon, lat)
        if not args["from_datetime"]:
            args["from_datetime"] = datetime.now().strftime("%Y%m%dT1337")

        response = NavitiaManager().dispatch(args, self.region, self.endpoint)

        if response.HasField("error"):
            return ManageError(response)

        return response, 200

date_time = {
    "date_time" : fields.String(attribute="stop_time"),
    "additional_informations" : additional_informations(),
    "links": notes_links
}
row = {
    "stop_point" : PbField(stop_point),
    "date_times" : NonNullList(fields.Nested(date_time), attribute="stop_times")
}

header = {
    "display_informations" :  display_informations_vj(),
    "additional_informations" : additional_informations_vj(),
    "links" : UrisToLinks()
}

table_field = {
    "rows" : NonNullList(NonNullNested(row)),
    "headers" : NonNullList(NonNullNested(header))
}

route_schedule_fields = {
    "table" : PbField(table_field),
    "display_informations" : display_informations_route()
}

route_schedules = {
    "error": PbField(error,attribute='error'),
    "route_schedules" : NonNullList(NonNullNested(route_schedule_fields)),
    "pagination" : NonNullNested(pagination)
    }

class RouteSchedules(Schedules):

    def __init__(self):
        super(RouteSchedules, self).__init__("route_schedules")
    @marshal_with(route_schedules)
    def get(self, uri=None, region=None, lon= None, lat=None):
        return super(RouteSchedules, self).get(uri=uri, region=region, lon=lon, lat=lat)


stop_schedule = {
    "stop_point" : PbField(stop_point),
    "route" : PbField(route, attribute="route"),
    "display_informations" : display_informations_route(),
    "stop_date_times" : NonNullList(NonNullNested(date_time)),
    "links" : UrisToLinks()
}
stop_schedules = {
    "stop_schedules" : NonNullList(NonNullNested(stop_schedule)),
    "pagination" : NonNullNested(pagination),
    "error": PbField(error,attribute='error')
}

class StopSchedules(Schedules):
    def __init__(self):
        super(StopSchedules, self).__init__("departure_boards")
        self.parsers["get"].add_argument("interface_version", type=int, default=1)
    @marshal_with(stop_schedules)
    def get(self, uri=None, region=None, lon= None, lat=None):
        return super(StopSchedules, self).get(uri=uri, region=region, lon=lon, lat=lat)


passage = {
    "route" : PbField(route, attribute="vehicle_journey.route"),
    "stop_point" : PbField(stop_point),
    "stop_date_time" : PbField(stop_date_time)
}

departures = {
    "departures" : NonNullList(NonNullNested(passage), attribute="next_departures"),
    "pagination" : NonNullNested(pagination),
    "error": PbField(error,attribute='error')
}

arrivals = {
    "arrivals" : NonNullList(NonNullNested(passage), attribute="next_arrivals"),
    "pagination" : NonNullNested(pagination),
    "error": PbField(error,attribute='error')
}

class NextDepartures(Schedules):
    def __init__(self):
        super(NextDepartures, self).__init__("next_departures")

    @marshal_with(departures)
    def get(self, uri=None, region=None, lon= None, lat=None, dest="nb_stoptimes"):
        return super(NextDepartures, self).get(uri=uri, region=region, lon=lon, lat=lat)

class NextArrivals(Schedules):
    def __init__(self):
        super(NextArrivals, self).__init__("next_arrivals")

    @marshal_with(arrivals)
    def get(self, uri=None, region=None, lon= None, lat=None):
        return super(NextArrivals, self).get(uri=uri, region=region, lon=lon, lat=lat)
