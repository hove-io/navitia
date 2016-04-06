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


class Cleverage(RealtimeProxy):
    """
    class managing calls to cleverage external service providing real-time next passages
    """
    def __init__(self, id, service_url, timezone, object_id_tag=None, timeout=10):
        self.service_url = service_url
        self.timeout = timeout  # timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_CLEVERAGE_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_CLEVERAGE_TIMEOUT_S'])
        self.timezone = pytz.timezone(timezone)

    def __repr__(self):
        """
         used as the cache key. we use the rt_system_id to share the cache between servers in production
        """
        return self.rt_system_id

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('CLEVERAGE_SYNTHESE', 30))
    def _call_synthese(self, url):
        """
        http call to cleverage
        """
        try:
            return self.breaker.call(requests.get, url, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('Cleverage RT service dead, using base '
                                              'schedule (error: {}'.format(e))
        except requests.Timeout as t:
            logging.getLogger(__name__).error('Cleverage RT service timeout, using base '
                                              'schedule (error: {}'.format(t))
        except:
            logging.getLogger(__name__).exception('Cleverage RT error, using base schedule')
        return None

