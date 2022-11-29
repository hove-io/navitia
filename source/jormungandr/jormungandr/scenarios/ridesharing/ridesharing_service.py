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

import abc
import six
from jormungandr import new_relic
from jormungandr.utils import decode_polyline
from navitiacommon import type_pb2
from collections import namedtuple
import pybreaker
import requests as requests


class RidesharingServiceError(RuntimeError):
    def __init__(self, reason, http_status=None, http_reason=None, http_txt=None):
        super(RuntimeError, self).__init__(reason)
        self.http_status = http_status
        self.http_reason = http_reason
        self.http_txt = http_txt

    def get_params(self):
        return {
            'reason': str(self),
            'http_status': self.http_status,
            'http_reason': self.http_reason,
            'http_response': self.http_txt,
        }


RsFeedPublisher = namedtuple('RsFeedPublisher', ['id', 'name', 'license', 'url'])


@six.add_metaclass(abc.ABCMeta)
class AbstractRidesharingService(object):
    @abc.abstractmethod
    def status(self):
        """

        :return: Ridesharing status
        """
        pass

    def _call_service(self, params, headers={}, verify=True):
        """
        :param params: call parameters list
        :return: response from external service
        """
        self.logger.debug('requesting %s', self.network)

        # Format call_params from parameters
        self.call_params = ''
        for key, value in params.items():
            self.call_params += '{}={}&'.format(key, value)

        try:
            return self.breaker.call(
                requests.get,
                url=self.service_url,
                headers=headers,
                params=params,
                timeout=self.timeout,
                verify=verify
            )
        except pybreaker.CircuitBreakerError as e:
            self.logger.error(
                '%s service dead (error: %s)',
                self.network,
                e,
                extra={'ridesharing_service_id': self._get_rs_id()},
            )
            raise RidesharingServiceError('circuit breaker open')
        except requests.Timeout as t:
            self.logger.error(
                '%s service timeout (error: %s)',
                self.network,
                t,
                extra={'ridesharing_service_id': self._get_rs_id()},
            )
            raise RidesharingServiceError('timeout')
        except Exception as e:
            self.logger.exception(
                '%s service error', self.network, extra={'ridesharing_service_id': self._get_rs_id()}
            )
            raise RidesharingServiceError(str(e))

    def request_journeys_with_feed_publisher(
        self, from_coord, to_coord, period_extremity, instance_params, limit=None
    ):
        """
        This function shouldn't be overwritten!

        :return: a list(mandatory) contains solutions and a feed_publisher
        """
        try:
            journeys = self._request_journeys(from_coord, to_coord, period_extremity, instance_params, limit)
            feed_publisher = self._get_feed_publisher()

            self.record_call('ok')

            return journeys, feed_publisher
        except RidesharingServiceError as e:
            self.record_call('failure', **e.get_params())
            self.logger.exception('')
            return [], None

    @abc.abstractmethod
    def _request_journeys(self, from_coord, to_coord, period_extremity, instance_params, limit=None):
        """
        :return: a list(mandatory) contains solutions
        """
        pass

    def _retreive_shape(self, json, field):
        shape = []
        decoded_shape = decode_polyline(json.get(field), precision=5)
        if decoded_shape:
            shape.extend((type_pb2.GeographicalCoord(lon=c[0], lat=c[1]) for c in decoded_shape))
        return shape

    def _retreive_main_shape(self, offer, field, pickup_place, dropoff_place):
        shape = self._retreive_shape(offer, field)
        if not shape or pickup_place.lon != shape[0].lon or pickup_place.lat != shape[0].lat:
            shape.insert(0, type_pb2.GeographicalCoord(lon=pickup_place.lon, lat=pickup_place.lat))
        if not shape or dropoff_place.lon != shape[-1].lon or dropoff_place.lat != shape[-1].lat:
            shape.append(type_pb2.GeographicalCoord(lon=dropoff_place.lon, lat=dropoff_place.lat))
        return shape

    @abc.abstractmethod
    def _get_feed_publisher(self):
        """
        :return: Rs_FeedPublisher
        """
        pass

    def _get_rs_id(self):
        return '{}_{}'.format(six.text_type(self.system_id), six.text_type(self.network))

    def record_internal_failure(self, message):
        params = {
            'ridesharing_service_id': self._get_rs_id(),
            'message': message,
            'ridesharing_service_url': self.service_url,
        }
        new_relic.record_custom_event('ridesharing_internal_failure', params)

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {
            'ridesharing_service_id': self._get_rs_id(),
            'status': status,
            'ridesharing_service_url': self.service_url + '?' + self.call_params,
        }
        params.update(kwargs)
        new_relic.record_custom_event('ridesharing_status', params)

    def record_additional_info(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {
            'ridesharing_service_id': self._get_rs_id(),
            'status': status,
            'ridesharing_service_url': self.service_url,
        }
        params.update(kwargs)
        new_relic.record_custom_event('ridesharing_proxy_additional_info', params)

    def __eq__(self, other):
        return all(
            [
                self.system_id == other.system_id,
                self.service_url == other.service_url,
                self.api_key == other.api_key,
                self.network == other.network,
                type(self).__name__ == type(other).__name__,
            ]
        )
