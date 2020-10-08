# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

import logging
import pybreaker
import requests as requests

from jormungandr import app
import jormungandr.scenarios.ridesharing.ridesharing_journey as rsj
from jormungandr.scenarios.ridesharing.ridesharing_service import (
    AbstractRidesharingService,
    RsFeedPublisher,
    RidesharingServiceError,
)
from jormungandr.utils import decode_polyline
from navitiacommon import type_pb2

DEFAULT_KLAXIT_FEED_PUBLISHER = {
    'id': 'Klaxit VIA API',
    'name': 'Klaxit VIA API',
    'license': 'Private',
    'url': 'https://dev.klaxit.com/swagger-ui?url=https://via.klaxit.com/v1/swagger.json',
}


class Klaxit(AbstractRidesharingService):
    def __init__(
        self,
        instance,
        service_url,
        api_key,
        network,
        feed_publisher=DEFAULT_KLAXIT_FEED_PUBLISHER,
        timedelta=3600,
        timeout=2,
        departure_radius=2,
        arrival_radius=2,
    ):
        self.instance = instance
        self.service_url = service_url
        self.api_key = api_key
        self.network = network
        self.system_id = 'Klaxit VIA API'
        self.timeout = timeout
        self.timedelta = timedelta
        self.departure_radius = departure_radius
        self.arrival_radius = arrival_radius
        self.feed_publisher = None if feed_publisher is None else RsFeedPublisher(**feed_publisher)

        self.journey_metadata = rsj.MetaData(
            system_id=self.system_id, network=self.network, rating_scale_min=None, rating_scale_max=None
        )

        self.logger = logging.getLogger("{} {}".format(__name__, self.system_id))

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_KLAXIT_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_KLAXIT_TIMEOUT_S'],
        )
        self.call_params = None

    def status(self):
        return {
            'id': self.system_id,
            'class': self.__class__.__name__,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
            'network': self.network,
        }

    def _call_service(self, params):
        self.logger.debug("requesting Klaxit VIA API")

        headers = {'Authorization': 'apiKey {}'.format(self.api_key)}
        try:
            return self.breaker.call(
                requests.get, url=self.service_url, headers=headers, params=params, timeout=self.timeout
            )
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error(
                'Klaxit VIA API service dead (error: %s)', e, extra={'ridesharing_service_id': self._get_rs_id()}
            )
            raise RidesharingServiceError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error(
                'Klaxit VIA API service timeout (error: %s)',
                t,
                extra={'ridesharing_service_id': self._get_rs_id()},
            )
            raise RidesharingServiceError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception(
                'Klaxit VIA API service error', extra={'ridesharing_service_id': self._get_rs_id()}
            )
            raise RidesharingServiceError(str(e))

    def _make_response(self, raw_json):
        if not raw_json:
            return []

        ridesharing_journeys = []

        for offer in raw_json:
            res = rsj.RidesharingJourney()
            res.metadata = self.journey_metadata
            res.distance = offer.get('distance')
            res.ridesharing_ad = offer.get('webUrl')
            res.duration = offer.get('duration')

            res.pickup_place = rsj.Place(
                addr='', lat=offer.get('driverDepartureLat'), lon=offer.get('driverDepartureLng')
            )
            res.dropoff_place = rsj.Place(
                addr='', lat=offer.get('driverArrivalLat'), lon=offer.get('driverArrivalLng')
            )

            # shape is a list of type_pb2.GeographicalCoord()
            res.shape = []
            shape = decode_polyline(offer.get('journeyPolyline'), precision=5)
            if not shape or res.pickup_place.lon != shape[0][0] or res.pickup_place.lat != shape[0][1]:
                coord = type_pb2.GeographicalCoord()
                coord.lon = res.pickup_place.lon
                coord.lat = res.pickup_place.lat
                res.shape.append(coord)

            res.shape.extend((type_pb2.GeographicalCoord(lon=c[0], lat=c[1]) for c in shape))

            if not shape or res.dropoff_place.lon != shape[0][0] or res.dropoff_place.lat != shape[0][1]:
                coord = type_pb2.GeographicalCoord()
                coord.lon = res.dropoff_place.lon
                coord.lat = res.dropoff_place.lat
                res.shape.append(coord)

            res.price = offer.get('price', {}).get('amount')
            res.currency = "centime"

            res.available_seats = offer.get('available_seats')
            res.total_seats = None

            # not specified
            res.pickup_date_time = offer.get('driverDepartureDate')
            res.dropoff_date_time = res.pickup_date_time + offer.get('duration')

            driver_alias = offer.get('driver', {}).get('alias')
            driver_grade = offer.get('driver', {}).get('grade')
            driver_image = offer.get('driver', {}).get('picture')

            res.driver = rsj.Individual(
                alias=driver_alias, gender=None, image=driver_image, rate=driver_grade, rate_count=None
            )

            ridesharing_journeys.append(res)

        return ridesharing_journeys

    def _request_journeys(self, from_coord, to_coord, period_extremity, limit=None):
        """

        :param from_coord: lat,lon ex: '48.109377,-1.682103'
        :param to_coord: lat,lon ex: '48.020335,-1.743929'
        :param period_extremity: a tuple of [timestamp(utc), clockwise]
        :param limit: optional
        :return:
        """
        dep_lat, dep_lon = from_coord.split(',')
        arr_lat, arr_lon = to_coord.split(',')

        # Parameters documenation : https://dev.klaxit.com/swagger-ui?url=https://via.klaxit.com/v1/swagger.json
        # #/carpoolJourneys/get_carpoolJourneys
        params = {
            'apiKey': self.api_key,
            'departureLat': dep_lat,
            'departureLng': dep_lon,
            'arrivalLat': arr_lat,
            'arrivalLng': arr_lon,
            'date': period_extremity.datetime,
            'timeDelta': self.timedelta,
            'departureRadius': 2,
            'arrivalRadius': 2,
        }

        # Format call_params from parameters
        self.call_params = ''
        for key, value in params.items():
            self.call_params += '{}={}&'.format(key, value)

        resp = self._call_service(params=params)

        if resp.status_code != 200:
            logging.getLogger(__name__).error(
                'Klaxit VIA API service unavailable, impossible to query : %s',
                resp.url,
                extra={'ridesharing_service_id': self._get_rs_id(), 'status_code': resp.status_code},
            )
            raise RidesharingServiceError('non 200 response', resp.status_code, resp.reason, resp.text)

        if resp:
            r = self._make_response(resp.json())
            self.record_additional_info('Received ridesharing offers', nb_ridesharing_offers=len(r))
            logging.getLogger('stat.ridesharing.instant-system').info(
                'Received ridesharing offers : %s',
                len(r),
                extra={'ridesharing_service_id': self._get_rs_id(), 'nb_ridesharing_offers': len(r)},
            )
            return r
        self.record_additional_info('Received ridesharing offers', nb_ridesharing_offers=0)
        logging.getLogger('stat.ridesharing.instant-system').info(
            'Received ridesharing offers : 0',
            extra={'ridesharing_service_id': self._get_rs_id(), 'nb_ridesharing_offers': 0},
        )
        return []

    def _get_feed_publisher(self):
        return self.feed_publisher
