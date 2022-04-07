# coding=utf-8

#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr import i_manager
from jormungandr.interfaces.v1.StatedResource import StatedResource
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.make_links import add_coverage_link, add_collection_links, clean_links
from jormungandr.interfaces.v1.converters_collection_type import collections_to_resource_type
from jormungandr.interfaces.v1.serializer import api
from jormungandr.interfaces.v1.serializer.api import CoveragesSerializer
from navitiacommon.parser_args_type import BooleanType
import flask

collections = list(collections_to_resource_type.keys())


class Coverage(StatedResource):
    def __init__(self, quota=True, *args, **kwargs):
        super(Coverage, self).__init__(quota=quota, output_type_serializer=CoveragesSerializer, *args, **kwargs)
        self.parsers["get"].add_argument(
            "disable_geojson",
            type=BooleanType(),
            default=False,
            help='hide the coverage geojson to reduce response size',
        )

    @clean_links()
    @add_coverage_link()
    @add_collection_links(collections)
    @get_serializer(serpy=api.CoveragesSerializer)
    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()

        request_id = "coverage_{}".format(flask.request.id)
        resp = i_manager.regions(region, lon, lat, request_id=request_id)
        if 'regions' in resp:
            resp['regions'] = sorted(resp['regions'], key=lambda r: r.get('name', r.get('region_id')))
        if args['disable_geojson']:
            for r in resp['regions']:
                if 'shape' in r:
                    del r['shape']
        return resp, 200

    def options(self, **kwargs):
        return self.api_description(**kwargs)
