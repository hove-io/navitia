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
from jormungandr import i_manager
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from navitiacommon.type_pb2 import _NAVITIATYPE
import datetime
from jormungandr.utils import is_coord, get_lon_lat
import six
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.serializer import api
import jormungandr


class Coord(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, output_type_serializer=api.DictAddressesSerializer, *args, **kwargs)
        self.parsers['get'].add_argument(
            "_autocomplete",
            type=six.text_type,
            hidden=True,
            help="name of the autocomplete service, used under the hood",
        )

    def _get_regions(self, region=None, lon=None, lat=None):
        return [region] if region else i_manager.get_regions("", lon, lat)

    def _response_from_place_uri(self, instance_name, args):
        response = i_manager.dispatch(args, "place_uri", instance_name=instance_name)
        if response.get('places', []):
            return {"address": response['places'][0]['address']}
        return None

    def _response_from_places_nearby(self, instance_name, args):
        response = i_manager.dispatch(args, "places_nearby", instance_name=instance_name)
        if len(response.places_nearby) > 0:
            e_type = response.places_nearby[0].embedded_type
            if _NAVITIATYPE.values_by_name["ADDRESS"].number == e_type:
                from jormungandr.interfaces.v1.serializer.api import PlacesNearbySerializer

                new_address = PlacesNearbySerializer(response).data
                return {"address": new_address["places_nearby"][0]["address"]}
        return None

    def _get_args(self, lon=None, lat=None):
        return {"uri": "{};{}".format(lon, lat), "_current_datetime": datetime.datetime.utcnow()}

    @get_serializer(serpy=api.DictAddressesSerializer)
    def get(self, region=None, lon=None, lat=None, id=None):
        if is_coord(id):
            lon, lat = get_lon_lat(id)

        params = self._get_args(lon=lon, lat=lat)

        args = self.parsers["get"].parse_args()
        args.update(params)
        self._register_interpreted_parameters(args)

        regions = self._get_regions(region=region, lon=lon, lat=lat)
        for r in regions:
            response = self._response_from_place_uri(r, args)
            if response:
                response.update({"regions": [r]})
                return response, 200

        return {"regions": regions, "message": "No address for these coords"}, 404

    def options(self, **kwargs):
        return self.api_description(**kwargs)
