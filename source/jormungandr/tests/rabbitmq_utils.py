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
import uuid
from .check_utils import get_not_null
from .tests_mechanism import AbstractTestFixture
from kombu.connection import BrokerConnection
from kombu.entity import Exchange
from kombu.pools import producers
from retrying import Retrying
import os


# we need to generate a unique topic not to have conflict between tests
rt_topic = 'rt_test_{}'.format(uuid.uuid1())


class RabbitMQCnxFixture(AbstractTestFixture):
    """
    Mock a message over RabbitMQ
    """

    def _get_producer(self):
        producer = producers[self._mock_rabbit_connection].acquire(block=True, timeout=2)
        self._connections.add(producer.connection)
        return producer

    def setUp(self):
        # Note: not a setup_class method, not to conflict with AbstractTestFixture's setup
        rabbit = os.getenv('JORMUNGANDR_BROKER_URL', "pyamqp://guest:guest@localhost:5672")
        self._mock_rabbit_connection = BrokerConnection(rabbit)
        self._connections = {self._mock_rabbit_connection}
        self._exchange = Exchange('navitia', durable=True, delivry_mode=2, type='topic')
        self._mock_rabbit_connection.connect()

        # wait for the cnx to run the test
        self._wait_for_rabbitmq_cnx()

    def tearDown(self):
        # we need to release the amqp connection
        self._mock_rabbit_connection.release()

    def _publish(self, item):
        with self._get_producer() as producer:
            producer.publish(item, exchange=self._exchange, routing_key=rt_topic, declare=[self._exchange])

    def _make_mock_item(self, *args, **kwargs):
        """
        method to overload to create a mock object to send over rabbitmq
        """
        raise NotImplementedError()

    def send_mock(self, *args, **kwargs):
        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')
        item = self._make_mock_item(*args, **kwargs)
        self._publish(item)
        # we sleep a bit to let kraken reload the data
        self._poll_until_reload(last_loaded_data)

    def _poll_until_reload(self, previous_val):
        """
        poll until the kraken have reloaded its data

        check that the last_rt_data_loaded field is different from the first call
        """
        Retrying(
            stop_max_delay=10 * 1000,
            wait_fixed=100,
            retry_on_result=lambda status: get_not_null(status['status'], 'last_rt_data_loaded') == previous_val,
        ).call(lambda: self.query_region('status'))

    def _wait_for_rabbitmq_cnx(self):
        """
        poll until the kraken is connected to rabbitmq

        small timeout because it must not be long (otherwise it way be a server configuration problem)
        """
        Retrying(
            stop_max_delay=1 * 1000,
            wait_fixed=50,
            retry_on_result=lambda status: get_not_null(status['status'], 'is_connected_to_rabbitmq') is False,
        ).call(lambda: self.query_region('status'))
