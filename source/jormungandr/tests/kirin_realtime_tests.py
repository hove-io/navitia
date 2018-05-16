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
from __future__ import absolute_import
from datetime import datetime
import uuid
from tests.tests_mechanism import dataset
from jormungandr.utils import str_to_time_stamp, make_namedtuple
from tests import gtfs_realtime_pb2, kirin_pb2
from tests.check_utils import is_valid_vehicle_journey, get_not_null, journey_basic_query, isochrone_basic_query, \
    get_used_vj, get_arrivals, get_valid_time, is_valid_disruption, check_journey, Journey, Section, \
    SectionStopDT, is_valid_graphical_isochrone
from tests.rabbitmq_utils import RabbitMQCnxFixture, rt_topic
from shapely.geometry import asShape


UpdatedStopTime = make_namedtuple('UpdatedStopTime',
                                  'stop_id', 'arrival', 'departure',
                                  arrival_delay=0, departure_delay=0,
                                  message=None, departure_skipped=False, arrival_skipped=False)


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

MAIN_ROUTING_TEST_SETTING = {'main_routing_test': {'kraken_args': ['--BROKER.rt_topics='+rt_topic,
                                                                   'spawn_maintenance_worker']}}

@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnVJDeletion(MockKirinDisruptionsFixture):
    def test_vj_deletion(self):
        """
        send a mock kirin vj cancellation and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        isochrone = self.query_region(isochrone_basic_query + "&data_freshness=realtime")

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        assert get_arrivals(response) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(response) == [['vjA'], []]

        # Disruption impacting lines A, B, C starts at 06:00 and ends at 11:59:59
        # Get VJ at 12:00 and disruption doesn't appear
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T120000')
        assert len(pt_response['disruptions']) == 0

        is_valid_graphical_isochrone(isochrone, self.tester, isochrone_basic_query + "&data_freshness=realtime")
        geojson = isochrone['isochrones'][0]['geojson']
        multi_poly = asShape(geojson)

        # we have 3 departures and 1 disruption (linked to line A departure)
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 1
        assert len(departures['departures']) == 3

        # A new disruption impacting vjA is created between 08:01:00 and 08:01:01
        self.send_mock("vjA", "20120614", 'canceled', disruption_id='disruption_bob')

        def _check_train_cancel_disruption(dis):
            is_valid_disruption(dis, chaos_disrup=False)
            assert dis['contributor'] == rt_topic
            assert dis['disruption_id'] == 'disruption_bob'
            assert dis['severity']['effect'] == 'NO_SERVICE'
            assert len(dis['impacted_objects']) == 1
            ptobj = dis['impacted_objects'][0]['pt_object']
            assert ptobj['embedded_type'] == 'trip'
            assert ptobj['id'] == 'vjA'
            assert ptobj['name'] == 'vjA'
            # for cancellation we do not output the impacted stops
            assert 'impacted_stops' not in dis['impacted_objects'][0]

        # We should see the disruption
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_cancel_disruption(pt_response['disruptions'][0])

        # and we should be able to query for the vj's disruption
        disrup_response = self.query_region('vehicle_journeys/vjA/disruptions')
        assert len(disrup_response['disruptions']) == 1
        _check_train_cancel_disruption(disrup_response['disruptions'][0])

        traffic_reports_response = self.query_region('traffic_reports?_current_datetime=20120614T0800')
        traffic_reports = get_not_null(traffic_reports_response, 'traffic_reports')
        assert len(traffic_reports) == 1
        vjs = get_not_null(traffic_reports[0], "vehicle_journeys")
        assert len(vjs) == 1
        assert vjs[0]['id'] == 'vjA'

        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        assert set(get_arrivals(new_response)) == set(['20120614T080436', '20120614T080223'])
        assert get_used_vj(new_response) == [['vjM'], []]

        isochrone_realtime = self.query_region(isochrone_basic_query + "&data_freshness=realtime")
        is_valid_graphical_isochrone(isochrone_realtime, self.tester, isochrone_basic_query + "&data_freshness=realtime")
        geojson_realtime = isochrone_realtime['isochrones'][0]['geojson']
        multi_poly_realtime = asShape(geojson_realtime)
        isochrone_base_schedule = self.query_region(isochrone_basic_query + "&data_freshness=base_schedule")
        is_valid_graphical_isochrone(isochrone_base_schedule, self.tester,
                                     isochrone_basic_query + "&data_freshness=base_schedule")
        geojson_base_schedule = isochrone_base_schedule['isochrones'][0]['geojson']
        multi_poly_base_schedule = asShape(geojson_base_schedule)
        assert not multi_poly.difference(multi_poly_realtime).is_empty
        assert multi_poly.equals(multi_poly_base_schedule)

        # We have one less departure (vjA because of disruption)
        # The disruption doesn't appear because the lines departing aren't impacted during the period
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 0
        assert len(departures['departures']) == 2

        # We still have 2 passages in base schedule, but we have the new disruption
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800&data_freshness=base_schedule")
        assert len(departures['disruptions']) == 2
        assert len(departures['departures']) == 3

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base) == [['vjA'], []]
        # see http://jira.canaltp.fr/browse/NAVP-266,
        # _current_datetime is needed to make it work
        # assert len(new_base['disruptions']) == 1

        # remove links as the calling url is not the same
        for j in new_base['journeys']:
            j.pop('links', None)
        for j in response['journeys']:
            j.pop('links', None)
        assert new_base['journeys'] == response['journeys']

@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnVJDelay(MockKirinDisruptionsFixture):
    def test_vj_delay(self):
        """
        send a mock kirin vj delay and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        assert get_arrivals(response) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(response) == [['vjA'], []]

        # we have 3 departures and 1 disruption (linked to the first passage)
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 1
        assert len(departures['departures']) == 3
        assert departures['departures'][0]['stop_date_time']['departure_date_time'] == '20120614T080100'

        pt_response = self.query_region('vehicle_journeys')
        initial_nb_vehicle_journeys = len(pt_response['vehicle_journeys'])
        assert initial_nb_vehicle_journeys == 7

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 0

        self.send_mock("vjA", "20120614", 'modified',
           [UpdatedStopTime("stop_point:stopB",
                            arrival=tstamp("20120614T080224"), departure=tstamp("20120614T080225"),
                            arrival_delay=60 + 24, departure_delay=60+25,
                            message='cow on tracks'),
            UpdatedStopTime("stop_point:stopA",
                            arrival=tstamp("20120614T080400"), departure=tstamp("20120614T080400"),
                            arrival_delay=3 * 60 + 58, departure_delay=3 * 60 + 58)],
           disruption_id='vjA_delayed')

        # A new vj is created, which the vj with the impact of the disruption
        pt_response = self.query_region('vehicle_journeys')
        assert len(pt_response['vehicle_journeys']) == (initial_nb_vehicle_journeys + 1)

        vj_ids = [vj['id'] for vj in pt_response['vehicle_journeys']]
        assert 'vjA:modified:0:vjA_delayed' in vj_ids

        def _check_train_delay_disruption(dis):
            is_valid_disruption(dis, chaos_disrup=False)
            assert dis['disruption_id'] == 'vjA_delayed'
            assert dis['severity']['effect'] == 'SIGNIFICANT_DELAYS'
            assert len(dis['impacted_objects']) == 1
            ptobj = dis['impacted_objects'][0]['pt_object']
            assert ptobj['embedded_type'] == 'trip'
            assert ptobj['id'] == 'vjA'
            assert ptobj['name'] == 'vjA'
            # for delay we should have detail on the impacted stops
            impacted_objs = get_not_null(dis['impacted_objects'][0], 'impacted_stops')
            assert len(impacted_objs) == 2
            imp_obj1 = impacted_objs[0]
            assert get_valid_time(get_not_null(imp_obj1, 'amended_arrival_time')) == _dt(h=8, m=2, s=24)
            assert get_valid_time(get_not_null(imp_obj1, 'amended_departure_time')) == _dt(h=8, m=2, s=25)
            assert get_not_null(imp_obj1, 'cause') == 'cow on tracks'
            assert get_not_null(imp_obj1, 'departure_status') == 'delayed'
            assert get_not_null(imp_obj1, 'arrival_status') == 'delayed'
            assert get_not_null(imp_obj1, 'stop_time_effect') == 'delayed'
            assert get_valid_time(get_not_null(imp_obj1, 'base_arrival_time')) == _dt(8, 1, 0)
            assert get_valid_time(get_not_null(imp_obj1, 'base_departure_time')) == _dt(8, 1, 0)

            imp_obj2 = impacted_objs[1]
            assert get_valid_time(get_not_null(imp_obj2, 'amended_arrival_time')) == _dt(h=8, m=4, s=0)
            assert get_valid_time(get_not_null(imp_obj2, 'amended_departure_time')) == _dt(h=8, m=4, s=0)
            assert imp_obj2['cause'] == ''
            assert get_not_null(imp_obj1, 'stop_time_effect') == 'delayed'
            assert get_not_null(imp_obj1, 'departure_status') == 'delayed'
            assert get_not_null(imp_obj1, 'arrival_status') == 'delayed'
            assert get_valid_time(get_not_null(imp_obj2, 'base_departure_time')) == _dt(8, 1, 2)
            assert get_valid_time(get_not_null(imp_obj2, 'base_arrival_time')) == _dt(8, 1, 2)

        # we should see the disruption
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # In order to not disturb the test, line M which was added afterwards for shared section tests, is forbidden here
        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime&forbidden_uris[]=M&")
        assert get_arrivals(new_response) == ['20120614T080436', '20120614T080520']
        assert get_used_vj(new_response) == [[], ['vjA:modified:0:vjA_delayed']]

        pt_journey = new_response['journeys'][1]

        check_journey(pt_journey,
                      Journey(sections=[Section(departure_date_time='20120614T080208', arrival_date_time='20120614T080225',
                                                base_departure_date_time=None, base_arrival_date_time=None,
                                                stop_date_times=[]),
                                        Section(departure_date_time='20120614T080225', arrival_date_time='20120614T080400',
                                                base_departure_date_time='20120614T080100', base_arrival_date_time='20120614T080102',
                                                stop_date_times=[SectionStopDT(departure_date_time='20120614T080225',
                                                                               arrival_date_time='20120614T080224',
                                                                               base_departure_date_time='20120614T080100',
                                                                               base_arrival_date_time='20120614T080100'),
                                                                 SectionStopDT(
                                                                     departure_date_time='20120614T080400',
                                                                     arrival_date_time='20120614T080400',
                                                                     base_departure_date_time='20120614T080102',
                                                                     base_arrival_date_time='20120614T080102')]),
                                        Section(departure_date_time='20120614T080400', arrival_date_time='20120614T080520',
                                                base_departure_date_time=None, base_arrival_date_time=None,
                                                stop_date_times=[])]))

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base), [['vjA'] == []]

        # we have one delayed departure
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 2
        assert len(departures['departures']) == 3
        assert departures['departures'][1]['stop_date_time']['departure_date_time'] == '20120614T080225'

        # Same as realtime except the departure date time
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800&data_freshness=base_schedule")
        assert len(departures['disruptions']) == 2
        assert len(departures['departures']) == 3
        assert departures['departures'][0]['stop_date_time']['departure_date_time'] == '20120614T080100'

        # We send again the same disruption
        self.send_mock("vjA", "20120614", 'modified',
           [UpdatedStopTime("stop_point:stopB",
                            arrival=tstamp("20120614T080224"), departure=tstamp("20120614T080225"),
                            arrival_delay=60 + 24, departure_delay=60+25,
                            message='cow on tracks'),
            UpdatedStopTime("stop_point:stopA",
                            arrival=tstamp("20120614T080400"), departure=tstamp("20120614T080400"),
                            arrival_delay=3 * 60 + 58, departure_delay=3 * 60 + 58)],
           disruption_id='vjA_delayed')

        # A new vj is created, but a useless vj has been cleaned, so the number of vj does not change
        pt_response = self.query_region('vehicle_journeys')
        assert len(pt_response['vehicle_journeys']) == (initial_nb_vehicle_journeys + 1)

        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # so the first real-time vj created for the first disruption should be deactivated
        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime&forbidden_uris[]=M&")
        assert get_arrivals(new_response) == ['20120614T080436', '20120614T080520']
        assert get_used_vj(new_response), [[] == ['vjA:modified:1:vjA_delayed']]

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base), [['vjA'] == []]

        # we then try to send a delay on another train.
        # we should not have lost the first delay
        self.send_mock("vjB", "20120614", 'modified',
           [UpdatedStopTime("stop_point:stopB", tstamp("20120614T180224"), tstamp("20120614T180225"),
                            arrival_delay=60 + 24, departure_delay=60 + 25,),
            UpdatedStopTime("stop_point:stopA", tstamp("20120614T180400"), tstamp("20120614T180400"),
                            message="bob's in the place")])

        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # we should also have the disruption on vjB
        assert len(self.query_region('vehicle_journeys/vjB?_current_datetime=20120614T1337')['disruptions']) == 1

        ###################################
        # We now send a partial delete on B
        ###################################
        self.send_mock("vjA", "20120614", 'modified',
           [UpdatedStopTime("stop_point:stopB",
                            arrival=tstamp("20120614T080100"), departure=tstamp("20120614T080100")),
            UpdatedStopTime("stop_point:stopA",
                            arrival=tstamp("20120614T080102"), departure=tstamp("20120614T080102"),
                            message='cow on tracks', arrival_skipped=True)],
           disruption_id='vjA_skip_A')

        # A new vj is created
        vjs = self.query_region('vehicle_journeys?_current_datetime=20120614T1337')
        assert len(vjs['vehicle_journeys']) == (initial_nb_vehicle_journeys + 2)

        vjA = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        # we now have 2 disruption on vjA
        assert len(vjA['disruptions']) == 2
        all_dis = {d['id']: d for d in vjA['disruptions']}
        assert 'vjA_skip_A' in all_dis

        dis = all_dis['vjA_skip_A']

        is_valid_disruption(dis, chaos_disrup=False)
        assert dis['disruption_id'] == 'vjA_skip_A'
        assert dis['severity']['effect'] == 'DETOUR'
        assert len(dis['impacted_objects']) == 1
        ptobj = dis['impacted_objects'][0]['pt_object']
        assert ptobj['embedded_type'] == 'trip'
        assert ptobj['id'] == 'vjA'
        assert ptobj['name'] == 'vjA'
        # for delay we should have detail on the impacted stops
        impacted_objs = get_not_null(dis['impacted_objects'][0], 'impacted_stops')
        assert len(impacted_objs) == 2
        imp_obj1 = impacted_objs[0]
        assert get_valid_time(get_not_null(imp_obj1, 'amended_arrival_time')) == _dt(8, 1, 0)
        assert get_valid_time(get_not_null(imp_obj1, 'amended_departure_time')) == _dt(8, 1, 0)
        assert get_not_null(imp_obj1, 'stop_time_effect') == 'unchanged'
        assert get_not_null(imp_obj1, 'arrival_status') == 'unchanged'
        assert get_not_null(imp_obj1, 'departure_status') == 'unchanged'
        assert get_valid_time(get_not_null(imp_obj1, 'base_arrival_time')) == _dt(8, 1, 0)
        assert get_valid_time(get_not_null(imp_obj1, 'base_departure_time')) == _dt(8, 1, 0)

        imp_obj2 = impacted_objs[1]
        assert 'amended_arrival_time' not in imp_obj2
        assert get_not_null(imp_obj2, 'cause') == 'cow on tracks'
        assert get_not_null(imp_obj2, 'stop_time_effect') == 'deleted'  # the stoptime is marked as deleted
        assert get_not_null(imp_obj2, 'arrival_status') == 'deleted'
        assert get_not_null(imp_obj2, 'departure_status') == 'unchanged'  # the departure is not changed
        assert get_valid_time(get_not_null(imp_obj2, 'base_departure_time')) == _dt(8, 1, 2)
        assert get_valid_time(get_not_null(imp_obj2, 'base_arrival_time')) == _dt(8, 1, 2)

