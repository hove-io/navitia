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

# Note: the tests_mechanism should be the first
# import for the conf to be loaded correctly when only this test is ran
from __future__ import absolute_import

from copy import deepcopy

from datetime import datetime
import uuid

from tests.chaos_disruptions_tests import make_mock_chaos_item
from tests.tests_mechanism import dataset
from jormungandr.utils import str_to_time_stamp, make_namedtuple
from tests import gtfs_realtime_pb2, kirin_pb2
from tests.check_utils import (
    get_not_null,
    journey_basic_query,
    isochrone_basic_query,
    get_used_vj,
    get_arrivals,
    get_valid_time,
    is_valid_disruption,
    check_journey,
    Journey,
    Section,
    SectionStopDT,
    is_valid_graphical_isochrone,
    sub_query,
    has_the_disruption,
    get_disruptions_by_id,
    get_links_dict,
)
from tests.rabbitmq_utils import RabbitMQCnxFixture, rt_topic
from shapely.geometry import asShape


UpdatedStopTime = make_namedtuple(
    'UpdatedStopTime',
    'stop_id',
    'arrival',
    'departure',
    arrival_delay=0,
    departure_delay=0,
    message=None,
    departure_skipped=False,
    arrival_skipped=False,
    is_added=False,
    is_detour=False,
)


class MockKirinOrChaosDisruptionsFixture(RabbitMQCnxFixture):
    """
    Mock a kirin or a chaos disruption message, in order to check the api
    """

    def _make_mock_item(self, *args, **kwargs):
        is_kirin = kwargs.pop('is_kirin', True)
        if is_kirin:
            return make_mock_kirin_item(*args, **kwargs)
        else:
            return make_mock_chaos_item(*args, **kwargs)


class MockKirinDisruptionsFixture(RabbitMQCnxFixture):
    """
    Mock a kirin disruption message, in order to check the api
    """

    def _make_mock_item(self, *args, **kwargs):
        return make_mock_kirin_item(*args, **kwargs)


def tstamp(str):
    """just to have clearer tests"""
    return str_to_time_stamp(str)


def _dt(h, m, s):
    """syntaxic sugar"""
    return datetime(1900, 1, 1, hour=h, minute=m, second=s)


