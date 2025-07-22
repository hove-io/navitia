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
import itertools

from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy, RealtimeProxyError
from jormungandr.schedule import RealTimePassage
from jormungandr.utils import PY3
import xml.etree.ElementTree as et
import pytz
import logging
import pybreaker
import requests as requests
from jormungandr import cache, app
from datetime import datetime
from navitiacommon.ratelimit import RateLimiter, FakeRateLimiter
from navitiacommon import type_pb2
from navitiacommon.parser_args_type import DateTimeFormat
import redis
import six


class SyntheseRoutePoint(object):
    def __init__(self, rt_id=None, sp_id=None, line_id=None):
        self.syn_route_id = rt_id
        self.syn_stop_point_id = sp_id
        self.syn_line_id = line_id

    def __key(self):
        return (self.syn_route_id, self.syn_stop_point_id)

    def __hash__(self):
        return hash(self.__key())

    def __eq__(self, other):
        return self.__key() == other.__key()

    def __repr__(self):
        return "SyntheseRoutePoint({})".format(self.__key())


class Synthese(RealtimeProxy):
    """
    class managing calls to timeo external service providing real-time next passages


    curl example to check/test that external service is working:
    curl -X GET '{server}?SERVICE=tdg&roid={stop_code}&rn={nb_desired}&date={datetime}'

    {nb_desired} and {datetime} can be empty
    {datetime} is on using format '%Y-%m-%d %H-%M' which is urlencoded (' ' > '%20')

    On the response, Navitia matches route-point's {route_codes} (see details in _find_route_point_passages()).
    {route_codes} and {stop_code} are provided using the same code key, named after
    the 'destination_id_tag' if provided on connector's init, or the 'id' otherwise.

    In practice it will look like:
    curl -X GET 'http://bobito.fr/?SERVICE=tdg&roid=68435211116990230&rn=5&date=2018-06-11%2011:13'
    """

    def __init__(
        self,
        id,
        service_url,
        timezone,
        object_id_tag=None,
        destination_id_tag=None,
        instance=None,
        timeout=10,
        redis_host=None,
        redis_db=0,
        redis_port=6379,
        redis_password=None,
        max_requests_by_second=15,
        redis_namespace='jormungandr.rate_limiter',
        **kwargs
    ):
        self.service_url = service_url
        self.timeout = timeout  # timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.destination_id_tag = destination_id_tag
        self.instance = instance

        fail_max = kwargs.get(
            'circuit_breaker_max_fail', app.config.get(str('CIRCUIT_BREAKER_MAX_SYNTHESE_FAIL'), 5)
        )
        reset_timeout = kwargs.get(
            'circuit_breaker_reset_timeout', app.config.get(str('CIRCUIT_BREAKER_SYNTHESE_TIMEOUT_S'), 60)
        )
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)
        self.timezone = pytz.timezone(timezone)
        if not redis_host:
            self.rate_limiter = FakeRateLimiter()
        else:
            self.rate_limiter = RateLimiter(
                conditions=[{'requests': max_requests_by_second, 'seconds': 1}],
                redis_host=redis_host,
                redis_port=redis_port,
                redis_db=redis_db,
                redis_password=redis_password,
                redis_namespace=redis_namespace,
            )

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

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_SYNTHESE'), 30))
    def _call_synthese(self, url):
        """
        http call to synthese
        """
        try:
            if not self.rate_limiter.acquire(self.rt_system_id, block=False):
                raise RealtimeProxyError('maximum rate reached')
            return self.breaker.call(requests.get, url, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error(
                'Synthese RT service dead, using base schedule (error: {}'.format(e)
            )
            raise RealtimeProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error(
                'Synthese RT service timeout, using base schedule (error: {}'.format(t)
            )
            raise RealtimeProxyError('timeout')
        except redis.ConnectionError:
            logging.getLogger(__name__).exception('there is an error with Redis')
            raise RealtimeProxyError('redis error')
        except Exception as e:
            logging.getLogger(__name__).exception('Synthese RT error, using base schedule')
            raise RealtimeProxyError(str(e))

    def _get_next_passage_for_route_point(
        self, route_point, count=None, from_dt=None, current_dt=None, duration=None
    ):
        url = self._make_url(route_point, count, from_dt)
        if not url:
            return None
        logging.getLogger(__name__).debug('Synthese RT service , call url : {}'.format(url))
        r = self._call_synthese(url)
        if not r:
            return None

        if r.status_code != 200:
            # TODO better error handling, the response might be in 200 but in error
            logging.getLogger(__name__).error(
                'Synthese RT service unavailable, impossible to query : {}'.format(r.url)
            )
            raise RealtimeProxyError('non 200 response')

        logging.getLogger(__name__).debug("synthese response: {}".format(r.text))
        passages = self._get_synthese_passages(r.content)

        return self._find_route_point_passages(route_point, passages)

    def _make_url(self, route_point, count=None, from_dt=None):
        """
        The url returns something like a departure on a stop point
        """

        stop_id = route_point.fetch_stop_id(self.object_id_tag)

        if not stop_id:
            # one a the id is missing, we'll not find any realtime
            logging.getLogger(__name__).debug(
                'missing realtime id for {obj}: stop code={s}'.format(obj=route_point, s=stop_id)
            )
            self.record_internal_failure('missing id')
            return None

        count_param = '&rn={c}'.format(c=count) if count else ''

        # if a custom datetime is provided we give it to timeo
        dt_param = (
            '&date={dt}'.format(dt=self._timestamp_to_date(from_dt).strftime('%Y-%m-%d %H:%M'))
            if from_dt
            else ''
        )

        url = "{base_url}?SERVICE=tdg&roid={stop_id}{count}{date}".format(
            base_url=self.service_url, stop_id=stop_id, count=count_param, date=dt_param
        )

        return url

    def _get_value(self, item, xpath, val):
        value = item.find(xpath)
        if value is None:
            logging.getLogger(__name__).debug("Path not found: {path}".format(path=xpath))
            return None
        return value.get(val)

    def _get_real_time_passage(self, xml_journey):
        """
        :return RealTimePassage: object real time passage
        :param xml_journey: journey information
        exceptions :
            ValueError: Unable to parse datetime, day is out of range for month (for example)
        """
        dt = DateTimeFormat()(xml_journey.get('dateTime'))
        utc_dt = self.timezone.normalize(self.timezone.localize(dt)).astimezone(pytz.utc)
        passage = RealTimePassage(utc_dt)
        passage.is_real_time = xml_journey.get('realTime') == 'yes'
        return passage

    @staticmethod
    def _build(xml):
        try:
            root = et.fromstring(xml)
        except et.ParseError as e:
            logging.getLogger(__name__).error("invalid xml: {}".format(e))
            raise
        for xml_journey in root.findall('journey'):
            yield xml_journey

    def _get_synthese_passages(self, xml):
        result = {}
        for xml_journey in self._build(xml):
            route_point = SyntheseRoutePoint(
                xml_journey.get('routeId'),
                self._get_value(xml_journey, 'stop', 'id'),
                self._get_value(xml_journey, 'line', 'id'),
            )
            if route_point not in result:
                result[route_point] = []
            passage = self._get_real_time_passage(xml_journey)
            result[route_point].append(passage)
        return result

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

    def _timestamp_to_date(self, timestamp):
        dt = datetime.utcfromtimestamp(timestamp)
        dt = pytz.utc.localize(dt)
        return dt.astimezone(self.timezone)

    def _find_route_point_passages(self, route_point, passages):
        """
        To find the right passage in synthese:

        As a reminder we query synthese only for a stoppoint and we get, for all the routes that pass by
        this stop, the next passages.
        The tricky part is to find the which route concerns our routepoint

         * we first look if by miracle we can find some routes with the synthese code of our route in it's
         external codes (it can have several if the route is a fusion of many routes)
            -> if we found the routes (we can have more than one), we concatenate their passages
         * else we query navitia to get all routes that pass by the stoppoint for the line of the route point
            * if we get only one route, we search for this route's line in the synthese response
                (because lines synthese code are move coherent)
                -> we concatenate all synthese passages on this line
         -> else we return the base schedule
        """
        log = logging.getLogger(__name__)
        stop_point_id = str(route_point.fetch_stop_id(self.object_id_tag))
        is_same_route = lambda syn_rp: syn_rp.syn_route_id in route_point.fetch_all_route_id(self.object_id_tag)
        route_passages = [
            p
            for syn_rp, p in passages.items()
            if is_same_route(syn_rp) and stop_point_id == syn_rp.syn_stop_point_id
        ]

        if route_passages:
            return sorted(list(itertools.chain(*route_passages)), key=lambda p: p.datetime)

        log.debug(
            'impossible to find the route in synthese response, '
            'looking for the line {}'.format(route_point.fetch_line_uri())
        )

        routes_gen = self.instance.ptref.get_objs(
            type_pb2.ROUTE,
            'stop_point.uri = {stop} and line.uri = {line}'.format(
                stop=route_point.pb_stop_point.uri, line=route_point.fetch_line_uri()
            ),
        )

        first_routes = list(itertools.islice(routes_gen, 2))

        if len(first_routes) == 1:
            # there is only one route that pass through our stoppoint for the line of the routepoint
            # we can concatenate all synthese's route of this line
            line_passages = [
                p
                for syn_rp, p in passages.items()
                if syn_rp.syn_line_id == route_point.fetch_line_id(self.object_id_tag)
            ]

            if line_passages:
                return sorted(list(itertools.chain(*line_passages)), key=lambda p: p.datetime)

            log.debug(
                'stoppoint {sp} has {nb_r} routes for line {l} ({l_codes}) in navitia and {nb_syn_r} '
                'in synthese (lines: {syn_lines})'.format(
                    sp=route_point.pb_stop_point.uri,
                    nb_r=len(first_routes),
                    l=route_point.fetch_line_uri(),
                    l_codes=route_point.fetch_line_id(self.object_id_tag),
                    nb_syn_r=len(passages),
                    syn_lines=[l.syn_line_id for l in passages.keys()],
                )
            )

        if passages:
            log.info('impossible to find a valid passage for {} (passage = {})'.format(route_point, passages))

        return None

    def __eq__(self, other):
        return self.rt_system_id == other.rt_system_id
