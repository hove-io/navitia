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
from datetime import datetime
import uuid
from nose.tools.trivial import eq_
from tests.tests_mechanism import dataset

from jormungandr.utils import str_to_time_stamp
from tests import gtfs_realtime_pb2, kirin_pb2
from tests.check_utils import is_valid_vehicle_journey, get_not_null, journey_basic_query, get_used_vj, \
    get_arrivals, get_valid_time, is_valid_disruption
from tests.rabbitmq_utils import RabbitMQCnxFixture, rt_topic


class MockKirinDisruptionsFixture(RabbitMQCnxFixture):
    """
    Mock a chaos disruption message, in order to check the api
    """
    def _make_mock_item(self, *args, **kwargs):
        return make_mock_kirin_item(*args, **kwargs)


def tstamp(str):
    """just to have clearer tests"""
    return str_to_time_stamp(str)


def _dt(h, m, s):
    """syntaxic sugar"""
    return datetime(1900, 1, 1, hour=h, minute=m, second=s)


@dataset([("main_routing_test", ['--BROKER.rt_topics='+rt_topic, 'spawn_maintenance_worker'])])
class TestKirinOnVJDeletion(MockKirinDisruptionsFixture):
    def test_vj_deletion(self):
        """
        send a mock kirin vj cancellation and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        eq_(get_arrivals(response), ['20120614T080222', '20120614T080435'])
        eq_(get_used_vj(response), [['vjA'], []])

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 0)

        self.send_mock("vjA", "20120614", 'canceled', disruption_id='disruption_bob')

        # we should see the disruption
        def _check_train_cancel_disruption(dis):
            is_valid_disruption(dis, chaos_disrup=False)
            eq_(dis['disruption_id'], 'disruption_bob')
            eq_(dis['severity']['effect'], 'NO_SERVICE')
            eq_(len(dis['impacted_objects']), 1)
            ptobj = dis['impacted_objects'][0]['pt_object']
            eq_(ptobj['embedded_type'], 'trip')
            eq_(ptobj['id'], 'vjA')
            eq_(ptobj['name'], 'vjA')
            # for cancellation we do not output the impacted stops
            assert 'impacted_stops' not in dis['impacted_objects'][0]

        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 1)
        _check_train_cancel_disruption(pt_response['disruptions'][0])

        # and we should be able to query for the vj's disruption
        disrup_response = self.query_region('vehicle_journeys/vjA/disruptions')
        eq_(len(disrup_response['disruptions']), 1)
        _check_train_cancel_disruption(disrup_response['disruptions'][0])

        traffic_reports_response = self.query_region('traffic_reports?_current_datetime=20120614T1337')
        traffic_reports = get_not_null(traffic_reports_response, 'traffic_reports')
        assert len(traffic_reports) == 1
        vjs = get_not_null(traffic_reports[0], "vehicle_journeys")
        assert len(vjs) == 1
        assert vjs[0]['id'] == 'vjA'

        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        eq_(get_arrivals(new_response), ['20120614T080435', '20120614T180222'])
        eq_(get_used_vj(new_response), [[], ['vjB']])

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        eq_(get_arrivals(new_base), ['20120614T080222', '20120614T080435'])
        eq_(get_used_vj(new_base), [['vjA'], []])
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
        eq_(get_arrivals(response), ['20120614T080222', '20120614T080435'])
        eq_(get_used_vj(response), [['vjA'], []])

        pt_response = self.query_region('vehicle_journeys')
        eq_(len(pt_response['vehicle_journeys']), 4)

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 0)

        self.send_mock("vjA", "20120614", 'delayed',
           [("stop_point:stopB", tstamp("20120614T080224"), tstamp("20120614T080225"), 'cow on tracks'),
            ("stop_point:stopA", tstamp("20120614T080400"), tstamp("20120614T080400"))],
           disruption_id='vjA_delayed')

        # A new vj is created
        pt_response = self.query_region('vehicle_journeys')
        eq_(len(pt_response['vehicle_journeys']), 5)

        vj_ids = [vj['id'] for vj in pt_response['vehicle_journeys']]
        assert 'vjA:modified:0:vjA_delayed' in vj_ids

        def _check_train_delay_disruption(dis):
            is_valid_disruption(dis, chaos_disrup=False)
            eq_(dis['disruption_id'], 'vjA_delayed')
            eq_(dis['severity']['effect'], 'SIGNIFICANT_DELAYS')
            eq_(len(dis['impacted_objects']), 1)
            ptobj = dis['impacted_objects'][0]['pt_object']
            eq_(ptobj['embedded_type'], 'trip')
            eq_(ptobj['id'], 'vjA')
            eq_(ptobj['name'], 'vjA')
            # for delay we should have detail on the impacted stops
            impacted_objs = get_not_null(dis['impacted_objects'][0], 'impacted_stops')
            assert len(impacted_objs) == 2
            imp_obj1 = impacted_objs[0]
            eq_(get_valid_time(get_not_null(imp_obj1, 'amended_arrival_time')), _dt(h=8, m=2, s=24))
            eq_(get_valid_time(get_not_null(imp_obj1, 'amended_departure_time')), _dt(h=8, m=2, s=25))
            eq_(get_not_null(imp_obj1, 'cause'), 'cow on tracks')
            # for the moment we output 00000, but we should output the right base departure/arrival
            eq_(get_valid_time(get_not_null(imp_obj1, 'base_arrival_time')), _dt(8, 1, 0))
            eq_(get_valid_time(get_not_null(imp_obj1, 'base_departure_time')), _dt(8, 1, 0))

            imp_obj2 = impacted_objs[1]
            eq_(get_valid_time(get_not_null(imp_obj2, 'amended_arrival_time')), _dt(h=8, m=4, s=0))
            eq_(get_valid_time(get_not_null(imp_obj2, 'amended_departure_time')), _dt(h=8, m=4, s=0))
            eq_(imp_obj2['cause'], '')
            eq_(get_valid_time(get_not_null(imp_obj2, 'base_departure_time')), _dt(8, 1, 2))
            eq_(get_valid_time(get_not_null(imp_obj2, 'base_arrival_time')), _dt(8, 1, 2))

        # we should see the disruption
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 1)
        _check_train_delay_disruption(pt_response['disruptions'][0])

        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        eq_(get_arrivals(new_response), ['20120614T080435', '20120614T080520'])
        eq_(get_used_vj(new_response), [[], ['vjA:modified:0:vjA_delayed']])

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        eq_(get_arrivals(new_base), ['20120614T080222', '20120614T080435'])
        eq_(get_used_vj(new_base), [['vjA'], []])

        # We send again the same disruption
        self.send_mock("vjA", "20120614", 'delayed',
           [("stop_point:stopB", tstamp("20120614T080224"), tstamp("20120614T080225"), 'cow on tracks'),
            ("stop_point:stopA", tstamp("20120614T080400"), tstamp("20120614T080400"))],
           disruption_id='vjA_delayed')

        # A new vj is created
        pt_response = self.query_region('vehicle_journeys')
        # No vj cleaning for the moment, vj nb SHOULD be 5, the first vj created for the first
        # disruption is useless
        eq_(len(pt_response['vehicle_journeys']), 6)

        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 1)
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # so the first real-time vj created for the first disruption should be deactivated
        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        eq_(get_arrivals(new_response), ['20120614T080435', '20120614T080520'])
        eq_(get_used_vj(new_response), [[], ['vjA:modified:1:vjA_delayed']])

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        eq_(get_arrivals(new_base), ['20120614T080222', '20120614T080435'])
        eq_(get_used_vj(new_base), [['vjA'], []])

        # we then try to send a delay on another train.
        # we should not have lost the first delay
        self.send_mock("vjB", "20120614", 'delayed',
           [("stop_point:stopB", tstamp("20120614T180224"), tstamp("20120614T180225")),
            ("stop_point:stopA", tstamp("20120614T180400"), tstamp("20120614T180400"), "bob's in the place")])

        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 1)
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # we should also have the disruption on vjB
        eq_(len(self.query_region('vehicle_journeys/vjB?_current_datetime=20120614T1337')['disruptions']), 1)


@dataset([("main_routing_test", ['--BROKER.rt_topics='+rt_topic, 'spawn_maintenance_worker'])])
class TestKirinOnVJDelayDayAfter(MockKirinDisruptionsFixture):
    def test_vj_delay_day_after(self):
        """
        send a mock kirin vj delaying on day after and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        eq_(get_arrivals(response), ['20120614T080222', '20120614T080435']) # pt_walk + vj 08:01
        eq_(get_used_vj(response), [['vjA'], []])

        pt_response = self.query_region('vehicle_journeys')
        eq_(len(pt_response['vehicle_journeys']), 4)

        # check that we have the next vj
        s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
        r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset
        journey_later_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T080500")
        later_response = self.query_region(journey_later_query + "&data_freshness=realtime")
        eq_(get_arrivals(later_response), ['20120614T080935', '20120614T180222']) # pt_walk + vj 18:01
        eq_(get_used_vj(later_response), [[], ['vjB']])

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 0)

        # sending disruption delaying VJ to the next day
        self.send_mock("vjA", "20120614", 'delayed',
           [("stop_point:stopB", tstamp("20120615T070224"), tstamp("20120615T070224")),
            ("stop_point:stopA", tstamp("20120615T070400"), tstamp("20120615T070400"))],
           disruption_id='96231_2015-07-28_0')

        # A new vj is created
        pt_response = self.query_region('vehicle_journeys')
        eq_(len(pt_response['vehicle_journeys']), 5)

        vj_ids = [vj['id'] for vj in pt_response['vehicle_journeys']]
        assert 'vjA:modified:0:96231_2015-07-28_0' in vj_ids

        # we should see the disruption
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        eq_(len(pt_response['disruptions']), 1)
        is_valid_disruption(pt_response['disruptions'][0], chaos_disrup=False)
        eq_(pt_response['disruptions'][0]['disruption_id'], '96231_2015-07-28_0')

        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        eq_(get_arrivals(new_response), ['20120614T080435', '20120614T180222']) # pt_walk + vj 18:01
        eq_(get_used_vj(new_response), [[], ['vjB']])

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        eq_(get_arrivals(new_base), ['20120614T080222', '20120614T080435'])
        eq_(get_used_vj(new_base), [['vjA'], []])

        # the day after, we can use the delayed vj
        journey_day_after_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord=s_coord, to_coord=r_coord, datetime="20120615T070000")
        day_after_response = self.query_region(journey_day_after_query + "&data_freshness=realtime")
        eq_(get_arrivals(day_after_response), ['20120615T070435', '20120615T070520']) # pt_walk + rt 07:02:24
        eq_(get_used_vj(day_after_response), [[], ['vjA:modified:0:96231_2015-07-28_0']])

        # it should not have changed anything for the theoric the day after
        day_after_base = self.query_region(journey_day_after_query + "&data_freshness=base_schedule")
        eq_(get_arrivals(day_after_base), ['20120615T070435', '20120615T080222'])
        eq_(get_used_vj(day_after_base), [[], ['vjA']])


def make_mock_kirin_item(vj_id, date, status='canceled', new_stop_time_list=[], disruption_id=None):
    feed_message = gtfs_realtime_pb2.FeedMessage()
    feed_message.header.gtfs_realtime_version = '1.0'
    feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
    feed_message.header.timestamp = 0

    entity = feed_message.entity.add()
    entity.id = disruption_id or "id={}".format(uuid.uuid1())
    trip_update = entity.trip_update

    trip = trip_update.trip
    trip.trip_id = vj_id  #
    trip.start_date = date

    if status == 'canceled':
        trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.CANCELED
    elif status == 'delayed':
        trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.SCHEDULED
        for st in new_stop_time_list:
            stop_time_update = trip_update.stop_time_update.add()
            stop_time_update.stop_id = st[0]
            stop_time_update.arrival.time = st[1]
            stop_time_update.departure.time = st[2]
            if len(st) > 3:
                stop_time_update.Extensions[kirin_pb2.stoptime_message] = st[3]
    else:
        #TODO
        pass

    return feed_message.SerializeToString()
