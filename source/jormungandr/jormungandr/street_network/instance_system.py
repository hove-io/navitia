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

import pybreaker
import requests as requests
from dateutil import parser
import time
import datetime
import logging
from jormungandr import utils
from ride_sharing_journey import RideSharingJourney


def _record_external_failure(message, sn_system_id):
    utils.record_external_failure(message, 'streetnetwork', sn_system_id)


def _call_service(url, api_key, sn_system_id):
    logger = logging.getLogger(__name__)
    breaker = pybreaker.CircuitBreaker(fail_max=100, reset_timeout=1000)
    headers = {u'Authorization': u'apiKey {}'.format(api_key)}
    try:
        return breaker.call(requests.get, url=url, headers=headers, timeout=1000)
    except pybreaker.CircuitBreakerError as e:
        logger.error('Ride sharing service dead (error: {})'.format(e))
        _record_external_failure('circuit breaker open', sn_system_id)
    except requests.Timeout as t:
        logging.getLogger(__name__).error('Ride sharing service dead (error: {})'.format(t))
        _record_external_failure('timeout', sn_system_id)
    except Exception as e:
        logging.getLogger(__name__).exception('Ride sharing service dead')
        _record_external_failure(str(e), sn_system_id)
    return None


def _make_timestamp_from_str(strftime):
    return time.mktime(parser.parse(strftime).utctimetuple())


def _get_ridesharing_journeys(raw_journeys):
    def has_ridesharing_path(j):
        return next((p for p in j.get('paths', [])
                     if p.get('mode') == 'RIDESHARINGAD'), None)
    return (j for j in raw_journeys if has_ridesharing_path(j))


def _make_response(raw_json):
    raw_journeys = raw_json.get('journeys')

    ridesharing_journeys = []

    for j in _get_ridesharing_journeys(raw_journeys):

        for p in j.get('paths'):

            if p.get('mode') != 'RIDESHARINGAD':
                continue

            res = RideSharingJourney()
            res.duration = j.get('duration')
            res.distance = j.get('distance')
            res.shape = j.get('shape')
            res.offer_url = j.get('url')

            from_data = p['rideSharingAd']['from']

            res.pickup_addr = from_data.get('name')
            res.pickup_lat = from_data.get('lat')
            res.pickup_lon = from_data.get('lon')

            to_data = p['rideSharingAd']['to']

            res.dropoff_addr = to_data.get('name')
            res.dropoff_lat = to_data.get('lat')
            res.dropoff_lon = to_data.get('lon')

            res.pickup_date_time = _make_timestamp_from_str(p['departureDate'])
            res.dropoff_date_time = _make_timestamp_from_str(p['arrivalDate'])

            user = p['rideSharingAd']['user']
            res.driver_name = user.get('alais')
            res.driver_gender = user.get('gender')
            res.dirve_image_url = user.get('imageUrl')
            res.driver_rate = user.get('rate')
            res.driver_rate_count = user.get('count')

            price = p['rideSharingAd']['price']
            res.price = price.get('amount')
            res.currency = price.get('currency')

            res.total_seats = None
            res.available_seats = p['rideSharingAd']['vehicle']['availableSeats']

            ridesharing_journeys.append(res)

    return ridesharing_journeys


def request_system_instance(api_key, from_coord, to_coord, request_datetime, datetime_represent=True, limit=-1):
    """

    :param api_key:
    :param from_coord: lat,lon
    :param to_coord: lat,lon
    :param request_datetime: a timestamp (utc)
    :param datetime_represent: bool
    :param limit:
    :return:
    """
    # TODO: url and apiKey should be read from config
    url = "https://dev-api.instant-system.com/core/v1/networks/33/journeys?from={from_coord}&to={to_coord}" \
          "&{datetime_represent}={datetime}&limit={limit}"

    # format of datetime: 2017-12-25T07:00:00Z
    datetime_str = datetime.datetime.fromtimestamp(request_datetime).strftime('%Y-%m-%dT%H:%M:%SZ')

    url = url.format(from_coord=from_coord,
                     to_coord=to_coord,
                     datetime_represent=('arrivalDate', 'departureDate')[datetime_represent],
                     datetime=datetime_str,
                     limit=limit)
    resp = _call_service(url, api_key, 'instance_system')
    if resp:
        return _make_response(resp.json())
    return None
