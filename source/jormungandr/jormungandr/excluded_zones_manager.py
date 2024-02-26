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
from botocore.client import Config
import logging
import json

from jormungandr import app, memory_cache, cache
from jormungandr.resource_s3_object import ResourceS3Object


class ExcludedZonesManager:
    @staticmethod
    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('ASGARD_S3_DATA_TIMEOUT'), 2 * 60))
    def get_object(resource_s3_object):
        logger = logging.getLogger(__name__)
        try:
            file_content = resource_s3_object.s3_object.get()['Body'].read().decode('utf-8')
            return json.loads(file_content)
        except Exception:
            logger.exception('Error while loading file: {}'.format(resource_s3_object.s3_object.key))
            return {}

    @staticmethod
    @memory_cache.memoize(
        app.config[str('MEMORY_CACHE_CONFIGURATION')].get(str('ASGARD_S3_DATA_TIMEOUT'), 1 * 60)
    )
    def get_excluded_zones(instance_name=None, mode=None):
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

                    if instance_name is not None and json_content.get('instance') != instance_name:
                        continue
                    if mode is not None and mode not in json_content.get("modes", []):
                        continue

                    excluded_zones.append(json_content)
                except Exception:
                    logger.exception(
                        "Error on fetching excluded zones: bucket: {}, instance: {}, mode ={}",
                        bucket_name,
                        instance_name,
                        mode,
                    )
                    continue

        except Exception:
            logger.exception(
                "Error on fetching excluded zones: bucket: {}, instance: {}, mode ={}",
                bucket_name,
                instance_name,
                mode,
            )

        return excluded_zones