@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnVJDelayDayAfter(MockKirinDisruptionsFixture):
    def test_vj_delay_day_after(self):
        """
        send a mock kirin vj delaying on day after and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        assert get_arrivals(response) == ['20120614T080222', '20120614T080436'] # pt_walk + vj 08:01
        assert get_used_vj(response), [['vjA'] == []]

        pt_response = self.query_region('vehicle_journeys')
        initial_nb_vehicle_journeys = len(pt_response['vehicle_journeys'])
        assert initial_nb_vehicle_journeys == 7

        # check that we have the next vj
        s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
        r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset
        journey_later_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T080500")
        later_response = self.query_region(journey_later_query + "&data_freshness=realtime")
        assert get_arrivals(later_response) == ['20120614T080936', '20120614T180222'] # pt_walk + vj 18:01
        assert get_used_vj(later_response), [[] == ['vjB']]

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 0

        # sending disruption delaying VJ to the next day
        self.send_mock("vjA", "20120614", 'modified',
           [UpdatedStopTime("stop_point:stopB", tstamp("20120615T070224"), tstamp("20120615T070224")),
            UpdatedStopTime("stop_point:stopA", tstamp("20120615T070400"), tstamp("20120615T070400"))],
           disruption_id='96231_2015-07-28_0')

        # A new vj is created
        pt_response = self.query_region('vehicle_journeys')
        assert len(pt_response['vehicle_journeys']) == (initial_nb_vehicle_journeys + 1)

        vj_ids = [vj['id'] for vj in pt_response['vehicle_journeys']]
        assert 'vjA:modified:0:96231_2015-07-28_0' in vj_ids

        # we should see the disruption
        pt_response = self.query_region('vehicle_journeys/vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        is_valid_disruption(pt_response['disruptions'][0], chaos_disrup=False)
        assert pt_response['disruptions'][0]['disruption_id'] == '96231_2015-07-28_0'

        # In order to not disturb the test, line M which was added afterwards for shared section tests, is forbidden here
        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime&forbidden_uris[]=M&")
        assert get_arrivals(new_response) == ['20120614T080436', '20120614T180222'] # pt_walk + vj 18:01
        assert get_used_vj(new_response), [[] == ['vjB']]

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base), [['vjA'] == []]

        # the day after, we can use the delayed vj
        journey_day_after_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord=s_coord, to_coord=r_coord, datetime="20120615T070000")
        day_after_response = self.query_region(journey_day_after_query + "&data_freshness=realtime")
        assert get_arrivals(day_after_response) == ['20120615T070436', '20120615T070520'] # pt_walk + rt 07:02:24
        assert get_used_vj(day_after_response), [[] == ['vjA:modified:0:96231_2015-07-28_0']]

        # it should not have changed anything for the theoric the day after
        day_after_base = self.query_region(journey_day_after_query + "&data_freshness=base_schedule")
        assert get_arrivals(day_after_base) == ['20120615T070436', '20120615T080222']
        assert get_used_vj(day_after_base), [[] == ['vjA']]


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnVJOnTime(MockKirinDisruptionsFixture):
    def test_vj_on_time(self):
        """
        We don't want to output an on time disruption on journeys,
        departures, arrivals and route_schedules (also on
        stop_schedules, but no vj disruption is outputed for the
        moment).
        """
        disruptions_before = self.query_region('disruptions?_current_datetime=20120614T080000')
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption same as base schedule
        self.send_mock("vjA", "20120614", 'modified',
           [UpdatedStopTime("stop_point:stopB",
                            arrival_delay=0, departure_delay=0,
                            arrival=tstamp("20120614T080100"), departure=tstamp("20120614T080100"),
                            message='on time'),
            UpdatedStopTime("stop_point:stopA",
                            arrival_delay=0, departure_delay=0,
                            arrival=tstamp("20120614T080102"), departure=tstamp("20120614T080102"))],
           disruption_id='vjA_on_time')

        def has_the_disruption(response):
            return len([d['id'] for d in response['disruptions'] if d['id'] == 'vjA_on_time']) != 0

        # We have a new diruption
        disruptions_after = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])
        assert has_the_disruption(disruptions_after)

        # it's not in journeys
        journey_query = journey_basic_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(journey_query)
        assert not has_the_disruption(response)
        self.is_valid_journey_response(response, journey_query)
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'

        # it's not in departures
        response = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T080000&data_freshness=realtime")
        assert not has_the_disruption(response)
        assert response['departures'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's not in arrivals
        response = self.query_region("stop_points/stop_point:stopA/arrivals?_current_datetime=20120614T080000&data_freshness=realtime")
        assert not has_the_disruption(response)
        assert response['arrivals'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's not in stop_schedules
        response = self.query_region("stop_points/stop_point:stopB/lines/A/stop_schedules?_current_datetime=20120614T080000&data_freshness=realtime")
        assert not has_the_disruption(response)
        assert response['stop_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'
        assert response['stop_schedules'][0]['date_times'][0]['base_date_time'] == '20120614T080100'
        assert response['stop_schedules'][0]['date_times'][0]['date_time'] == '20120614T080100'

        # it's not in route_schedules
        response = self.query_region("stop_points/stop_point:stopB/lines/A/route_schedules?_current_datetime=20120614T080000&data_freshness=realtime")
        assert not has_the_disruption(response)
        #no realtime flags on route_schedules yet

        # New disruption one second late
        self.send_mock("vjA", "20120614", 'modified',
           [UpdatedStopTime("stop_point:stopB",
                            arrival_delay=1, departure_delay=1,
                            arrival=tstamp("20120614T080101"), departure=tstamp("20120614T080101"),
                            message='on time'),
            UpdatedStopTime("stop_point:stopA",
                            arrival_delay=1, departure_delay=1,
                            arrival=tstamp("20120614T080103"), departure=tstamp("20120614T080103"))],
           disruption_id='vjA_late')

        def has_the_disruption(response):
            return len([d['id'] for d in response['disruptions'] if d['id'] == 'vjA_late']) != 0

        # We have a new diruption
        disruptions_after = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert nb_disruptions_before + 2 == len(disruptions_after['disruptions'])
        assert has_the_disruption(disruptions_after)

        # it's in journeys
        response = self.query_region(journey_query)
        assert has_the_disruption(response)
        self.is_valid_journey_response(response, journey_query)
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'

        # it's in departures
        response = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T080000&data_freshness=realtime")
        assert has_the_disruption(response)
        assert response['departures'][0]['stop_date_time']['departure_date_time'] == '20120614T080101'
        assert response['departures'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's in arrivals
        response = self.query_region("stop_points/stop_point:stopA/arrivals?_current_datetime=20120614T080000&data_freshness=realtime")
        assert has_the_disruption(response)
        assert response['arrivals'][0]['stop_date_time']['arrival_date_time'] == '20120614T080103'
        assert response['arrivals'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's in stop_schedules
        response = self.query_region("stop_points/stop_point:stopB/lines/A/stop_schedules?_current_datetime=20120614T080000&data_freshness=realtime")
        assert has_the_disruption(response)
        assert response['stop_schedules'][0]['date_times'][0]['links'][1]['type'] == 'disruption'
        assert response['stop_schedules'][0]['date_times'][0]['date_time'] == '20120614T080101'
        assert response['stop_schedules'][0]['date_times'][0]['base_date_time'] == '20120614T080100'
        assert response['stop_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'

        # it's in route_schedules
        response = self.query_region("stop_points/stop_point:stopB/lines/A/route_schedules?_current_datetime=20120614T080000&data_freshness=realtime")
        assert has_the_disruption(response)
        #no realtime flags on route_schedules yet


def make_mock_kirin_item(vj_id, date, status='canceled', new_stop_time_list=[], disruption_id=None):
    feed_message = gtfs_realtime_pb2.FeedMessage()
    feed_message.header.gtfs_realtime_version = '1.0'
    feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
    feed_message.header.timestamp = 0

    entity = feed_message.entity.add()
    entity.id = disruption_id or "{}".format(uuid.uuid1())
    trip_update = entity.trip_update

    trip = trip_update.trip
    trip.trip_id = vj_id
    trip.start_date = date
    trip.Extensions[kirin_pb2.contributor] = rt_topic

    if status == 'canceled':
        trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.CANCELED
    elif status == 'modified':
        trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.SCHEDULED
        for st in new_stop_time_list:
            stop_time_update = trip_update.stop_time_update.add()
            stop_time_update.stop_id = st.stop_id
            stop_time_update.arrival.time = st.arrival
            stop_time_update.arrival.delay = st.arrival_delay
            stop_time_update.departure.time = st.departure
            stop_time_update.departure.delay = st.departure_delay

            def get_relationship(is_skipped):
                if is_skipped:
                    return gtfs_realtime_pb2.TripUpdate.StopTimeUpdate.SKIPPED
                return gtfs_realtime_pb2.TripUpdate.StopTimeUpdate.SCHEDULED
            stop_time_update.arrival.Extensions[kirin_pb2.stop_time_event_relationship] = \
                get_relationship(st.arrival_skipped)
            stop_time_update.departure.Extensions[kirin_pb2.stop_time_event_relationship] = \
                get_relationship(st.departure_skipped)
            if st.message:
                stop_time_update.Extensions[kirin_pb2.stoptime_message] = st.message
    else:
        #TODO
        pass

    return feed_message.SerializeToString()
