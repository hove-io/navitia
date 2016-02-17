# coding=utf-8

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
from flask import logging
import pytz
import requests as requests
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy
from jormungandr.schedule import NextRTPassage
from datetime import datetime, time


def _to_duration(hour_str):
    t = datetime.strptime(hour_str, "%H:%M:%S")
    return time(hour=t.hour, minute=t.minute, second=t.second)


def _get_current_date():
    """
    encapsulate the current date in a method to be able to mock it
    """
    # Note: we use now() and not utc_now() because we want a local time, the same used by timeo
    return datetime.now()


class Timeo(RealtimeProxy):
    """
    class managing calls to timeo external service providing real-time next passages
    """

    def __init__(self, id, service_url, service_args, timezone, timeout=10):
        self.service_url = service_url
        self.service_args = service_args
        self.timeout = timeout  # timeout in seconds
        self.id = id

        # Note: if the timezone is not know, pytz raise an error
        self.timezone = pytz.timezone(timezone)

    def next_passage_for_route_point(self, route_point):
        url = self._make_url(route_point)
        if not url:
            return None

        r = requests.get(url, timeout=self.timeout)
        if r.status_code != 200:
            # TODO better error handling, the response might be in 200 but in error
            logging.getLogger(__name__).error('Timeo RT service unavailable, impossible to query : {}'
                                              .format(r.url))
            return None

        return self._get_passages(r.json())

    def _get_passages(self, timeo_resp):
        logging.getLogger(__name__).debug('timeo response: {}'.format(timeo_resp))

        st_responses = timeo_resp.get('StopTimesResponse')
        # by construction there should be only one StopTimesResponse
        if not st_responses or len(st_responses) != 1:
            logging.getLogger(__name__).warning('invalid timeo response: {}'.format(timeo_resp))
            return None

        next_st = st_responses[0]['NextStopTimesMessage']

        next_passages = []
        for next_expected_st in next_st.get('NextExpectedStopTime', []):
            # for the moment we handle only the NextStop
            dt = self._get_dt(next_expected_st['NextStop'])
            next_passage = NextRTPassage(dt)
            next_passages.append(next_passage)

        return next_passages

    def _make_url(self, route_point):
        """
        the route point identifier is set with the StopDescription argument
         this argument is split in 3 arguments (given between '?' and ';' symbol....)
         * StopTimeoCode: timeo code for the stop
         * LineTimeoCode: timeo code for the line
         * Way: 'A' if the route is forward, 'R' if it is backward
         2 additionals args are needed in this StopDescription ...:
         * NextStopTimeNumber: the number of next departure we want
         * StopTimeType: if we want base schedule data ('TH') or real time one ('TR')

         Note: since there are some strange symbol ('?' and ';') in the url we can't use param as dict in
         requests
         """

        base_params = '&'.join([k + '=' + v for k, v in self.service_args.iteritems()])

        stop = route_point.fetch_stop_id(self.id)
        line = route_point.fetch_line_id(self.id)
        route = route_point.fetch_route_id(self.id)

        if not all((stop, line, route)):
            # one a the id is missing, we'll not find any realtime
            logging.getLogger(__name__).debug('missing realtime id for {obj}: '
                                              'stop code={s}, line code={l}, route code={r}'.
                                              format(obj=route_point, s=stop, l=line, r=route))
            return None

        stop_id_url = ("StopDescription=?"
                       "StopTimeoCode={stop}"
                       "&LineTimeoCode={line}"
                       "&Way={route}"
                       "&NextStopTimeNumber={count}"
                       "&StopTimeType={data_freshness};").format(stop=stop,
                                                                 line=line,
                                                                 route=route,
                                                                 count='5',  # TODO better pagination
                                                                 data_freshness='TR')

        url = "{base_url}?{base_params}&{stop_id}".format(base_url=self.service_url,
                                                          base_params=base_params,
                                                          stop_id=stop_id_url)

        return url

    def _get_dt(self, hour_str):
        hour = _to_duration(hour_str)
        # we then have to complete the hour with the date to have a datetime
        now = _get_current_date()
        dt = datetime.combine(now.date(), hour)

        utc_dt = self.timezone.normalize(self.timezone.localize(dt)).astimezone(pytz.utc)

        return utc_dt
