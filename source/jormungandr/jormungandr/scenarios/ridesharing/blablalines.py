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
from jormungandr.utils import Coords
from jormungandr.street_network.utils import crowfly_distance_between
from navitiacommon import type_pb2
import jormungandr.street_network.utils

DEFAULT_BLABLALINES_FEED_PUBLISHER = {
    'id': 'blablalines',
    'name': 'blablalines',
    'license': 'Private',
    'url': 'https://www.blablalines.com/terms',
}

# Departure time should be between now to 1 week from now
# Parameters documentation : https://www.blablalines.com/public-api-v2
MIN_BLABLALINES_MARGIN_DEPARTURE_TIME = 15 * 60  # 15 min
MAX_BLABLALINES_MARGIN_DEPARTURE_TIME = 60 * 60 * 168  # 168 hours


class Blablalines(AbstractRidesharingService):
    def __init__(
        self,
        service_url,
        api_key,
        network,
        feed_publisher=DEFAULT_BLABLALINES_FEED_PUBLISHER,
        timedelta=3600,
        timeout=2,
    ):
        self.service_url = service_url
        self.api_key = api_key
        self.network = network
        self.system_id = 'blablalines'
        self.timeout = timeout
        self.timedelta = timedelta
        self.feed_publisher = None if feed_publisher is None else RsFeedPublisher(**feed_publisher)

        self.logger = logging.getLogger("{} {}".format(__name__, self.system_id))

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config.get(str('CIRCUIT_BREAKER_MAX_BLABLALINES_FAIL'), 4),
            reset_timeout=app.config.get(str('CIRCUIT_BREAKER_BLABLALINES_TIMEOUT_S'), 60),
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

    def _make_response(self, raw_json, request_datetime, from_coord, to_coord):

        if not raw_json:
            return []

        ridesharing_journeys = []

        for offer in raw_json:

            res = rsj.RidesharingJourney()

            res.metadata = self.journey_metadata
            res.distance = offer.get('distance')
            res.duration = offer.get('duration')
            res.ridesharing_ad = offer.get('web_url')

            # departure coord
            lat, lon = from_coord.split(',')
            departure_coord = Coords(lat=lat, lon=lon)

            # pick up coord
            pickup_lat = offer.get('pickup_latitude')
            pickup_lon = offer.get('pickup_longitude')
            pickup_coord = Coords(lat=pickup_lat, lon=pickup_lon)

            res.pickup_place = rsj.Place(addr='', lat=pickup_lat, lon=pickup_lon)

            res.origin_pickup_duration = offer.get('departure_to_pickup_walking_time')
            res.origin_pickup_shape = None  # Not specified
            res.origin_pickup_distance = int(crowfly_distance_between(departure_coord, pickup_coord))

            # drop off coord
            dropoff_lat = offer.get('dropoff_latitude')
            dropoff_lon = offer.get('dropoff_longitude')
            dropoff_coord = Coords(lat=dropoff_lat, lon=dropoff_lon)

            res.dropoff_place = rsj.Place(addr='', lat=dropoff_lat, lon=dropoff_lon)

            # arrival coord
            lat, lon = to_coord.split(',')
            arrival_coord = Coords(lat=lat, lon=lon)

            res.dropoff_dest_duration = offer.get('dropoff_to_arrival_walking_time')
            res.dropoff_dest_shape = None  # Not specified
            res.dropoff_dest_distance = int(crowfly_distance_between(dropoff_coord, arrival_coord))

            res.shape = self._retreive_main_shape(offer, 'journey_polyline', res.pickup_place, res.dropoff_place)

            currency = offer.get('price', {}).get('currency')
            if currency == "EUR":
                res.currency = "centime"
                res.price = offer.get('price', {}).get('amount') * 100.0
            else:
                res.currency = currency
                res.price = offer.get('price', {}).get('amount')

            res.available_seats = offer.get('available_seats')
            res.total_seats = None

            # departure coord is absent in the offer and hence to be generated from the request
            res.departure_date_time = request_datetime
            res.pickup_date_time = res.departure_date_time + res.origin_pickup_duration
            res.dropoff_date_time = res.pickup_date_time + res.duration
            res.arrival_date_time = res.dropoff_date_time + res.dropoff_dest_duration

            res.driver = rsj.Individual(alias=None, gender=None, image=None, rate=None, rate_count=None)

            ridesharing_journeys.append(res)

        return ridesharing_journeys

    def _request_journeys(self, from_coord, to_coord, period_extremity, instance, limit=None):
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

        if period_extremity.datetime < now + MIN_BLABLALINES_MARGIN_DEPARTURE_TIME:
            logging.getLogger(__name__).info(
                'blablalines ridesharing request departure time < now + 15 min. Force to now + 15 min'
            )
            departure_epoch = now + MIN_BLABLALINES_MARGIN_DEPARTURE_TIME
        elif period_extremity.datetime > now + MAX_BLABLALINES_MARGIN_DEPARTURE_TIME:
            logging.getLogger(__name__).error(
                'Blablalines error, request departure time should be between now to 1 week from now. departure is greater than now + 1 week'
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
                'Blablalines service unavailable, impossible to query : %s',
                resp.url,
                extra={'ridesharing_service_id': self._get_rs_id(), 'status_code': resp.status_code},
            )
            raise RidesharingServiceError('non 200 response', resp.status_code, resp.reason, resp.text)

        if resp:
            r = self._make_response(resp.json(), period_extremity.datetime, from_coord, to_coord)
            self.record_additional_info('Received ridesharing offers', nb_ridesharing_offers=len(r))
            logging.getLogger('stat.ridesharing.blablalines').info(
                'Received ridesharing offers : %s',
                len(r),
                extra={'ridesharing_service_id': self._get_rs_id(), 'nb_ridesharing_offers': len(r)},
            )
            return r
        self.record_additional_info('Received ridesharing offers', nb_ridesharing_offers=0)
        logging.getLogger('stat.ridesharing.blablalines').info(
            'Received ridesharing offers : 0',
            extra={'ridesharing_service_id': self._get_rs_id(), 'nb_ridesharing_offers': 0},
        )
        return []

    def _get_feed_publisher(self):
        return self.feed_publisher
