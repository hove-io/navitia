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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

import datetime
import logging
import pybreaker
import requests as requests

from jormungandr import utils
from jormungandr import app
import jormungandr.scenarios.ridesharing.ridesharing_journey as rsj
from jormungandr.scenarios.ridesharing.ridesharing_service import AbstractRidesharingService, RsFeedPublisher
from jormungandr.utils import decode_polyline
from navitiacommon import type_pb2

DEFAULT_INSTANT_SYSTEM_FEED_PUBLISHER = {
    'id': 'Instant System',
    'name': 'Instant System',
    'license': 'Private',
    'url': 'https://instant-system.com/disclaimers/disclaimer_XX.html'
}


class InstantSystem(AbstractRidesharingService):

    def __init__(self, instance, service_url, api_key, network, feed_publisher=DEFAULT_INSTANT_SYSTEM_FEED_PUBLISHER,
                 rating_scale_min=None, rating_scale_max=None, timeout=2):
        self.instance = instance
        self.service_url = service_url
        self.api_key = api_key
        self.network = network
        self.rating_scale_min = rating_scale_min
        self.rating_scale_max = rating_scale_max
        self.system_id = 'Instant System'
        self.timeout = timeout
        self.feed_publisher = None if feed_publisher is None else RsFeedPublisher(**feed_publisher)

        self.journey_metadata = rsj.MetaData(system_id=self.system_id,
                                             network=self.network,
                                             rating_scale_min=self.rating_scale_min,
                                             rating_scale_max=self.rating_scale_max)

        self.logger = logging.getLogger("{} {}".format(__name__,
                                                       self.system_id))

        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_INSTANT_SYSTEM_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_INSTANT_SYSTEM_TIMEOUT_S'])

    def status(self):
        return {'id': self.system_id,
                'class': self.__class__.__name__,
                'circuit_breaker': {'current_state': self.breaker.current_state,
                                    'fail_counter': self.breaker.fail_counter,
                                    'reset_timeout': self.breaker.reset_timeout},
                }

    def _call_service(self, params):
        self.logger.debug("requesting instant system")

        headers = {'Authorization': 'apiKey {}'.format(self.api_key)}
        try:
            return self.breaker.call(requests.get, url=self.service_url, headers=headers,
                                     params=params, timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            self.logger.error('Instant System service dead (error: {})'.format(e))
            raise
        except requests.Timeout as t:
            self.logger.error('Instant System service dead (error: {})'.format(t))
            raise
        except Exception as e:
            self.logger.exception('Instant System service dead')
            raise

    @staticmethod
    def _get_ridesharing_journeys(raw_journeys):
        """
        This function gives us journeys that contain at least a pure ridesharing offer

        :param raw_journeys:
        :return:
        """
        def has_ridesharing_path(j):
            return next((p for p in j.get('paths', [])
                         if p.get('mode') == 'RIDESHARINGAD'), None)
        return (j for j in raw_journeys if has_ridesharing_path(j))

    def _make_response(self, raw_json):
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
                from_data = p['from']

                res.pickup_place = rsj.Place(addr=from_data.get('name'),
                                             lat=from_data.get('lat'),
                                             lon=from_data.get('lon'))

                to_data = p['to']

                res.dropoff_place = rsj.Place(addr=to_data.get('name'),
                                              lat=to_data.get('lat'),
                                              lon=to_data.get('lon'))

                # shape is a list of type_pb2.GeographicalCoord()
                res.shape = []
                shape = decode_polyline(p.get('shape'), precision=5)
                if not shape or res.pickup_place.lon != shape[0][0] or res.pickup_place.lat != shape[0][1]:
                    coord = type_pb2.GeographicalCoord()
                    coord.lon = res.pickup_place.lon
                    coord.lat = res.pickup_place.lat
                    res.shape.append(coord)
                for c in shape:
                    coord = type_pb2.GeographicalCoord()
                    coord.lon = c[0]
                    coord.lat = c[1]
                    res.shape.append(coord)
                if not shape or res.dropoff_place.lon != shape[0][0] or res.dropoff_place.lat != shape[0][1]:
                    coord = type_pb2.GeographicalCoord()
                    coord.lon = res.dropoff_place.lon
                    coord.lat = res.dropoff_place.lat
                    res.shape.append(coord)

                res.pickup_date_time = utils.make_timestamp_from_str(p['departureDate'])
                res.dropoff_date_time = utils.make_timestamp_from_str(p['arrivalDate'])

                user = ridesharing_ad['user']

                gender = user.get('gender')
                gender_map = {'MALE': rsj.Gender.MALE,
                              'FEMALE': rsj.Gender.FEMALE}

                res.driver = rsj.Individual(alias=user.get('alias'),
                                            gender=gender_map.get(gender, rsj.Gender.UNKNOWN),
                                            image=user.get('imageUrl'),
                                            rate=user.get('rating', {}).get('rate'),
                                            rate_count=user.get('rating', {}).get('count'))

                # the usual form of the price for InstantSystem is: "170 EUR"
                # which means "170 EURO cents" or "1.70 EURO"
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

    def _request_journeys(self, from_coord, to_coord, period_extremity, limit=None):
        """

        :param from_coord: lat,lon ex: '48.109377,-1.682103'
        :param to_coord: lat,lon ex: '48.020335,-1.743929'
        :param period_extremity: a tuple of [timestamp(utc), clockwise]
        :param limit: optional
        :return:
        """
        # format of datetime: 2017-12-25T07:00:00Z
        datetime_str = datetime.datetime.fromtimestamp(period_extremity.datetime)\
            .strftime('%Y-%m-%dT%H:%M:%SZ')

        params = {'from': from_coord,
                  'to': to_coord,
                  ('arrivalDate', 'departureDate')[bool(period_extremity.represents_start)]: datetime_str}

        if limit is not None:
            params.update({'limit', limit})

        resp = self._call_service(params=params)
        if resp:
            r = self._make_response(resp.json())
            self.logger.info('%s ridesharing offers received', len(r))
            return r
        self.logger.debug('0 ridesharing offers received')
        return []

    def _get_feed_publisher(self):
        return self.feed_publisher
