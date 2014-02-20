# coding=utf-8
from flask.ext.restful import Resource, fields, marshal_with, reqparse
from jormungandr import i_manager
from jormungandr.interfaces.argument import ArgumentDoc
from make_links import clean_links

calendar_field = {
    "id": fields.String(),
}

calendars_field = {
    "calendar": fields.List(fields.Nested(calendar_field))
}

class Calendars(Resource):

    def __init__(self, endpoint):
        super(Calendars, self).__init__()
        self.parsers["get"] = reqparse.RequestParser(argument_class=ArgumentDoc)

    @clean_links()
    @marshal_with(calendars_field)
    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()

        return i_manager.dispatch(args, self.region, "calendars")
