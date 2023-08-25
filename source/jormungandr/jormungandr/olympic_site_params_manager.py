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

from __future__ import absolute_import, print_function, unicode_literals, division
import os
import logging
import json
from collections import namedtuple
from jormungandr.utils import get_olympic_site


AttractivityVirtualFallback = namedtuple("AttractivityVirtualFallback", "attractivity, virtual_duration")


class OlympicSiteParamsManager:
    def __init__(self, file_path, instance_name):
        # {"poi:BCD":{"departure":"default","arrival":"default",
        # "scenarios":{"default":{"stop_point:463685":{"attractivity":1,"virtual_fallback":10},
        # "stop_point:463686":{"attractivity":3,"virtual_fallback":150}}}}}
        self.olympic_site_params = dict()
        self.load_olympic_site_params(file_path, instance_name)

    def build_olympic_site_params(self, scenario, data):
        if not scenario:
            return {}
        return {
            spt_id: AttractivityVirtualFallback(d["attractivity"], d["virtual_fallback"])
            for spt_id, d in data.get("scenarios", {}).get(scenario, {}).items()
        }

    def get_departure_olympic_site_params(self, poi_uri):
        data = self.olympic_site_params.get(poi_uri)
        if not data:
            return {}
        return self.build_olympic_site_params(data.get("departure"), data)

    def get_arrival_olympic_site_params(self, poi_uri):
        data = self.olympic_site_params.get(poi_uri)
        if not data:
            return {}
        return self.build_olympic_site_params(data.get("arrival"), data)

    def load_olympic_site_params(self, file_path, instance_name):
        logger = logging.getLogger(__name__)
        if not file_path:
            logger.warning("Reading stop points attractivities, path does not found")
            return
        olympic_site_params_file = os.path.join(file_path, "{}.json".format(instance_name))
        if not os.path.exists(olympic_site_params_file):
            logger.warning(
                "Reading stop points attractivities, file: %s does not exist", olympic_site_params_file
            )
            return

        logger.info("Reading stop points attractivities from file: %s", olympic_site_params_file)
        try:
            with open(olympic_site_params_file) as f:
                self.olympic_site_params = json.load(f)
        except Exception as e:
            logger.exception(
                'Error while loading od_allowed_ids file: {} with exception: {}'.format(
                    olympic_site_params_file, str(e)
                )
            )

    def get_olympic_site_params(self, pt_origin_detail, pt_destination_detail, api_request, instance):
        attractivities = dict()
        virtual_duration = dict()
        attractivities.update(api_request.get('_olympics_sites_attractivities[]') or [])
        virtual_duration.update(api_request.get('_olympics_sites_virtual_fallback[]') or [])
        if attractivities:
            result = dict()
            for spt_id, attractivity in attractivities.items():
                virtual_fallback = virtual_duration.get(spt_id, 0)
                result[spt_id] = AttractivityVirtualFallback(attractivity, virtual_fallback)
            if api_request.get("criteria") == "departure_stop_attractivity":
                return {"departure": result}
            return {"arrival": result}

        origin_olympic_site = get_olympic_site(pt_origin_detail, instance)
        destination_olympic_site = get_olympic_site(pt_destination_detail, instance)

        if origin_olympic_site and destination_olympic_site:
            origin_olympic_site = None

        departure_olympic_site_params = (
            self.get_departure_olympic_site_params(origin_olympic_site.uri) if origin_olympic_site else {}
        )
        arrival_olympic_site_params = (
            self.get_arrival_olympic_site_params(destination_olympic_site.uri)
            if destination_olympic_site
            else {}
        )

        if departure_olympic_site_params:
            return {"departure": departure_olympic_site_params}
        if arrival_olympic_site_params:
            return {"arrival": arrival_olympic_site_params}
        return {}
