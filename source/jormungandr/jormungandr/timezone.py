# encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
import logging
import pytz
from flask import g, request
from jormungandr.exceptions import TechnicalError, RegionNotFound

def set_request_timezone(region):
    from jormungandr import i_manager
    instance = i_manager.instances.get(region, None)
    if not instance:
        raise RegionNotFound(region)
    set_request_instance_timezone(instance)

def set_request_instance_timezone(instance):
    logger = logging.getLogger(__name__)

    if not instance:
        raise RegionNotFound()

    if not instance.timezone:
        logger.warn("region {} has no timezone".format(instance.name))
        try:
            request.timezone = None
        except RuntimeError:
            pass#working outside application context...
        return

    tz = pytz.timezone(instance.timezone)

    if not tz:
        logger.warn("impossible to find timezone: '{}' for region {}".format(instance.timezone, instance.name))

    try:
        request.timezone = tz

    except RuntimeError:
        pass#working outside of an application context...

import functools32


@functools32.lru_cache(1)
def get_timezone(req_id):
    """
    return the time zone associated with the query

    Note: for the moment ONLY ONE time zone is used for a region (a kraken instances)
    It is this default timezone that is returned in this method
    """
    if not hasattr(request, 'timezone'):
        raise TechnicalError("No timezone set for this API")  # the set_request_timezone has to be called at init
    return request.timezone


