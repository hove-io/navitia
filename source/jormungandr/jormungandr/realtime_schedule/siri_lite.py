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
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy
from flask import logging
import pybreaker
import pytz
import requests as requests
from jormungandr import cache, app
from jormungandr.schedule import RealTimePassage
from datetime import datetime


class SiriLite(RealtimeProxy):
    """
    class managing calls to a siri lite external service providing real-time next passages
    """
    def __init__(self, id, service_url, object_id_tag=None, timeout=10, **kwargs):
        self.service_url = service_url
        self.timeout = timeout  # timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config.get('CIRCUIT_BREAKER_MAX_SIRILITE_FAIL', 5),
                                                reset_timeout=app.config.get('CIRCUIT_BREAKER_SIRILITE_TIMEOUT_S', 60))

    def __repr__(self):
        """
         used as the cache key. we use the rt_system_id to share the cache between servers in production
        """
        return self.rt_system_id

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_SIRILITE', 30))
    def _call(self, url):
        logging.getLogger(__name__).debug('sirilite RT service, call url: {}'.format(url))
        try:
            return self.breaker.call(requests.get, url, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('sirilite RT service dead, using base '
                                              'schedule (error: {}'.format(e))
            self.record_external_failure('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('sitelite RT service timeout, using base '
                                              'schedule (error: {}'.format(t))
            self.record_external_failure('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('sirilite RT error, using base schedule')
            self.record_external_failure(str(e))
        return None

    def _make_url(self, route_point):
        """
        The url returns something like a departure on a stop point
        """

        stop_id = route_point.fetch_stop_id(self.object_id_tag)
        line_id = route_point.fetch_line_id(self.object_id_tag)

        #we don't use the line_id now, but we want to check that we have it
        if not stop_id or not line_id:
            # one of the id is missing, we'll not find any realtime
            logging.getLogger(__name__).debug('missing realtime id for {obj}: stop code={s}'.
                                              format(obj=route_point, s=stop_id))
            self.record_internal_failure('missing id')
            return None

        url = "{base_url}{stop_id}".format(base_url=self.service_url, stop_id=stop_id)

        return url

    def _get_dt(self, datetime_str):
        return pytz.utc.localize(datetime.strptime(datetime_str, "%Y-%m-%dT%H:%M:%S.%fZ"))

    def _get_passages(self, route_point, resp):
        logging.getLogger(__name__).debug('sirilite response: {}'.format(resp))

        line_code = route_point.fetch_line_id(self.object_id_tag)
        monitored_stop_visit = resp.get('siri', {})\
                                   .get('serviceDelivery', {})\
                                   .get('stopMonitoringDelivery', {})\
                                   .get('monitoredStopVisit', [])
        schedules = (vj for vj in monitored_stop_visit
                        if vj.get('monitoredVehicleJourney', {}).get('lineRef', {}).get('value', '') == line_code)
        #TODO: we should use the destination to find the correct route
        if schedules:
            next_passages = []
            for next_expected_st in schedules:
                # for the moment we handle only the NextStop and the direction
                expected_dt = next_expected_st.get('monitoredVehicleJourney', {})\
                                              .get('monitoredCall', {})\
                                              .get('expectedDepartureTime')
                if not expected_dt:
                    continue
                dt = self._get_dt(expected_dt)
                destination = next_expected_st.get('monitoredVehicleJourney', {}).get('destinationName', [])
                if destination:
                    direction = destination[0].get('value')
                else:
                    direction = None
                is_real_time = True
                next_passage = RealTimePassage(dt, direction, is_real_time)
                next_passages.append(next_passage)

            return next_passages
        else:
            return None

    def _get_next_passage_for_route_point(self, route_point, count=None, from_dt=None, current_dt=None):
        url = self._make_url(route_point)
        if not url:
            return None
        r = self._call(url)
        if not r:
            return None

        if r.status_code != 200:
            # TODO better error handling, the response might be in 200 but in error
            logging.getLogger(__name__).error('sirilite RT service unavailable, impossible to query : {}'
                                              .format(r.url))
            self.record_external_failure('non 200 response')
            return None

        return self._get_passages(route_point, r.json())

    def status(self):
        return {'id': self.rt_system_id,
                'timeout': self.timeout,
                'circuit_breaker': {'current_state': self.breaker.current_state,
                                    'fail_counter': self.breaker.fail_counter,
                                    'reset_timeout': self.breaker.reset_timeout},
                }
