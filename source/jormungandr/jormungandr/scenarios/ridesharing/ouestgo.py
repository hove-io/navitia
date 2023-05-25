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

from jormungandr import app
import jormungandr.scenarios.ridesharing.ridesharing_journey as rsj
from jormungandr.scenarios.ridesharing.ridesharing_service import (
    AbstractRidesharingService,
    RsFeedPublisher,
    RidesharingServiceError,
)
from jormungandr.utils import (
    get_weekday,
    timestamp_to_date_str,
    DATE_FORMAT,
    local_str_date_to_utc,
    date_to_timestamp,
)
from jormungandr.timezone import get_timezone_or_paris

DEFAULT_OUESTGO_FEED_PUBLISHER = {
    'id': 'OUESTGO',
    'name': 'OuestGo',
    'license': 'Private',
    'url': 'https://www.ouestgo.fr/',
}


def format_str_datetime(str_date, str_time):
    return '{}T{}'.format(str_date.replace('-', ''), str_time.replace(':', ''))


class Ouestgo(AbstractRidesharingService):
    def __init__(
        self,
        service_url,
        api_key,
        network,
        feed_publisher=DEFAULT_OUESTGO_FEED_PUBLISHER,
        timeout=2,
        driver_state=1,
        passenger_state=0,
        verify=True,
    ):
        self.service_url = service_url
        self.api_key = api_key
        self.network = network
        self.system_id = 'ouestgo'
        self.timeout = timeout
        self.feed_publisher = None if feed_publisher is None else RsFeedPublisher(**feed_publisher)
        self.driver_state = driver_state
        self.passenger_state = passenger_state
        self.verify = verify

        self.journey_metadata = rsj.MetaData(
            system_id=self.system_id, network=self.network, rating_scale_min=None, rating_scale_max=None
        )

        self.logger = logging.getLogger("{} {}".format(__name__, self.system_id))

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config.get(str('CIRCUIT_BREAKER_MAX_OUESTGO_FAIL'), 4),
            reset_timeout=app.config.get(str('CIRCUIT_BREAKER_OUESTGO_TIMEOUT_S'), 60),
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
            'network': self.network,
        }

    def get_mean_pickup_datetime(self, json_outward, circulation_day, timezone):
        if not json_outward.get(circulation_day, {}):
            return None
        min_time = json_outward.get(circulation_day, {}).get('mintime')
        max_time = json_outward.get(circulation_day, {}).get('maxtime')
        min_date = json_outward.get('mindate')
        max_date = json_outward.get('maxdate')
        min_datetime_str = format_str_datetime(min_date, min_time)
        max_datetime_str = format_str_datetime(max_date, max_time)

        min_datetime = date_to_timestamp(local_str_date_to_utc(min_datetime_str, timezone))
        max_datetime = date_to_timestamp(local_str_date_to_utc(max_datetime_str, timezone))
        return int((min_datetime + max_datetime) / 2)

    def _make_response(self, raw_json, request_datetime, from_coord, to_coord):
        if not raw_json:
            return []
        ridesharing_journeys = []
        circulation_day = get_weekday(request_datetime)
        timezone = get_timezone_or_paris()
        for offer in raw_json:
            json_journeys = offer.get('journeys', {})
            pickup_datetime = self.get_mean_pickup_datetime(
                json_journeys.get('outward', {}), circulation_day, timezone.zone
            )
            if not pickup_datetime:
                continue
            if pickup_datetime > request_datetime:
                res = rsj.RidesharingJourney()
                res.metadata = self.journey_metadata
                res.distance = json_journeys.get('distance')
                res.ridesharing_ad = json_journeys.get('url')
                res.duration = json_journeys.get('duration')

                # ride-sharing pick up point is the same as departure
                lat, lon = from_coord.split(',')
                res.pickup_place = rsj.Place(addr='', lat=float(lat), lon=float(lon))
                res.origin_pickup_shape = None  # Not specified
                res.origin_pickup_distance = 0
                res.origin_pickup_duration = 0

                # ride-sharing drop off point is same as arrival to final destination or any mode of transport
                lat, lon = to_coord.split(',')
                res.dropoff_place = rsj.Place(addr='', lat=float(lat), lon=float(lon))
                res.dropoff_dest_shape = None  # Not specified
                res.dropoff_dest_distance = 0
                res.dropoff_dest_duration = 0
                res.shape = None

                res.price = float(json_journeys.get('cost', {}).get('variable')) * 100.0
                res.currency = "centime"

                json_driver = json_journeys.get('driver', {})
                res.available_seats = json_driver.get('seats')
                res.total_seats = None

                res.pickup_date_time = pickup_datetime
                res.departure_date_time = res.pickup_date_time - res.origin_pickup_duration
                res.dropoff_date_time = res.pickup_date_time + res.duration
                res.arrival_date_time = res.dropoff_date_time + res.dropoff_dest_duration

                gender_map = {'male': rsj.Gender.MALE, 'female': rsj.Gender.FEMALE}
                res.driver = rsj.Individual(
                    alias=json_driver.get('alias'),
                    gender=gender_map.get(json_driver.get('gender'), rsj.Gender.UNKNOWN),
                    image=json_driver.get('image'),
                    rate=None,
                    rate_count=None,
                )

                ridesharing_journeys.append(res)

        return ridesharing_journeys

    def _request_journeys(self, from_coord, to_coord, period_extremity, instance_params=None, limit=None):
        """

        :param from_coord: lat,lon ex: '48.109377,-1.682103'
        :param to_coord: lat,lon ex: '48.020335,-1.743929'
        :param period_extremity: a tuple of [timestamp(utc), clockwise]
        :param limit: optional
        :return:
        """
        dep_lat, dep_lon = from_coord.split(',')
        arr_lat, arr_lon = to_coord.split(',')
        timezone = get_timezone_or_paris()  # using coverage's TZ (or Paris) for mindate and maxdate
        params = {
            'apikey': self.api_key,
            'p[passenger][state]': self.passenger_state,
            'p[driver][state]': self.driver_state,
            'p[from][latitude]': dep_lat,
            'p[from][longitude]': dep_lon,
            'p[to][latitude]': arr_lat,
            'p[to][longitude]': arr_lon,
            'signature': 'toto',
            'timestamp': period_extremity.datetime,
            'p[outward][mindate]': timestamp_to_date_str(
                period_extremity.datetime, timezone, _format=DATE_FORMAT
            ),
            'p[outward][maxdate]': timestamp_to_date_str(
                period_extremity.datetime, timezone, _format=DATE_FORMAT
            ),
        }

        headers = {'Authentication': 'key={}'.format(self.api_key)}

        resp = self._call_service(params=params, headers=headers, verify=self.verify)

        if not resp or resp.status_code != 200:
            logging.getLogger(__name__).error(
                'Ouestgo API service unavailable, impossible to query : %s',
                resp.url,
                extra={'ridesharing_service_id': self._get_rs_id(), 'status_code': resp.status_code},
            )
            raise RidesharingServiceError('non 200 response', resp.status_code, resp.reason, resp.text)

        if resp:
            r = self._make_response(resp.json(), period_extremity.datetime, from_coord, to_coord)
            self.record_additional_info('Received ridesharing offers', nb_ridesharing_offers=len(r))
            logging.getLogger('stat.ridesharing.ouestgo').info(
                'Received ridesharing offers : %s',
                len(r),
                extra={'ridesharing_service_id': self._get_rs_id(), 'nb_ridesharing_offers': len(r)},
            )
            return r

    def _get_feed_publisher(self):
        return self.feed_publisher
