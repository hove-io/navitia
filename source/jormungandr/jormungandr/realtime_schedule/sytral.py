# coding=utf-8

# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import, print_function, division
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy, RealtimeProxyError
import logging
import pybreaker
import pytz
import requests as requests
from jormungandr import cache, app
from jormungandr.schedule import RealTimePassage
import aniso8601


class Sytral(RealtimeProxy):
    """
    class managing calls to sytral-rt service providing real-time next passages

    curl example to check/test that external service is working:
    curl -X GET '{server}/departures?stop_id={stop_code}'
    {stop_code} is the code of type "source" for a stop_point
    So in practice it will look like:
    curl -X GET 'http://sytralrt/departures?stop_id=472'

    """

    def __init__(
        self,
        id,
        service_url,
        object_id_tag="source",
        destination_id_tag="source",
        instance=None,
        timeout=2,
        **kwargs
    ):
        self.service_url = service_url
        self.timeout = timeout  # timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag
        self.destination_id_tag = destination_id_tag
        self.instance = instance
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config.get('CIRCUIT_BREAKER_MAX_SYTRAL_FAIL', 5),
            reset_timeout=app.config.get('CIRCUIT_BREAKER_SYTRAL_TIMEOUT_S', 60),
        )

    def __repr__(self):
        """
         used as the cache key. We use the rt_system_id to share the cache between servers in production
        """
        try:
            return self.rt_system_id.encode('utf-8', 'backslashreplace')
        except:
            return self.rt_system_id

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_SYTRAL', 30))
    def _call(self, url):
        """
        http call to sytralRT
        """
        logging.getLogger(__name__).debug(
            'systralRT RT service , call url : {}'.format(url),
            extra={'rt_system_id': unicode(self.rt_system_id)},
        )
        try:
            return self.breaker.call(requests.get, url, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error(
                'systralRT service dead, using base ' 'schedule (error: {}'.format(e),
                extra={'rt_system_id': unicode(self.rt_system_id)},
            )
            raise RealtimeProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error(
                'systralRT service timeout, using base ' 'schedule (error: {}'.format(t),
                extra={'rt_system_id': unicode(self.rt_system_id)},
            )
            raise RealtimeProxyError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception(
                'systralRT RT error, using base schedule', extra={'rt_system_id': unicode(self.rt_system_id)}
            )
            raise RealtimeProxyError(str(e))

    def _make_url(self, route_point):
        """
        The url returns something like a departure on a stop point
        """

        stop_id_list = route_point.fetch_all_stop_id(self.object_id_tag)

        if not stop_id_list:
            logging.getLogger(__name__).debug(
                'missing realtime id for {obj}: stop code={s}'.format(obj=route_point, s=stop_id_list),
                extra={'rt_system_id': unicode(self.rt_system_id)},
            )
            self.record_internal_failure('missing id')
            return None

        url = self.service_url + "?stop_id={stop_id}".format(stop_id=stop_id_list[0])
        for stop_id in stop_id_list[1:]:
            url += "&stop_id={stop_id}".format(stop_id=stop_id)

        return url

    def _get_dt(self, datetime_str):
        dt = aniso8601.parse_datetime(datetime_str)

        utc_dt = dt.astimezone(pytz.utc)

        return utc_dt

    def _get_passages(self, route_point, resp):
        logging.getLogger(__name__).debug(
            'sytralrt response: {}'.format(resp), extra={'rt_system_id': unicode(self.rt_system_id)}
        )

        # One line navitia can be multiple lines on the SAE side
        line_ids = route_point.fetch_all_line_id(self.object_id_tag)

        departures = resp.get('departures', [])
        next_passages = []
        for next_expected_st in departures:
            if next_expected_st['line'] not in line_ids:
                continue
            dt = self._get_dt(next_expected_st['datetime'])
            direction = next_expected_st.get('direction_name')
            stop_id = next_expected_st.get('stop')
            is_real_time = next_expected_st.get('type') == 'E'
            next_passage = RealTimePassage(dt, direction, is_real_time)
            next_passages.append(next_passage)

        return next_passages

    def _get_next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=None
    ):
        url = self._make_url(route_point)
        if not url:
            return None
        r = self._call(url)

        if r.status_code != requests.codes.ok:
            logging.getLogger(__name__).error(
                'sytralrt service unavailable, impossible to query : {}'.format(r.url),
                extra={'rt_system_id': unicode(self.rt_system_id)},
            )
            raise RealtimeProxyError('non 200 response')

        return self._get_passages(route_point, r.json())

    def status(self):
        return {
            'id': unicode(self.rt_system_id),
            'timeout': self.timeout,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
        }
