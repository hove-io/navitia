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

import abc
import six
import logging
from jormungandr import utils, new_relic
from collections import namedtuple

class RidesharingServiceError(RuntimeError):
    pass

RsFeedPublisher = namedtuple('RsFeedPublisher', ['id', 'name', 'license', 'url'])

@six.add_metaclass(abc.ABCMeta)
class AbstractRidesharingService(object):

    @abc.abstractmethod
    def status(self):
        """
        :return: a dict contains the status of the service
        """
        pass

    def request_journeys_with_feed_publisher(self, from_coord, to_coord, period_extremity, limit=None):
        """
        This function shouldn't be overwritten!

        :return: a list(mandatory) contains solutions and a feed_publisher
        """
        try:
            journeys = self._request_journeys(from_coord, to_coord, period_extremity, limit)
            feed_publisher = self._get_feed_publisher()

            self.record_call('ok')

            return journeys, feed_publisher
        except RidesharingServiceError as e:
            self.record_call('failure', reason=str(e))
            return [], None

    @abc.abstractmethod
    def _request_journeys(self, from_coord, to_coord, period_extremity, limit=None):
        """
        :return: a list(mandatory) contains solutions
        """
        pass

    @abc.abstractmethod
    def _get_feed_publisher(self):
        """
        :return: Rs_FeedPublisher
        """
        pass

    def _get_rs_id(self):
        return '{}_{}'.format(repr(self.system_id), repr(self.network))

    def record_internal_failure(self, message):
        params = {'ridesharing_service_id': self._get_rs_id(),
                  'message': message}
        new_relic.record_custom_event('ridesharing_internal_failure', params)

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'ridesharing_service_id': self._get_rs_id(),
                  'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('ridesharing_status', params)

    def record_additional_info(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'ridesharing_service_id': self._get_rs_id(),
                  'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('ridesharing_proxy_additional_info', params)



# read the configurations and return the wanted service instance
class Ridesharing(object):

    @staticmethod
    def get_ridesharing_services(instance, ridesharing_configurations):
        logger = logging.getLogger(__name__)
        ridesharing_services = []
        for config in ridesharing_configurations:
            # Set default arguments
            if 'args' not in config:
                config['args'] = {}
            if 'service_url' not in config['args']:
                config['args'].update({'service_url': None})
            if 'instance' not in config['args']:
                config['args'].update({'instance': instance})

            try:
                service = utils.create_object(config)
            except KeyError as e:
                raise KeyError('impossible to build a ridesharing service for {}, '
                               'missing mandatory field in configuration: {}'
                               .format(instance.name, e.message))

            ridesharing_services.append(service)
            logger.info('** Ridesharing: {} used for instance: {} **'
                        .format(type(service).__name__, instance.name))
        return ridesharing_services