MAIN_ROUTING_TEST_SETTING = {
    'main_routing_test': {'kraken_args': ['--BROKER.rt_topics=' + rt_topic, 'spawn_maintenance_worker']}
}


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
        assert get_used_vj(response) == [['vehicle_journey:vjA'], []]

        # Disruption impacting lines A, B, C starts at 06:00 and ends at 11:59:59
        # Get VJ at 12:00 and disruption doesn't appear
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T120000')
        assert len(pt_response['disruptions']) == 0

        is_valid_graphical_isochrone(isochrone, self.tester, isochrone_basic_query + "&data_freshness=realtime")
        geojson = isochrone['isochrones'][0]['geojson']
        multi_poly = asShape(geojson)

        # we have 3 departures and 1 disruption (linked to line A departure)
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 1
        assert len(departures['departures']) == 4

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
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_cancel_disruption(pt_response['disruptions'][0])

        # and we should be able to query for the vj's disruption
        disrup_response = self.query_region('vehicle_journeys/vehicle_journey:vjA/disruptions')
        assert len(disrup_response['disruptions']) == 1
        _check_train_cancel_disruption(disrup_response['disruptions'][0])

        traffic_reports_response = self.query_region('traffic_reports?_current_datetime=20120614T0800')
        traffic_reports = get_not_null(traffic_reports_response, 'traffic_reports')
        assert len(traffic_reports) == 1
        vjs = get_not_null(traffic_reports[0], "vehicle_journeys")
        assert len(vjs) == 1
        assert vjs[0]['id'] == 'vehicle_journey:vjA'

        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        assert set(get_arrivals(new_response)) == set(['20120614T080436', '20120614T080223'])
        assert get_used_vj(new_response) == [['vehicle_journey:vjM'], []]

        isochrone_realtime = self.query_region(isochrone_basic_query + "&data_freshness=realtime")
        is_valid_graphical_isochrone(
            isochrone_realtime, self.tester, isochrone_basic_query + "&data_freshness=realtime"
        )
        geojson_realtime = isochrone_realtime['isochrones'][0]['geojson']
        multi_poly_realtime = asShape(geojson_realtime)
        isochrone_base_schedule = self.query_region(isochrone_basic_query + "&data_freshness=base_schedule")
        is_valid_graphical_isochrone(
            isochrone_base_schedule, self.tester, isochrone_basic_query + "&data_freshness=base_schedule"
        )
        geojson_base_schedule = isochrone_base_schedule['isochrones'][0]['geojson']
        multi_poly_base_schedule = asShape(geojson_base_schedule)
        assert not multi_poly.difference(multi_poly_realtime).is_empty
        assert multi_poly.equals(multi_poly_base_schedule)

        # We have one less departure (vjA because of disruption)
        # The disruption doesn't appear because the lines departing aren't impacted during the period
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 0
        assert len(departures['departures']) == 3

        # We still have 2 passages in base schedule, but we have the new disruption
        departures = self.query_region(
            "stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800&data_freshness=base_schedule"
        )
        assert len(departures['disruptions']) == 2
        assert len(departures['departures']) == 4

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base) == [['vehicle_journey:vjA'], []]
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
class TestMainStopAreaWeightFactorWithKirinUpdate(MockKirinDisruptionsFixture):
    def test_main_stop_area_weight_factor_with_kirin_update(self):
        response = self.query_region("places?type[]=stop_area&q=stop")
        places = response['places']
        assert len(places) == 3
        assert places[0]['id'] == 'stopA'
        assert places[1]['id'] == 'stopB'
        assert places[2]['id'] == 'stopC'

        # only used to activate the autocomplete rebuild process
        self.send_mock("id", "20120614", 'type', disruption_id='disruption_bob')

        response = self.query_region("places?type[]=stop_area&q=stop&_main_stop_area_weight_factor=5")
        places = response['places']
        assert len(places) == 3
        assert places[0]['id'] == 'stopC'
        assert places[1]['id'] == 'stopA'
        assert places[2]['id'] == 'stopB'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestAutocompleteOnWaysWithKirinUpdate(MockKirinDisruptionsFixture):
    def test_autocomplete_on_ways_with_kirin_update(self):
        response = self.query_region("places?&q=rue ts")
        places = response['places']
        assert len(places) == 1
        assert places[0]['embedded_type'] == 'address'
        assert places[0]['name'] == 'rue ts (Condom)'

        # only used to activate the autocomplete rebuild process
        self.send_mock("id", "20120614", 'type', disruption_id='disruption_bob')

        # After injection of realtime, we should not return way with visible=false.
        response = self.query_region("places?&q=rue ts")
        places = response['places']
        assert len(places) == 1
        assert places[0]['embedded_type'] == 'address'
        assert places[0]['name'] == 'rue ts (Condom)'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnVJDelay(MockKirinDisruptionsFixture):
    def test_vj_delay(self):
        """
        send a mock kirin vj delay and test that the vj is not taken
        """
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        assert get_arrivals(response) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(response) == [['vehicle_journey:vjA'], []]

        # we have 3 departures and 1 disruption (linked to the first passage)
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 1
        assert len(departures['departures']) == 4
        assert departures['departures'][0]['stop_date_time']['departure_date_time'] == '20120614T080100'

        pt_response = self.query_region('vehicle_journeys')
        initial_nb_vehicle_journeys = len(pt_response['vehicle_journeys'])
        assert initial_nb_vehicle_journeys == 9

        # Search with code on vehicle_journey returns only one element having the code.
        pt_response = self.query_region(
            'vehicle_journeys?filter=vehicle_journey.has_code(source, source_code_vjA)'
        )
        vjs = pt_response['vehicle_journeys']
        assert len(vjs) == 1
        assert len(vjs[0]['codes']) == 2
        assert vjs[0]['id'] == 'vehicle_journey:vjA'

        # Test that vehicle_journey vehicle_journey:vjB doesn't have any code
        pt_response = self.query_region('trips/vjB/vehicle_journeys')
        vjs = pt_response['vehicle_journeys']
        assert len(vjs) == 1
        assert len(vjs[0]['codes']) == 0
        assert vjs[0]['id'] == 'vehicle_journey:vjB'

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 0

        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120614T080224"),
                    departure=tstamp("20120614T080225"),
                    arrival_delay=60 + 24,
                    departure_delay=60 + 25,
                    message='cow on tracks',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080400"),
                    departure=tstamp("20120614T080400"),
                    arrival_delay=3 * 60 + 58,
                    departure_delay=3 * 60 + 58,
                ),
            ],
            disruption_id='vjA_delayed',
        )

        # A new vj is created, which the vj with the impact of the disruption
        pt_response = self.query_region('vehicle_journeys')
        assert len(pt_response['vehicle_journeys']) == (initial_nb_vehicle_journeys + 1)

        # Search with code on vehicle_journey returns two elements.
        # Here the realtime vehicle_journey takes the code of it's base vehicle_journey(vehicle_journey:vjA)
        pt_response = self.query_region(
            'vehicle_journeys?filter=vehicle_journey.has_code(source, source_code_vjA)'
        )
        vjs = pt_response['vehicle_journeys']
        assert len(vjs) == 2
        assert len(vjs[0]['codes']) == 2
        assert len(vjs[1]['codes']) == 2
        assert vjs[0]['id'] == 'vehicle_journey:vjA'
        assert 'vehicle_journey:vjA:RealTime:' in vjs[1]['id']

        vj_ids = [vj['id'] for vj in pt_response['vehicle_journeys']]
        assert vjs[1]['id'] in vj_ids

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
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # In order to not disturb the test, line M which was added afterwards for shared section tests, is forbidden here
        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime&forbidden_uris[]=M&")
        assert get_arrivals(new_response) == ['20120614T080436', '20120614T080520']
        used_vjs = get_used_vj(new_response)
        assert used_vjs[0] == []
        assert "vehicle_journey:vjA:RealTime:" in used_vjs[1][0]

        pt_journey = new_response['journeys'][1]

        check_journey(
            pt_journey,
            Journey(
                sections=[
                    Section(
                        departure_date_time='20120614T080208',
                        arrival_date_time='20120614T080225',
                        base_departure_date_time=None,
                        base_arrival_date_time=None,
                        stop_date_times=[],
                    ),
                    Section(
                        departure_date_time='20120614T080225',
                        arrival_date_time='20120614T080400',
                        base_departure_date_time='20120614T080100',
                        base_arrival_date_time='20120614T080102',
                        stop_date_times=[
                            SectionStopDT(
                                departure_date_time='20120614T080225',
                                arrival_date_time='20120614T080224',
                                base_departure_date_time='20120614T080100',
                                base_arrival_date_time='20120614T080100',
                            ),
                            SectionStopDT(
                                departure_date_time='20120614T080400',
                                arrival_date_time='20120614T080400',
                                base_departure_date_time='20120614T080102',
                                base_arrival_date_time='20120614T080102',
                            ),
                        ],
                    ),
                    Section(
                        departure_date_time='20120614T080400',
                        arrival_date_time='20120614T080520',
                        base_departure_date_time=None,
                        base_arrival_date_time=None,
                        stop_date_times=[],
                    ),
                ]
            ),
        )

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base), [['vehicle_journey:vjA'] == []]

        # we have one delayed departure
        departures = self.query_region("stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800")
        assert len(departures['disruptions']) == 2
        assert len(departures['departures']) == 4
        assert departures['departures'][1]['stop_date_time']['departure_date_time'] == '20120614T080225'

        # Same as realtime except the departure date time
        departures = self.query_region(
            "stop_points/stop_point:stopB/departures?_current_datetime=20120614T0800&data_freshness=base_schedule"
        )
        assert len(departures['disruptions']) == 2
        assert len(departures['departures']) == 4
        assert departures['departures'][0]['stop_date_time']['departure_date_time'] == '20120614T080100'

        # We send again the same disruption
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120614T080224"),
                    departure=tstamp("20120614T080225"),
                    arrival_delay=60 + 24,
                    departure_delay=60 + 25,
                    message='cow on tracks',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080400"),
                    departure=tstamp("20120614T080400"),
                    arrival_delay=3 * 60 + 58,
                    departure_delay=3 * 60 + 58,
                ),
            ],
            disruption_id='vjA_delayed',
        )

        # A new vj is created, but a useless vj has been cleaned, so the number of vj does not change
        pt_response = self.query_region('vehicle_journeys')
        assert len(pt_response['vehicle_journeys']) == (initial_nb_vehicle_journeys + 1)

        # Search with code on vehicle_journey returns two elements.
        pt_response = self.query_region(
            'vehicle_journeys?filter=vehicle_journey.has_code(source, source_code_vjA)'
        )
        vjs = pt_response['vehicle_journeys']
        assert len(vjs) == 2
        assert len(vjs[0]['codes']) == 2
        assert len(vjs[1]['codes']) == 2
        assert vjs[0]['id'] == 'vehicle_journey:vjA'
        assert 'vehicle_journey:vjA:RealTime:' in vjs[1]['id']

        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # So the first real-time vj created for the first disruption should be deactivated
        # In order to not disturb the test, line M which was added afterwards for shared section tests, is forbidden here
        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime&forbidden_uris[]=M&")
        assert get_arrivals(new_response) == ['20120614T080436', '20120614T080520']
        assert get_used_vj(new_response), [[] == ['vehicle_journey:vjA:modified:1:vjA_delayed']]

        # it should not have changed anything for the theoric
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base), [['vehicle_journey:vjA'] == []]

        # we then try to send a delay on another train.
        # we should not have lost the first delay
        self.send_mock(
            "vjB",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    tstamp("20120614T180224"),
                    tstamp("20120614T180225"),
                    arrival_delay=60 + 24,
                    departure_delay=60 + 25,
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    tstamp("20120614T180400"),
                    tstamp("20120614T180400"),
                    message="bob's in the place",
                ),
            ],
        )

        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        _check_train_delay_disruption(pt_response['disruptions'][0])

        # we should also have the disruption on vjB
        assert (
            len(
                self.query_region('vehicle_journeys/vehicle_journey:vjB?_current_datetime=20120614T1337')[
                    'disruptions'
                ]
            )
            == 1
        )

        ###################################
        # We now send a partial delete on B
        ###################################
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB", arrival=tstamp("20120614T080100"), departure=tstamp("20120614T080100")
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                    message='cow on tracks',
                    arrival_skipped=True,
                ),
            ],
            disruption_id='vjA_skip_A',
        )

        # A new vj is created
        vjs = self.query_region('vehicle_journeys?_current_datetime=20120614T1337')
        assert len(vjs['vehicle_journeys']) == (initial_nb_vehicle_journeys + 2)

        # The realtime vehicle_journey on a base vehicle_journey without any code also should not have any code
        pt_response = self.query_region('trips/vjB/vehicle_journeys')
        vjs = pt_response['vehicle_journeys']
        assert len(vjs) == 2
        assert len(vjs[0]['codes']) == 0
        assert len(vjs[1]['codes']) == 0
        assert vjs[0]['id'] == 'vehicle_journey:vjB'
        assert 'vehicle_journey:vjB:RealTime' in vjs[1]['id']

        vjA = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        # we now have 2 disruption on vjA
        assert len(vjA['disruptions']) == 2
        all_dis = {d['id']: d for d in vjA['disruptions']}
        assert 'vjA_skip_A' in all_dis

        dis = all_dis['vjA_skip_A']

        is_valid_disruption(dis, chaos_disrup=False)
        assert dis['disruption_id'] == 'vjA_skip_A'
        assert dis['severity']['effect'] == 'REDUCED_SERVICE'
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
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')

        # with no cancellation, we have 2 journeys, one direct and one with the vj:A:0
        assert get_arrivals(response) == ['20120614T080222', '20120614T080436']  # pt_walk + vj 08:01
        assert get_used_vj(response), [['vjA'] == []]

        pt_response = self.query_region('vehicle_journeys')
        initial_nb_vehicle_journeys = len(pt_response['vehicle_journeys'])
        assert initial_nb_vehicle_journeys == 9

        # check that we have the next vj
        s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
        r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset
        journey_later_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord=s_coord, to_coord=r_coord, datetime="20120614T080500"
        )
        later_response = self.query_region(journey_later_query + "&data_freshness=realtime")
        assert get_arrivals(later_response) == ['20120614T080936', '20120614T180222']  # pt_walk + vj 18:01
        assert get_used_vj(later_response), [[] == ['vehicle_journey:vjB']]

        # no disruption yet
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 0

        # sending disruption delaying VJ to the next day
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime("stop_point:stopB", tstamp("20120615T070224"), tstamp("20120615T070224")),
                UpdatedStopTime("stop_point:stopA", tstamp("20120615T070400"), tstamp("20120615T070400")),
            ],
            disruption_id='96231_2015-07-28_0',
            effect='unknown',
        )

        # A new vj is created
        pt_response = self.query_region('vehicle_journeys')
        assert len(pt_response['vehicle_journeys']) == (initial_nb_vehicle_journeys + 1)

        vj_ids = [vj['id'] for vj in pt_response['vehicle_journeys']]
        assert any(('vehicle_journey:vjA:RealTime:' in vj_id for vj_id in vj_ids))

        # we should see the disruption
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')
        assert len(pt_response['disruptions']) == 1
        is_valid_disruption(pt_response['disruptions'][0], chaos_disrup=False)
        assert pt_response['disruptions'][0]['disruption_id'] == '96231_2015-07-28_0'

        # In order to not disturb the test, line M which was added afterwards for shared section tests, is forbidden here
        new_response = self.query_region(journey_basic_query + "&data_freshness=realtime&forbidden_uris[]=M&")
        assert get_arrivals(new_response) == ['20120614T080436', '20120614T180222']  # pt_walk + vj 18:01
        assert get_used_vj(new_response), [[] == ['vjB']]

        # it should not have changed anything for the base-schedule
        new_base = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert get_arrivals(new_base) == ['20120614T080222', '20120614T080436']
        assert get_used_vj(new_base), [['vjA'] == []]

        # the day after, we can use the delayed vj
        journey_day_after_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord=s_coord, to_coord=r_coord, datetime="20120615T070000"
        )
        day_after_response = self.query_region(journey_day_after_query + "&data_freshness=realtime")
        assert get_arrivals(day_after_response) == [
            '20120615T070436',
            '20120615T070520',
        ]  # pt_walk + rt 07:02:24
        assert get_used_vj(day_after_response), [[] == ['vehicle_journey:vjA:modified:0:96231_2015-07-28_0']]

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
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id='vjA_on_time',
            effect='unknown',
        )

        # We have a new diruption
        disruptions_after = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])
        assert has_the_disruption(disruptions_after, 'vjA_on_time')

        # it's not in journeys
        journey_query = journey_basic_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(journey_query)
        assert not has_the_disruption(response, 'vjA_on_time')
        self.is_valid_journey_response(response, journey_query)
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'

        # it's not in departures
        response = self.query_region(
            "stop_points/stop_point:stopB/departures?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert not has_the_disruption(response, 'vjA_on_time')
        assert response['departures'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's not in arrivals
        response = self.query_region(
            "stop_points/stop_point:stopA/arrivals?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert not has_the_disruption(response, 'vjA_on_time')
        assert response['arrivals'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's not in stop_schedules
        response = self.query_region(
            "stop_points/stop_point:stopB/lines/A/stop_schedules?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert not has_the_disruption(response, 'vjA_on_time')
        assert response['stop_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'
        assert response['stop_schedules'][0]['date_times'][0]['base_date_time'] == '20120614T080100'
        assert response['stop_schedules'][0]['date_times'][0]['date_time'] == '20120614T080100'

        # it's not in terminus_schedules
        response = self.query_region(
            "stop_points/stop_point:stopB/lines/A/terminus_schedules?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert not has_the_disruption(response, 'vjA_on_time')
        assert response['terminus_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'
        assert response['terminus_schedules'][0]['date_times'][0]['base_date_time'] == '20120614T080100'
        assert response['terminus_schedules'][0]['date_times'][0]['date_time'] == '20120614T080100'

        # it's not in route_schedules
        response = self.query_region(
            "stop_points/stop_point:stopB/lines/A/route_schedules?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert not has_the_disruption(response, 'vjA_on_time')
        # no realtime flags on route_schedules yet

        # New disruption one second late
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=1,
                    departure_delay=1,
                    arrival=tstamp("20120614T080101"),
                    departure=tstamp("20120614T080101"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival_delay=1,
                    departure_delay=1,
                    arrival=tstamp("20120614T080103"),
                    departure=tstamp("20120614T080103"),
                ),
            ],
            disruption_id='vjA_late',
        )

        # We have a new diruption
        disruptions_after = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert nb_disruptions_before + 2 == len(disruptions_after['disruptions'])
        assert has_the_disruption(disruptions_after, 'vjA_late')

        # it's in journeys
        response = self.query_region(journey_query)
        assert has_the_disruption(response, 'vjA_late')
        self.is_valid_journey_response(response, journey_query)
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'

        # it's in departures
        response = self.query_region(
            "stop_points/stop_point:stopB/departures?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert has_the_disruption(response, 'vjA_late')
        assert response['departures'][0]['stop_date_time']['departure_date_time'] == '20120614T080101'
        assert response['departures'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's in arrivals
        response = self.query_region(
            "stop_points/stop_point:stopA/arrivals?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert has_the_disruption(response, 'vjA_late')
        assert response['arrivals'][0]['stop_date_time']['arrival_date_time'] == '20120614T080103'
        assert response['arrivals'][0]['stop_date_time']['data_freshness'] == 'realtime'

        # it's in stop_schedules
        response = self.query_region(
            "stop_points/stop_point:stopB/lines/A/stop_schedules?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert has_the_disruption(response, 'vjA_late')
        assert response['stop_schedules'][0]['date_times'][0]['links'][1]['type'] == 'disruption'
        assert response['stop_schedules'][0]['date_times'][0]['date_time'] == '20120614T080101'
        assert response['stop_schedules'][0]['date_times'][0]['base_date_time'] == '20120614T080100'
        assert response['stop_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'

        # it's in terminus_schedules
        response = self.query_region(
            "stop_points/stop_point:stopB/lines/A/terminus_schedules?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert has_the_disruption(response, 'vjA_late')
        assert response['terminus_schedules'][0]['date_times'][0]['links'][1]['type'] == 'disruption'
        assert response['terminus_schedules'][0]['date_times'][0]['date_time'] == '20120614T080101'
        assert response['terminus_schedules'][0]['date_times'][0]['base_date_time'] == '20120614T080100'
        assert response['terminus_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'

        # it's in route_schedules
        response = self.query_region(
            "stop_points/stop_point:stopB/lines/A/route_schedules?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert has_the_disruption(response, 'vjA_late')
        # no realtime flags on route_schedules yet


MAIN_ROUTING_TEST_SETTING_NO_ADD = {
    'main_routing_test': {
        'kraken_args': [
            '--BROKER.rt_topics=' + rt_topic,
            'spawn_maintenance_worker',
        ]  # also check that by 'default is_realtime_add_enabled=0'
    }
}


MAIN_ROUTING_TEST_SETTING = deepcopy(MAIN_ROUTING_TEST_SETTING_NO_ADD)
MAIN_ROUTING_TEST_SETTING['main_routing_test']['kraken_args'].append('--GENERAL.is_realtime_add_enabled=1')
MAIN_ROUTING_TEST_SETTING['main_routing_test']['kraken_args'].append('--GENERAL.is_realtime_add_trip_enabled=1')


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnNewStopTimeAtTheEnd(MockKirinDisruptionsFixture):
    def test_add_and_delete_one_stop_time_at_the_end(self):
        """
        1. create a new_stop_time to add a final stop in C
        test that a new journey is possible with section type = public_transport from B to C
        2. delete the added stop_time and verify that the public_transport section is absent
        3. delete again stop_time and verify that the public_transport section is absent
        """
        disruptions_before = self.query_region('disruptions?_current_datetime=20120614T080000')
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption with two stop_times same as base schedule and
        # a new stop_time on stop_point:stopC added at the end
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080104"),
                    departure=tstamp("20120614T080104"),
                ),
            ],
            disruption_id='new_stop_time',
        )

        # We have a new disruption to add a new stop_time at stop_point:stopC in vehicle_journey 'VJA'
        disruptions_after = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])
        assert has_the_disruption(disruptions_after, 'new_stop_time')
        last_disrupt = disruptions_after['disruptions'][-1]
        assert last_disrupt['severity']['effect'] == 'MODIFIED_SERVICE'

        journey_query = journey_basic_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(journey_query)
        assert has_the_disruption(response, 'new_stop_time')
        self.is_valid_journey_response(response, journey_query)
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'
        assert response['journeys'][0]['sections'][1]['display_informations']['physical_mode'] == 'Tramway'

        B_C_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord='stop_point:stopB', to_coord='stop_point:stopC', datetime='20120614T080000'
        )

        # The result with base_schedule should not have a journey with public_transport from B to C
        base_journey_query = B_C_query + "&data_freshness=base_schedule&_current_datetime=20120614T080000"
        response = self.query_region(base_journey_query)
        assert not has_the_disruption(response, 'new_stop_time')
        self.is_valid_journey_response(response, base_journey_query)
        assert len(response['journeys']) == 1  # check we only have one journey
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert 'data_freshness' not in response['journeys'][0]['sections'][0]  # means it's base_schedule

        # The result with realtime should have a journey with public_transport from B to C
        rt_journey_query = B_C_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(rt_journey_query)
        assert has_the_disruption(response, 'new_stop_time')
        self.is_valid_journey_response(response, rt_journey_query)
        assert len(response['journeys']) == 2  # check there's a new journey possible
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'
        assert response['journeys'][0]['sections'][0]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][0]['to']['id'] == 'stop_point:stopC'
        assert response['journeys'][0]['sections'][0]['duration'] == 4
        assert response['journeys'][0]['status'] == 'MODIFIED_SERVICE'
        assert 'data_freshness' not in response['journeys'][1]['sections'][0]  # means it's base_schedule
        assert response['journeys'][1]['sections'][0]['type'] == 'street_network'

        # New disruption with a deleted stop_time recently added at stop_point:stopC
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080104"),
                    departure=tstamp("20120614T080104"),
                    message='stop_time deleted',
                    arrival_skipped=True,
                )
            ],
            disruption_id='deleted_stop_time',
        )

        # We have a new disruption with a deleted stop_time at stop_point:stopC in vehicle_journey 'VJA'
        disruptions_with_deleted = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disruptions_after['disruptions']) + 1 == len(disruptions_with_deleted['disruptions'])
        assert has_the_disruption(disruptions_with_deleted, 'deleted_stop_time')

        # The result with realtime should not have a journey with public_transport from B to C
        # since the added stop_time has been deleted by the last disruption
        rt_journey_query = B_C_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(rt_journey_query)
        assert not has_the_disruption(response, 'added_stop_time')
        self.is_valid_journey_response(response, rt_journey_query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert 'data_freshness' not in response['journeys'][0]['sections'][0]

        # New disruption with a deleted stop_time already deleted at stop_point:stopC
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080104"),
                    departure=tstamp("20120614T080104"),
                    message='stop_time deleted',
                    arrival_skipped=True,
                )
            ],
            disruption_id='re_deleted_stop_time',
        )

        # We have a new disruption with a deleted stop_time at stop_point:stopC in vehicle_journey 'VJA'
        disruptions_with_deleted = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disruptions_after['disruptions']) + 2 == len(disruptions_with_deleted['disruptions'])
        assert has_the_disruption(disruptions_with_deleted, 're_deleted_stop_time')

        # The result with realtime should not have a journey with public_transport from B to C
        rt_journey_query = B_C_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(rt_journey_query)
        assert not has_the_disruption(response, 'added_stop_time')
        self.is_valid_journey_response(response, rt_journey_query)
        assert len(response['journeys']) == 1


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinReadTripEffectFromTripUpdate(MockKirinDisruptionsFixture):
    def test_read_trip_effect_from_tripupdate(self):
        disruptions_before = self.query_region('disruptions?_current_datetime=20120614T080000')
        nb_disruptions_before = len(disruptions_before['disruptions'])
        assert nb_disruptions_before == 12

        vjs_before = self.query_region('vehicle_journeys')
        assert len(vjs_before['vehicle_journeys']) == 9

        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120614T080224"),
                    departure=tstamp("20120614T080225"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080400"),
                    departure=tstamp("20120614T080400"),
                    message='stop_time deleted',
                    arrival_skipped=True,
                    departure_skipped=True,
                ),
            ],
            disruption_id='reduced_service_vjA',
            effect='reduced_service',
        )
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == 13
        assert has_the_disruption(disrupts, 'reduced_service_vjA')
        last_disrupt = disrupts['disruptions'][-1]
        assert last_disrupt['severity']['effect'] == 'REDUCED_SERVICE'
        assert last_disrupt['severity']['name'] == 'reduced service'

        vjs_after = self.query_region('vehicle_journeys')
        # we got a new vj due to the disruption, which means the disruption is handled correctly
        assert len(vjs_after['vehicle_journeys']) == 10


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinSchedulesNewStopTimeInBetween(MockKirinDisruptionsFixture):
    def test_schedules_add_one_stop_time(self):
        """
        Checking that when a stop is added on a trip, /departures and /stop_schedules are updated
        """
        disruptions_before = self.query_region('disruptions?_current_datetime=20120614T080000')

        base_query = 'stop_areas/stopC/{api}?from_datetime={dt}&_current_datetime={dt}&data_freshness={df}'

        departures = self.query_region(base_query.format(api='departures', dt='20120614T080100', df='realtime'))
        assert len(departures['departures']) == 0

        stop_schedules = self.query_region(
            base_query.format(api='stop_schedules', dt='20120614T080100', df='realtime')
        )
        assert len(stop_schedules['stop_schedules']) == 1
        assert stop_schedules['stop_schedules'][0]['display_informations']['label'] == '1D'
        assert not stop_schedules['stop_schedules'][0]['date_times']

        # New disruption with a new stop_time in between B and A of the VJ = vjA
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080330"),
                    departure=tstamp("20120614T080331"),
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080400"),
                    departure=tstamp("20120614T080400"),
                    arrival_delay=3 * 60 + 58,
                    departure_delay=3 * 60 + 58,
                ),
            ],
            disruption_id='vjA_delayed_with_new_stop_time',
            effect='modified',
        )

        disruptions_after = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disruptions_before['disruptions']) + 1 == len(disruptions_after['disruptions'])
        assert has_the_disruption(disruptions_after, 'vjA_delayed_with_new_stop_time')

        # still nothing for base_schedule
        departures = self.query_region(
            base_query.format(api='departures', dt='20120614T080100', df='base_schedule')
        )
        assert len(departures['departures']) == 0
        stop_schedules = self.query_region(
            base_query.format(api='stop_schedules', dt='20120614T080100', df='base_schedule')
        )
        assert len(stop_schedules['stop_schedules']) == 2  # a new route is linked (not used in base_schedule)
        assert not stop_schedules['stop_schedules'][0]['date_times']
        assert not stop_schedules['stop_schedules'][1]['date_times']

        # departures updated in realtime
        departures = self.query_region(base_query.format(api='departures', dt='20120614T080100', df='realtime'))
        assert len(departures['departures']) == 1
        assert departures['departures'][0]['stop_date_time']['data_freshness'] == 'realtime'
        assert (
            departures['departures'][0]['stop_date_time']['arrival_date_time'] == '20120614T080330'
        )  # new stop
        assert departures['departures'][0]['stop_date_time']['departure_date_time'] == '20120614T080331'
        assert 'vjA_delayed_with_new_stop_time' in [
            l['id'] for l in departures['departures'][0]['display_informations']['links']
        ]  # link to disruption
        assert 'vjA_delayed_with_new_stop_time' in [
            d['id'] for d in departures['disruptions']
        ]  # disruption in collection

        # stop_schedules updated in realtime
        stop_schedules = self.query_region(
            base_query.format(api='stop_schedules', dt='20120614T080100', df='realtime')
        )
        assert len(stop_schedules['stop_schedules']) == 2
        assert stop_schedules['stop_schedules'][1]['display_informations']['label'] == '1D'
        assert not stop_schedules['stop_schedules'][1]['date_times']  # still no departure on other route
        assert stop_schedules['stop_schedules'][0]['display_informations']['label'] == '1A'
        assert stop_schedules['stop_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'
        assert (
            stop_schedules['stop_schedules'][0]['date_times'][0]['date_time'] == '20120614T080331'
        )  # new departure
        assert 'vjA_delayed_with_new_stop_time' in [
            l['id'] for l in stop_schedules['stop_schedules'][0]['date_times'][0]['links']
        ]  # link to disruption
        assert 'vjA_delayed_with_new_stop_time' in [
            d['id'] for d in departures['disruptions']
        ]  # disruption in collection


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnNewStopTimeInBetween(MockKirinDisruptionsFixture):
    def test_add_modify_and_delete_one_stop_time(self):
        """
        1. Create a disruption with delay on VJ = vjA (with stop_time B and A) and verify the journey
        for a query from S to R: S-> walk-> B -> public_transport -> A -> walk -> R
        2. Add a new stop_time (stop_point C) in between B and A in the VJ = vjA and verify the journey as above
        3. Verify the journey for a query from S to C: S-> walk-> B -> public_transport -> C
        4. Delete the added stop_time and verify the journey  for a query in 3.
        """
        # New disruption with a delay of VJ = vjA
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120614T080224"),
                    departure=tstamp("20120614T080225"),
                    arrival_delay=60 + 24,
                    departure_delay=60 + 25,
                    message='cow on tracks',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080400"),
                    departure=tstamp("20120614T080400"),
                    arrival_delay=3 * 60 + 58,
                    departure_delay=3 * 60 + 58,
                ),
            ],
            disruption_id='vjA_delayed',
        )

        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == 13
        assert has_the_disruption(disrupts, 'vjA_delayed')

        # query from S to R: Journey without delay with departure from B at 20120614T080100
        # and arrival to A  at 20120614T080102 returned
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'base_schedule'
        assert response['journeys'][0]['sections'][1]['departure_date_time'] == '20120614T080101'
        assert response['journeys'][0]['sections'][1]['arrival_date_time'] == '20120614T080103'
        assert len(response['journeys'][0]['sections'][1]['stop_date_times']) == 2
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'

        # A new request with departure after 2 minutes gives us journey with delay
        response = self.query_region(sub_query + "&data_freshness=realtime&datetime=20120614T080200")
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'
        assert response['journeys'][0]['sections'][1]['departure_date_time'] == '20120614T080225'
        assert response['journeys'][0]['sections'][1]['arrival_date_time'] == '20120614T080400'
        assert len(response['journeys'][0]['sections'][1]['stop_date_times']) == 2
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'

        # New disruption with a new stop_time in between B and A of the VJ = vjA
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120614T080224"),
                    departure=tstamp("20120614T080225"),
                    arrival_delay=60 + 24,
                    departure_delay=60 + 25,
                    message='cow on tracks',
                ),
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080330"),
                    departure=tstamp("20120614T080330"),
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080400"),
                    departure=tstamp("20120614T080400"),
                    arrival_delay=3 * 60 + 58,
                    departure_delay=3 * 60 + 58,
                ),
            ],
            disruption_id='vjA_delayed_with_new_stop_time',
            effect='modified',
        )

        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == 14
        assert has_the_disruption(disrupts, 'vjA_delayed_with_new_stop_time')
        last_disrupt = disrupts['disruptions'][-1]
        assert last_disrupt['severity']['effect'] == 'MODIFIED_SERVICE'

        # the journey has the new stop_time in its section of public_transport
        response = self.query_region(sub_query + "&data_freshness=realtime&datetime=20120614T080200")
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'
        assert response['journeys'][0]['sections'][1]['departure_date_time'] == '20120614T080225'
        assert response['journeys'][0]['sections'][1]['arrival_date_time'] == '20120614T080400'
        assert len(response['journeys'][0]['sections'][1]['stop_date_times']) == 3
        assert (
            response['journeys'][0]['sections'][1]['stop_date_times'][1]['stop_point']['name']
            == 'stop_point:stopC'
        )
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'

        # Query from S to C: Uses a public_transport from B to C
        S_to_C_query = "journeys?from={from_coord}&to={to_coord}".format(
            from_coord='0.0000898312;0.0000898312', to_coord='stop_point:stopC'
        )
        base_journey_query = S_to_C_query + "&data_freshness=realtime&datetime=20120614T080200"
        response = self.query_region(base_journey_query)
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 2
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'realtime'
        assert response['journeys'][0]['sections'][1]['departure_date_time'] == '20120614T080225'
        assert response['journeys'][0]['sections'][1]['arrival_date_time'] == '20120614T080330'

        # New disruption with a deleted stop_time recently added at stop_point:stopC
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080330"),
                    departure=tstamp("20120614T080330"),
                    message='stop_time deleted',
                    arrival_skipped=True,
                )
            ],
            disruption_id='deleted_stop_time',
        )

        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == 15
        assert has_the_disruption(disrupts, 'deleted_stop_time')

        # the journey doesn't have public_transport
        response = self.query_region(base_journey_query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['type'] == 'best'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinOnNewStopTimeAtTheBeginning(MockKirinDisruptionsFixture):
    def test_add_modify_and_delete_one_stop_time(self):
        """
        1. create a new_stop_time to add a final stop in C
        test that a new journey is possible with section type = public_transport from B to C
        2. delete the added stop_time and verify that the public_transport section is absent
        3. delete again stop_time and verify that the public_transport section is absent
        """
        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == 12

        C_to_R_query = "journeys?from={from_coord}&to={to_coord}".format(
            from_coord='stop_point:stopC', to_coord='stop_point:stopA'
        )

        # Query from C to R: the journey doesn't have any public_transport
        base_journey_query = C_to_R_query + "&data_freshness=realtime&datetime=20120614T080000&walking_speed=0.7"
        response = self.query_region(base_journey_query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert 'data_freshness' not in response['journeys'][0]['sections'][0]
        assert response['journeys'][0]['durations']['walking'] == 127

        # New disruption with two stop_times same as base schedule and
        # a new stop_time on stop_point:stopC added at the beginning
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080000"),
                    departure=tstamp("20120614T080000"),
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080001"),
                    departure=tstamp("20120614T080001"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080002"),
                    departure=tstamp("20120614T080002"),
                ),
            ],
            disruption_id='new_stop_time',
            effect='delayed',
        )

        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == 13
        assert has_the_disruption(disrupts, 'new_stop_time')
        last_disruption = disrupts['disruptions'][-1]
        assert last_disruption['impacted_objects'][0]['impacted_stops'][0]['arrival_status'] == 'added'
        assert last_disruption['impacted_objects'][0]['impacted_stops'][0]['departure_status'] == 'added'
        assert last_disruption['severity']['effect'] == 'SIGNIFICANT_DELAYS'
        assert last_disruption['severity']['name'] == 'trip delayed'

        # Query from C to R: the journey should have a public_transport from C to A
        response = self.query_region(base_journey_query)
        assert len(response['journeys']) == 2
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'
        assert response['journeys'][0]['sections'][0]['departure_date_time'] == '20120614T080000'
        assert response['journeys'][0]['sections'][0]['arrival_date_time'] == '20120614T080002'
        assert response['journeys'][1]['sections'][0]['type'] == 'street_network'

        # New disruption with a deleted stop_time recently added at stop_point:stopC
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080000"),
                    departure=tstamp("20120614T080000"),
                    message='stop_time deleted',
                    arrival_skipped=True,
                )
            ],
            disruption_id='deleted_stop_time',
        )

        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == 14
        assert has_the_disruption(disrupts, 'deleted_stop_time')
        last_disruption = disrupts['disruptions'][-1]
        assert last_disruption['impacted_objects'][0]['impacted_stops'][0]['arrival_status'] == 'deleted'
        assert (
            last_disruption['impacted_objects'][0]['impacted_stops'][0]['departure_status'] == 'unchanged'
        )  # Why?
        assert last_disruption['severity']['effect'] == 'REDUCED_SERVICE'
        assert last_disruption['severity']['name'] == 'reduced service'

        response = self.query_region(base_journey_query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert 'data_freshness' not in response['journeys'][0]['sections'][0]
        assert response['journeys'][0]['durations']['walking'] == 127

        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T080000')
        assert len(pt_response['disruptions']) == 2


@dataset(MAIN_ROUTING_TEST_SETTING_NO_ADD)
class TestKrakenNoAdd(MockKirinDisruptionsFixture):
    def test_no_rt_add_possible(self):
        """
        trying to add new_stop_time without allowing it in kraken
        test that it is ignored
        (same test as test_add_one_stop_time_at_the_end(), different result expected)
        """
        disruptions_before = self.query_region('disruptions?_current_datetime=20120614T080000')
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption same as base schedule
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080104"),
                    departure=tstamp("20120614T080104"),
                ),
            ],
            disruption_id='new_stop_time',
        )

        # No new disruption
        disruptions_after = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert nb_disruptions_before == len(disruptions_after['disruptions'])
        assert not has_the_disruption(disruptions_after, 'new_stop_time')

        journey_query = journey_basic_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(journey_query)
        assert not has_the_disruption(response, 'new_stop_time')
        self.is_valid_journey_response(response, journey_query)
        assert response['journeys'][0]['sections'][1]['data_freshness'] == 'base_schedule'

        B_C_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord='stop_point:stopB', to_coord='stop_point:stopC', datetime='20120614T080000'
        )
        base_journey_query = B_C_query + "&data_freshness=base_schedule&_current_datetime=20120614T080000"
        response = self.query_region(base_journey_query)
        assert not has_the_disruption(response, 'new_stop_time')
        self.is_valid_journey_response(response, base_journey_query)
        assert len(response['journeys']) == 1  # check we only have one journey
        assert 'data_freshness' not in response['journeys'][0]['sections'][0]  # means it's base_schedule

        B_C_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord='stop_point:stopB', to_coord='stop_point:stopC', datetime='20120614T080000'
        )
        rt_journey_query = B_C_query + "&data_freshness=realtime&_current_datetime=20120614T080000"
        response = self.query_region(rt_journey_query)
        assert not has_the_disruption(response, 'new_stop_time')
        self.is_valid_journey_response(response, rt_journey_query)
        assert len(response['journeys']) == 1  # check there's no new journey possible
        assert 'data_freshness' not in response['journeys'][0]['sections'][0]  # means it's base_schedule


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinStopTimeOnDetourAtTheEnd(MockKirinDisruptionsFixture):
    def test_stop_time_with_detour_at_the_end(self):
        """
        1. create a new_stop_time at C to replace existing one at A so that we have
            A deleted_for_detour and C added_for_detour
        2. test that a new journey is possible with section type = public_transport from B to C
        """
        disruptions_before = self.query_region('disruptions?_current_datetime=20120614T080000')
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption with one stop_time same as base schedule, another one deleted and
        # a new stop_time on stop_point:stopC added at the end
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                    arrival_skipped=True,
                    is_detour=True,
                    message='deleted for detour',
                ),
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080104"),
                    departure=tstamp("20120614T080104"),
                    is_added=True,
                    is_detour=True,
                    message='added for detour',
                ),
            ],
            disruption_id='stop_time_with_detour',
        )

        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == nb_disruptions_before + 1
        assert has_the_disruption(disrupts, 'stop_time_with_detour')
        last_disrupt = disrupts['disruptions'][-1]
        assert last_disrupt['severity']['effect'] == 'DETOUR'

        # Verify impacted objects
        assert len(last_disrupt['impacted_objects']) == 1
        impacted_stops = last_disrupt['impacted_objects'][0]['impacted_stops']
        assert len(impacted_stops) == 3
        assert bool(impacted_stops[0]['is_detour']) is False
        assert impacted_stops[0]['cause'] == 'on time'

        assert bool(impacted_stops[1]['is_detour']) is True
        assert impacted_stops[1]['cause'] == 'deleted for detour'
        assert impacted_stops[1]['departure_status'] == 'unchanged'
        assert impacted_stops[1]['arrival_status'] == 'deleted'

        assert bool(impacted_stops[2]['is_detour']) is True
        assert impacted_stops[2]['cause'] == 'added for detour'
        assert impacted_stops[2]['departure_status'] == 'added'
        assert impacted_stops[2]['arrival_status'] == 'added'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinStopTimeOnDetourAndArrivesBeforeDeletedAtTheEnd(MockKirinDisruptionsFixture):
    def test_stop_time_with_detour_and_arrival_before_deleted_at_the_end(self):
        """
        1. create a new_stop_time at C to replace existing one at A so that we have A deleted_for_detour
        and C added_for_detour with arrival time < to arrival time of A (deleted)
        2. Kraken accepts this disruption
        """
        disruptions_before = self.query_region('disruptions?_current_datetime=20120614T080000')
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption with one stop_time same as base schedule, another one deleted and
        # a new stop_time on stop_point:stopC added at the end
        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                    arrival_skipped=True,
                    departure_skipped=True,
                    is_detour=True,
                    message='deleted for detour',
                ),
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120614T080101"),
                    departure=tstamp("20120614T080101"),
                    is_added=True,
                    is_detour=True,
                    message='added for detour',
                ),
            ],
            disruption_id='stop_time_with_detour',
        )

        # Verify disruptions
        disrupts = self.query_region('disruptions?_current_datetime=20120614T080000')
        assert len(disrupts['disruptions']) == nb_disruptions_before + 1
        assert has_the_disruption(disrupts, 'stop_time_with_detour')
        last_disrupt = disrupts['disruptions'][-1]
        assert last_disrupt['severity']['effect'] == 'DETOUR'

        # Verify impacted objects
        assert len(last_disrupt['impacted_objects']) == 1
        impacted_stops = last_disrupt['impacted_objects'][0]['impacted_stops']
        assert len(impacted_stops) == 3
        assert bool(impacted_stops[0]['is_detour']) is False
        assert impacted_stops[0]['cause'] == 'on time'

        assert bool(impacted_stops[1]['is_detour']) is True
        assert impacted_stops[1]['cause'] == 'deleted for detour'
        assert impacted_stops[1]['departure_status'] == 'deleted'
        assert impacted_stops[1]['arrival_status'] == 'deleted'

        assert bool(impacted_stops[2]['is_detour']) is True
        assert impacted_stops[2]['cause'] == 'added for detour'
        assert impacted_stops[2]['departure_status'] == 'added'
        assert impacted_stops[2]['arrival_status'] == 'added'

        B_C_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord='stop_point:stopB', to_coord='stop_point:stopC', datetime='20120614T080000'
        )

        # Query with data_freshness=base_schedule
        base_journey_query = B_C_query + "&data_freshness=base_schedule&_current_datetime=20120614T080000"

        # There is no public transport from B to C
        response = self.query_region(base_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['type'] == 'best'
        assert 'data_freshness' not in response['journeys'][0]['sections'][0]  # means it's base_schedule

        # Query with data_freshness=realtime
        base_journey_query = B_C_query + "&data_freshness=realtime&_current_datetime=20120614T080000"

        # There is a public transport from B to C with realtime having only two stop_date_times
        # as the deleted-for-detour stop should not be displayed
        response = self.query_region(base_journey_query)
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['status'] == 'DETOUR'
        assert response['journeys'][0]['sections'][0]['type'] == 'public_transport'
        assert len(response['journeys'][0]['sections'][0]['stop_date_times']) == 2
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'
        assert response['journeys'][0]['sections'][0]['display_informations']['physical_mode'] == 'Tramway'
        assert has_the_disruption(response, 'stop_time_with_detour')

        # Tramway is the first physical_mode in NTFS, but we might pick mode in a smarter way in the future
        response = self.query_region('physical_modes')
        assert response['physical_modes'][0]['name'] == 'Tramway'

        # Check attributes of deleted stop_time in the concerned vehicle_journey

        # since the ids of impacted vjs are random uuid which cannot be know in advance... we have to request all vjs then retrieve the impacted one
        vjs_query = 'vehicle_journeys/?_current_datetime={dt}'.format(dt='20120614T080000')
        vjs = self.query_region(vjs_query)['vehicle_journeys']

        impacted_vj = next((vj for vj in vjs if 'vehicle_journey:vjA:RealTime:' in vj['id']))
        impacted_vj_query = 'vehicle_journeys/{vj}?_current_datetime={dt}'.format(
            vj=impacted_vj['id'], dt='20120614T080000'
        )
        response = self.query_region(impacted_vj_query)

        assert has_the_disruption(response, 'stop_time_with_detour')
        assert len(response['vehicle_journeys']) == 1
        assert len(response['vehicle_journeys'][0]['stop_times']) == 3
        assert response['vehicle_journeys'][0]['stop_times'][1]['drop_off_allowed'] is False
        assert response['vehicle_journeys'][0]['stop_times'][1]['pickup_allowed'] is False


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinAddNewTrip(MockKirinDisruptionsFixture):
    def test_add_new_trip(self):
        """
        0. test that no PT-Ref object related to the new trip exists and that no PT-journey exists
        1. create a new trip
        2. test that journey is possible using this new trip
        3. test some PT-Ref objects were created
        4. test that /pt_objects returns those objects
        5. test that PT-Ref filters are working
        6. test /departures and stop_schedules
        """
        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # /journeys before (only direct walk)
        C_B_query = (
            "journeys?from={f}&to={to}&data_freshness=realtime&"
            "datetime={dt}&_current_datetime={dt}".format(
                f='stop_point:stopC', to='stop_point:stopB', dt='20120614T080000'
            )
        )
        response = self.query_region(C_B_query)
        assert not has_the_disruption(response, 'new_trip')
        self.is_valid_journey_response(response, C_B_query)
        assert len(response['journeys']) == 1
        assert 'non_pt_walking' in response['journeys'][0]['tags']

        # /pt_objects before
        ptobj_query = 'pt_objects?q={q}&_current_datetime={dt}'.format(q='adi', dt='20120614T080000')  # ++typo
        response = self.query_region(ptobj_query)
        assert 'pt_objects' not in response

        # since the ids of impacted vjs are random uuid which cannot be know in advance... we have to request all vjs then retrieve the impacted one
        vjs_query = 'vehicle_journeys/?_current_datetime={dt}'.format(dt='20120614T080000')
        vjs = self.query_region(vjs_query)['vehicle_journeys']

        impacted_vj = next((vj for vj in vjs if 'vehicle_journey:vjA:RealTime:' in vj['id']), None)
        # No vj has been impacted yet
        assert impacted_vj is None

        # Check that no additional line exists
        line_query = 'lines/{l}?_current_datetime={dt}'.format(l='line:stopC_stopB', dt='20120614T080000')
        response, status = self.query_region(line_query, check=False)
        assert status == 404
        assert 'lines' not in response

        # Check that PT-Ref filter fails as no object exists
        vj_filter_query = 'commercial_modes/{cm}/vehicle_journeys?_current_datetime={dt}'.format(
            cm='commercial_mode:additional_service', dt='20120614T080000'
        )
        response, status = self.query_region(vj_filter_query, check=False)
        assert status == 404
        assert response['error']['message'] == 'ptref : Filters: Unable to find object'

        network_filter_query = 'vehicle_journeys/{vj}/networks?_current_datetime={dt}'.format(
            vj='vehicle_journey:additional-trip:modified:0:new_trip', dt='20120614T080000'
        )
        response, status = self.query_region(network_filter_query, check=False)
        assert status == 404
        assert response['error']['message'] == 'ptref : Filters: Unable to find object'

        # Check that no departure exist on stop_point stop_point:stopC for neither base_schedule nor realtime
        departure_query = "stop_points/stop_point:stopC/departures?_current_datetime=20120614T080000"
        departures = self.query_region(departure_query + '&data_freshness=base_schedule')
        assert len(departures['departures']) == 0
        departures = self.query_region(departure_query + '&data_freshness=realtime')
        assert len(departures['departures']) == 0

        # Check stop_schedules on stop_point stop_point:stopC for base_schedule and realtime with
        # Date_times list empty
        ss_on_sp_query = "stop_points/stop_point:stopC/stop_schedules?_current_datetime=20120614T080000"
        stop_schedules = self.query_region(ss_on_sp_query + '&data_freshness=realtime')
        assert len(stop_schedules['stop_schedules']) == 1
        assert stop_schedules['stop_schedules'][0]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][0]['links'][0]['id'] == 'D'
        assert len(stop_schedules['stop_schedules'][0]['date_times']) == 0

        # Check that no stop_schedule exist on line:stopC_stopB and stop_point stop_point:stopC
        ss_on_line_query = (
            "stop_points/stop_point:stopC/lines/line:stopC_stopB/"
            "stop_schedules?_current_datetime=20120614T080000"
        )
        stop_schedules, status = self.query_region(ss_on_line_query + '&data_freshness=realtime', check=False)
        assert status == 404
        assert len(stop_schedules['stop_schedules']) == 0

        # New disruption, a new trip without headsign with 2 stop_times in realtime
        self.send_mock(
            "additional-trip",
            "20120614",
            'added',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id='new_trip',
            effect='additional_service',
            physical_mode_id='physical_mode:Bus',  # this physical mode exists in kraken
        )

        # Check new disruption 'additional-trip' to add a new trip
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])
        new_trip_disruptions = get_disruptions_by_id(disruptions_after, 'new_trip')
        assert len(new_trip_disruptions) == 1
        new_trip_disrupt = new_trip_disruptions[0]
        assert new_trip_disrupt['id'] == 'new_trip'
        assert new_trip_disrupt['severity']['effect'] == 'ADDITIONAL_SERVICE'
        assert len(new_trip_disrupt['impacted_objects'][0]['impacted_stops']) == 2
        assert all(
            [
                (s['departure_status'] == 'added' and s['arrival_status'] == 'added')
                for s in new_trip_disrupt['impacted_objects'][0]['impacted_stops']
            ]
        )
        assert new_trip_disrupt['application_periods'][0]['begin'] == '20120614T080100'
        assert new_trip_disrupt['application_periods'][0]['end'] == '20120614T080102'

        # Check that a PT journey now exists
        response = self.query_region(C_B_query)
        assert has_the_disruption(response, 'new_trip')
        self.is_valid_journey_response(response, C_B_query)
        assert len(response['journeys']) == 2
        pt_journey = response['journeys'][0]
        assert 'non_pt_walking' not in pt_journey['tags']
        assert pt_journey['status'] == 'ADDITIONAL_SERVICE'
        assert pt_journey['sections'][0]['data_freshness'] == 'realtime'
        assert pt_journey['sections'][0]['display_informations']['commercial_mode'] == 'additional service'
        assert pt_journey['sections'][0]['display_informations']['physical_mode'] == 'Bus'

        # Check date_times
        assert pt_journey['sections'][0]['departure_date_time'] == '20120614T080100'
        assert pt_journey['sections'][0]['arrival_date_time'] == '20120614T080102'
        assert pt_journey['sections'][0]['stop_date_times'][0]['arrival_date_time'] == '20120614T080100'
        assert pt_journey['sections'][0]['stop_date_times'][-1]['arrival_date_time'] == '20120614T080102'

        # Check /pt_objects after: new objects created
        response = self.query_region(ptobj_query)
        assert len(response['pt_objects']) == 4
        assert len([o for o in response['pt_objects'] if o['id'] == 'network:additional_service']) == 1
        assert len([o for o in response['pt_objects'] if o['id'] == 'commercial_mode:additional_service']) == 1
        assert len([o for o in response['pt_objects'] if o['id'] == 'line:stopC_stopB']) == 1
        assert len([o for o in response['pt_objects'] if o['id'] == 'route:stopC_stopB']) == 1

        vjs_query = 'vehicle_journeys/?_current_datetime={dt}'.format(dt='20120614T080000')
        vjs = self.query_region(vjs_query)['vehicle_journeys']
        impacted_vj = next((vj for vj in vjs if 'vehicle_journey:additional-trip:RealTime:' in vj['id']), None)
        assert impacted_vj

        vj_query = 'vehicle_journeys/{vj}?_current_datetime={dt}'.format(
            vj=impacted_vj['id'], dt='20120614T080000'
        )

        # Check that the vehicle_journey has been created
        response = self.query_region(vj_query)
        assert has_the_disruption(response, 'new_trip')
        assert len(response['vehicle_journeys']) == 1
        # Check that name and headsign are empty
        assert response['vehicle_journeys'][0]['name'] == ''
        assert response['vehicle_journeys'][0]['headsign'] == ''
        assert response['vehicle_journeys'][0]['disruptions'][0]['id'] == 'new_trip'
        assert len(response['vehicle_journeys'][0]['stop_times']) == 2
        assert response['vehicle_journeys'][0]['stop_times'][0]['drop_off_allowed'] is True
        assert response['vehicle_journeys'][0]['stop_times'][0]['pickup_allowed'] is True

        # Check that the new line has been created with necessary information
        response = self.query_region(line_query)
        assert len(response['lines']) == 1
        assert response['lines'][0]['name'] == 'stopC - stopB'
        assert response['lines'][0]['network']['id'] == 'network:additional_service'
        assert response['lines'][0]['commercial_mode']['id'] == 'commercial_mode:additional_service'
        assert response['lines'][0]['routes'][0]['id'] == 'route:stopC_stopB'
        assert response['lines'][0]['routes'][0]['name'] == 'stopC - stopB'
        assert response['lines'][0]['routes'][0]['direction']['id'] == 'stopB'
        assert response['lines'][0]['routes'][0]['direction_type'] == 'forward'

        # Check that objects created are linked in PT-Ref filter
        response = self.query_region(vj_filter_query)
        assert has_the_disruption(response, 'new_trip')
        assert len(response['vehicle_journeys']) == 1

        # Check that the newly created vehicle journey are well filtered by &since and &until
        # Note: For backward compatibility parameter &data_freshness with base_schedule is added
        # and works with &since and &until
        vj_base_query = (
            'commercial_modes/commercial_mode:additional_service/vehicle_journeys?'
            '_current_datetime={dt}&since={sin}&until={un}&data_freshness={df}'
        )
        response, status = self.query_region(
            vj_base_query.format(
                dt='20120614T080000', sin='20120614T080100', un='20120614T080102', df='base_schedule'
            ),
            check=False,
        )
        assert status == 404
        assert 'vehicle_journeys' not in response

        response = self.query_region(
            vj_base_query.format(
                dt='20120614T080000', sin='20120614T080100', un='20120614T080102', df='realtime'
            )
        )
        assert len(response['vehicle_journeys']) == 1

        response, status = self.query_region(
            vj_base_query.format(
                dt='20120614T080000', sin='20120614T080101', un='20120614T080102', df='realtime'
            ),
            check=False,
        )
        assert status == 404
        assert 'vehicle_journeys' not in response

        network_filter_query = 'vehicle_journeys/{vj}/networks?_current_datetime={dt}'.format(
            vj=impacted_vj['id'], dt='20120614T080000'
        )
        response = self.query_region(network_filter_query)
        assert len(response['networks']) == 1
        assert response['networks'][0]['name'] == 'additional service'

        # Check that no departure exist on stop_point stop_point:stopC for base_schedule
        departures = self.query_region(departure_query + '&data_freshness=base_schedule')
        assert len(departures['departures']) == 0

        # Check that departures on stop_point stop_point:stopC exists with disruption
        departures = self.query_region(departure_query + '&data_freshness=realtime')
        assert len(departures['disruptions']) == 1
        assert departures['disruptions'][0]['disruption_uri'] == 'new_trip'
        assert departures['departures'][0]['display_informations']['name'] == 'stopC - stopB'

        # Check that stop_schedule on line "line:stopC_stopB" and stop_point stop_point:stopC
        # for base_schedule date_times list is empty.
        stop_schedules = self.query_region(ss_on_line_query + '&data_freshness=base_schedule')
        assert len(stop_schedules['stop_schedules']) == 1
        assert stop_schedules['stop_schedules'][0]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][0]['links'][0]['id'] == 'line:stopC_stopB'
        assert len(stop_schedules['stop_schedules'][0]['date_times']) == 0

        # Check that stop_schedule on line "line:stopC_stopB" and stop_point stop_point:stopC
        # exists with disruption.
        stop_schedules = self.query_region(ss_on_line_query + '&data_freshness=realtime')
        assert len(stop_schedules['stop_schedules']) == 1
        assert stop_schedules['stop_schedules'][0]['links'][0]['id'] == 'line:stopC_stopB'
        assert len(stop_schedules['disruptions']) == 1
        assert stop_schedules['disruptions'][0]['uri'] == 'new_trip'
        assert len(stop_schedules['stop_schedules'][0]['date_times']) == 1
        assert stop_schedules['stop_schedules'][0]['date_times'][0]['date_time'] == '20120614T080100'
        assert stop_schedules['stop_schedules'][0]['date_times'][0]['data_freshness'] == 'realtime'

        # Check stop_schedules on stop_point stop_point:stopC for base_schedule
        # Date_times list is empty for both stop_schedules
        stop_schedules = self.query_region(ss_on_sp_query + '&data_freshness=base_schedule')
        assert len(stop_schedules['stop_schedules']) == 2
        assert stop_schedules['stop_schedules'][0]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][0]['links'][0]['id'] == 'D'
        assert len(stop_schedules['stop_schedules'][0]['date_times']) == 0
        assert stop_schedules['stop_schedules'][1]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][1]['links'][0]['id'] == 'line:stopC_stopB'
        assert len(stop_schedules['stop_schedules'][1]['date_times']) == 0

        # Check stop_schedules on stop_point stop_point:stopC for realtime
        # Date_times list is empty for line 'D' but not for the new line added
        stop_schedules = self.query_region(ss_on_sp_query + '&data_freshness=realtime')
        assert len(stop_schedules['stop_schedules']) == 2
        assert stop_schedules['stop_schedules'][0]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][0]['links'][0]['id'] == 'D'
        assert len(stop_schedules['stop_schedules'][0]['date_times']) == 0
        assert stop_schedules['stop_schedules'][1]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][1]['links'][0]['id'] == 'line:stopC_stopB'
        assert len(stop_schedules['stop_schedules'][1]['date_times']) == 1
        assert stop_schedules['stop_schedules'][1]['date_times'][0]['date_time'] == '20120614T080100'
        assert stop_schedules['stop_schedules'][1]['date_times'][0]['data_freshness'] == 'realtime'

        # Check stop_schedules on stop_area stopC for base_schedule
        # Date_times list is empty for both stop_schedules
        ss_on_sa_query = "stop_areas/stopC/stop_schedules?_current_datetime=20120614T080000"
        stop_schedules = self.query_region(ss_on_sa_query + '&data_freshness=base_schedule')
        assert len(stop_schedules['stop_schedules']) == 2
        assert stop_schedules['stop_schedules'][0]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][0]['links'][0]['id'] == 'D'
        assert len(stop_schedules['stop_schedules'][0]['date_times']) == 0
        assert stop_schedules['stop_schedules'][1]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][1]['links'][0]['id'] == 'line:stopC_stopB'
        assert len(stop_schedules['stop_schedules'][1]['date_times']) == 0

        # Check stop_schedules on stop_area stopC for realtime
        # Date_times list is empty for line 'D' but not for the new line added
        ss_on_sa_query = "stop_areas/stopC/stop_schedules?_current_datetime=20120614T080000"
        stop_schedules = self.query_region(ss_on_sa_query + '&data_freshness=realtime')
        assert len(stop_schedules['stop_schedules']) == 2
        assert stop_schedules['stop_schedules'][0]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][0]['links'][0]['id'] == 'D'
        assert len(stop_schedules['stop_schedules'][0]['date_times']) == 0
        assert stop_schedules['stop_schedules'][1]['links'][0]['type'] == 'line'
        assert stop_schedules['stop_schedules'][1]['links'][0]['id'] == 'line:stopC_stopB'
        assert len(stop_schedules['stop_schedules'][1]['date_times']) == 1
        assert stop_schedules['stop_schedules'][1]['date_times'][0]['date_time'] == '20120614T080100'
        assert stop_schedules['stop_schedules'][1]['date_times'][0]['data_freshness'] == 'realtime'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestPtRefOnAddedTrip(MockKirinDisruptionsFixture):
    def test_ptref_on_added_trip(self):
        """
        1. Test all possibles ptref calls with/without filters before adding a new trip
        2. Test all possibles ptref calls with/without filters after adding a new trip
        3. Test all possibles ptref calls with/without filters after modifying the recently added trip
        Note: physical_mode is present in gtfs-rt whereas for network and commercial_mode default value is used
        """
        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # Verify that network, line, commercial_mode of the new trip to be added in future is absent
        resp, status = self.query_region("networks/network:additional_service", check=False)
        assert status == 404
        assert resp['error']['message'] == 'ptref : Filters: Unable to find object'
        resp, status = self.query_region("lines/line:stopC_stopB", check=False)
        assert status == 404
        assert resp['error']['message'] == 'ptref : Filters: Unable to find object'
        resp, status = self.query_region("commercial_modes/commercial_mode:additional_service", check=False)
        assert status == 404
        assert resp['error']['message'] == 'ptref : Filters: Unable to find object'

        # The following ptref search should work with base-schedule data.
        # network <-> datasets
        resp = self.query_region("networks/base_network/datasets")
        assert resp["datasets"][0]["id"] == "default:dataset"
        resp = self.query_region("datasets/default:dataset/networks")
        assert resp["networks"][0]["id"] == "base_network"

        # line <-> company
        resp = self.query_region("lines/A/companies")
        assert resp["companies"][0]["id"] == "base_company"
        resp = self.query_region("companies/base_company/lines")
        assert resp["lines"][0]["id"] == "A"

        # company <-> commercial_modes
        resp = self.query_region("companies/base_company/commercial_modes")
        assert resp['commercial_modes'][0]['id'] == '0x0'
        resp = self.query_region("commercial_modes/0x0/companies")
        assert resp["companies"][0]["id"] == "base_company"

        # route <-> dataset
        resp = self.query_region("routes/B:3/datasets")
        assert resp["datasets"][0]["id"] == "default:dataset"
        resp = self.query_region("datasets/default:dataset/routes")
        routes = [rt["id"] for rt in resp["routes"]]
        assert "B:3" in routes

        # vehicle_journey <-> company
        resp = self.query_region("vehicle_journeys/vehicle_journey:vjA/companies")
        assert resp["companies"][0]["id"] == "base_company"
        resp = self.query_region("companies/base_company/vehicle_journeys")
        assert len(resp["vehicle_journeys"]) == 9

        # network <-> contributor
        resp = self.query_region("networks/base_network/contributors")
        assert resp["contributors"][0]["id"] == "default:contributor"
        resp = self.query_region("contributors/default:contributor/networks")
        assert resp["networks"][0]["id"] == "base_network"

        # New disruption, a new trip with 2 stop_times in realtime
        self.send_mock(
            "additional-trip",
            "20120614",
            "added",
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id="new_trip",
            effect="additional_service",
            physical_mode_id="physical_mode:Bus",  # this physical mode exists in kraken
        )

        # Check new disruption 'additional-trip' to add a new trip
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])

        # Verify that network, line, commercial_mode of the new trip are present
        resp = self.query_region("networks/network:additional_service")
        assert "networks" in resp
        resp = self.query_region("lines/line:stopC_stopB")
        assert "lines" in resp
        resp = self.query_region("commercial_modes/commercial_mode:additional_service")
        assert "commercial_modes" in resp

        resp = self.query_region("networks/network:additional_service/physical_modes")
        assert resp["physical_modes"][0]["id"] == "physical_mode:Bus"

        resp = self.query_region("physical_modes/physical_mode:Bus/networks")
        networks = [nw["id"] for nw in resp["networks"]]
        assert "network:additional_service" in networks

        # network by line should work
        resp = self.query_region("lines/line:stopC_stopB/networks")
        assert resp["networks"][0]["id"] == "network:additional_service"

        # The physical_mode sent in gtfs-rt should be present in the new line added
        resp = self.query_region("lines/line:stopC_stopB/physical_modes")
        assert resp["physical_modes"][0]["id"] == "physical_mode:Bus"

        # The default commercial_mode used for a new line should be present
        resp = self.query_region("lines/line:stopC_stopB/commercial_modes")
        assert resp["commercial_modes"][0]["id"] == "commercial_mode:additional_service"

        # Newly added lines should have a route, vehicle_journey,
        resp = self.query_region("lines/line:stopC_stopB/routes")
        assert resp["routes"][0]["id"] == "route:stopC_stopB"
        resp = self.query_region("lines/line:stopC_stopB/vehicle_journeys")
        assert "vehicle_journey:additional-trip:RealTime:" in resp["vehicle_journeys"][0]["id"]
        # Name and headsign are empty
        assert resp["vehicle_journeys"][0]["name"] == ""
        assert resp["vehicle_journeys"][0]["headsign"] == ""

        # We should be able to get the line from vehicle_journey recently added
        vjs_query = 'trips/additional-trip/vehicle_journeys/?_current_datetime={dt}'.format(dt='20120614T080000')
        vjs = self.query_region(vjs_query)['vehicle_journeys']
        impacted_vj = next((vj for vj in vjs if 'vehicle_journey:additional-trip:RealTime:' in vj['id']), None)
        assert impacted_vj

        resp = self.query_region("vehicle_journeys/{vj_id}/lines".format(vj_id=impacted_vj['id']))
        assert resp["lines"][0]["id"] == "line:stopC_stopB"

        # We should be able to get the physical_mode sent in gtfs-rt from vehicle_journey recently added
        resp = self.query_region("vehicle_journeys/{vj_id}/physical_modes".format(vj_id=impacted_vj['id']))
        assert resp["physical_modes"][0]["id"] == "physical_mode:Bus"

        # The following ptref search should work with a trip added.
        # network <-> datasets
        resp = self.query_region("networks/network:additional_service/datasets")
        assert resp["datasets"][0]["id"] == "default:dataset"
        resp = self.query_region("datasets/default:dataset/networks")
        networks = [nw["id"] for nw in resp["networks"]]
        assert "network:additional_service" in networks

        # route <-> dataset
        resp = self.query_region("routes/route:stopC_stopB/datasets")
        assert resp["datasets"][0]["id"] == "default:dataset"
        resp = self.query_region("datasets/default:dataset/routes")
        routes = [rt["id"] for rt in resp["routes"]]
        assert "route:stopC_stopB" in routes

        # route <-> physical_mode
        resp = self.query_region("routes/route:stopC_stopB/physical_modes")
        assert resp["physical_modes"][0]["id"] == "physical_mode:Bus"
        resp = self.query_region("physical_modes/physical_mode:Bus/routes")
        routes = [rt["id"] for rt in resp["routes"]]
        assert "route:stopC_stopB" in routes

        # route <-> stop_point
        resp = self.query_region("routes/route:stopC_stopB/stop_points")
        sps = [sp["id"] for sp in resp["stop_points"]]
        assert "stop_point:stopC" in sps
        assert "stop_point:stopB" in sps
        resp = self.query_region("stop_points/stop_point:stopC/routes")
        routes = [rt["id"] for rt in resp["routes"]]
        assert "route:stopC_stopB" in routes
        resp = self.query_region("stop_points/stop_point:stopB/routes")
        routes = [rt["id"] for rt in resp["routes"]]
        assert "route:stopC_stopB" in routes

        # network <-> contributor
        resp = self.query_region("networks/network:additional_service/contributors")
        assert resp["contributors"][0]["id"] == "default:contributor"
        resp = self.query_region("contributors/default:contributor/networks")
        networks = [nw["id"] for nw in resp["networks"]]
        assert "network:additional_service" in networks

        # line <-> company
        resp = self.query_region("lines/line:stopC_stopB/companies")
        assert resp["companies"][0]["id"] == "base_company"
        resp = self.query_region("companies/base_company/lines")
        assert resp["lines"][7]["id"] == "line:stopC_stopB"

        # vehicle_journey <-> company
        resp = self.query_region("vehicle_journeys/{vj_id}/companies".format(vj_id=impacted_vj['id']))
        assert resp["companies"][0]["id"] == "base_company"
        resp = self.query_region("companies/base_company/vehicle_journeys")
        vjs = [vj["id"] for vj in resp["vehicle_journeys"]]
        assert impacted_vj['id'] in vjs

        # commercial_mode <-> company
        resp = self.query_region("commercial_modes/commercial_mode:additional_service/companies")
        assert resp["companies"][0]["id"] == "base_company"
        resp = self.query_region("companies/base_company/commercial_modes")
        commercial_modes = [cm["id"] for cm in resp["commercial_modes"]]
        assert "commercial_mode:additional_service" in commercial_modes

        # stop_point <-> dataset
        resp = self.query_region("stop_points/stop_point:stopC/datasets")
        assert resp["datasets"][0]["id"] == "default:dataset"
        resp = self.query_region("stop_points/stop_point:stopB/datasets")
        assert resp["datasets"][0]["id"] == "default:dataset"
        resp = self.query_region("datasets/default:dataset/stop_points")
        sps = [sp["id"] for sp in resp["stop_points"]]
        assert "stop_point:stopC" in sps
        assert "stop_point:stopB" in sps


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinAddNewTripWithWrongPhysicalMode(MockKirinDisruptionsFixture):
    def test_add_new_trip_with_wrong_physical_mode(self):
        """
        1. send a disruption to create a new trip with physical_mode absent in kaken
        2. check of journey, disruption and PT-Ref objects to verify that no trip is added
        """
        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption, a new trip with 2 stop_times in realtime
        self.send_mock(
            "additional-trip",
            "20120614",
            'added',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id='new_trip',
            effect='additional_service',
            physical_mode_id='physical_mode:Toto',  # this physical mode doesn't exist in kraken
        )

        # Check there is no new disruption
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before == len(disruptions_after['disruptions'])

        # / Journeys: as no trip on pt added, only direct walk.
        C_B_query = (
            "journeys?from={f}&to={to}&data_freshness=realtime&"
            "datetime={dt}&_current_datetime={dt}".format(
                f='stop_point:stopC', to='stop_point:stopB', dt='20120614T080000'
            )
        )
        response = self.query_region(C_B_query)
        assert not has_the_disruption(response, 'new_trip')
        self.is_valid_journey_response(response, C_B_query)
        assert len(response['journeys']) == 1
        assert 'non_pt_walking' in response['journeys'][0]['tags']

        # Check that no vehicle_journey is added
        vj_query = 'vehicle_journeys/{vj}?_current_datetime={dt}'.format(
            vj='vehicle_journey:additional-trip:modified:0:new_trip', dt='20120614T080000'
        )
        response, status = self.query_region(vj_query, check=False)
        assert status == 404
        assert 'vehicle_journeys' not in response


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinAddNewTripWithoutPhysicalMode(MockKirinDisruptionsFixture):
    def test_add_new_trip_without_physical_mode(self):
        """
        1. send a disruption to create a new trip without physical_mode absent in kaken
        2. check physical_mode of journey
        """
        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption, a new trip with 2 stop_times in realtime
        self.send_mock(
            "additional-trip",
            "20120614",
            'added',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id='new_trip',
            effect='additional_service',
        )

        # Check that a new disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])

        C_B_query = (
            "journeys?from={f}&to={to}&data_freshness=realtime&"
            "datetime={dt}&_current_datetime={dt}".format(
                f='stop_point:stopC', to='stop_point:stopB', dt='20120614T080000'
            )
        )

        # Check that a PT journey exists with first physical_mode in the NTFS('Tramway')
        response = self.query_region(C_B_query)
        assert has_the_disruption(response, 'new_trip')
        self.is_valid_journey_response(response, C_B_query)
        assert len(response['journeys']) == 2
        pt_journey = response['journeys'][0]
        assert 'non_pt_walking' not in pt_journey['tags']
        assert pt_journey['status'] == 'ADDITIONAL_SERVICE'
        assert pt_journey['sections'][0]['data_freshness'] == 'realtime'
        assert pt_journey['sections'][0]['display_informations']['commercial_mode'] == 'additional service'
        assert pt_journey['sections'][0]['display_informations']['physical_mode'] == 'Tramway'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinUpdateTripWithPhysicalMode(MockKirinDisruptionsFixture):
    def test_update_trip_with_physical_mode(self):
        """
        1. send a disruption with a physical_mode to update a trip
        2. check physical_mode of journey
        """
        # we have 8 vehicle_jouneys
        pt_response = self.query_region('vehicle_journeys')
        initial_nb_vehicle_journeys = len(pt_response['vehicle_journeys'])
        assert initial_nb_vehicle_journeys == 9

        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # physical_mode of base vehicle_journey
        pt_response = self.query_region(
            'vehicle_journeys/vehicle_journey:vjA/physical_modes?_current_datetime=20120614T1337'
        )
        assert len(pt_response['physical_modes']) == 1
        assert pt_response['physical_modes'][0]['name'] == 'Tramway'

        self.send_mock(
            "vjA",
            "20120614",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120614T080224"),
                    departure=tstamp("20120614T080225"),
                    arrival_delay=60 + 24,
                    departure_delay=60 + 25,
                    message='cow on tracks',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120614T080400"),
                    departure=tstamp("20120614T080400"),
                    arrival_delay=3 * 60 + 58,
                    departure_delay=3 * 60 + 58,
                ),
            ],
            disruption_id='vjA_delayed',
            physical_mode_id='physical_mode:Bus',  # this physical mode exists in kraken
        )

        # Check that a new disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])

        # A new vj is created
        pt_response = self.query_region('vehicle_journeys')
        assert len(pt_response['vehicle_journeys']) == (initial_nb_vehicle_journeys + 1)

        # physical_mode of the newly created vehicle_journey is the base vehicle_journey physical mode (Tramway)
        vjs = self.query_region('trips/vjA/vehicle_journeys')['vehicle_journeys']
        impacted_vj = next((vj for vj in vjs if 'vehicle_journey:vjA:RealTime:' in vj['id']), None)
        assert impacted_vj

        pt_response = self.query_region(
            'vehicle_journeys/{vj_id}/physical_modes'.format(vj_id=impacted_vj['id'])
        )
        assert len(pt_response['physical_modes']) == 1
        assert pt_response['physical_modes'][0]['name'] == 'Tramway'

        # Check disruptions with physical_modes
        disruptions_from_mode = self.query_region('physical_modes/physical_mode:0x0/disruptions')
        assert len(disruptions_from_mode['disruptions']) == 1


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinAddTripWithHeadSign(MockKirinDisruptionsFixture):
    def test_add_trip_with_headsign(self):
        """
        1. send a disruption with a headsign to add a trip
        2. check that headsign is present in journey.section.display_informations
        """
        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption, a new trip with 2 stop_times in realtime
        self.send_mock(
            "additional-trip",
            "20120614",
            'added',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id='new_trip',
            effect='additional_service',
            headsign='trip_headsign',
        )

        # Check that a new disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])

        C_B_query = (
            "journeys?from={f}&to={to}&data_freshness=realtime&"
            "datetime={dt}&_current_datetime={dt}".format(
                f='stop_point:stopC', to='stop_point:stopB', dt='20120614T080000'
            )
        )

        # Check that a PT journey exists with trip_headsign in display_informations
        response = self.query_region(C_B_query)
        assert has_the_disruption(response, 'new_trip')
        self.is_valid_journey_response(response, C_B_query)
        assert len(response['journeys']) == 2
        pt_journey = response['journeys'][0]
        assert pt_journey['status'] == 'ADDITIONAL_SERVICE'
        assert pt_journey['sections'][0]['data_freshness'] == 'realtime'
        assert pt_journey['sections'][0]['display_informations']['headsign'] == 'trip_headsign'

        # Check the vehicle_journey created by real-time
        vjs = self.query_region('trips/additional-trip/vehicle_journeys')['vehicle_journeys']
        new_vjs = [vj for vj in vjs if 'vehicle_journey:additional-trip:RealTime:' in vj['id']]
        assert len(new_vjs) == 1

        new_vj = new_vjs[0]
        assert new_vj['name'] == 'trip_headsign'
        assert new_vj['headsign'] == 'trip_headsign'


