# Copyright (c) 2001-2024, Hove and/or its affiliates. All rights reserved.
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
import boto3
from botocore.client import Config
import json
import logging
from jormungandr.interfaces.v1.StatedResource import StatedResource
from jormungandr.interfaces.v1.make_links import create_external_link
from jormungandr import i_manager, app
from jormungandr.excluded_zones_manager import ExcludedZonesManager


class OpgExcludedZones(StatedResource):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        parser_get = self.parsers["get"]
        parser_get.add_argument("mode", type=str, default=None, help="travelling mode")

    @staticmethod
    def get_poi_place(json_data):
        place = {"embedded_type": "poi", "poi": json_data, "name": json_data.get("poi")}

        place["poi"]["label"] = json_data.get("poi")
        place["poi"]["id"] = json_data.get("poi")

        return place

    @staticmethod
    def add_link(places):
        if not places:
            return places
        args = {'_type': 'poi', 'id': "{poi.id}", 'region': 'idfm-jo', 'rel': 'pois', 'templated': True}
        link = create_external_link(url='v1.pois.id', **args)
        # ugly hack...
        link["href"] = link["href"].replace("%7B", "{")
        link["href"] = link["href"].replace("%7D", "}")
        return {"places": places, "links": [link]}

    def fetch_and_get_data(self, bucket_name, mode=None):
        if not bucket_name:
            return {}
        places = []
        logger = logging.getLogger(__name__)
        try:
            for json_content in ExcludedZonesManager.get_excluded_zones(mode=mode, date=None):
                place = self.get_poi_place(json_content)
                places.append(place)
        except Exception:
            logger.exception("Error on OpgExcludedZones")
            return {}
        return self.add_link(places)

    def get(self, region=None):
        args = self.parsers["get"].parse_args()

        return (
            self.fetch_and_get_data(
                bucket_name=app.config.get(str("ASGARD_S3_BUCKET")),
                mode=args['mode'],
            ),
            200,
        )
