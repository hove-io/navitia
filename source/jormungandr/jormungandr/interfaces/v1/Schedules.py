# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from flask.ext.restful import fields, marshal_with, reqparse
from jormungandr import i_manager
from jormungandr import timezone
from fields import stop_point, route, pagination, PbField, stop_date_time, \
    additional_informations, stop_time_properties_links, display_informations_vj, \
    display_informations_route, additional_informations_vj, UrisToLinks, error, \
    enum_type, SplitDateTime
from ResourceUri import ResourceUri, complete_links
from datetime import datetime
from jormungandr.interfaces.argument import ArgumentDoc
from errors import ManageError
from flask.ext.restful.types import natural, boolean


class Schedules(ResourceUri):
    parsers = {}

    def __init__(self, endpoint):
        super(Schedules, self).__init__()
        self.endpoint = endpoint
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("filter", type=str)
        parser_get.add_argument("from_datetime", type=str,
                                description="The datetime from which you want\
                                the schedules")
        parser_get.add_argument("duration", type=int, default=3600 * 24,
                                description="Maximum duration between datetime\
                                and the retrieved stop time")
        parser_get.add_argument("depth", type=int, default=2)
        parser_get.add_argument("count", type=int, default=10,
                                description="Number of schedules per page")
        parser_get.add_argument("start_page", type=int, default=0,
                                description="The current page")
        parser_get.add_argument("max_date_times", type=natural,
                                description="Maximum number of schedule per\
                                stop_point/route")
        parser_get.add_argument("forbidden_id[]", type=unicode,
                                description="forbidden ids",
                                dest="forbidden_uris[]",
                                action="append")
        parser_get.add_argument("calendar", type=str,
                                description="Id of the calendar")
        parser_get.add_argument("show_codes", type=boolean, default=False,
                            description="show more identification codes")
        self.method_decorators.append(complete_links(self))

    def get(self, uri=None, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        args["nb_stoptimes"] = args["count"]
        args["interface_version"] = 1
        if uri is None:
            first_filter = args["filter"].lower().split("and")[0].strip()
            parts = first_filter.lower().split("=")
            if len(parts) != 2:
                error = "Unable to parse filter {filter}"
                return {"error": error.format(filter=args["filter"])}, 503
            else:
                self.region = i_manager.key_of_id(parts[1].strip())
        else:
            self.collection = 'schedules'
            args["filter"] = self.get_filter(uri.split("/"))
            self.region = i_manager.get_region(region, lon, lat)
        if not args["from_datetime"]:
            args["from_datetime"] = datetime.now().strftime("%Y%m%dT1337")
        timezone.set_request_timezone(self.region)

        return i_manager.dispatch(args, self.endpoint,
                                  instance_name=self.region)


date_time = {
    "date_time": SplitDateTime(date='date', time='time'),
    "additional_informations": additional_informations(),
    "links": stop_time_properties_links()
}

row = {
    "stop_point": PbField(stop_point),
    "date_times": fields.List(fields.Nested(date_time))
}

header = {
    "display_informations": PbField(display_informations_vj,
                                    attribute='pt_display_informations'),
    "additional_informations": additional_informations_vj(),
    "links": UrisToLinks()
}

table_field = {
    "rows": fields.List(fields.Nested(row)),
    "headers": fields.List(fields.Nested(header))
}

route_schedule_fields = {
    "table": PbField(table_field),
    "display_informations": PbField(display_informations_route,
                                    attribute='pt_display_informations')
}

route_schedules = {
    "error": PbField(error, attribute='error'),
    "route_schedules": fields.List(fields.Nested(route_schedule_fields)),
    "pagination": fields.Nested(pagination)
}


class RouteSchedules(Schedules):

    def __init__(self):
        super(RouteSchedules, self).__init__("route_schedules")

    @marshal_with(route_schedules)
    @ManageError()
    def get(self, uri=None, region=None, lon=None, lat=None):
        return super(RouteSchedules, self).get(uri=uri, region=region, lon=lon,
                                               lat=lat)


stop_schedule = {
    "stop_point": PbField(stop_point),
    "route": PbField(route, attribute="route"),
    "additional_informations": enum_type(attribute="response_status"),
    "display_informations": PbField(display_informations_route,
                                    attribute='pt_display_informations'),
    "date_times": fields.List(fields.Nested(date_time)),
    "links": UrisToLinks()
}
stop_schedules = {
    "stop_schedules": fields.List(fields.Nested(stop_schedule)),
    "pagination": fields.Nested(pagination),
    "error": PbField(error, attribute='error')
}


class StopSchedules(Schedules):

    def __init__(self):
        super(StopSchedules, self).__init__("departure_boards")
        self.parsers["get"].add_argument("interface_version", type=int,
                                         default=1, hidden=True)

    @marshal_with(stop_schedules)
    @ManageError()
    def get(self, uri=None, region=None, lon=None, lat=None):
        return super(StopSchedules, self).get(uri=uri, region=region, lon=lon,
                                              lat=lat)


passage = {
    "route": PbField(route, attribute="vehicle_journey.route"),
    "stop_point": PbField(stop_point),
    "stop_date_time": PbField(stop_date_time)
}

departures = {
    "departures": fields.List(fields.Nested(passage),
                              attribute="next_departures"),
    "pagination": fields.Nested(pagination),
    "error": PbField(error, attribute='error')
}

arrivals = {
    "arrivals": fields.List(fields.Nested(passage), attribute="next_arrivals"),
    "pagination": fields.Nested(pagination),
    "error": PbField(error, attribute='error')
}


class NextDepartures(Schedules):

    def __init__(self):
        super(NextDepartures, self).__init__("next_departures")

    @marshal_with(departures)
    @ManageError()
    def get(self, uri=None, region=None, lon=None, lat=None,
            dest="nb_stoptimes"):
        return super(NextDepartures, self).get(uri=uri, region=region, lon=lon,
                                               lat=lat)


class NextArrivals(Schedules):

    def __init__(self):
        super(NextArrivals, self).__init__("next_arrivals")

    @marshal_with(arrivals)
    @ManageError()
    def get(self, uri=None, region=None, lon=None, lat=None):
        return super(NextArrivals, self).get(uri=uri, region=region, lon=lon,
                                             lat=lat)
