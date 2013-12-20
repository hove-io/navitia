# coding=utf-8
from flask.ext.restful import marshal_with, reqparse
from jormungandr import i_manager
from fields import PbField, error, network, line,\
    NonNullList, NonNullNested, pagination
from ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from errors import ManageError
from datetime import datetime

disruption = {
    "network": PbField(network, attribute='network'),
    "lines": NonNullList(NonNullNested(line))
}

disruptions = {
    "disruptions": NonNullList(NonNullNested(disruption)),
    "error": PbField(error, attribute='error'),
    "pagination": NonNullNested(pagination)
}


class Disruptions(ResourceUri):
    def __init__(self):
        ResourceUri.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)
        parser_get = self.parsers["get"]
        parser_get.add_argument("depth", type=int, default=1)
        parser_get.add_argument("count", type=int, default=10,
                                description="Number of distruption per page")
        parser_get.add_argument("start_page", type=int, default=0,
                                description="The current page")
        parser_get.add_argument("datetime", type=str,
                                description="The datetime from which you want\
                                the disruption")
        parser_get.add_argument("uri_filter", type=str, default="",
                                description="The filter parameter")

    @marshal_with(disruptions)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()

        if not args["datetime"]:
            args["datetime"] = datetime.now().strftime("%Y%m%dT1337")

        args["uri_filter"] = self.get_filter(uri)

        response = i_manager.dispatch(args, self.region, "disruptions")
        return response

    def get_filter(self, items):
        if items is None:
            return ""
        else:
            filter = ""
            collection = []
            collection.append("network")
            collection.append("line")
            uri_split = items.split(":")
            if uri_split[0] in collection:
                filter = ".uri=" . join([uri_split[0], items])
            return filter
