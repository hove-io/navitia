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
from jormungandr import i_manager, bss_provider_manager
from functools import wraps
import logging

class ManageStands(object):

    def __init__(self, resource, attribute):
        """
        resource: the element to apply the decorator
        attribute: the attribute name containing the list (pois, places, places_nearby, journeys)
        """
        self.resource = resource
        self.attribute = attribute

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            response, status, e = f(*args, **kwargs)
            if status == 200 and self.attribute in response:
                instance = i_manager.instances.get(self.resource.region)
                if instance and instance.bss_provider:
                    handle_bss = bss_provider_manager.handle_journeys if self.attribute == 'journeys' \
                        else bss_provider_manager.handle_places
                    try:
                        response[self.attribute] = handle_bss(response[self.attribute])
                    except:
                        logger = logging.getLogger(__name__)
                        logger.exception('Error while handling BSS realtime availability')
            return response, status, e
        return wrapper