@dataset(MAIN_ROUTING_TEST_SETTING_NO_ADD)
class TestKirinAddNewTripBlocked(MockKirinDisruptionsFixture):
    def test_add_new_trip_blocked(self):
        """
        Disable realtime trip-add in Kraken
        1. send a disruption to create a new trip
        2. test that no journey is possible using this new trip
        3. test that no PT-Ref objects were created
        4. test that /pt_objects doesn't return objects
        5. test that PT-Ref filters find nothing
        6. test /departures and stop_schedules
        """
        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # New disruption, a new trip with 2 stop_times in realtime
        self.send_mock(
            "additional-trip",
            "20120614",
            'added',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id='new_trip',
            effect='additional_service',
        )

        # Check there is no new disruption
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before == len(disruptions_after['disruptions'])

        # /journeys before (only direct walk)
        C_B_query = (
            "journeys?from={f}&to={to}&data_freshness=realtime&"
            "datetime={dt}&_current_datetime={dt}".format(
                f='stop_point:stopC', to='stop_point:stopB', dt='20120614T080000'
            )
        )
        response = self.query_region(C_B_query)
        assert not has_the_disruption(response, 'new_trip')
        self.is_valid_journey_response(response, C_B_query)
        assert len(response['journeys']) == 1
        assert 'non_pt_walking' in response['journeys'][0]['tags']

        # /pt_objects before
        ptobj_query = 'pt_objects?q={q}&_current_datetime={dt}'.format(q='adi', dt='20120614T080000')  # ++typo
        response = self.query_region(ptobj_query)
        assert 'pt_objects' not in response

        # Check that no vehicle_journey exists on the future realtime-trip
        vj_query = 'vehicle_journeys/{vj}?_current_datetime={dt}'.format(
            vj='vehicle_journey:additional-trip:modified:0:new_trip', dt='20120614T080000'
        )
        response, status = self.query_region(vj_query, check=False)
        assert status == 404
        assert 'vehicle_journeys' not in response

        # Check that no additional line exists
        line_query = 'lines/{l}?_current_datetime={dt}'.format(l='line:stopC_stopB', dt='20120614T080000')
        response, status = self.query_region(line_query, check=False)
        assert status == 404
        assert 'lines' not in response

        # Check that PT-Ref filter fails as no object exists
        vj_filter_query = 'commercial_modes/{cm}/vehicle_journeys?_current_datetime={dt}'.format(
            cm='commercial_mode:additional_service', dt='20120614T080000'
        )
        response, status = self.query_region(vj_filter_query, check=False)
        assert status == 404
        assert response['error']['message'] == 'ptref : Filters: Unable to find object'

        network_filter_query = 'vehicle_journeys/{vj}/networks?_current_datetime={dt}'.format(
            vj='vehicle_journey:additional-trip:modified:0:new_trip', dt='20120614T080000'
        )
        response, status = self.query_region(network_filter_query, check=False)
        assert status == 404
        assert response['error']['message'] == 'ptref : Filters: Unable to find object'

        # Check that no departure exist on stop_point stop_point:stopC
        departure_query = "stop_points/stop_point:stopC/departures?_current_datetime=20120614T080000"
        departures = self.query_region(departure_query)
        assert len(departures['departures']) == 0

        # Check that no stop_schedule exist on line:stopC_stopB and stop_point stop_point:stopC
        ss_query = (
            "stop_points/stop_point:stopC/lines/line:stopC_stopB/"
            "stop_schedules?_current_datetime=20120614T080000&data_freshness=realtime"
        )
        stop_schedules, status = self.query_region(ss_query, check=False)
        assert status == 404
        assert len(stop_schedules['stop_schedules']) == 0


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinAddNewTripPresentInNavitiaTheSameDay(MockKirinDisruptionsFixture):
    def test_add_new_trip_present_in_navitia_the_same_day(self):

        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # The vehicle_journey vjA is present in navitia
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')

        assert len(pt_response['vehicle_journeys']) == 1
        assert len(pt_response['disruptions']) == 0

        # New disruption, a new trip with vehicle_journey id = vjA and having 2 stop_times in realtime
        self.send_mock(
            "vjA",
            "20120614",
            'added',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080100"),
                    departure=tstamp("20120614T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120614T080102"),
                    departure=tstamp("20120614T080102"),
                ),
            ],
            disruption_id='new_trip',
            effect='additional_service',
        )

        # Check that there should not be a new disruption
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before == len(disruptions_after['disruptions'])


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinAddNewTripPresentInNavitiaWithAShift(MockKirinDisruptionsFixture):
    def test_add_new_trip_present_in_navitia_with_a_shift(self):

        disruption_query = 'disruptions?_current_datetime={dt}'.format(dt='20120614T080000')
        disruptions_before = self.query_region(disruption_query)
        nb_disruptions_before = len(disruptions_before['disruptions'])

        # The vehicle_journey vjA is present in navitia
        pt_response = self.query_region('vehicle_journeys/vehicle_journey:vjA?_current_datetime=20120614T1337')

        assert len(pt_response['vehicle_journeys']) == 1
        assert len(pt_response['disruptions']) == 0

        # New disruption, a new trip with meta vehicle journey id = vjA and having 2 stop_times in realtime
        self.send_mock(
            "vjA",
            "20120620",
            'added',
            [
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120620T080100"),
                    departure=tstamp("20120620T080100"),
                    message='on time',
                ),
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival_delay=0,
                    departure_delay=0,
                    is_added=True,
                    arrival=tstamp("20120620T080102"),
                    departure=tstamp("20120620T080102"),
                ),
            ],
            disruption_id='new_trip',
            effect='additional_service',
        )

        # The new trip is accepted because, it is not the same day of the base vj
        # So a disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert nb_disruptions_before + 1 == len(disruptions_after['disruptions'])


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinDelayPassMidnightTowardsNextDay(MockKirinDisruptionsFixture):
    def test_delay_pass_midnight_towards_next_day(self):
        """
        Relates to "test_cots_update_trip_with_delay_pass_midnight_on_first_station" in kirin
        1. Add a disruption with a delay in second station (stop_point:stopA) so that there is a pass midnight
        2. Verify disruption count, vehicle_journeys count and journey
        3. Update the disruption so that  departure station stop_point:stopB is replaced by stop_point:stopC
        with a delay so that there is no more pass midnight
        4. Verify disruption count, vehicle_journeys count and journey
        Note: '&forbidden_uris[]=PM' used to avoid line 'PM' and it's vj=vjPB in /journey
        """
        disruption_query = 'disruptions?_current _datetime={dt}'.format(dt='20120614T080000')
        initial_nb_disruptions = len(self.query_region(disruption_query)['disruptions'])

        pt_response = self.query_region('vehicle_journeys')
        initial_nb_vehicle_journeys = len(pt_response['vehicle_journeys'])

        empty_query = (
            "journeys?from={f}&to={to}&data_freshness=realtime&max_duration_to_pt=0&"
            "datetime={dt}&_current_datetime={dt}&forbidden_uris[]=PM"
        )

        # Check journeys in realtime for 20120615(the day of the future disruption) from B to A
        # vjB circulates everyday with departure at 18:01:00 and arrival at 18:01:02
        ba_15T18_journey_query = empty_query.format(
            f='stop_point:stopB', to='stop_point:stopA', dt='20120615T180000'
        )

        response = self.query_region(ba_15T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120615T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120615T180102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        # vjB circulates the day before at 18:01:00 and arrival at 18:01:02
        ba_14T18_journey_query = empty_query.format(
            f='stop_point:stopB', to='stop_point:stopA', dt='20120614T180000'
        )
        response = self.query_region(ba_14T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120614T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120614T180102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        # vjB circulates the day after at 18:01:00 and arrival at 18:01:02
        ba_16T18_journey_query = empty_query.format(
            f='stop_point:stopB', to='stop_point:stopA', dt='20120616T180000'
        )
        response = self.query_region(ba_16T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120616T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T180102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        # A new disruption with a delay on arrival station to have  a pass midnight
        self.send_mock(
            "vjB",
            "20120615",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120615T180100"),
                    departure=tstamp("20120615T180100"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120616T010102"),
                    departure=tstamp("20120616T010102"),
                    arrival_delay=7 * 60 * 60,
                    message="Delayed to have pass midnight",
                ),
            ],
            disruption_id='stop_time_with_detour',
            effect='delayed',
        )

        # A new disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert initial_nb_disruptions + 1 == len(disruptions_after['disruptions'])

        # A new vehicle_journey is added
        pt_response = self.query_region('vehicle_journeys')
        assert initial_nb_vehicle_journeys + 1 == len(pt_response['vehicle_journeys'])

        # Check journeys in realtime for 20120615, the day of the disruption from B to A
        # vjB circulates with departure at 20120615T18:01:00 and arrival at 20120616T01:01:02
        response = self.query_region(ba_15T18_journey_query + '&forbidden_uris[]=PM')
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120615T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T010102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['base_departure_date_time'] == '20120615T180100'
        assert response['journeys'][0]['sections'][0]['departure_date_time'] == '20120615T180100'
        assert response['journeys'][0]['sections'][0]['base_arrival_date_time'] == '20120615T180102'
        assert response['journeys'][0]['sections'][0]['arrival_date_time'] == '20120616T010102'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'

        # vjB circulates the day before at 18:01:00 and arrival at 18:01:02
        response = self.query_region(ba_14T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120614T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120614T180102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        # vjB circulates the day after at 18:01:00 and arrival at 18:01:02
        response = self.query_region(ba_16T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120616T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T180102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        # Disruption is modified with first station on detour and delay so that there is no more pass midnight
        self.send_mock(
            "vjB",
            "20120615",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120615T000100"),
                    departure=tstamp("20120615T180100"),
                    arrival_delay=0,
                    departure_delay=0,
                    arrival_skipped=True,
                    departure_skipped=True,
                    is_detour=True,
                    message='deleted for detour',
                ),
                UpdatedStopTime(
                    "stop_point:stopC",
                    arrival_delay=0,
                    departure_delay=0,
                    arrival=tstamp("20120616T003000"),
                    departure=tstamp("20120616T003000"),
                    is_added=True,
                    is_detour=True,
                    message='added for detour',
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120616T010102"),
                    departure=tstamp("20120616T010102"),
                    arrival_delay=7 * 60 * 60,
                    message="No more pass midnight",
                ),
            ],
            disruption_id='stop_time_with_detour',
            effect='delayed',
        )

        # The disruption created above is modified so no disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert initial_nb_disruptions + 1 == len(disruptions_after['disruptions'])

        # The disruption created above is modified so no vehicle_journey is added
        pt_response = self.query_region('vehicle_journeys')
        assert initial_nb_vehicle_journeys + 1 == len(pt_response['vehicle_journeys'])

        # Query for 20120615T180000 makes wait till 003000 the next day
        # vjB circulates on 20120616 with departure at 00:30:00 and arrival at 01:01:02
        ca_15T18_journey_query = empty_query.format(
            f='stop_point:stopC', to='stop_point:stopA', dt='20120615T180000'
        )
        response = self.query_region(ca_15T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120616T003000'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T010102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'
        assert len(response['journeys'][0]['sections'][0]['stop_date_times']) == 2

        # vjB circulates the day before at 18:01:00 and arrival at 18:01:02
        response = self.query_region(ba_14T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120614T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120614T180102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        # vjB circulates the day after at 18:01:00 and arrival at 18:01:02
        response = self.query_region(ba_16T18_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120616T180100'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T180102'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'B'
        assert response['journeys'][0]['sections'][0]['base_departure_date_time'] == '20120616T180100'
        assert response['journeys'][0]['sections'][0]['base_arrival_date_time'] == '20120616T180102'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestKirinDelayOnBasePassMidnightTowardsNextDay(MockKirinDisruptionsFixture):
    def test_delay_on_base_pass_midnight_towards_next_day(self):
        """
        Relates to "test_cots_update_trip_with_delay_pass_midnight_on_first_station" in kirin
        Test on a vehicle_journey with departure from stop_point:stopB at 23:55:00 and arrival
        to stop_point:stopA at 00:01:00 the next day.
        1. Verify disruption count, vehicle_journeys count and journey
        2. Add a disruption with a delay = 2 minutes at first station (stop_point:stopB) so that
        there is still pass midnight
        3. Update the disruption with a delay = 6 minutes at first station  and delay = 5 minutes
        at second station so that there is no more pass midnight and the departure is the day after
        4. Update the disruption with a smaller delay on first station and advance on arrival station
        so that there is no pass midnight and the departure is the same day as original (base_schedule)
        """

        def journey_base_schedule_for_day_before(resp):
            assert resp['journeys'][0]['departure_date_time'] == '20120614T235500'
            assert resp['journeys'][0]['arrival_date_time'] == '20120615T000100'
            assert resp['journeys'][0]['sections'][0]['display_informations']['name'] == 'PM'
            assert resp['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        def journey_base_schedule_for_next_day(resp):
            assert resp['journeys'][0]['departure_date_time'] == '20120616T235500'
            assert resp['journeys'][0]['arrival_date_time'] == '20120617T000100'
            assert resp['journeys'][0]['sections'][0]['display_informations']['name'] == 'PM'
            assert resp['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        disruption_query = 'disruptions?_current _datetime={dt}'.format(dt='20120615T080000')
        initial_nb_disruptions = len(self.query_region(disruption_query)['disruptions'])

        pt_response = self.query_region('vehicle_journeys')
        initial_nb_vehicle_journeys = len(pt_response['vehicle_journeys'])

        empty_query = (
            "journeys?from={f}&to={to}&data_freshness=realtime&max_duration_to_pt=0&"
            "datetime={dt}&_current_datetime={dt}"
        )

        # Check journeys in realtime for 20120615(the day of the future disruption) from B to A
        # vjPM circulates everyday with departure at 23:55:00 and arrival at 00:01:00 the day after
        ba_15T23_journey_query = empty_query.format(
            f='stop_point:stopB', to='stop_point:stopA', dt='20120615T235000'
        )

        response = self.query_region(ba_15T23_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120615T235500'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T000100'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'PM'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'base_schedule'

        # vjPM circulates the day before at 23:55:00 and arrival at 00:01:00 the day after
        ba_14T23_journey_query = empty_query.format(
            f='stop_point:stopB', to='stop_point:stopA', dt='20120614T235000'
        )
        response = self.query_region(ba_14T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_day_before(response)

        # vjPM circulates the day after at 23:55:00 and arrival at 00:01:00 the day after
        ba_16T23_journey_query = empty_query.format(
            f='stop_point:stopB', to='stop_point:stopA', dt='20120616T235000'
        )
        response = self.query_region(ba_16T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_next_day(response)

        # A new disruption with a delay on departure station before midnight
        self.send_mock(
            "vjPM",
            "20120615",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120615T235700"),
                    departure=tstamp("20120615T235700"),
                    arrival_delay=2 * 60,
                    departure_delay=2 * 60,
                    message="Delay before pass midnight",
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120616T000100"),
                    departure=tstamp("20120616T000100"),
                    arrival_delay=0,
                ),
            ],
            disruption_id='delay_before_pm',
            effect='delayed',
        )

        # A new disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert initial_nb_disruptions + 1 == len(disruptions_after['disruptions'])

        # Now we have 1 more vehicle_journey than before
        pt_response = self.query_region('vehicle_journeys')
        assert initial_nb_vehicle_journeys + 1 == len(pt_response['vehicle_journeys'])

        # Check journeys in realtime for 20120615, the day of the disruption from B to A
        # vjB circulates with departure at 23:57:00 and arrival at 00:01:00 the day after
        response = self.query_region(ba_15T23_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120615T235700'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T000100'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'PM'
        assert response['journeys'][0]['sections'][0]['base_departure_date_time'] == '20120615T235500'
        assert response['journeys'][0]['sections'][0]['departure_date_time'] == '20120615T235700'
        assert response['journeys'][0]['sections'][0]['base_arrival_date_time'] == '20120616T000100'
        assert response['journeys'][0]['sections'][0]['arrival_date_time'] == '20120616T000100'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'

        # vjPM circulates the day before at 23:55:00 and arrival at 00:01:00 the day after
        response = self.query_region(ba_14T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_day_before(response)

        # vjPM circulates the day after at 23:55:00 and arrival at 00:01:00 the day after
        response = self.query_region(ba_16T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_next_day(response)

        # Disruption is modified with a delay on first station so that there is no more pass midnight
        # and the departure is the day after
        self.send_mock(
            "vjPM",
            "20120615",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120616T000100"),
                    departure=tstamp("20120616T000100"),
                    arrival_delay=6 * 60,
                    departure_delay=6 * 60,
                    message="Departure the next day",
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120616T000600"),
                    departure=tstamp("20120616T000600"),
                    arrival_delay=5 * 60,
                    message="Arrival delayed",
                ),
            ],
            disruption_id='delay_before_pm',
            effect='delayed',
        )

        # The disruption created above is modified so no disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert initial_nb_disruptions + 1 == len(disruptions_after['disruptions'])

        # We have 1 more vehicle_journey than initial as realtime vj is deleted and a new one is added
        pt_response = self.query_region('vehicle_journeys')
        assert initial_nb_vehicle_journeys + 1 == len(pt_response['vehicle_journeys'])

        # Check journeys in realtime for 20120615, the day of the disruption from B to A
        # vjB circulates with departure at 23:55:00 and arrival at 00:06:00 the day after
        response = self.query_region(ba_15T23_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120616T000100'
        assert response['journeys'][0]['arrival_date_time'] == '20120616T000600'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'PM'
        assert response['journeys'][0]['sections'][0]['base_departure_date_time'] == '20120615T235500'
        assert response['journeys'][0]['sections'][0]['departure_date_time'] == '20120616T000100'
        assert response['journeys'][0]['sections'][0]['base_arrival_date_time'] == '20120616T000100'
        assert response['journeys'][0]['sections'][0]['arrival_date_time'] == '20120616T000600'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'

        # vjPM circulates the day before at 23:55:00 and arrival at 00:01:00 the day after
        response = self.query_region(ba_14T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_day_before(response)

        # vjPM circulates the day after at 23:55:00 and arrival at 00:01:00 the day after
        response = self.query_region(ba_16T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_next_day(response)

        # Disruption is modified with a smaller delay on first station and advance on arrival station
        #  so that there is no pass midnight and the departure is the same day as original (base_schedule)
        self.send_mock(
            "vjPM",
            "20120615",
            'modified',
            [
                UpdatedStopTime(
                    "stop_point:stopB",
                    arrival=tstamp("20120615T235600"),
                    departure=tstamp("20120615T235600"),
                    arrival_delay=1 * 60,
                    departure_delay=1 * 60,
                    message="Departure the same day",
                ),
                UpdatedStopTime(
                    "stop_point:stopA",
                    arrival=tstamp("20120615T235900"),
                    departure=tstamp("20120615T235900"),
                    arrival_delay=-2 * 60,
                    message="Arrival advanced",
                ),
            ],
            disruption_id='delay_before_pm',
            effect='delayed',
        )

        # The disruption created above is modified so no disruption is added
        disruptions_after = self.query_region(disruption_query)
        assert initial_nb_disruptions + 1 == len(disruptions_after['disruptions'])

        # We have 1 more vehicle_journey than initial as realtime vj is deleted and a new one is added
        pt_response = self.query_region('vehicle_journeys')
        assert initial_nb_vehicle_journeys + 1 == len(pt_response['vehicle_journeys'])

        # Check journeys in realtime for 20120615, the day of the disruption from B to A
        # vjB circulates with departure at 23:56:00 and arrival at 23:59:00 the same day
        response = self.query_region(ba_15T23_journey_query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == '20120615T235600'
        assert response['journeys'][0]['arrival_date_time'] == '20120615T235900'
        assert response['journeys'][0]['sections'][0]['display_informations']['name'] == 'PM'
        assert response['journeys'][0]['sections'][0]['base_departure_date_time'] == '20120615T235500'
        assert response['journeys'][0]['sections'][0]['departure_date_time'] == '20120615T235600'
        assert response['journeys'][0]['sections'][0]['base_arrival_date_time'] == '20120616T000100'
        assert response['journeys'][0]['sections'][0]['arrival_date_time'] == '20120615T235900'
        assert response['journeys'][0]['sections'][0]['data_freshness'] == 'realtime'

        # vjPM circulates the day before at 23:55:00 and arrival at 00:01:00 the day after
        response = self.query_region(ba_14T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_day_before(response)

        # vjPM circulates the day after at 23:55:00 and arrival at 00:01:00 the day after
        response = self.query_region(ba_16T23_journey_query)
        assert len(response['journeys']) == 1
        journey_base_schedule_for_next_day(response)


RAIL_SECTIONS_TEST_SETTING = {
    'rail_sections_test': {
        'kraken_args': [
            '--BROKER.rt_topics=' + rt_topic,
            'spawn_maintenance_worker',
            '--GENERAL.is_realtime_add_enabled=1',
            '--GENERAL.is_realtime_add_trip_enabled=1',
        ]
    }
}


@dataset(RAIL_SECTIONS_TEST_SETTING)
class TestNoServiceJourney(MockKirinOrChaosDisruptionsFixture):
    def test_no_service_journey_disruptions(self):
        # see tests/mock-kraken/rail_sections_test.cpp to detail disruptions already applied and existing VJs

        disruptions_before = self.query_region('disruptions?_current_datetime=20170120T080000')
        nb_disruptions_before = len(disruptions_before['disruptions'])
        assert nb_disruptions_before == 10

        vjs_before = self.query_region('vehicle_journeys?count=100')
        assert len(vjs_before['vehicle_journeys']) == 27

        # Test that a journey affected by a DETOUR rail-section is NO_SERVICE when stop used is deleted
        journey_AADD_query = (
            "journeys?from=stopAA&to=stopDD&datetime=20170108T080000&_current_datetime=20170108T080000"
        )
        journey_AADD = self.query_region(journey_AADD_query)
        assert len(journey_AADD['journeys']) == 1
        assert journey_AADD['journeys'][0]['status'] == 'NO_SERVICE'
        assert len(journey_AADD['disruptions']) == 1
        assert journey_AADD['disruptions'][0]['severity']['effect'] == 'DETOUR'
        links = get_links_dict(journey_AADD)
        assert 'bypass_disruptions' in links

        # "XFail" test: not mandatory to keep it as-is
        journey_AADD_after_publish_query = (
            "journeys?from=stopAA&to=stopDD&datetime=20170108T080000&_current_datetime=20170112T080000"
        )
        journey_AADD_after_publish = self.query_region(journey_AADD_after_publish_query)
        # Out of publication period, journey is still marked as NO_SERVICE
        assert len(journey_AADD_after_publish['journeys']) == 1
        assert journey_AADD_after_publish['journeys'][0]['status'] == 'NO_SERVICE'
        # Meanwhile, disruptions still respect the publication period
        assert len(journey_AADD_after_publish['disruptions']) == 0
        links = get_links_dict(journey_AADD)
        assert 'bypass_disruptions' in links

        # Test that before sending any "Kirin" disruption on a journey, all is fine
        journey_AC_query = (
            "journeys?from=stopA&to=stopC&datetime=20170120T080000&_current_datetime=20170120T080000"
        )
        journey_AC = self.query_region(journey_AC_query)
        assert len(journey_AC['journeys']) == 1
        assert journey_AC['journeys'][0]['status'] == ''
        assert len(journey_AC['disruptions']) == 0
        links = get_links_dict(journey_AC)
        assert 'bypass_disruptions' not in links

        journey_AE_query = (
            "journeys?from=stopA&to=stopE&datetime=20170120T080000&_current_datetime=20170120T080000"
        )
        journey_AE = self.query_region(journey_AE_query)
        assert len(journey_AE['journeys']) == 1
        assert journey_AE['journeys'][0]['status'] == ''
        assert len(journey_AE['disruptions']) == 0
        links = get_links_dict(journey_AE)
        assert 'bypass_disruptions' not in links

        # realtime deletes stop-time in stopC (and REDUCED_SERVICE status)
        self.send_mock(
            "vj:1",
            "20170120",
            'modified',
            [
                UpdatedStopTime(
                    "stopA",
                    arrival=tstamp("20170120T080000"),
                    departure=tstamp("20170120T080000"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stopB",
                    arrival=tstamp("20170120T080500"),
                    departure=tstamp("20170120T080500"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stopC",
                    arrival=tstamp("20170120T081000"),
                    departure=tstamp("20170120T081000"),
                    message='stop_time deleted',
                    arrival_skipped=True,
                    departure_skipped=True,
                ),
                UpdatedStopTime(
                    "stopD",
                    arrival=tstamp("20170120T081500"),
                    departure=tstamp("20170120T081500"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stopE",
                    arrival=tstamp("20170120T082000"),
                    departure=tstamp("20170120T082000"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stopF",
                    arrival=tstamp("20170120T082500"),
                    departure=tstamp("20170120T082500"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stopG",
                    arrival=tstamp("20170120T083000"),
                    departure=tstamp("20170120T083000"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stopH",
                    arrival=tstamp("20170120T083500"),
                    departure=tstamp("20170120T083500"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
                UpdatedStopTime(
                    "stopG",
                    arrival=tstamp("20170120T084000"),
                    departure=tstamp("20170120T084000"),
                    arrival_delay=0,
                    departure_delay=0,
                ),
            ],
            disruption_id='reduced_service_vj1',
            effect='reduced_service',
            is_kirin=True,
        )
        disrupts = self.query_region('disruptions?_current_datetime=20170120T080000')
        assert len(disrupts['disruptions']) == 11
        assert has_the_disruption(disrupts, 'reduced_service_vj1')
        last_disrupt = disrupts['disruptions'][-1]
        assert last_disrupt['severity']['effect'] == 'REDUCED_SERVICE'
        assert last_disrupt['severity']['name'] == 'reduced service'

        vjs_after = self.query_region('vehicle_journeys?count=100')
        # we got a new vj due to the disruption, which means the disruption is handled correctly
        assert len(vjs_after['vehicle_journeys']) == 28

        # After "Kirin" disruption, journey using a deleted stop has its status changed to NO_SERVICE
        journey_AC = self.query_region(journey_AC_query)
        assert len(journey_AC['journeys']) == 1
        assert journey_AC['journeys'][0]['status'] == 'NO_SERVICE'
        assert len(journey_AC['disruptions']) == 1
        assert journey_AC['disruptions'][0]['severity']['effect'] == 'REDUCED_SERVICE'
        links = get_links_dict(journey_AC)
        assert 'bypass_disruptions' in links

        # Meanwhile, journey using a VJ with deleted stop, but not using that stop keeps its status
        journey_AE = self.query_region(journey_AE_query)
        assert len(journey_AE['journeys']) == 1
        assert journey_AE['journeys'][0]['status'] == 'REDUCED_SERVICE'
        assert len(journey_AE['disruptions']) == 1
        assert journey_AE['disruptions'][0]['severity']['effect'] == 'REDUCED_SERVICE'
        links = get_links_dict(journey_AE)
        assert 'bypass_disruptions' not in links

        # Test that a stop NO_SERVICE disruption only affects journeys using that stop
        journey_AB_query = (
            "journeys?from=stopA&to=stopB&datetime=20170118T080000&_current_datetime=20170118T080000"
        )
        journey_AB = self.query_region(journey_AB_query)
        assert len(journey_AB['journeys']) == 1
        assert journey_AB['journeys'][0]['status'] == ''
        assert len(journey_AB['disruptions']) == 0
        links = get_links_dict(journey_AB)
        assert 'bypass_disruptions' not in links

        journey_AD_query = (
            "journeys?from=stopA&to=stopD&datetime=20170118T080000&_current_datetime=20170118T080000"
        )
        journey_AD = self.query_region(journey_AD_query)
        assert len(journey_AD['journeys']) == 1
        assert journey_AD['journeys'][0]['status'] == ''
        assert len(journey_AD['disruptions']) == 0
        links = get_links_dict(journey_AD)
        assert 'bypass_disruptions' not in links

        self.send_mock(
            "bob_the_disruption",
            "stopAreaB",
            "stop_area",
            start=None,
            end=None,
            blocking=True,
            start_period="20170117T0000",
            end_period="20170119T0000",
            is_kirin=False,
        )

        journey_AB = self.query_region(journey_AB_query)
        assert len(journey_AB['journeys']) == 1
        assert journey_AB['journeys'][0]['status'] == 'NO_SERVICE'
        assert len(journey_AB['disruptions']) == 1
        assert journey_AB['disruptions'][0]['severity']['effect'] == 'NO_SERVICE'
        links = get_links_dict(journey_AB)
        assert 'bypass_disruptions' in links

        journey_AD = self.query_region(journey_AD_query)
        assert len(journey_AD['journeys']) == 1
        assert journey_AD['journeys'][0]['status'] == ''
        assert len(journey_AD['disruptions']) == 0
        links = get_links_dict(journey_AD)
        assert 'bypass_disruptions' not in links


@dataset(RAIL_SECTIONS_TEST_SETTING)
class TestKirinThenChaosRailSectionJourney(MockKirinOrChaosDisruptionsFixture):
    def test_kirin_then_rail_section_journey_disruptions(self):
        # Mainly testing that applying Chaos (adapted) after Kirin (realtime) keeps realtime unchanged
        # this is really depending on the sorting of disruption's sending

        def using_vj1(resp):
            return resp['journeys'][0]['sections'][0]['display_informations']['trip_short_name'] == 'vj:1'

        # Test before any disruption that vj:1 is usable in any freshness level
        journey_AE_rt_query = "journeys?from=stopA&to=stopE&datetime=20170120T080000&_current_datetime=20170120T080000&data_freshness=realtime"
        journey_AE_rt = self.query_region(journey_AE_rt_query)
        assert len(journey_AE_rt['journeys']) == 1
        assert journey_AE_rt['journeys'][0]['status'] == ''
        assert journey_AE_rt['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_rt)
        assert len(journey_AE_rt['disruptions']) == 0

        journey_EH_rt_query = "journeys?from=stopE&to=stopH&datetime=20170120T082000&_current_datetime=20170120T082000&data_freshness=realtime"
        journey_EH_rt = self.query_region(journey_EH_rt_query)
        assert len(journey_EH_rt['journeys']) == 1
        assert journey_EH_rt['journeys'][0]['status'] == ''
        assert journey_EH_rt['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_rt)
        assert len(journey_EH_rt['disruptions']) == 0

        journey_AE_adapted_query = "journeys?from=stopA&to=stopE&datetime=20170120T080000&_current_datetime=20170120T080000&data_freshness=adapted_schedule"
        journey_AE_adapted = self.query_region(journey_AE_adapted_query)
        assert len(journey_AE_adapted['journeys']) == 1
        assert journey_AE_adapted['journeys'][0]['status'] == ''
        assert journey_AE_adapted['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_adapted)
        assert len(journey_AE_adapted['disruptions']) == 0

        journey_EH_adapted_query = "journeys?from=stopE&to=stopH&datetime=20170120T082000&_current_datetime=20170120T082000&data_freshness=adapted_schedule"
        journey_EH_adapted = self.query_region(journey_EH_adapted_query)
        assert len(journey_EH_adapted['journeys']) == 1
        assert journey_EH_adapted['journeys'][0]['status'] == ''
        assert journey_EH_adapted['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_adapted)
        assert len(journey_EH_adapted['disruptions']) == 0

        journey_AE_base_query = "journeys?from=stopA&to=stopE&datetime=20170120T080000&_current_datetime=20170120T080000&data_freshness=base_schedule"
        journey_AE_base = self.query_region(journey_AE_base_query)
        assert len(journey_AE_base['journeys']) == 1
        assert journey_AE_base['journeys'][0]['status'] == ''
        assert journey_AE_base['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_base)
        assert len(journey_AE_base['disruptions']) == 0

        journey_EH_base_query = "journeys?from=stopE&to=stopH&datetime=20170120T082000&_current_datetime=20170120T082000&data_freshness=base_schedule"
        journey_EH_base = self.query_region(journey_EH_base_query)
        assert len(journey_EH_base['journeys']) == 1
        assert journey_EH_base['journeys'][0]['status'] == ''
        assert journey_EH_base['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_base)
        assert len(journey_EH_base['disruptions']) == 0

        # realtime deletes stop-time in stopC (and REDUCED_SERVICE status)
        stus = [
            UpdatedStopTime(
                "stopA",
                arrival=tstamp("20170120T080000"),
                departure=tstamp("20170120T080000"),
                message='stop_time deleted',
                arrival_skipped=True,
                departure_skipped=True,
            ),
            UpdatedStopTime(
                "stopB",
                arrival=tstamp("20170120T080500"),
                departure=tstamp("20170120T080500"),
                message='stop_time deleted',
                arrival_skipped=True,
                departure_skipped=True,
            ),
            UpdatedStopTime(
                "stopC",
                arrival=tstamp("20170120T081000"),
                departure=tstamp("20170120T081000"),
                message='stop_time deleted',
                arrival_skipped=True,
                departure_skipped=True,
            ),
            UpdatedStopTime(
                "stopD",
                arrival=tstamp("20170120T081600"),
                departure=tstamp("20170120T081600"),
                arrival_delay=1,
                departure_delay=1,
            ),
            UpdatedStopTime(
                "stopE",
                arrival=tstamp("20170120T082000"),
                departure=tstamp("20170120T082000"),
                arrival_delay=0,
                departure_delay=0,
            ),
            UpdatedStopTime(
                "stopF",
                arrival=tstamp("20170120T082500"),
                departure=tstamp("20170120T082500"),
                arrival_delay=0,
                departure_delay=0,
            ),
            UpdatedStopTime(
                "stopG",
                arrival=tstamp("20170120T083000"),
                departure=tstamp("20170120T083000"),
                arrival_delay=0,
                departure_delay=0,
            ),
            UpdatedStopTime(
                "stopH",
                arrival=tstamp("20170120T083500"),
                departure=tstamp("20170120T083500"),
                arrival_delay=0,
                departure_delay=0,
            ),
            UpdatedStopTime(
                "stopG",
                arrival=tstamp("20170120T084000"),
                departure=tstamp("20170120T084000"),
                arrival_delay=0,
                departure_delay=0,
            ),
        ]

        # Send a "Kirin" disruption: affects only realtime on given section (except status)
        self.send_mock(
            "vj:1",
            "20170120",
            'modified',
            stus,
            disruption_id='reduced_service_vj1',
            effect='reduced_service',
            is_kirin=True,
        )

        journey_AE_rt = self.query_region(journey_AE_rt_query)
        assert "journeys" not in journey_AE_rt
        assert journey_AE_rt['error']['message'] == 'no solution found for this journey'

        journey_EH_rt = self.query_region(journey_EH_rt_query)
        assert len(journey_EH_rt['journeys']) == 1
        assert journey_EH_rt['journeys'][0]['status'] == 'REDUCED_SERVICE'
        assert journey_EH_rt['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_rt)
        assert len(journey_EH_rt['disruptions']) == 1

        journey_AE_adapted = self.query_region(journey_AE_adapted_query)
        assert len(journey_AE_adapted['journeys']) == 1
        assert journey_AE_adapted['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_AE_adapted['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_adapted)
        assert len(journey_AE_adapted['disruptions']) == 1

        journey_EH_adapted = self.query_region(journey_EH_adapted_query)
        assert len(journey_EH_adapted['journeys']) == 1
        assert journey_EH_adapted['journeys'][0]['status'] == 'REDUCED_SERVICE'
        assert journey_EH_adapted['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_adapted)
        assert len(journey_EH_adapted['disruptions']) == 1

        journey_AE_base = self.query_region(journey_AE_base_query)
        assert len(journey_AE_base['journeys']) == 1
        assert journey_AE_base['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_AE_base['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_base)
        assert len(journey_AE_base['disruptions']) == 1

        journey_EH_base = self.query_region(journey_EH_base_query)
        assert len(journey_EH_base['journeys']) == 1
        assert journey_EH_base['journeys'][0]['status'] == 'REDUCED_SERVICE'
        assert journey_EH_base['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_base)
        assert len(journey_EH_base['disruptions']) == 1

        # Send a "Chaos" rail-section disruption: affects only adapted as realtime is already affected by
        # a kirin disruption (Kirin is prioritary over Chaos)
        self.send_mock(
            "bobette_the_disruption",
            "line:1",
            "rail_section",
            start="stopAreaA",
            end="stopAreaC",
            blocked_sa=["stopAreaA", "stopAreaB", "stopAreaC"],
            routes=["route1"],
            start_period="20170120T000000",
            end_period="20170120T220000",
            blocking=True,
            is_kirin=False,
        )

        journey_AE_rt = self.query_region(journey_AE_rt_query)
        assert "journeys" not in journey_AE_rt
        assert journey_AE_rt['error']['message'] == 'no solution found for this journey'

        journey_EH_rt = self.query_region(journey_EH_rt_query)
        assert len(journey_EH_rt['journeys']) == 1
        assert journey_EH_rt['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_EH_rt['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_rt)
        assert len(journey_EH_rt['disruptions']) == 2

        journey_AE_adapted = self.query_region(journey_AE_adapted_query)
        assert "journeys" not in journey_AE_adapted
        assert journey_AE_adapted['error']['message'] == 'no solution found for this journey'

        journey_EH_adapted = self.query_region(journey_EH_adapted_query)
        assert "journeys" not in journey_EH_adapted
        assert journey_EH_adapted['error']['message'] == 'no solution found for this journey'

        journey_AE_base = self.query_region(journey_AE_base_query)
        assert len(journey_AE_base['journeys']) == 1
        assert journey_AE_base['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_AE_base['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_base)
        assert len(journey_AE_base['disruptions']) == 2

        journey_EH_base = self.query_region(journey_EH_base_query)
        assert len(journey_EH_base['journeys']) == 1
        assert journey_EH_base['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_EH_base['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_base)
        assert len(journey_EH_base['disruptions']) == 2

        # Delete "Chaos" rail-section disruption: back to situation before any Chaos
        self.send_mock(
            "bobette_the_disruption",
            "line:1",
            "rail_section",
            start="stopAreaA",
            end="stopAreaC",
            is_deleted=True,
            is_kirin=False,
        )

        journey_AE_rt = self.query_region(journey_AE_rt_query)
        assert "journeys" not in journey_AE_rt
        assert journey_AE_rt['error']['message'] == 'no solution found for this journey'

        journey_EH_rt = self.query_region(journey_EH_rt_query)
        assert len(journey_EH_rt['journeys']) == 1
        assert journey_EH_rt['journeys'][0]['status'] == 'REDUCED_SERVICE'
        assert journey_EH_rt['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_rt)
        assert len(journey_EH_rt['disruptions']) == 1

        journey_AE_adapted = self.query_region(journey_AE_adapted_query)
        assert len(journey_AE_adapted['journeys']) == 1
        assert journey_AE_adapted['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_AE_adapted['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_adapted)
        assert len(journey_AE_adapted['disruptions']) == 1

        journey_EH_adapted = self.query_region(journey_EH_adapted_query)
        assert len(journey_EH_adapted['journeys']) == 1
        assert journey_EH_adapted['journeys'][0]['status'] == 'REDUCED_SERVICE'
        assert journey_EH_adapted['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_adapted)
        assert len(journey_EH_adapted['disruptions']) == 1

        journey_AE_base = self.query_region(journey_AE_base_query)
        assert len(journey_AE_base['journeys']) == 1
        assert journey_AE_base['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_AE_base['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_base)
        assert len(journey_AE_base['disruptions']) == 1

        journey_EH_base = self.query_region(journey_EH_base_query)
        assert len(journey_EH_base['journeys']) == 1
        assert journey_EH_base['journeys'][0]['status'] == 'REDUCED_SERVICE'
        assert journey_EH_base['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_base)
        assert len(journey_EH_base['disruptions']) == 1


@dataset(RAIL_SECTIONS_TEST_SETTING)
class TestChaosRailSectionOnlyJourney(MockKirinOrChaosDisruptionsFixture):
    def test_rail_section_only_journey_disruptions(self):
        # Testing that applying Chaos (adapted) works aas expected on every data_freshness level

        def using_vj1(resp):
            return resp['journeys'][0]['sections'][0]['display_informations']['trip_short_name'] == 'vj:1'

        # Test before any disruption that vj:1 is usable in any freshness level
        journey_AE_rt_query = "journeys?from=stopA&to=stopE&datetime=20170120T080000&_current_datetime=20170120T080000&data_freshness=realtime"
        journey_AE_rt = self.query_region(journey_AE_rt_query)
        assert len(journey_AE_rt['journeys']) == 1
        assert journey_AE_rt['journeys'][0]['status'] == ''
        assert journey_AE_rt['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_rt)
        assert len(journey_AE_rt['disruptions']) == 0

        journey_EH_rt_query = "journeys?from=stopE&to=stopH&datetime=20170120T082000&_current_datetime=20170120T082000&data_freshness=realtime"
        journey_EH_rt = self.query_region(journey_EH_rt_query)
        assert len(journey_EH_rt['journeys']) == 1
        assert journey_EH_rt['journeys'][0]['status'] == ''
        assert journey_EH_rt['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_rt)
        assert len(journey_EH_rt['disruptions']) == 0

        journey_AE_adapted_query = "journeys?from=stopA&to=stopE&datetime=20170120T080000&_current_datetime=20170120T080000&data_freshness=adapted_schedule"
        journey_AE_adapted = self.query_region(journey_AE_adapted_query)
        assert len(journey_AE_adapted['journeys']) == 1
        assert journey_AE_adapted['journeys'][0]['status'] == ''
        assert journey_AE_adapted['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_adapted)
        assert len(journey_AE_adapted['disruptions']) == 0

        journey_EH_adapted_query = "journeys?from=stopE&to=stopH&datetime=20170120T082000&_current_datetime=20170120T082000&data_freshness=adapted_schedule"
        journey_EH_adapted = self.query_region(journey_EH_adapted_query)
        assert len(journey_EH_adapted['journeys']) == 1
        assert journey_EH_adapted['journeys'][0]['status'] == ''
        assert journey_EH_adapted['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_adapted)
        assert len(journey_EH_adapted['disruptions']) == 0

        journey_AE_base_query = "journeys?from=stopA&to=stopE&datetime=20170120T080000&_current_datetime=20170120T080000&data_freshness=base_schedule"
        journey_AE_base = self.query_region(journey_AE_base_query)
        assert len(journey_AE_base['journeys']) == 1
        assert journey_AE_base['journeys'][0]['status'] == ''
        assert journey_AE_base['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_base)
        assert len(journey_AE_base['disruptions']) == 0

        journey_EH_base_query = "journeys?from=stopE&to=stopH&datetime=20170120T082000&_current_datetime=20170120T082000&data_freshness=base_schedule"
        journey_EH_base = self.query_region(journey_EH_base_query)
        assert len(journey_EH_base['journeys']) == 1
        assert journey_EH_base['journeys'][0]['status'] == ''
        assert journey_EH_base['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_base)
        assert len(journey_EH_base['disruptions']) == 0

        # Send a "Chaos" rail-section disruption: affects only adapted as realtime is already affected by
        # a kirin disruption (Kirin is prioritary over Chaos)
        self.send_mock(
            "bobette_the_disruption",
            "line:1",
            "rail_section",
            start="stopAreaA",
            end="stopAreaC",
            blocked_sa=["stopAreaA", "stopAreaB", "stopAreaC"],
            routes=["route1"],
            start_period="20170120T000000",
            end_period="20170120T220000",
            blocking=True,
            is_kirin=False,
        )

        journey_AE_rt = self.query_region(journey_AE_rt_query)
        assert "journeys" not in journey_AE_rt
        assert journey_AE_rt['error']['message'] == 'no solution found for this journey'

        journey_EH_rt = self.query_region(journey_EH_rt_query)
        assert "journeys" not in journey_EH_rt
        assert journey_EH_rt['error']['message'] == 'no solution found for this journey'

        journey_AE_adapted = self.query_region(journey_AE_adapted_query)
        assert "journeys" not in journey_AE_adapted
        assert journey_AE_adapted['error']['message'] == 'no solution found for this journey'

        journey_EH_adapted = self.query_region(journey_EH_adapted_query)
        assert "journeys" not in journey_EH_adapted
        assert journey_EH_adapted['error']['message'] == 'no solution found for this journey'

        journey_AE_base = self.query_region(journey_AE_base_query)
        assert len(journey_AE_base['journeys']) == 1
        assert journey_AE_base['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_AE_base['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_base)
        assert len(journey_AE_base['disruptions']) == 1

        journey_EH_base = self.query_region(journey_EH_base_query)
        assert len(journey_EH_base['journeys']) == 1
        assert journey_EH_base['journeys'][0]['status'] == 'NO_SERVICE'
        assert journey_EH_base['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_base)
        assert len(journey_EH_base['disruptions']) == 1

        # Delete "Chaos" rail-section disruption: back to no disruption
        self.send_mock(
            "bobette_the_disruption",
            "line:1",
            "rail_section",
            start="stopAreaA",
            end="stopAreaC",
            is_deleted=True,
            is_kirin=False,
        )

        journey_AE_rt = self.query_region(journey_AE_rt_query)
        assert len(journey_AE_rt['journeys']) == 1
        assert journey_AE_rt['journeys'][0]['status'] == ''
        assert journey_AE_rt['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_rt)
        assert len(journey_AE_rt['disruptions']) == 0

        journey_EH_rt = self.query_region(journey_EH_rt_query)
        assert len(journey_EH_rt['journeys']) == 1
        assert journey_EH_rt['journeys'][0]['status'] == ''
        assert journey_EH_rt['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_rt)
        assert len(journey_EH_rt['disruptions']) == 0

        journey_AE_adapted = self.query_region(journey_AE_adapted_query)
        assert len(journey_AE_adapted['journeys']) == 1
        assert journey_AE_adapted['journeys'][0]['status'] == ''
        assert journey_AE_adapted['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_adapted)
        assert len(journey_AE_adapted['disruptions']) == 0

        journey_EH_adapted = self.query_region(journey_EH_adapted_query)
        assert len(journey_EH_adapted['journeys']) == 1
        assert journey_EH_adapted['journeys'][0]['status'] == ''
        assert journey_EH_adapted['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_adapted)
        assert len(journey_EH_adapted['disruptions']) == 0

        journey_AE_base = self.query_region(journey_AE_base_query)
        assert len(journey_AE_base['journeys']) == 1
        assert journey_AE_base['journeys'][0]['status'] == ''
        assert journey_AE_base['journeys'][0]['arrival_date_time'] == '20170120T082000'
        assert using_vj1(journey_AE_base)
        assert len(journey_AE_base['disruptions']) == 0

        journey_EH_base = self.query_region(journey_EH_base_query)
        assert len(journey_EH_base['journeys']) == 1
        assert journey_EH_base['journeys'][0]['status'] == ''
        assert journey_EH_base['journeys'][0]['arrival_date_time'] == '20170120T083500'
        assert using_vj1(journey_EH_base)
        assert len(journey_EH_base['disruptions']) == 0


def make_mock_kirin_item(
    vj_id,
    date,
    status='canceled',
    new_stop_time_list=[],
    disruption_id=None,
    effect=None,
    physical_mode_id=None,
    headsign=None,
):
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
    if headsign:
        trip_update.Extensions[kirin_pb2.headsign] = headsign
    if physical_mode_id:
        trip_update.vehicle.Extensions[kirin_pb2.physical_mode_id] = physical_mode_id
    if effect == 'unknown':
        trip_update.Extensions[kirin_pb2.effect] = gtfs_realtime_pb2.Alert.UNKNOWN_EFFECT
    elif effect == 'modified':
        trip_update.Extensions[kirin_pb2.effect] = gtfs_realtime_pb2.Alert.MODIFIED_SERVICE
    elif effect == 'delayed':
        trip_update.Extensions[kirin_pb2.effect] = gtfs_realtime_pb2.Alert.SIGNIFICANT_DELAYS
    elif effect == 'detour':
        trip_update.Extensions[kirin_pb2.effect] = gtfs_realtime_pb2.Alert.DETOUR
    elif effect == 'reduced_service':
        trip_update.Extensions[kirin_pb2.effect] = gtfs_realtime_pb2.Alert.REDUCED_SERVICE
    elif effect == 'additional_service':
        trip_update.Extensions[kirin_pb2.effect] = gtfs_realtime_pb2.Alert.ADDITIONAL_SERVICE

    if status == 'canceled':
        # TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
        trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.CANCELED
    elif status in ['modified', 'added']:
        # TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
        if status == 'modified':
            trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.SCHEDULED
        elif status == 'added':
            trip.schedule_relationship = gtfs_realtime_pb2.TripDescriptor.ADDED

        for st in new_stop_time_list:
            stop_time_update = trip_update.stop_time_update.add()
            stop_time_update.stop_id = st.stop_id
            stop_time_update.arrival.time = st.arrival
            stop_time_update.arrival.delay = st.arrival_delay
            stop_time_update.departure.time = st.departure
            stop_time_update.departure.delay = st.departure_delay

            def get_stop_time_status(is_skipped=False, is_added=False, is_detour=False):
                if is_skipped:
                    if is_detour:
                        return kirin_pb2.DELETED_FOR_DETOUR
                    return kirin_pb2.DELETED
                if is_added:
                    if is_detour:
                        return kirin_pb2.ADDED_FOR_DETOUR
                    return kirin_pb2.ADDED
                return kirin_pb2.SCHEDULED

            stop_time_update.arrival.Extensions[kirin_pb2.stop_time_event_status] = get_stop_time_status(
                st.arrival_skipped, st.is_added, st.is_detour
            )
            stop_time_update.departure.Extensions[kirin_pb2.stop_time_event_status] = get_stop_time_status(
                st.departure_skipped, st.is_added, st.is_detour
            )
            if st.message:
                stop_time_update.Extensions[kirin_pb2.stoptime_message] = st.message
    else:
        # TODO
        pass

    return feed_message.SerializeToString()
