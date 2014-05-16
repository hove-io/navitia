# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from flask.ext.restful import Resource, fields, marshal_with
from jormungandr import i_manager
from jormungandr.interfaces.v1.StatedResource import StatedResource
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


class Coverage(StatedResource):

    @clean_links()
    @add_coverage_link()
    @add_collection_links(collections)
    @marshal_with(regions_fields)
    def get(self, region=None, lon=None, lat=None):
        return i_manager.regions(region, lon, lat), 200
