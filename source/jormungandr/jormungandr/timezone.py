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
from flask.globals import current_app
import pytz
from flask import g
from jormungandr.exceptions import TechnicalError, RegionNotFound
from jormungandr import i_manager


def set_request_timezone(region):
    instance = i_manager.instances.get(region, None)

    if not instance:
        raise RegionNotFound(region)

    if not instance.timezone:
        current_app.logger.warn("region {} hos no timezone".format(region))
        g.timezone = None
        return

    tz = pytz.timezone(instance.timezone)

    if not tz:
        current_app.logger.warn("impossible to find timezone: '{}' for region {}".format(instance.timezone, region))

    g.timezone = tz


def get_timezone():
    """
    return the time zone associated with the query

    Note: for the moment ONLY ONE time zone is used for a region (a kraken instances)
    It is this default timezone that is returned in this method
    """
    if not hasattr(g, 'timezone'):
        raise TechnicalError("No timezone set for this API")  # the set_request_timezone has to be called at init
    return g.timezone


