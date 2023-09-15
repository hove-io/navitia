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
from flask_restful.utils import unpack
from jormungandr import i_manager, bss_provider_manager, car_park_provider_manager
from functools import wraps
import logging


def _add_feed_publisher(response, providers):
    """
    add the feed publisher of the providers to the global feed_publishers of the response
    """
    feeds = response.get('feed_publishers', [])
    for p in providers or []:
        f = p.feed_publisher()
        if f:
            feeds.append(f)


def _handle(response, provider_manager, attr, logger, err_msg):
    try:
        providers = provider_manager.handle(response, attr)
        _add_feed_publisher(response, providers)
    except:
        logger.exception(err_msg)


class ManageParkingPlaces(object):
    def __init__(self, resource, attribute):
        """
        resource: the element to apply the decorator
        attribute: the attribute name containing the list (pois, places, places_nearby, journeys)
        """
        self.resource = resource
        self.attribute = attribute
        self.logger = logging.getLogger(__name__)

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            response, status, h = unpack(f(*args, **kwargs))
            if status == 200 and self.attribute in response:
                instance = i_manager.instances.get(self.resource.region)

                resource_args = self.resource.parsers["get"].parse_args()

                show_bss_stands = 'bss_stands' in resource_args.get('add_poi_infos')
                show_car_park = 'car_park' in resource_args.get('add_poi_infos')

                if show_bss_stands and instance and instance.bss_provider:
                    _handle(
                        response,
                        bss_provider_manager,
                        self.attribute,
                        self.logger,
                        'Error while handling global BSS realtime availability',
                    )

                if show_bss_stands and instance and instance.bss_provider_manager and instance.bss_provider_manager.exist_provider():
                    _handle(
                        response,
                        instance.bss_provider_manager,
                        self.attribute,
                        self.logger,
                        'Error while handling individual BSS realtime availability',
                    )

                if show_car_park and instance and instance.car_park_provider:
                    _handle(
                        response,
                        car_park_provider_manager,
                        self.attribute,
                        self.logger,
                        'Error while handling global car park realtime availability',
                    )

            return response, status, h

        return wrapper
