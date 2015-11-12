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
from nose.tools.trivial import eq_
from tests.tests_mechanism import dataset

from jormungandr import utils
from tests import gtfs_realtime_pb2
from tests.check_utils import is_valid_vehicle_journey, get_not_null, journey_basic_query
from tests.rabbitmq_utils import RabbitMQCnxFixture, rt_topic


class MockKirinDisruptionsFixture(RabbitMQCnxFixture):
    """
    Mock a chaos disruption message, in order to check the api
    """
    def _make_mock_item(self, *args, **kwargs):
        return make_mock_kirin_item(*args, **kwargs)


def _get_arrivals(response):
    """
    return a list with the journeys arrival times
    """
    return [j['arrival_date_time'] for j in get_not_null(response, 'journeys')]


def _get_used_vj(response):
    """
    return for each journeys the list of taken vj
    """
    journeys_vj = []
    for j in get_not_null(response, 'journeys'):
        vjs = []
        for s in get_not_null(j, 'sections'):
            for l in s.get('links', []):
                if l['type'] == 'vehicle_journey':
                    vjs.append(l['id'])
                    break
        journeys_vj.append(vjs)

    return journeys_vj


@dataset([("main_routing_test", ['--BROKER.rt_topics='+rt_topic, 'spawn_maintenance_worker'])])
class TestKirinOnVJDeletion(MockKirinDisruptionsFixture):
    def test_vj_deletion(self):
        """
        send a mock kirin vj cancellation and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        eq_(_get_arrivals(response), ['20120614T080222', '20120614T080435'])
        eq_(_get_used_vj(response), [['vjA'], []])

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 0)

        self.send_mock("vjA", "20120614", 'canceled')

        # we should see the disruption
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 1)
        eq_(pt_response['disruptions'][0]['disruption_id'], '96231_2015-07-28_0')

        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        eq_(_get_arrivals(new_response), ['20120614T080435', '20120614T180222'])
        eq_(_get_used_vj(new_response), [[], ['vj:B:1']])

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        eq_(_get_arrivals(new_base), ['20120614T080222', '20120614T080435'])
        eq_(_get_used_vj(new_base), [['vjA'], []])
        # see http://jira.canaltp.fr/browse/NAVP-266,
        # _current_datetime is needed to make it working
        #eq_(len(new_base['disruptions']), 1)
        assert new_base['journeys'] == response['journeys']

@dataset([("main_routing_test", ['--BROKER.rt_topics='+rt_topic, 'spawn_maintenance_worker'])])
class TestKirinOnVJDelay(MockKirinDisruptionsFixture):
    def test_vj_delay(self):
        """
        send a mock kirin vj delay and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        eq_(_get_arrivals(response), ['20120614T080222', '20120614T080435'])
        eq_(_get_used_vj(response), [['vjA'], []])

        pt_response = self.query_region('vehicle_journeys')
        eq_(len(pt_response['vehicle_journeys']), 4)

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 0)

        self.send_mock("vjA", "20120614", 'delayed')

        # A new vj is created
        pt_response = self.query_region('vehicle_journeys')
        eq_(len(pt_response['vehicle_journeys']), 5)

        vj_ids = [vj['id'] for vj in pt_response['vehicle_journeys']]
        assert 'vjA:modified:4' in vj_ids

        # we should see the disruption
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 1)
        eq_(pt_response['disruptions'][0]['disruption_id'], '96231_2015-07-28_0')

        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        eq_(_get_arrivals(new_response), ['20120614T080224', '20120614T080435'])
        eq_(_get_used_vj(new_response), [['vjA:modified:4'], []])

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        eq_(_get_arrivals(new_base), ['20120614T080222', '20120614T080435'])
        eq_(_get_used_vj(new_base), [['vjA'], []])



def make_mock_kirin_item(vj_id, date, status='canceled'):
    feed_message = gtfs_realtime_pb2.FeedMessage()
    feed_message.header.gtfs_realtime_version = '1.0'
    feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
    feed_message.header.timestamp = 0

    entity = feed_message.entity.add()
    entity.id = "96231_2015-07-28_0"
    trip_update = entity.trip_update

    trip = trip_update.trip
    trip.trip_id = vj_id  #
    trip.start_date = date

    if status == 'canceled':
        trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.CANCELED
    elif status == 'delayed':
        trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.SCHEDULED
        # delay on stopA
        stop_time_update = trip_update.stop_time_update.add()
        stop_time_update.stop_id = "stop_point:stopB"
        stop_time_update.arrival.time = 28862
        stop_time_update.departure.time = 28862

        stop_time_update = trip_update.stop_time_update.add()
        stop_time_update.stop_id = "stop_point:stopA"
        stop_time_update.arrival.time = 28864
        stop_time_update.departure.time = 28864

    else:
        #TODO
        pass

    return feed_message.SerializeToString()
