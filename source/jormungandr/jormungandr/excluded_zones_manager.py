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

import boto3
import pytz
import shapely.iterops
from dateutil import parser
from botocore.client import Config
import logging
import json
import shapely
import datetime
from typing import Dict

from jormungandr import app, memory_cache, cache
from jormungandr.resource_s3_object import ResourceS3Object


class ExcludedZonesManager:
    excluded_shapes = dict()  # type: Dict[str, shapely.geometry]

    @staticmethod
    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('ASGARD_S3_DATA_TIMEOUT'), 24 * 60))
    def get_object(resource_s3_object):
        logger = logging.getLogger(__name__)
        try:
            file_content = resource_s3_object.s3_object.get()['Body'].read().decode('utf-8')
            return json.loads(file_content)
        except Exception:
            logger.exception('Error while loading file: {}'.format(resource_s3_object.s3_object.key))
            return {}

    @staticmethod
    def is_activated(activation_period, date):
        def is_between(period, d):
            from_date = parser.parse(period['from']).date()
            to_date = parser.parse(period['to']).date()
            return from_date <= d < to_date

        return any((is_between(period, date) for period in activation_period))

    @classmethod
    @memory_cache.memoize(
        app.config[str('MEMORY_CACHE_CONFIGURATION')].get(str('ASGARD_S3_DATA_TIMEOUT'), 10 * 60)
    )
    def get_all_excluded_zones(cls):
        bucket_name = app.config.get(str("ASGARD_S3_BUCKET"))
        folder = "excluded_zones"

        logger = logging.getLogger(__name__)
        args = {"connect_timeout": 2, "read_timeout": 2, "retries": {'max_attempts': 0}}
        s3_resource = boto3.resource('s3', config=Config(**args))
        excluded_zones = []
        try:
            my_bucket = s3_resource.Bucket(bucket_name)
            for obj in my_bucket.objects.filter(Prefix="{}/".format(folder)):
                if not obj.key.endswith('.json'):
                    continue
                try:
                    json_content = ExcludedZonesManager.get_object(ResourceS3Object(obj, None))
                    excluded_zones.append(json_content)
                except Exception:
                    logger.exception("Error on fetching excluded zones: bucket: {}", bucket_name)
                    continue
        except Exception:
            logger.exception(
                "Error on fetching excluded zones: bucket: {}",
                bucket_name,
            )
        excluded_shapes = dict()
        for zone in excluded_zones:
            # remove the DAMN MYPY to use walrus operator!!!!!
            shape_str = zone.get('shape')
            if shape_str:
                continue
            try:
                shape = shapely.wkt.loads(shape_str)
            except Exception as e:
                logger.error("error occurred when load shapes of excluded zones: " + str(e))
                continue
            excluded_shapes[zone.get("poi")] = shape

        cls.excluded_shapes = excluded_shapes

        return excluded_zones

    @staticmethod
    @memory_cache.memoize(
        app.config[str('MEMORY_CACHE_CONFIGURATION')].get(str('ASGARD_S3_DATA_TIMEOUT'), 10 * 60)
    )
    def get_excluded_zones(instance_name=None, mode=None, date=None):
        excluded_zones = []
        for json_content in ExcludedZonesManager.get_all_excluded_zones():
            if instance_name is not None and json_content.get('instance') != instance_name:
                continue
            if mode is not None and mode not in json_content.get("modes", []):
                continue
            if date is not None and not ExcludedZonesManager.is_activated(
                json_content['activation_periods'], date
            ):
                continue
            excluded_zones.append(json_content)

        return excluded_zones

    @classmethod
    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('ASGARD_S3_DATA_TIMEOUT'), 10 * 60))
    def is_excluded(cls, obj, mode, timestamp):
        date = datetime.datetime.fromtimestamp(timestamp, tz=pytz.timezone("UTC")).date()
        # update excluded zones
        excluded_zones = ExcludedZonesManager.get_excluded_zones(instance_name=None, mode=mode, date=date)
        poi_ids = set((zone.get("poi") for zone in excluded_zones))
        shapes = (cls.excluded_shapes.get(poi_id) for poi_id in poi_ids if cls.excluded_shapes.get(poi_id))
        p = shapely.geometry.Point(obj.lon, obj.lat)
        return any((shape.contains(p) for shape in shapes))
