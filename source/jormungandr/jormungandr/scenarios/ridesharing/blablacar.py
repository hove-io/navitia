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
# but WITHOUT ANY WARRANTY; without even the implied warr60anty of
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

import datetime
import calendar
import logging
import pybreaker

from jormungandr import app
import jormungandr.scenarios.ridesharing.ridesharing_journey as rsj
from jormungandr.scenarios.ridesharing.ridesharing_service import (
    AbstractRidesharingService,
    RsFeedPublisher,
    RidesharingServiceError,
)
from jormungandr.utils import decode_polyline
from navitiacommon import type_pb2

DEFAULT_BLABLACAR_FEED_PUBLISHER = {
    'id': 'blablacar',
    'name': 'Blablacar',
    'license': 'Private',
    'url': 'https://www.blablalines.com/terms',
}

# Departure time should be between now to 1 week from now
# Paramaeters documenation : https://www.blablalines.com/public-api-v2
MIN_BLABLACAR_MARGIN_DEPARTURE_TIME = 15 * 60  # 15 min
MAX_BLABLACAR_MARGIN_DEPARTURE_TIME = 60 * 60 * 168  # 168 hours


class Blablacar(AbstractRidesharingService):
    def __init__(
        self,
        service_url,
        api_key,
        network,
        feed_publisher=DEFAULT_BLABLACAR_FEED_PUBLISHER,
        timedelta=3600,
        timeout=2,
    ):
        self.service_url = service_url
        self.api_key = api_key
        self.network = network
        self.system_id = 'Blablacar'
        self.timeout = timeout
        self.timedelta = timedelta
        self.feed_publisher = None if feed_publisher is None else RsFeedPublisher(**feed_publisher)

        self.logger = logging.getLogger("{} {}".format(__name__, self.system_id))

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config.get(str('CIRCUIT_BREAKER_MAX_BLABLACAR_FAIL'), 4),
            reset_timeout=app.config.get(str('CIRCUIT_BREAKER_BLABLACAR_TIMEOUT_S'), 60),
        )
        self.call_params = ''

        self.journey_metadata = rsj.MetaData(
            system_id=self.system_id, network=self.network, rating_scale_min=None, rating_scale_max=None
        )

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

    def _make_response(self, raw_json):

        if not raw_json:
            return []

        ridesharing_journeys = []

        for offer in raw_json:

            res = rsj.RidesharingJourney()

            res.metadata = self.journey_metadata
            res.distance = offer.get('distance')
            res.duration = offer.get('duration')
            res.ridesharing_ad = offer.get('web_url')

            res.pickup_place = rsj.Place(
                addr='', lat=offer.get('pickup_latitude'), lon=offer.get('pickup_longitude')
            )
            res.dropoff_place = rsj.Place(
                addr='', lat=offer.get('dropoff_latitude'), lon=offer.get('dropoff_longitude')
            )

            # shape is a list of type_pb2.GeographicalCoord()
            res.shape = []
            shape = decode_polyline(offer.get('journey_polyline'), precision=5)
            if not shape or res.pickup_place.lon != shape[0][0] or res.pickup_place.lat != shape[0][1]:
                res.shape.append(type_pb2.GeographicalCoord(lon=res.pickup_place.lon, lat=res.pickup_place.lat))

            if shape:
                res.shape.extend((type_pb2.GeographicalCoord(lon=c[0], lat=c[1]) for c in shape))

            if not shape or res.dropoff_place.lon != shape[-1][0] or res.dropoff_place.lat != shape[-1][1]:
                res.shape.append(
                    type_pb2.GeographicalCoord(lon=res.dropoff_place.lon, lat=res.dropoff_place.lat)
                )

            res.price = offer.get('price', {}).get('amount')
            currency = offer.get('price', {}).get('currency')
            res.currency = "centime" if currency == "EUR" else currency

            res.available_seats = offer.get('available_seats')
            res.total_seats = None

            # not specified
            res.pickup_date_time = None
            res.dropoff_date_time = None

            res.driver = rsj.Individual(alias=None, gender=None, image=None, rate=None, rate_count=None)

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

        from_coords = from_coord.split(',')
        to_coords = to_coord.split(',')

        dt = datetime.datetime.now()
        now = calendar.timegm(dt.utctimetuple())

        if period_extremity.datetime < now + MIN_BLABLACAR_MARGIN_DEPARTURE_TIME:
            logging.getLogger(__name__).info(
                'blablacar ridesharing request departure time < now + 15 min. Force to now + 15 min'
            )
            departure_epoch = now + MIN_BLABLACAR_MARGIN_DEPARTURE_TIME
        elif period_extremity.datetime > now + MAX_BLABLACAR_MARGIN_DEPARTURE_TIME:
            logging.getLogger(__name__).error(
                'Blablacar error, request departure time should be between now to 1 week from now. departure is greater than now + 1 week'
            )
            return []
        else:
            departure_epoch = period_extremity.datetime

        # Paramaeters documenation : https://www.blablalines.com/public-api-v2
        params = {
            'access_token': self.api_key,
            'departure_latitude': from_coords[0],
            'departure_longitude': from_coords[1],
            'arrival_latitude': to_coords[0],
            'arrival_longitude': to_coords[1],
            'departure_epoch': departure_epoch,
            'departure_timedelta': self.timedelta,
        }

        resp = self._call_service(params=params)

        if not resp or resp.status_code != 200:
            logging.getLogger(__name__).error(
                'Blablacar service unavailable, impossible to query : %s',
                resp.url,
                extra={'ridesharing_service_id': self._get_rs_id(), 'status_code': resp.status_code},
            )
            raise RidesharingServiceError('non 200 response', resp.status_code, resp.reason, resp.text)

        if resp:
            r = self._make_response(resp.json())
            self.record_additional_info('Received ridesharing offers', nb_ridesharing_offers=len(r))
            logging.getLogger('stat.ridesharing.blablacar').info(
                'Received ridesharing offers : %s',
                len(r),
                extra={'ridesharing_service_id': self._get_rs_id(), 'nb_ridesharing_offers': len(r)},
            )
            return r
        self.record_additional_info('Received ridesharing offers', nb_ridesharing_offers=0)
        logging.getLogger('stat.ridesharing.blablacar').info(
            'Received ridesharing offers : 0',
            extra={'ridesharing_service_id': self._get_rs_id(), 'nb_ridesharing_offers': 0},
        )
        return []

    def _get_feed_publisher(self):
        return self.feed_publisher
