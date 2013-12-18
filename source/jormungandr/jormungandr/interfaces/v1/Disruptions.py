# coding=utf-8
from flask.ext.restful import marshal_with, reqparse
from jormungandr import i_manager
from fields import PbField, error, network, line, NonNullList, NonNullNested
from ResourceUri import ResourceUri
from jormungandr.interfaces.argument import ArgumentDoc
from errors import ManageError

disruption = {
    "network": PbField(network, attribute='network'),
    "lines": NonNullList(NonNullNested(line))
}

disruptions = {
    "disruptions": NonNullList(NonNullNested(disruption)),
    "error": PbField(error, attribute='error')
}


class Disruptions(ResourceUri):
    def __init__(self):
        ResourceUri.__init__(self)
        self.parsers = {}
        self.parsers["get"] = reqparse.RequestParser(
            argument_class=ArgumentDoc)

    @marshal_with(disruptions)
    @ManageError()
    def get(self, region=None, lon=None, lat=None):
        self.region = i_manager.get_region(region, lon, lat)
        args = self.parsers["get"].parse_args()
        response = i_manager.dispatch(args, self.region, "disruptions")
        return response
