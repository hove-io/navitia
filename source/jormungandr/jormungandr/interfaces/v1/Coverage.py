# coding=utf-8
from flask.ext.restful import Resource, fields, marshal_with
from jormungandr import i_manager
from make_links import add_coverage_link, add_coverage_link, add_collection_links, clean_links
from converters_collection_type import collections_to_resource_type
from collections import OrderedDict
from fields import NonNullNested


region_fields = {
    "id": fields.String(attribute="region_id"),
    "start_production_date": fields.String,
    "end_production_date": fields.String,
    "status": fields.String,
    "shape": fields.String,
    "error": NonNullNested({
        "code": fields.String,
        "value": fields.String
    })
}
regions_fields = OrderedDict([
    ("regions", fields.List(fields.Nested(region_fields)))
])

collections = collections_to_resource_type.keys()


class Coverage(Resource):

    @clean_links()
    @add_coverage_link()
    @add_collection_links(collections)
    @marshal_with(regions_fields)
    def get(self, region=None, lon=None, lat=None):
        return i_manager.regions(region, lon, lat), 200
