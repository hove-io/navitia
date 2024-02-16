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
from jormungandr import app, cache, memory_cache
from jormungandr.interfaces.v1.StatedResource import StatedResource
from jormungandr.interfaces.v1.make_links import create_external_link
import shapely


class OpgExcludedZones(StatedResource):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        parser_get = self.parsers["get"]
        parser_get.add_argument("mode", type=str, default=None, help="A geojson in polyline path as base64")
        parser_get.add_argument("datetime", type=str, default=None, help="Distance range of the query in meters")

    @staticmethod
    @memory_cache.memoize(app.config[str('MEMORY_CACHE_CONFIGURATION')].get(str('TIMEOUT_ASGARD_S3'), 120))
    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('TIMEOUT_ASGARD_S3'), 300))
    def fetch_and_get_data(bucket_name, folder, instance=None, lon=None, lat=None, mode=None, datetime=None):
        if not bucket_name:
            return []
        places = []
        logger = logging.getLogger(__name__)
        args = {"connect_timeout": 2, "read_timeout": 2, "retries": {'max_attempts': 0}}
        s3_resource = boto3.resource('s3', config=Config(**args))
        try:
            my_bucket = s3_resource.Bucket(bucket_name)
            for obj in my_bucket.objects.filter(Prefix="{}/".format(folder)):
                if not obj.key.endswith('.json'):
                    continue
                try:
                    place = {"embedded_type": "poi", "poi": {}}
                    file_content = obj.get()['Body'].read().decode('utf-8')
                    json_content = json.loads(file_content)

                    if instance is not None and json_content.get('instance') != instance:
                        continue

                    if lon is not None and lat is not None:
                        p = shapely.geometry.Point(lon, lat)
                        zone = shapely.wkt.loads(json_content.get("shape", ""))
                        if not zone.contains(p):
                            continue

                    if mode is not None:
                        if mode not in json_content.get("modes"):
                            continue

                    place["id"] = json_content.get("poi")
                    place["name"] = json_content.get("poi")
                    place["poi"].update(json_content)
                    place["poi"]["label"] = json_content.get("poi")
                    place["poi"]["id"] = json_content.get("poi")
                    place["poi"]["poi_type"] = {"id": "poi_type:site_jo2024", "name": "Site Olympique JO2024"}
                    places.append(place)
                except Exception as e:
                    logger.exception("Error on OlympicSiteParamsManager")
                    continue
        except Exception:
            logger.exception("Error on OlympicSiteParamsManager")
            return {}
        args = {'_type': 'poi', 'id': "{poi.id}", 'region': 'idfm-jo', 'rel': 'pois', 'templated': True}
        link = create_external_link(url='v1.pois.id', **args)
        # ugly hack...
        link["href"] = link["href"].replace("%7B", "{")
        link["href"] = link["href"].replace("%7D", "}")
        return {"places": places, "links": [link]}

    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()

        return (
            self.fetch_and_get_data(
                bucket_name=app.config.get(str("ASGARD_S3_BUCKET"), ""),
                folder="excluded_zones",
                instance=region,
                lon=lon,
                lat=lat,
                mode=args['mode'],
                datetime=args['datetime'],
            ),
            200,
        )
