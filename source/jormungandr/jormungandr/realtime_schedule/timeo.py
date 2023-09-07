# coding=utf-8

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
import logging
import pybreaker
import pytz
import requests as requests
from jormungandr import cache, app
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy, RealtimeProxyError, floor_datetime
from jormungandr.schedule import RealTimePassage
from datetime import datetime, time
from navitiacommon.ratelimit import RateLimiter, FakeRateLimiter
from jormungandr.utils import PY3
import six
from jormungandr import new_relic


def _to_duration(hour_str):
    t = datetime.strptime(hour_str, "%H:%M:%S")
    return time(hour=t.hour, minute=t.minute, second=t.second)


class Timeo(RealtimeProxy):
    """
    class managing calls to timeo external service providing real-time next passages


    curl example to check/test that external service is working:
    curl -X GET '{server}?serviceID={service}&EntityID={entity}&Media={spec}&StopDescription=?StopTimeoCode={stop_code}&NextStopTimeNumber={nb_desired}&LineTimeoCode={line_code}&Way={route_code}&StopTimeType=TR;'

    {line_code}, {route_code} and {stop_code} are provided using the same code key, named after
    the 'destination_id_tag' if provided on connector's init, or the 'id' otherwise.
    {route_code} is in pratice 'A' or 'R' (for 'aller' or 'retour', French for 'forth' or 'back').

    So in practice it will look like:
    curl -X GET 'http://bobimeo.fr/cgi/api_compagnon.cgi?serviceID=9&EntityID=289&Media=spec_navit_comp&StopDescription=?StopTimeoCode=1586&NextStopTimeNumber=10&LineTimeoCode=52&Way=A&StopTimeType=TR;'
    """

    def __init__(
        self,
        id,
        service_url,
        service_args,
        timezone,
        object_id_tag=None,
        destination_id_tag=None,
        instance=None,
        timeout=10,
        **kwargs
    ):
        self.service_url = service_url
        self.service_args = service_args
        self.timeout = timeout  # timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.destination_id_tag = destination_id_tag if destination_id_tag else "source"
        self.timeo_stop_code = kwargs.get("source_stop_code", "StopTimeoCode")
        self.timeo_line_code = kwargs.get("source_line_code", "LineTimeoCode")
        self.next_stop_time_number = kwargs.get("next_stop_time_number", 5)
        self.verify = kwargs.get("verify", True)

        self.instance = instance
        fail_max = kwargs.get(
            'circuit_breaker_max_fail', app.config.get(str('CIRCUIT_BREAKER_MAX_TIMEO_FAIL'), 5)
        )
        reset_timeout = kwargs.get(
            'circuit_breaker_reset_timeout', app.config.get(str('CIRCUIT_BREAKER_TIMEO_TIMEOUT_S'), 60)
        )
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)
        # A step is applied on from_datetime to discretize calls and allow caching them
        self.from_datetime_step = kwargs.get(
            'from_datetime_step', app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_TIMEO'), 60)
        )

        # Note: if the timezone is not know, pytz raise an error
        self.timezone = pytz.timezone(timezone)

        if kwargs.get('redis_host') and kwargs.get('rate_limit_count'):
            self.rate_limiter = RateLimiter(
                conditions=[
                    {'requests': kwargs.get('rate_limit_count'), 'seconds': kwargs.get('rate_limit_duration', 1)}
                ],
                redis_host=kwargs.get('redis_host'),
                redis_port=kwargs.get('redis_port', 6379),
                redis_db=kwargs.get('redis_db', 0),
                redis_password=kwargs.get('redis_password'),
                redis_namespace=kwargs.get('redis_namespace', 'jormungandr.rate_limiter'),
            )
        else:
            self.rate_limiter = FakeRateLimiter()

        # We consider that all errors, greater than or equal to 100, are blocking
        self.INTERNAL_TIMEO_ERROR_CODE_LIMIT = 100

    def __repr__(self):
        """
        used as the cache key. we use the rt_system_id to share the cache between servers in production
        """
        if PY3:
            return self.rt_system_id
        try:
            return self.rt_system_id.encode('utf-8', 'backslashreplace')
        except:
            return self.rt_system_id

    def _is_valid_direction(self, direction_uri, passage_direction_uri):
        return direction_uri == passage_direction_uri

    @new_relic.distributedEvent("call_timeo", "timeo")
    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_TIMEO'), 60))
    def _call_timeo(self, url):
        """
        http call to timeo

        The call is handled by a circuit breaker not to continue calling timeo if the service is dead.

        The call is also cached
        """
        try:
            if not self.rate_limiter.acquire(self.rt_system_id, block=False):
                return None
            return self.breaker.call(requests.get, url, timeout=self.timeout, verify=self.verify)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error(
                'Timeo RT service dead, using base schedule (error: {}'.format(e),
                extra={'rt_system_id': six.text_type(self.rt_system_id)},
            )
            raise RealtimeProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error(
                'Timeo RT service timeout, using base schedule (error: {}'.format(t),
                extra={'rt_system_id': six.text_type(self.rt_system_id)},
            )
            raise RealtimeProxyError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception(
                'Timeo RT error, using base schedule', extra={'rt_system_id': six.text_type(self.rt_system_id)}
            )
            raise RealtimeProxyError(str(e))

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

    def _get_next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=None
    ):
        if self._is_tomorrow(from_dt, current_dt):
            logging.getLogger(__name__).info(
                'Timeo RT service , Can not call Timeo for tomorrow.',
                extra={'rt_system_id': six.text_type(self.rt_system_id)},
            )
            return None
        url = self._make_url(route_point, count, from_dt)
        if not url:
            return None
        logging.getLogger(__name__).debug(
            'Timeo RT service , call url : {}'.format(url),
            extra={'rt_system_id': six.text_type(self.rt_system_id)},
        )
        r = self._call_timeo(url)
        return self._get_passages(r, current_dt, route_point.fetch_line_uri())

    def _get_passages(self, response, current_dt, line_uri=None):
        status_code = response.status_code
        # Handling http error
        if status_code != 200:
            logging.getLogger(__name__).error(
                'Timeo RT service unavailable, impossible to query : {}'.format(response.url),
                extra={'rt_system_id': six.text_type(self.rt_system_id), 'status_code': status_code},
            )
            raise RealtimeProxyError('non 200 response')

        timeo_resp = response.json()
        logging.getLogger(__name__).debug(
            'timeo response: {}'.format(timeo_resp), extra={'rt_system_id': six.text_type(self.rt_system_id)}
        )

        # internal timeo error handling
        message_responses = timeo_resp.get('MessageResponse')
        for message_response in message_responses:
            if (
                'ResponseCode' in message_response
                and int(message_response['ResponseCode']) >= self.INTERNAL_TIMEO_ERROR_CODE_LIMIT
            ):
                resp_code = message_response['ResponseCode']
                if 'ResponseComment' in message_response:
                    resp_comment = message_response['ResponseComment']
                else:
                    resp_comment = ''
                self.record_internal_failure(
                    'Timeo RT internal service error',
                    'ResponseCode: {} - ResponseComment: {}'.format(resp_code, resp_comment),
                )
                timeo_internal_error_message = (
                    'Timeo RT internal service error, ResponseCode: {} - ResponseComment: {}'.format(
                        resp_code, resp_comment
                    )
                )
                logging.getLogger(__name__).error(timeo_internal_error_message)
                raise RealtimeProxyError(timeo_internal_error_message)

        st_responses = timeo_resp.get('StopTimesResponse')
        # by construction there should be only one StopTimesResponse
        if not st_responses or len(st_responses) != 1:
            logging.getLogger(__name__).warning(
                'invalid timeo response: {}'.format(timeo_resp),
                extra={'rt_system_id': six.text_type(self.rt_system_id)},
            )
            raise RealtimeProxyError('invalid response')

        next_stoptimes_message = st_responses[0]['NextStopTimesMessage']
        next_st = next_stoptimes_message.get("NextExpectedStopTime", [])
        new_next_st = [n_st for n_st in next_st if n_st.get("is_realtime", True)]
        if not len(new_next_st) and len(next_st):
            return None
        next_passages = []
        for next_expected_st in new_next_st:
            # for the moment we handle only the NextStop and the direction
            dt = self._get_dt(next_expected_st['NextStop'], current_dt)
            direction = self._get_direction(
                line_uri=line_uri,
                object_code=next_expected_st.get('TerminusSAECode'),
                default_value=next_expected_st.get('Destination'),
            )
            next_passage = RealTimePassage(dt, direction.label, direction_uri=direction.uri)
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
            logging.getLogger(__name__).debug(
                'missing realtime id for {obj}: '
                'stop code={s}, line code={l}, route code={r}'.format(obj=route_point, s=stop, l=line, r=route),
                extra={'rt_system_id': six.text_type(self.rt_system_id)},
            )
            self.record_internal_failure('missing id')
            return None

        # timeo can only handle items_per_schedule if it's < 10
        count = min(count or self.next_stop_time_number, 10)  # if no value defined we ask for 10 passages

        # if a custom datetime is provided we give it to timeo but we round it to improve cachability
        dt_param = (
            '&NextStopReferenceTime={dt}'.format(
                dt=floor_datetime(self._timestamp_to_date(from_dt), self.from_datetime_step).strftime(
                    '%Y-%m-%dT%H:%M:%S'
                )
            )
            if from_dt
            else ''
        )

        # We want to have StopTimeType as it make parsing of the request way easier
        # for alternative implementation of timeo since we can ignore this params
        stop_id_url = (
            "StopDescription=?"
            "StopTimeType={data_freshness}"
            "&{line_timeo_code}={line}"
            "&Way={route}"
            "&NextStopTimeNumber={count}"
            "&{stop_timeo_code}={stop}{dt};"
        ).format(
            stop=stop,
            line=line,
            route=route,
            count=count,
            data_freshness='TR',
            dt=dt_param,
            stop_timeo_code=self.timeo_stop_code,
            line_timeo_code=self.timeo_line_code,
        )

        url = "{base_url}?{base_params}&{stop_id}".format(
            base_url=self.service_url, base_params=base_params, stop_id=stop_id_url
        )

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
        return {
            'id': six.text_type(self.rt_system_id),
            'timeout': self.timeout,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
        }

    def __eq__(self, other):
        return self.rt_system_id == other.rt_system_id
