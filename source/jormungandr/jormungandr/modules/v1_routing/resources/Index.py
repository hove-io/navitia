# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from __future__ import absolute_import, print_function, unicode_literals, division

from flask import url_for
from jormungandr.interfaces.v1.make_links import create_external_link
from flask_restful import marshal
from jormungandr._version import __version__
from jormungandr.exceptions import DeadSocketException
from jormungandr.module_resource import ModuleResource
from navitiacommon import type_pb2, request_pb2
from jormungandr import i_manager
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.interfaces.v1 import fields


class Index(ModuleResource):
    def get(self):
        response = {
            "links": [
                create_external_link(self.module_name + '.coverage',
                                     rel='coverage',
                                     description='Coverage of navitia'),
                # TODO find a way to display {long:lat} in the url
                create_external_link(self.module_name + '.coord', rel='coord',
                                     templated=True,
                                     description='Inverted geocoding for a given coordinate',
                                     lon=.0, lat=.0),
                create_external_link(self.module_name + '.journeys',
                                     rel='journeys',
                                     description='Compute journeys'),
                create_external_link(self.module_name + '.places',
                                     rel='places',
                                     description='Compute places'),
            ]
        }
        return response, 200


class TechnicalStatus(ModuleResource):
    """
    Status is mainly used for supervision

    return status for all instances
    """

    def get(self):
        response = {
            "jormungandr_version": __version__,
            "regions": []
        }
        regions = i_manager.get_regions()
        for key_region in regions:
            req = request_pb2.Request()

            req.requested_api = type_pb2.STATUS
            try:
                resp = i_manager.instances[key_region].send_and_receive(req,
                                                                        timeout=1000)

                raw_resp_dict = protobuf_to_dict(resp, use_enum_labels=True)

                resp_dict = marshal(raw_resp_dict['status'],
                                    fields.instance_status)
            except DeadSocketException:
                resp_dict = {
                    "status": "dead",
                    "error": {
                        "code": "dead_socket",
                        "value": "The region {} is dead".format(key_region)
                    }
                }
            resp_dict['region_id'] = key_region
            response['regions'].append(resp_dict)

        return response
