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

import logging
import json
from collections import namedtuple
from jormungandr.utils import (
    is_olympic_poi,
    local_str_date_to_utc,
    date_to_timestamp,
    str_to_dt,
    str_datetime_utc_to_local,
)
import boto3
from jormungandr import app
from navitiacommon import type_pb2
from botocore.client import Config
from jormungandr import app, memory_cache, cache


AttractivityVirtualFallback = namedtuple("AttractivityVirtualFallback", "attractivity, virtual_duration")


class ResourceS3Object:
    def __init__(self, s3_object, instance_name):
        self.s3_object = s3_object
        self.instance_name = instance_name

    def __repr__(self):
        return "{}-{}-{}".format(self.instance_name, self.s3_object.key, self.s3_object.e_tag)


def has_applicable_scenario(api_request):
    return bool(api_request.get("olympic_site_params"))


class OlympicSiteParamsManager:
    def __init__(self, instance, config):
        self.olympic_site_params = dict()
        self.map_filename_last_modified = dict()
        self.instance = instance
        self.bucket_name = config.get("name")
        self.folder = config.get("folder", "olympic_site_params")
        self.args = config.get("args", {"connect_timeout": 2, "read_timeout": 2, "retries": {'max_attempts': 0}})

        self.check_conf()

    def check_conf(self):
        logger = logging.getLogger(__name__)
        if not self.bucket_name:
            logger.warning(
                "Reading stop points attractivities, undefined bucket_name for instance {}".format(
                    self.instance.name
                )
            )

    def __repr__(self):
        return "opg-{}".format(self.instance.name)

    def build_olympic_site_params(self, scenario, data):
        if not scenario:
            logging.getLogger(__name__).warning("Impossible to build olympic_site_params: Empty scenario")
            return {}
        return {
            spt_id: AttractivityVirtualFallback(d["attractivity"], d["virtual_fallback"])
            for spt_id, d in data.get("scenarios", {}).get(scenario, {}).get("stop_points", {}).items()
        }

    def get_valid_scenario_name(self, scenario_list, key, datetime):
        for event in scenario_list:
            if event["from_timestamp"] <= datetime <= event["to_timestamp"]:
                return event.get(key)
        return None

    def get_dict_scenario(self, poi_uri, key, datetime):
        data = self.olympic_site_params.get(poi_uri)
        if not data:
            return {}
        scenario_name = self.get_valid_scenario_name(data.get("events", []), key, datetime)
        if scenario_name:
            return self.build_olympic_site_params(scenario_name, data)
        return {}

    def get_show_natural_opg_journeys(self, conf_additional_parameters, query_show_natural_opg_journeys):
        return query_show_natural_opg_journeys or conf_additional_parameters.get(
            "show_natural_opg_journeys", False
        )

    def filter_and_get_additional_parameters(self, conf_additional_parameters):
        return {
            key: value for key, value in conf_additional_parameters.items() if key != 'show_natural_opg_journeys'
        }

    def get_dict_additional_parameters(self, poi_uri, key, datetime):
        data = self.olympic_site_params.get(poi_uri)
        if not data:
            return {}
        scenario_name = self.get_valid_scenario_name(data.get("events", []), key, datetime)
        if scenario_name:
            return data.get("scenarios", {}).get(scenario_name, {}).get("additional_parameters", {})
        return {}

    def get_strict_parameter(self, poi_uri):
        data = self.olympic_site_params.get(poi_uri)
        if not data:
            return False
        return data.get("strict", False)

    def get_timestamp(self, str_datetime):
        if self.instance.timezone:
            dt = local_str_date_to_utc(str_datetime, self.instance.timezone)
        else:
            dt = str_to_dt(str_datetime)
        return date_to_timestamp(dt) if dt else None

    def str_datetime_time_stamp(self, json_data):
        """
        transform from_datetime, to_datetime to time_stamp
        """
        for key, value in json_data.items():
            for event in value.get("events", []):
                str_from_datetime = event.get("from_datetime")
                str_to_datetime = event.get("to_datetime")
                if str_from_datetime and str_to_datetime:
                    event["from_timestamp"] = self.get_timestamp(str_from_datetime)
                    event["to_timestamp"] = self.get_timestamp(str_to_datetime)

    def get_json_content(self, s3_object):
        logger = logging.getLogger(__name__)
        try:
            file_content = s3_object.get()['Body'].read().decode('utf-8')
            return json.loads(file_content)
        except Exception:
            logger.exception('Error while loading file: {}'.format(s3_object.key))
            return {}

    def add_metadata(self, json_content, s3_object):
        if s3_object:
            filename = ""
            try:
                filename = s3_object.key.split("/")[-1]
            except Exception:
                pass
            for key, value in json_content.items():
                value["metadata"] = {
                    "last_load_at": str_datetime_utc_to_local(None, self.instance.timezone),
                    "filename": filename,
                }

    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('FETCH_S3_DATA_TIMEOUT'), 24 * 60))
    def load_data(self, resource_s3_object):
        """
        the POI conf is hidden in REDIS by the instance name, file name and Etag of the S3 object
        """
        json_content = self.get_json_content(resource_s3_object.s3_object)
        self.str_datetime_time_stamp(json_content)
        self.add_metadata(json_content, resource_s3_object.s3_object)
        return json_content

    @memory_cache.memoize(
        app.config[str('MEMORY_CACHE_CONFIGURATION')].get(str('FETCH_S3_DATA_TIMEOUT'), 2 * 60)
    )
    def fetch_and_get_data(self, instance_name, bucket_name, folder, **kwargs):
        result = dict()
        logger = logging.getLogger(__name__)
        s3_resource = boto3.resource('s3', config=Config(**kwargs))
        try:
            my_bucket = s3_resource.Bucket(bucket_name)
            for obj in my_bucket.objects.filter(Prefix="{}/{}/".format(folder, instance_name)):
                if obj.key.endswith('.json'):
                    resource_s3_object = ResourceS3Object(obj, instance_name)
                    json_content = self.load_data(resource_s3_object)
                    result.update(json_content)
        except Exception:
            logger.exception("Error on OlympicSiteParamsManager")
        return result

    @property
    def opg_params(self):
        self.fill_olympic_site_params_from_s3()
        return self.olympic_site_params

    def fill_olympic_site_params_from_s3(self):
        if not self.instance.olympics_forbidden_uris:
            return
        logger = logging.getLogger(__name__)
        if not self.bucket_name:
            logger.debug("Reading stop points attractivities, undefined bucket_name")
            return

        self.olympic_site_params = self.fetch_and_get_data(
            instance_name=self.instance.name, bucket_name=self.bucket_name, folder=self.folder, **self.args
        )

    def manage_navette(self, api_request):
        # Add forbidden_uri
        if self.instance.olympics_forbidden_uris and self.instance.olympics_forbidden_uris:
            self.manage_forbidden_uris(
                api_request, self.instance.olympics_forbidden_uris.pt_object_olympics_forbidden_uris
            )

    def build(self, pt_object_origin, pt_object_destination, api_request):
        # Warning, the order of functions is important
        # Order 1 : get_olympic_site_params
        # Order 2 : build_api_request
        api_request["olympic_site_params"] = self.get_olympic_site_params(
            pt_object_origin, pt_object_destination, api_request
        )

        self.build_api_request(api_request)

    def manage_forbidden_uris(self, api_request, forbidden_uris):
        if not forbidden_uris:
            return
        if api_request.get("forbidden_uris[]"):
            api_request["forbidden_uris[]"] += forbidden_uris
        else:
            api_request["forbidden_uris[]"] = forbidden_uris

    def build_api_request(self, api_request):
        if not has_applicable_scenario(api_request):
            return
        olympic_site_params = api_request.get("olympic_site_params")
        # Add keep_olympics_journeys parameter
        if api_request.get("_keep_olympics_journeys") is None and olympic_site_params:
            api_request["_keep_olympics_journeys"] = True

        # Add additional parameters
        for key, value in olympic_site_params.get("additional_parameters", {}).items():
            if key == "forbidden_uris":
                self.manage_forbidden_uris(api_request, value)
            else:
                api_request[key] = value

        # Add criteria
        if api_request.get("criteria") in ["departure_stop_attractivity", "arrival_stop_attractivity"]:
            return
        if olympic_site_params.get("departure_scenario"):
            api_request["criteria"] = "departure_stop_attractivity"
        elif olympic_site_params.get("arrival_scenario"):
            api_request["criteria"] = "arrival_stop_attractivity"

    def get_olympic_site(self, entry_point):
        if not self.instance or not self.instance.olympics_forbidden_uris:
            return None
        if not entry_point:
            return None
        if is_olympic_poi(entry_point, self.instance):
            return entry_point
        if entry_point.embedded_type == type_pb2.ADDRESS:
            if not (hasattr(entry_point.address, 'within_zones') and entry_point.address.within_zones):
                return None
            for within_zone in entry_point.address.within_zones:
                if is_olympic_poi(within_zone, self.instance):
                    return within_zone
        return None

    def get_olympic_site_params(self, pt_origin_detail, pt_destination_detail, api_request):

        origin_olympic_site = self.get_olympic_site(pt_origin_detail)
        destination_olympic_site = self.get_olympic_site(pt_destination_detail)

        if not origin_olympic_site and not destination_olympic_site:
            self.manage_navette(api_request)
            return {}

        self.fill_olympic_site_params_from_s3()

        if origin_olympic_site and destination_olympic_site:
            origin_olympic_site = None

        attractivities = dict()
        virtual_duration = dict()
        attractivities.update(api_request.get('_olympics_sites_attractivities[]') or [])
        virtual_duration.update(api_request.get('_olympics_sites_virtual_fallback[]') or [])
        if attractivities:
            result = dict()
            for spt_id, attractivity in attractivities.items():
                virtual_fallback = virtual_duration.get(spt_id, 0)
                result[spt_id] = AttractivityVirtualFallback(attractivity, virtual_fallback)
            if origin_olympic_site:
                return {
                    "departure_scenario": result,
                    "show_natural_opg_journeys": api_request.get("_show_natural_opg_journeys", False),
                }
            return {
                "arrival_scenario": result,
                "show_natural_opg_journeys": api_request.get("_show_natural_opg_journeys", False),
            }

        if not self.olympic_site_params:
            return {}

        departure_olympic_site_params = (
            self.get_dict_scenario(origin_olympic_site.uri, "departure_scenario", api_request["datetime"])
            if origin_olympic_site
            else {}
        )
        arrival_olympic_site_params = (
            self.get_dict_scenario(destination_olympic_site.uri, "arrival_scenario", api_request["datetime"])
            if destination_olympic_site
            else {}
        )

        if departure_olympic_site_params:
            conf_additional_parameters = self.get_dict_additional_parameters(
                origin_olympic_site.uri, "departure_scenario", api_request["datetime"]
            )
            return {
                "departure_scenario": departure_olympic_site_params,
                "additional_parameters": self.filter_and_get_additional_parameters(conf_additional_parameters),
                "strict": self.get_strict_parameter(origin_olympic_site.uri) if origin_olympic_site else False,
                "show_natural_opg_journeys": self.get_show_natural_opg_journeys(
                    conf_additional_parameters, api_request.get("_show_natural_opg_journeys")
                ),
            }
        if arrival_olympic_site_params:
            conf_additional_parameters = self.get_dict_additional_parameters(
                destination_olympic_site.uri, "arrival_scenario", api_request["datetime"]
            )
            return {
                "arrival_scenario": arrival_olympic_site_params,
                "additional_parameters": self.filter_and_get_additional_parameters(conf_additional_parameters),
                "strict": self.get_strict_parameter(destination_olympic_site.uri)
                if destination_olympic_site
                else False,
                "show_natural_opg_journeys": self.get_show_natural_opg_journeys(
                    conf_additional_parameters, api_request.get("_show_natural_opg_journeys")
                ),
            }
        return {}
