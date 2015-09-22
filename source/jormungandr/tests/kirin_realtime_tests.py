# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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

# Note: the tests_mechanism should be the first
# import for the conf to be loaded correctly when only this test is ran
from tests.tests_mechanism import dataset

from jormungandr import utils
from tests import gtfs_realtime_pb2
from tests.check_utils import is_valid_vehicle_journey, get_not_null
from tests.rabbitmq_utils import RabbitMQCnxFixture, rt_topic


class MockKirinDisruptionsFixture(RabbitMQCnxFixture):
    """
    Mock a chaos disruption message, in order to check the api
    """
    def _make_mock_item(self, *args, **kwargs):
        return make_mock_kirin_item(*args, **kwargs)


@dataset([("main_routing_test", ['--BROKER.rt_topics='+rt_topic, 'spawn_maintenance_worker'])])
class TestKirinOnVJDeletion(MockKirinDisruptionsFixture):
    def test_vj_deletion(self):
        """
        send a mock kirin vj deletion and test the VJ API
        """
        self.send_mock()

        response = self.query_region('vehicle_journeys')
        assert len(response) > 0


def make_mock_kirin_item(*args, **kwargs):
    feed_message = gtfs_realtime_pb2.FeedMessage()
    feed_message.header.gtfs_realtime_version = '1.0'
    feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
    feed_message.header.timestamp = 0

    entity = feed_message.entity.add()
    entity.id = "96231_2015-07-28_0"
    trip_update = entity.trip_update

    trip = trip_update.trip
    trip.trip_id = "vehicle_journey:OCETrainTER-87212027-85000109-3:15554"
    trip.start_date = "20150728"
    trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.SCHEDULED

    stop_time = trip_update.stop_time_update.add()
    stop_time.stop_id = "stop_point:OCE:SP:TrainTER-87212027"
    stop_time.arrival.time = utils.str_to_time_stamp("20150728T1721")
    stop_time.departure.time = utils.str_to_time_stamp("20150728T1721")

    return feed_message.SerializeToString()