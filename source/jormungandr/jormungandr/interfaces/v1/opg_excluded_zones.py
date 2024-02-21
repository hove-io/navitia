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


class OpgExcludedZones(StatedResource):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        parser_get = self.parsers["get"]
        parser_get.add_argument("mode", type=str, default=None, help="A geojson in polyline path as base64")
        parser_get.add_argument("datetime", type=str, default=None, help="Distance range of the query in meters")

    def get_poi_place(self, instance, json_data):
        place = {"embedded_type": "poi", "poi": json_data, "name": json_data.get("poi")}

        place["poi"]["label"] = json_data.get("poi")
        place["poi"]["id"] = json_data.get("poi")
        place["poi"]["poi_type"] = {
            "id": instance.olympics_forbidden_uris.poi_property_key
            if instance.olympics_forbidden_uris
            else "poi_type:site_jo2024",
            "name": instance.olympics_forbidden_uris.poi_property_value
            if instance.olympics_forbidden_uris
            else "Site Olympique JO2024",
        }
        return place

    def add_link(self, places):
        if not places:
            return places
        args = {'_type': 'poi', 'id': "{poi.id}", 'region': 'idfm-jo', 'rel': 'pois', 'templated': True}
        link = create_external_link(url='v1.pois.id', **args)
        # ugly hack...
        link["href"] = link["href"].replace("%7B", "{")
        link["href"] = link["href"].replace("%7D", "}")
        return {"places": places, "links": [link]}

    def fetch_and_get_data(self, instance, bucket_name, folder, mode=None):
        if not bucket_name:
            return {}
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
                    file_content = obj.get()['Body'].read().decode('utf-8')
                    json_content = json.loads(file_content)
                    if json_content.get('instance') != instance.name:
                        continue
                    if mode is not None:
                        if mode not in json_content.get("modes"):
                            continue
                    place = self.get_poi_place(instance, json_content)
                    places.append(place)

                except Exception:
                    logger.exception("Error on OpgExcludedZones")
                    continue
        except Exception:
            logger.exception("Error on OpgExcludedZones")
            return {}
        return self.add_link(places)

    def get(self, region=None, lon=None, lat=None):
        args = self.parsers["get"].parse_args()
        region_str = i_manager.get_region(region, lon, lat)
        instance = i_manager.instances[region_str]

        return (
            self.fetch_and_get_data(
                instance=instance,
                bucket_name=app.config.get(str("ASGARD_S3_BUCKET")),
                folder="excluded_zones",
                mode=args['mode'],
            ),
            200,
        )
