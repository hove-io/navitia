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
from __future__ import absolute_import, print_function, division
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy, RealtimeProxyError
from jormungandr.utils import PY3
import logging
import pybreaker
import pytz
import requests as requests
from jormungandr import cache, app
from jormungandr.schedule import RealTimePassage
from datetime import datetime
import six


class SiriLite(RealtimeProxy):
    """
    class managing calls to a siri lite external service providing real-time next passages
    """

    def __init__(self, id, service_url, object_id_tag=None, instance=None, timeout=10, **kwargs):
        self.service_url = service_url
        self.timeout = timeout  # timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id

        fail_max = kwargs.get(
            'circuit_breaker_max_fail', app.config.get(str('CIRCUIT_BREAKER_MAX_SIRILITE_FAIL'), 5)
        )
        reset_timeout = kwargs.get(
            'circuit_breaker_reset_timeout', app.config.get(str('CIRCUIT_BREAKER_SIRILITE_TIMEOUT_S'), 60)
        )
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)

        self.instance = instance
        self.log = logging.LoggerAdapter(logging.getLogger(__name__), extra={'rt_proxy': id})

    def __repr__(self):
        """
        used as the cache key. we use the rt_system_id to share the cache between servers in production
        """
        if PY3:
            self.rt_system_id
        try:
            return self.rt_system_id.encode('utf-8', 'backslashreplace')
        except:
            return self.rt_system_id

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_SIRILITE', 30))
    def _call(self, url):
        self.log.debug('sirilite RT service, call url: {}'.format(url))
        try:
            return self.breaker.call(requests.get, url, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            self.log.error('sirilite RT service dead, using base schedule (error: {}'.format(e))
            raise RealtimeProxyError('circuit breaker open')
        except requests.Timeout as t:
            self.log.error('sitelite RT service timeout, using base schedule (error: {}'.format(t))
            raise RealtimeProxyError('timeout')
        except Exception as e:
            self.log.exception('sirilite RT error, using base schedule')
            raise RealtimeProxyError(str(e))

    def _make_url(self, route_point):
        """
        The url returns something like a departure on a stop point
        """

        stop_id = route_point.fetch_stop_id(self.object_id_tag)
        line_id = route_point.fetch_line_id(self.object_id_tag)

        # Note: we don't use the line_id now because the stif's SIRILite does not handle it
        # but we want to check that we have it
        # if the stif starts to handle it, we can add it with '&LineRef={line_id}'
        if not stop_id or not line_id:
            # one of the id is missing, we'll not find any realtime
            self.log.debug('missing realtime id for {obj}: stop code={s}'.format(obj=route_point, s=stop_id))
            self.record_internal_failure('missing id')
            return None

        url = "{base_url}&MonitoringRef={stop_id}".format(base_url=self.service_url, stop_id=stop_id)

        return url

    def _get_dt(self, datetime_str):
        return pytz.utc.localize(datetime.strptime(datetime_str, "%Y-%m-%dT%H:%M:%S.%fZ"))

    def _get_passages(self, route_point, resp):
        self.log.debug('sirilite response: {}'.format(resp))

        # Add STIF line code
        line_code = route_point.fetch_line_id(self.object_id_tag)

        # If STIF line code match with the response, we select it
        stop_deliveries = resp.get('Siri', {}).get('ServiceDelivery', {}).get('StopMonitoringDelivery', [])
        schedules = [
            vj
            for d in stop_deliveries
            for vj in d.get('MonitoredStopVisit', [])
            if vj.get('MonitoredVehicleJourney', {}).get('LineRef', {}).get('value') == line_code
        ]
        if not schedules:
            self.record_additional_info('no_departure')
            return []

        # In each matched realtime time reponse,
        next_passages = []
        for next_expected_st in schedules:
            destination = (
                next_expected_st.get('MonitoredVehicleJourney', {}).get('DestinationRef', {}).get('value')
            )

            if not destination:
                # Check destination
                self.log.debug(
                    'no destination for next st {} for routepoint {}, skipping departure'.format(
                        next_expected_st, route_point
                    )
                )
                self.record_additional_info('no_destination')
                continue

            # We find possible routes from our stop point to destination
            possible_routes = self.get_matching_routes(
                destination=destination, line=route_point.fetch_line_uri(), start=route_point.pb_stop_point.uri
            )

            if not possible_routes:
                # Check possible routes
                self.log.debug(
                    'no possible routes for next st {} for routepoint {}, skipping departure'.format(
                        next_expected_st, route_point
                    )
                )
                continue

            if len(possible_routes) > 1:
                self.log.info(
                    'multiple possible routes for destination {} and routepoint {}, we add the '
                    'passages on all the routes'.format(destination, route_point)
                )

            if route_point.pb_route.uri not in possible_routes:
                # the next passage does not concern our route point, we can skip it
                continue

            # for the moment we handle only the NextStop and the direction
            expected_dt = (
                next_expected_st.get('MonitoredVehicleJourney', {})
                .get('MonitoredCall', {})
                .get('ExpectedDepartureTime')
            )
            if not expected_dt:
                # TODO, if needed we could add a check on the line opening/closing time
                self.record_additional_info('no_departure_time')
                continue
            dt = self._get_dt(expected_dt)
            destination = next_expected_st.get('MonitoredVehicleJourney', {}).get('DestinationName', [])
            if destination:
                direction = destination[0].get('value')
            else:
                direction = None
            is_real_time = True
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

        if r.status_code != 200:
            self.log.error('sirilite RT service unavailable, impossible to query : {}'.format(r.url))
            raise RealtimeProxyError('non 200 response')

        return self._get_passages(route_point, r.json())

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

    def get_matching_routes(self, line, start, destination):
        pb_routes = self.instance.ptref.get_matching_routes(
            line_uri=line,
            start_sp_uri=start,
            destination_code=destination,
            destination_code_key=self.object_id_tag,
        )

        return [r.uri for r in pb_routes]

    def __eq__(self, other):
        return self.rt_system_id == other.rt_system_id
