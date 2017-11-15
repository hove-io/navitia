# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
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
from flask.ext.restful import abort, marshal
from jormungandr import i_manager
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.v1.fields import address
from navitiacommon.type_pb2 import _NAVITIATYPE
from collections import OrderedDict
import datetime
from jormungandr.utils import is_coord, get_lon_lat


class Coord(ResourceUri):
    def _get_regions(self, region=None, lon=None, lat=None):
        return [region] if region else i_manager.get_regions("", lon, lat)

    def _get_args(self, lon=None, lat=None, id=None):
        args = {
            "uri": "{};{}".format(lon, lat),
            "_current_datetime": datetime.datetime.utcnow()
        }
        if all([lon, lat, is_coord(id)]):
            return args
        args.update({
            "count": 1,
            "distance": 200,
            "type[]": ["address"],
            "depth": 1,
            "start_page": 0,
            "filter": ""
        })
        return args

    def get(self, region=None, lon=None, lat=None, id=None, *args, **kwargs):

        if is_coord(id):
            lon, lat = get_lon_lat(id)

        regions = self._get_regions(region=region, lon=lon, lat=lat)

        result = OrderedDict()

        args = self._get_args(id=id, lon=lon, lat=lat)
        self._register_interpreted_parameters(args)
        for r in regions:
            self.region = r
            result.update(regions=[r])
            if all([lon, lat, is_coord(id)]):
                response = i_manager.dispatch(args, "place_uri", instance_name=r)
                if response.get('places', []):
                    result.update({"regions": [r], "address": response['places'][0]['address']})
                    return result, 200
            else:
                response = i_manager.dispatch(args, "places_nearby", instance_name=r)
                if len(response.places_nearby) > 0:
                    e_type = response.places_nearby[0].embedded_type
                    if _NAVITIATYPE.values_by_name["ADDRESS"].number == e_type:
                        new_address = marshal(response.places_nearby[0].address,
                                              address)
                        result.update(address=new_address)
                        return result, 200
        result.update({"regions": regions, "message": "No address for these coords"})
        return result, 404
