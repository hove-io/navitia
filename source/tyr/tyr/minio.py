# coding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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


import os
import requests
from minio import Minio

from flask import current_app


class MinioWrapper:
    def __init__(self):
        self.endpoint = current_app.config.get('MINIO_URL', None)
        self.bucket_name = current_app.config.get('MINIO_BUCKET_NAME', None)
        if self.endpoint is None:
            raise Exception("MINIO_URL is not configured")
        if self.bucket_name is None:
            raise Exception("MINIO_BUCKET_NAME is not configured")

        self.use_iam_provider = current_app.config.get('MINIO_USE_IAM_PROVIDER', False)

        if not self.use_iam_provider:
            self.access_key = current_app.config.get("MINIO_ACCESS_KEY", None)
            self.secret_key = current_app.config.get("MINIO_SECRET_KEY", None)
            self.session_token = None
            if self.access_key is None:
                raise Exception(
                    "MINIO_ACCESS_KEY is not configured but needed since MINIO_IAM_PROVIDER is False"
                )
            if self.secret_key is None:
                raise Exception(
                    "MINIO_SECRET_KEY is not configured but needed since MINIO_IAM_PROVIDER is False"
                )

    def upload_file(
        self,
        filename,  # path to the file to upload
        file_key,  # path where the file will be put in the bucket
        metadata={},  # tags that will be applied to the file in the bucket
        content_type="application/zip",
    ):
        if self.use_iam_provider:
            self.retrieve_credentials()
        client = Minio(
            endpoint=self.endpoint,
            access_key=self.access_key,
            secret_key=self.secret_key,
            token=self.session_token,
        )
        client.fput_object(self.bucket_name, file_key, filename, metadata=metadata, content_type=content_type)

    def retrieve_credentials(self):
        """Retrieve credentials from ECS IAM Role"""

        # see https://stackoverflow.com/questions/57065458/cannot-access-instance-metadata-from-within-a-fargate-task
        # and https://docs.aws.amazon.com/AmazonECS/latest/developerguide/task-iam-roles.html

        relative_uri = os.environ.get("AWS_CONTAINER_CREDENTIALS_RELATIVE_URI", None)
        if relative_uri is None:
            raise Exception("AWS_CONTAINER_CREDENTIALS_RELATIVE_URI is not configured")

        credentials_url = "http://169.254.170.2" + relative_uri

        resp = requests.get(credentials_url)
        if resp.status_code != 200:
            raise Exception(
                "Failed to retreive IAM credentials from {} : status code is {}".format(
                    credentials_url, resp.status_code
                )
            )

        json = resp.json()
        self.access_key = json["AccessKeyId"]
        self.secret_key = json["SecretAccessKey"]
        self.session_token = json["Token"]
