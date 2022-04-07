# Copyright (c) 2001-2018, Hove and/or its affiliates. All rights reserved.
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

import datetime
import logging
import pybreaker
import pytz

from jormungandr.utils import Coords, make_timestamp_from_str
from navitiacommon import default_values
from jormungandr import app
import jormungandr.scenarios.ridesharing.ridesharing_journey as rsj
from jormungandr.scenarios.ridesharing.ridesharing_service import (
    AbstractRidesharingService,
    RsFeedPublisher,
    RidesharingServiceError,
)
from jormungandr.utils import decode_polyline
from navitiacommon import type_pb2
import jormungandr.street_network.utils

DEFAULT_INSTANT_SYSTEM_FEED_PUBLISHER = {
    'id': 'Instant System',
    'name': 'Instant System',
    'license': 'Private',
    'url': 'https://instant-system.com/disclaimers/disclaimer_XX.html',
}


class InstantSystem(AbstractRidesharingService):
    def __init__(
        self,
        service_url,
        api_key,
        network,
        feed_publisher=DEFAULT_INSTANT_SYSTEM_FEED_PUBLISHER,
        rating_scale_min=None,
        rating_scale_max=None,
        timeout=5,
        crowfly_radius=200,
        timeframe_duration=1800,
    ):
        self.service_url = service_url
        self.api_key = api_key
        self.network = network
        self.rating_scale_min = rating_scale_min
        self.rating_scale_max = rating_scale_max
        self.system_id = 'instant_system'
        self.timeout = timeout
        self.feed_publisher = None if feed_publisher is None else RsFeedPublisher(**feed_publisher)
        self.crowfly_radius = crowfly_radius
        self.timeframe_duration = timeframe_duration

        self.journey_metadata = rsj.MetaData(
            system_id=self.system_id,
            network=self.network,
            rating_scale_min=self.rating_scale_min,
            rating_scale_max=self.rating_scale_max,
        )

        self.logger = logging.getLogger("{} {}".format(__name__, self.system_id))

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config.get(str('CIRCUIT_BREAKER_MAX_INSTANT_SYSTEM_FAIL'), 4),
            reset_timeout=app.config.get(str('CIRCUIT_BREAKER_INSTANT_SYSTEM_TIMEOUT_S'), 60),
        )
        self.call_params = ''

    def status(self):
        return {
            'id': self.system_id,
            'class': self.__class__.__name__,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
            'rating_scale_min': self.rating_scale_min,
            'rating_scale_max': self.rating_scale_max,
            'crowfly_radius': self.crowfly_radius,
            'network': self.network,
        }

    @staticmethod
    def _get_ridesharing_journeys(raw_journeys):
        """
        This function gives us journeys that contain at least a pure ridesharing offer

        :param raw_journeys:
        :return:
        """

        def has_ridesharing_path(j):
            return next((p for p in j.get('paths', []) if p.get('mode') == 'RIDESHARINGAD'), None)

        return (j for j in raw_journeys if has_ridesharing_path(j))

    def _make_response(self, raw_json, from_coord, to_coord, instance_params):
        raw_journeys = raw_json.get('journeys')

        if not raw_journeys:
            return []

        ridesharing_journeys = []

        for j in self._get_ridesharing_journeys(raw_journeys):

            for p in j.get('paths'):

                if p.get('mode') != 'RIDESHARINGAD':
                    continue

                res = rsj.RidesharingJourney()

                res.metadata = self.journey_metadata

                res.distance = j.get('distance')

                res.ridesharing_ad = j.get('url')

                ridesharing_ad = p['rideSharingAd']

                # departure coord
                lat, lon = from_coord.split(',')
                departure_coord = Coords(lat=lat, lon=lon)

                # pick up coord
                from_data = p['from']
                pickup_lat = from_data.get('lat')
                pickup_lon = from_data.get('lon')
                pickup_coord = Coords(lat=pickup_lat, lon=pickup_lon)

                res.pickup_place = rsj.Place(addr=from_data.get('name'), lat=pickup_lat, lon=pickup_lon)

                res.origin_pickup_distance = int(
                    jormungandr.street_network.utils.crowfly_distance_between(departure_coord, pickup_coord)
                )
                # we choose to calculate with speed=1.12 the average speed for a walker
                res.origin_pickup_duration = jormungandr.street_network.utils.get_manhattan_duration(
                    res.origin_pickup_distance, instance_params.walking_speed
                )
                res.origin_pickup_shape = None  # Not specified

                # drop off coord
                to_data = p['to']
                dropoff_lat = to_data.get('lat')
                dropoff_lon = to_data.get('lon')
                dropoff_coord = Coords(lat=dropoff_lat, lon=dropoff_lon)

                res.dropoff_place = rsj.Place(addr=to_data.get('name'), lat=dropoff_lat, lon=dropoff_lon)

                # arrival coord
                lat, lon = to_coord.split(',')
                arrival_coord = Coords(lat=lat, lon=lon)

                res.dropoff_dest_distance = int(
                    jormungandr.street_network.utils.crowfly_distance_between(dropoff_coord, arrival_coord)
                )
                # we choose to calculate with speed=1.12 the average speed for a walker
                res.dropoff_dest_duration = jormungandr.street_network.utils.get_manhattan_duration(
                    res.dropoff_dest_distance, instance_params.walking_speed
                )
                res.dropoff_dest_shape = None  # Not specified

                res.shape = self._retreive_main_shape(p, 'shape', res.pickup_place, res.dropoff_place)

                res.pickup_date_time = make_timestamp_from_str(p['departureDate'])
                res.departure_date_time = res.pickup_date_time - res.origin_pickup_duration
                res.dropoff_date_time = make_timestamp_from_str(p['arrivalDate'])
                res.arrival_date_time = res.dropoff_date_time + res.dropoff_dest_duration
                res.duration = res.dropoff_date_time - res.pickup_date_time

                user = ridesharing_ad['user']

                gender = user.get('gender')
                gender_map = {'MALE': rsj.Gender.MALE, 'FEMALE': rsj.Gender.FEMALE}

                res.driver = rsj.Individual(
                    alias=user.get('alias'),
                    gender=gender_map.get(gender, rsj.Gender.UNKNOWN),
                    image=user.get('imageUrl'),
                    rate=user.get('rating', {}).get('rate'),
                    rate_count=user.get('rating', {}).get('count'),
                )

                # the usual form of the price for InstantSystem is: "170 EUR"
                # which means "170 EURO cents" also equals "1.70 EURO"
                # In Navitia so far prices are in "centime" so we transform it to: "170 centime"
                price = ridesharing_ad['price']
                res.price = price.get('amount')
                if price.get('currency') == "EUR":
                    res.currency = "centime"
                else:
                    res.currency = price.get('currency')

                res.available_seats = ridesharing_ad['vehicle']['availableSeats']
                res.total_seats = None

                ridesharing_journeys.append(res)

        return ridesharing_journeys

    def _request_journeys(self, from_coord, to_coord, period_extremity, instance_params, limit=None):
        """

        :param from_coord: lat,lon ex: '48.109377,-1.682103'
        :param to_coord: lat,lon ex: '48.020335,-1.743929'
        :param period_extremity: a tuple of [timestamp(utc), clockwise]
        :param limit: optional
        :return:
        """
        # format of datetime: 2017-12-25T07:00:00Z
        datetime_str = datetime.datetime.fromtimestamp(period_extremity.datetime, pytz.utc).strftime(
            '%Y-%m-%dT%H:%M:%SZ'
        )
        if period_extremity.represents_start:
            datetime_str = '{}/PT{}S'.format(datetime_str, self.timeframe_duration)
        else:
            datetime_str = 'PT{}S/{}'.format(self.timeframe_duration, datetime_str)

        params = {
            'from': from_coord,
            'to': to_coord,
            'fromRadius': self.crowfly_radius,
            'toRadius': self.crowfly_radius,
            ('arrivalDate', 'departureDate')[bool(period_extremity.represents_start)]: datetime_str,
        }

        if limit is not None:
            params.update({'limit', limit})

        headers = {'apikey': '{}'.format(self.api_key)}
        resp = self._call_service(params=params, headers=headers)

        if not resp or resp.status_code != 200:
            # TODO better error handling, the response might be in 200 but in error
            logging.getLogger(__name__).error(
                'Instant System service unavailable, impossible to query : %s',
                resp.url,
                extra={'ridesharing_service_id': self._get_rs_id(), 'status_code': resp.status_code},
            )
            raise RidesharingServiceError('non 200 response', resp.status_code, resp.reason, resp.text)

        if resp:
            r = self._make_response(resp.json(), from_coord, to_coord, instance_params)
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
