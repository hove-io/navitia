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
from __future__ import absolute_import, print_function, unicode_literals, division
from flask import logging
import pybreaker
import pytz
import requests as requests
from jormungandr import cache, app
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy
from jormungandr.schedule import RealTimePassage
from datetime import datetime, time
from navitiacommon.ratelimit import RateLimiter, FakeRateLimiter


def _to_duration(hour_str):
    t = datetime.strptime(hour_str, "%H:%M:%S")
    return time(hour=t.hour, minute=t.minute, second=t.second)


class Timeo(RealtimeProxy):
    """
    class managing calls to timeo external service providing real-time next passages
    """

    def __init__(self, id, service_url, service_args, timezone,
                 object_id_tag=None, destination_id_tag=None, instance=None, timeout=10, **kwargs):
        self.service_url = service_url
        self.service_args = service_args
        self.timeout = timeout  #timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.destination_id_tag = destination_id_tag
        self.instance = instance
        fail_max = kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_TIMEO_FAIL'])
        reset_timeout = kwargs.get('circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_TIMEO_TIMEOUT_S'])
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)

        # Note: if the timezone is not know, pytz raise an error
        self.timezone = pytz.timezone(timezone)

        if kwargs.get('redis_host') and kwargs.get('rate_limit_count'):
            self.rate_limiter = RateLimiter(conditions=[{'requests': kwargs.get('rate_limit_count'),
                                                         'seconds': kwargs.get('rate_limit_duration', 1)}],
                                            redis_host=kwargs.get('redis_host'),
                                            redis_port=kwargs.get('redis_port', 6379),
                                            redis_db=kwargs.get('redis_db', 0),
                                            redis_password=kwargs.get('redis_password'),
                                            redis_namespace=kwargs.get('redis_namespace', 'jormungandr.rate_limiter'))
        else:
            self.rate_limiter = FakeRateLimiter()


    def __repr__(self):
        """
         used as the cache key. we use the rt_system_id to share the cache between servers in production
        """
        return self.rt_system_id

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_TIMEO', 60))
    def _call_timeo(self, url):
        """
        http call to timeo

        The call is handled by a circuit breaker not to continue calling timeo if the service is dead.

        The call is also cached
        """
        try:
            if not self.rate_limiter.acquire(self.rt_system_id, block=False):
                return None
            return self.breaker.call(requests.get, url, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Timeo RT service dead, using base '
                                              'schedule (error: {}'.format(e))
            self.record_external_failure('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Timeo RT service timeout, using base '
                                              'schedule (error: {}'.format(t))
            self.record_external_failure('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('Timeo RT error, using base schedule')
            self.record_external_failure(str(e))
        return None

    def _get_dt_local(self, utc_dt):
        return pytz.utc.localize(utc_dt).astimezone(self.timezone)

    def _is_tomorrow(self, request_dt, current_dt):
        if not request_dt:
            return False
        if not current_dt:
            now = self._get_dt_local(datetime.utcnow())
        else:
            now = self._get_dt_local(current_dt)
        req_dt = self._timestamp_to_date(request_dt)
        return now.date() < req_dt.date()

    def _get_next_passage_for_route_point(self, route_point, count=None, from_dt=None, current_dt=None):
        if self._is_tomorrow(from_dt, current_dt):
            logging.getLogger(__name__).info('Timeo RT service , Can not call Timeo for tomorrow.')
            return None
        url = self._make_url(route_point, count, from_dt)
        if not url:
            return None
        logging.getLogger(__name__).debug('Timeo RT service , call url : {}'.format(url))
        r = self._call_timeo(url)
        if not r:
            return None

        if r.status_code != 200:
            # TODO better error handling, the response might be in 200 but in error
            logging.getLogger(__name__).error('Timeo RT service unavailable, impossible to query : {}'
                                              .format(r.url))
            self.record_external_failure('non 200 response')
            return None

        return self._get_passages(r.json(), current_dt, route_point.fetch_line_uri())

    def _get_passages(self, timeo_resp, current_dt, line_uri=None):
        logging.getLogger(__name__).debug('timeo response: {}'.format(timeo_resp))

        st_responses = timeo_resp.get('StopTimesResponse')
        # by construction there should be only one StopTimesResponse
        if not st_responses or len(st_responses) != 1:
            logging.getLogger(__name__).warning('invalid timeo response: {}'.format(timeo_resp))
            self.record_external_failure('invalid response')
            return None

        next_st = st_responses[0]['NextStopTimesMessage']

        next_passages = []
        for next_expected_st in next_st.get('NextExpectedStopTime', []):
            # for the moment we handle only the NextStop and the direction
            dt = self._get_dt(next_expected_st['NextStop'], current_dt)
            direction = self._get_direction_name(line_uri=line_uri,
                                                 object_code=next_expected_st.get('Terminus'),
                                                 default_value=next_expected_st.get('Destination'))
            next_passage = RealTimePassage(dt, direction)
            next_passages.append(next_passage)

        return next_passages

    def _make_url(self, route_point, count=None, from_dt=None):
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

        base_params = '&'.join([k + '=' + v for k, v in self.service_args.items()])

        stop = route_point.fetch_stop_id(self.object_id_tag)
        line = route_point.fetch_line_id(self.object_id_tag)
        route = route_point.fetch_route_id(self.object_id_tag)

        if not all((stop, line, route)):
            # one a the id is missing, we'll not find any realtime
            logging.getLogger(__name__).debug('missing realtime id for {obj}: '
                                              'stop code={s}, line code={l}, route code={r}'.
                                              format(obj=route_point, s=stop, l=line, r=route))
            self.record_internal_failure('missing id')
            return None

        # timeo can only handle items_per_schedule if it's < 5
        count = min(count or 5, 5)# if no value defined we ask for 5 passages

        # if a custom datetime is provided we give it to timeo
        dt_param = '&NextStopReferenceTime={dt}'\
            .format(dt=self._timestamp_to_date(from_dt).strftime('%Y-%m-%dT%H:%M:%S')) \
            if from_dt else ''

        stop_id_url = ("StopDescription=?"
                       "StopTimeoCode={stop}"
                       "&LineTimeoCode={line}"
                       "&Way={route}"
                       "&NextStopTimeNumber={count}"
                       "&StopTimeType={data_freshness}{dt};").format(stop=stop,
                                                                 line=line,
                                                                 route=route,
                                                                 count=count,
                                                                 data_freshness='TR',
                                                                 dt=dt_param)

        url = "{base_url}?{base_params}&{stop_id}".format(base_url=self.service_url,
                                                          base_params=base_params,
                                                          stop_id=stop_id_url)

        return url

    def _get_dt(self, hour_str, current_dt):
        hour = _to_duration(hour_str)
        # we then have to complete the hour with the date to have a datetime
        now = current_dt
        dt = datetime.combine(now.date(), hour)

        utc_dt = self.timezone.normalize(self.timezone.localize(dt)).astimezone(pytz.utc)

        return utc_dt

    def _timestamp_to_date(self, timestamp):
        dt = datetime.utcfromtimestamp(timestamp)
        return self._get_dt_local(dt)

    def status(self):
        return {'id': self.rt_system_id,
                'timeout': self.timeout,
                'circuit_breaker': {'current_state': self.breaker.current_state,
                                    'fail_counter': self.breaker.fail_counter,
                                    'reset_timeout': self.breaker.reset_timeout},
                }

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_PTOBJECTS', 600))
    def _get_direction_name(self, line_uri, object_code, default_value):
        stop_point = self.instance.ptref.get_stop_point(line_uri, self.destination_id_tag, object_code)

        if stop_point and stop_point.HasField('stop_area') \
                and stop_point.stop_area.HasField('label') \
                and stop_point.stop_area.label != '':
            return stop_point.stop_area.label
        return default_value
