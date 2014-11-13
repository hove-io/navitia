# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
from kombu.connection import BrokerConnection
from kombu.entity import Exchange
from kombu.pools import producers
from jormungandr.interfaces.parsers import parse_input_date
from jormungandr.utils import str_to_time_stamp
from tests import gtfs_realtime_pb2
from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *
import chaos_pb2
from time import sleep


class ChaosDisruptionsFixture(AbstractTestFixture):
    """
    Mock a chaos disruption message, in order to check the api
    """
    def _get_producer(self):
        producer = producers[self.mock_chaos_connection].acquire(block=True, timeout=2)
        self._connections.add(producer.connection)
        return producer

    def setup(self):
        self.mock_chaos_connection = BrokerConnection("pyamqp://guest:guest@localhost:5672")
        self._connections = {self.mock_chaos_connection}
        self._exchange = Exchange('navitia', durable=True, delivry_mode=2, type='topic')
        self.mock_chaos_connection.connect()

    def teardown(self):
        #we need to release the amqp connection
        self.mock_chaos_connection.release()

    def send_chaos_disruption(self, disruption_name, impacted_obj, impacted_obj_type):
        item = make_mock_chaos_item(disruption_name, impacted_obj, impacted_obj_type)
        with self._get_producer() as producer:
            producer.publish(item, exchange=self._exchange, routing_key='bob', declare=[self._exchange])

    def poll_until_reload(self, previous_val):
        """
        poll until the kraken have reloaded its data

        check the reload_at field
        """
        total_wait = 0
        waiting_time = 0.2  # in seconds
        while True:
            status = self.query_region('status')

            last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

            if last_loaded_data != previous_val:
                return True
            print "preivous = {}, new = {}".format(previous_val, last_loaded_data)

            if total_wait > 30:
                # more than that is not normal, we fail
                assert False, "kraken has not yet reloaded the data, something's wrong"

            total_wait += waiting_time
            sleep(waiting_time)


@dataset([("main_routing_test", ['--BROKER.rt_topics=bob', 'spawn_maintenance_worker'])])
class TestChaosDisruptions(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """
    def test_disruption_on_stop_area_b(self):
        """
        when calling the pt object stopB, at first we have no disruptions,

        then we mock a disruption sent from chaos, and we call again the pt object stopB
        we then must have a disruption
        """
        response = self.query_region('stop_areas/stopB')

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]
        #at first no disruption
        assert 'disruptions' not in stop

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        self.send_chaos_disruption("bob_the_disruption", "stopB", "stop_area")

        #we sleep a bit to let kraken reload the data
        self.poll_until_reload(last_loaded_data)

        #and we call again, we must have the disruption now
        response = self.query_region('stop_areas/stopB')
        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        disruptions = get_not_null(stop, 'disruptions')

        #at first we got only one disruption on B
        assert len(disruptions) == 1

        assert any(d['uri'] == 'bob_the_disruption' for d in disruptions)


@dataset([("main_routing_test", ['--BROKER.rt_topics=bob', 'spawn_maintenance_worker'])])
class TestChaosDisruptions2(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """
    def test_disruption_on_journey(self):
        """
        same kind of test with a call on journeys

        at first no disruptions, we add one and we should get it
        """
        response = self.query_region(journey_basic_query)

        stops_b_to = [s['to']['stop_point'] for j in response['journeys'] for s in j['sections']
                      if s['to']['embedded_type'] == 'stop_point' and s['to']['id'] == 'stop_point:stopB']

        assert stops_b_to

        for b in stops_b_to:
            assert 'disruptions' not in b['stop_area']

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        #we create a list with every 'to' section to the stop B (the one we added the disruption on)
        self.send_chaos_disruption("bob_the_disruption", "stopB", "stop_area")

        #we sleep a bit to let kraken reload the data
        self.poll_until_reload(last_loaded_data)

        response = self.query_region(journey_basic_query)

        #the response must be still valid (this test the kraken data reloading)
        is_valid_journey_response(response, self.tester, journey_basic_query)

        stops_b_to = [s['to']['stop_point'] for j in response['journeys'] for s in j['sections']
                      if s['to']['embedded_type'] == 'stop_point' and s['to']['id'] == 'stop_point:stopB']

        assert stops_b_to

        for b in stops_b_to:
            disruptions = get_not_null(b['stop_area'], 'disruptions')

            #at first we got only one disruption on A
            assert len(disruptions) == 1

            assert any(d['uri'] == 'bob_the_disruption' for d in disruptions)


def make_mock_chaos_item(disruption_name, impacted_obj, impacted_obj_type):
    feed_message = gtfs_realtime_pb2.FeedMessage()
    feed_message.header.gtfs_realtime_version = '1.0'
    feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
    feed_message.header.timestamp = 0

    feed_entity = feed_message.entity.add()
    feed_entity.id = "toto"
    feed_entity.is_deleted = False

    disruption = feed_entity.Extensions[chaos_pb2.disruption]

    disruption.id = disruption_name
    disruption.cause.id = "CauseTest"
    disruption.cause.wording = "CauseTest"
    disruption.reference = "DisruptionTest"
    disruption.publication_period.start = str_to_time_stamp("20100412T165200")
    disruption.publication_period.end = str_to_time_stamp("20200412T165200")

    # Tag
    tag = disruption.tags.add()
    tag.name = "rer"
    tag.id = "rer"
    tag = disruption.tags.add()
    tag.name = "metro"
    tag.id = "rer"

    # Impacts
    impact = disruption.impacts.add()
    impact.id = "impact_" + disruption_name + "_1"
    impact.severity.id = "SeverityTest"
    impact.severity.wording = "SeverityTest"
    impact.severity.color = "#FFFF00"

    # ApplicationPeriods
    application_period = impact.application_periods.add()
    application_period.start = str_to_time_stamp("20100412T165200")
    application_period.end = str_to_time_stamp("20200412T165200")

    # PTobject
    type_col = {
        "network": chaos_pb2.PtObject.network,
        "stop_area": chaos_pb2.PtObject.stop_area,
        "line": chaos_pb2.PtObject.line,
        "line_section": chaos_pb2.PtObject.line_section,
        "route": chaos_pb2.PtObject.route
    }

    ptobject = impact.informed_entities.add()
    ptobject.uri = impacted_obj
    ptobject.pt_object_type = type_col.get(impacted_obj_type, chaos_pb2.PtObject.unkown_type)

    # Messages
    message = impact.messages.add()
    message.text = "Meassage1 test"
    message.channel.id = "sms"
    message.channel.name = "sms"
    message.channel.max_size = 60
    message.channel.content_type = "text"

    message = impact.messages.add()
    message.text = "Meassage2 test"
    message.channel.name = "email"
    message.channel.id = "email"
    message.channel.max_size = 250
    message.channel.content_type = "html"

    return feed_message.SerializeToString()