from flask import Flask
from flask.ext.restful import Resource, fields, marshal_with, reqparse
from instance_manager import NavitiaManager
from converters_collection_type import collections_to_resource_type
from fields import stop_point, stop_area, route, line, physical_mode,\
                   commercial_mode, company, network, pagination, PbField,\
                   stop_date_time, enum_type, NonNullList, NonNullNested,\
                   additional_informations, equipments, notes,notes_links,\
                   get_label,StopScheduleLinks
from make_links import add_collection_links, add_id_links
from collections import OrderedDict
from ResourceUri import ResourceUri
from datetime import datetime

class Schedules(ResourceUri):
    def __init__(self, endpoint):
        super(Schedules, self).__init__(self)
        self.endpoint = endpoint
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("filter", type=str)
        self.parser.add_argument("from_datetime", type=str)
        self.parser.add_argument("duration", type=int, default=3600*24)
        self.parser.add_argument("depth", type=int, default=2)
        self.parser.add_argument("count", type=int, default=10)
        self.parser.add_argument("start_page", type=int, default=0)

    def get(self, uri=None, region=None, lon=None, lat=None):
        args = self.parser.parse_args()
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
    "id" : fields.String(attribute="vehiclejourney.uri"),
    "headsign" : fields.String(attribute="vehiclejourney.name"),
    "direction" : fields.String(),
    "physical_mode" : fields.String(attribute="vehiclejourney.physical_mode.name"),
    "description" : fields.String(attribute="vehiclejourney.odt_message"),
    "wheelchair_accessible" : fields.Boolean(default=True),
    "bike_accessible" : fields.Boolean(default=True),
    "equipments" : equipments()
}
table_field = {
    "rows" : NonNullList(NonNullNested(row)),
    "headers" : NonNullList(NonNullNested(header))
}

display_information = {
       "network" : fields.String(attribute="line.network.name"),
       "direction": fields.String(attribute="name"),
       "label" : get_label(),
       "color" : fields.String(attribute="line.color"),
       "commercial_mode" : fields.String(attribute="line.commercial_mode.name")
        }


route_schedule_fields = {
    "table" : PbField(table_field),
    "display_informations" : PbField(display_information, attribute="route"),
}

route_schedules = {
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
    "display_informations" : PbField(display_information, attribute="route"),
    "stop_date_times" : NonNullList(NonNullNested(date_time)),
    "links" : StopScheduleLinks()
}
stop_schedules = {
    "stop_schedules" : NonNullList(NonNullNested(stop_schedule)),
    "pagination" : NonNullNested(pagination),
}

class StopSchedules(Schedules):
    def __init__(self):
        super(StopSchedules, self).__init__("departure_boards")
        self.parser.add_argument("interface_version", type=int, default=1)
    @marshal_with(stop_schedules)
    def get(self, uri=None, region=None, lon= None, lat=None):
        return super(StopSchedules, self).get(uri=uri, region=region, lon=lon, lat=lat)


passage = {
    "route" : PbField(route, attribute="vehicle_journey.route"),
    "stop_point" : PbField(stop_point),
    "stop_date_time" : PbField(stop_date_time)
}

departures = {
    "departures" : NonNullList(NonNullNested(passage), attribute="next_departures")
}

arrivals = {
    "arrivals" : NonNullList(NonNullNested(passage), attribute="next_arrivals")
}

class NextDepartures(Schedules):
    def __init__(self):
        super(NextDepartures, self).__init__("next_departures")
        self.parser.add_argument("max_departures", type=int,
                                 dest="nb_stoptimes", default=20)
    @marshal_with(departures)
    def get(self, uri=None, region=None, lon= None, lat=None, dest="nb_stoptimes"):
        return super(NextDepartures, self).get(uri=uri, region=region, lon=lon, lat=lat)

class NextArrivals(Schedules):
    def __init__(self):
        super(NextArrivals, self).__init__("next_arrivals")
        self.parser.add_argument("max_arrivals", type=int,
                                 dest="nb_stoptimes", default=20)
    @marshal_with(arrivals)
    def get(self, uri=None, region=None, lon= None, lat=None):
        return super(NextArrivals, self).get(uri=uri, region=region, lon=lon, lat=lat)