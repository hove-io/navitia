# coding=utf-8
from flask.ext.restful import marshal_with, reqparse
from jormungandr import i_manager
from ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from errors import ManageError
from fields import fields, enum_type, NonNullList,\
    NonNullNested, PbField, error, pagination

week_pattern = {
    "monday": fields.Boolean(),
    "tuesday": fields.Boolean(),
    "wednesday": fields.Boolean(),
    "thursday": fields.Boolean(),
    "friday": fields.Boolean(),
    "saturday": fields.Boolean(),
    "sunday": fields.Boolean(),
}

calendar_period = {
    "begin": fields.String(),
    "end": fields.String(),
}

calendar_exception = {
    "datetime": fields.String(attribute="date"),
    "type": enum_type(),
}
validity_pattern = {
    'beginning_date': fields.String(),
    'days': fields.String(),
}
calendar = {
    "id": fields.String(attribute="uri"),
    "name": fields.String(),
    "week_pattern": NonNullNested(week_pattern),
    "active_periods": NonNullList(NonNullNested(calendar_period)),
    "exceptions": NonNullList(NonNullNested(calendar_exception)),
    "validity_pattern": NonNullNested(validity_pattern)
}

calendars = {
    "calendars": NonNullList(NonNullNested(calendar)),
    "error": PbField(error, attribute='error'),
    "pagination": NonNullNested(pagination)
}

class Calendars(ResourceUri):
    def __init__(self):
        ResourceUri.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("depth", type=int, default=1)
        parser_get.add_argument("count", type=int, default=10,
                                description="Number of calendars per page")
        parser_get.add_argument("start_page", type=int, default=0,
                                description="The current page")
        parser_get.add_argument("start_date", type=str, default="",
                                description="Start date")
        parser_get.add_argument("end_date", type=str, default="",
                                description="End date")
        parser_get.add_argument("forbidden_id[]", type=unicode,
                                description="forbidden ids",
                                dest="forbidden_uris[]",
                                action="append")

    @marshal_with(calendars)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None, id=None):
        self.region = i_manager.get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()

        if(id):
            args["filter"] = "calendar.uri=" + id
        elif(uri):
            # Calendars of line
            if uri[-1] == "/":
                uri = uri[:-1]
            uris = uri.split("/")
            args["filter"] = self.get_filter(uris)
        else:
            args["filter"] = ""
        response = i_manager.dispatch(args, "calendars",
                                      instance_name=self.region)
        return response
