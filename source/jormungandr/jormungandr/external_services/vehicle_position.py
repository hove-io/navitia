# Copyright (c) 2001-2021, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

import pybreaker
import logging
from jormungandr.external_services.external_service import AbstractExternalService
from jormungandr import utils
import gevent
import gevent.pool
from jormungandr import cache, app
from navitiacommon import type_pb2


class VehiclePosition(AbstractExternalService):
    def __init__(self, service_url, timeout=2, **kwargs):
        self.logger = logging.getLogger(__name__)
        self.service_url = service_url
        self.timeout = timeout
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=kwargs.get('circuit_breaker_max_fail', app.config['CIRCUIT_BREAKER_MAX_FORSETI_FAIL']),
            reset_timeout=kwargs.get(
                'circuit_breaker_reset_timeout', app.config['CIRCUIT_BREAKER_FORSETI_TIMEOUT_S']
            ),
        )

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_FORSETI'), 10))
    def get_response(self, arguments):
        """
        Get vehicle_position information from Forseti webservice
        """
        raw_response = self._call_webservice(arguments)

        # We don't need any further action if raw_response is None
        if raw_response is None:
            return None
        resp = self.response_marshaller(raw_response)
        if resp is None:
            return None
        return resp.get('vehicle_positions', [])

    def _vehicle_journey_position(self, vehicle_journey_position, response_json):
        if not response_json:
            logging.getLogger(__name__).debug(
                "Vehicle position not found for vehicle_journey {}".format(
                    vehicle_journey_position.vehicle_journey.uri
                )
            )
            return
        if len(response_json) >= 2:
            logging.getLogger(__name__).warning(
                "Multiple vehicles positions for vehicle_journey {}".format(
                    vehicle_journey_position.vehicle_journey.uri
                )
            )
        if response_json:
            first_vehicle_position = response_json[0]
            vehicle_journey_position.coord.lon = first_vehicle_position.get("longitude")
            vehicle_journey_position.coord.lat = first_vehicle_position.get("latitude")
            vehicle_journey_position.bearing = first_vehicle_position.get("bearing")
            vehicle_journey_position.speed = first_vehicle_position.get("speed")
            vehicle_journey_position.data_freshness = type_pb2.REALTIME
            occupancy = first_vehicle_position.get("occupancy", None)
            if occupancy:
                vehicle_journey_position.occupancy = occupancy
            feed_created_at = first_vehicle_position.get("feed_created_at", None)
            if feed_created_at:
                vehicle_journey_position.feed_created_at = utils.make_timestamp_from_str(feed_created_at)

    def update_response(self, instance, vehicle_positions, **kwargs):
        futures = []
        # TODO define new parameter forseti_pool_size ?
        pool = gevent.pool.Pool(instance.realtime_pool_size)

        # Copy the current request context to be used in greenlet
        reqctx = utils.copy_flask_request_context()

        def worker(vehicle_journey_position, args):
            # Use the copied request context in greenlet
            with utils.copy_context_in_greenlet_stack(reqctx):
                return vehicle_journey_position, self.get_response(args)

        for vehicle_position in vehicle_positions:
            for vehicle_journey_position in vehicle_position.vehicle_journey_positions:
                args = self.get_codes('vehicle_journey', vehicle_journey_position.vehicle_journey.codes)
                futures.append(pool.spawn(worker, vehicle_journey_position, args))

        for future in gevent.iwait(futures):
            vehicle_journey_position, response = future.get()
            self._vehicle_journey_position(vehicle_journey_position, response)
