# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from jormungandr.interfaces.v1.serializer.api import GeoStatusSerializer
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.StatedResource import StatedResource
from flask_restful import abort


class GeoStatus(StatedResource):
    def __init__(self, *args, **kwargs):
        super(GeoStatus, self).__init__(output_type_serializer=GeoStatusSerializer, *args, **kwargs)

    @get_serializer(serpy=GeoStatusSerializer)
    def get(self, region=None, lon=None, lat=None):
        region_str = i_manager.get_region(region, lon, lat)
        instance = i_manager.instances[region_str]
        try:
            geo_status = instance.autocomplete.geo_status(instance)
            return {'geo_status': geo_status}, 200
        except NotImplementedError:
            abort(404, message="geo_status not implemented")

    def options(self, **kwargs):
        return self.api_description(**kwargs)
