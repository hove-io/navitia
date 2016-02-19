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

from datetime import datetime, timedelta
from operator import attrgetter


def extremes(resp, request):
    if len(resp.journeys) == 0:
        return None, None

    asap_journey = min([journey for journey in
                        resp.journeys if journey.arrival_date_time is not None],
                       key=attrgetter('arrival_date_time'))
    query_args = ""
    for arg in request.args:
        if arg != "datetime" and arg != "clockwise":
            if isinstance(request.args.get(arg), type([])):
                for v in request.args.get(arg):
                    query_args += arg + "=" + unicode(v) + "&"
            else:
                query_args += arg + "=" + unicode(request.args.get(arg)) + "&"

    before = after = None
    if asap_journey.arrival_date_time and asap_journey.departure_date_time:
        minute = 60
        datetime_after = asap_journey.departure_date_time + minute
        d_after = datetime.utcfromtimestamp(datetime_after)
        after = query_args + "clockwise=true&datetime=" + d_after.strftime("%Y%m%dT%H%M%S")

        datetime_before = asap_journey.arrival_date_time - minute
        d_before = datetime.utcfromtimestamp(datetime_before)
        before = query_args + "clockwise=false&datetime="

        before += d_before.strftime("%Y%m%dT%H%M%S")

    return before, after
