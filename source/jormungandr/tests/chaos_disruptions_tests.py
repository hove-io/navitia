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
from tests import gtfs_realtime_pb2
from tests_mechanism import dataset
from check_utils import *
import chaos_pb2
from jormungandr import utils
from rabbitmq_utils import RabbitMQCnxFixture, rt_topic


class ChaosDisruptionsFixture(RabbitMQCnxFixture):
    """
    Mock a chaos disruption message, in order to check the api
    """
    def _make_mock_item(self, disruption_name, impacted_obj, impacted_obj_type, start=None, end=None,
                              message='default_message', is_deleted=False, blocking=False,
                         start_period="20100412T165200", end_period="20200412T165200"):
        return make_mock_chaos_item(disruption_name, impacted_obj, impacted_obj_type, start, end, message, is_deleted,
                                    blocking, start_period, end_period)


MAIN_ROUTING_TEST_SETTING = {"main_routing_test": {'kraken_args': ['--BROKER.sleeptime=0',
                                                                   '--BROKER.rt_topics='+rt_topic,
                                                                   'spawn_maintenance_worker']}}

@dataset(MAIN_ROUTING_TEST_SETTING)
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

        self.send_mock("bob_the_disruption", "stopB", "stop_area")

        #and we call again, we must have the disruption now
        response = self.query_region('stop_areas/stopB')
        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        disruptions = get_disruptions(stop, response)

        #at first we got only one disruption on B
        assert len(disruptions) == 1

        assert any(d['disruption_id'] == 'bob_the_disruption' for d in disruptions)

        #here we test messages in disruption: message, channel and types
        messages = get_not_null(disruptions[0], 'messages')
        assert len(messages) == 2
        eq_(messages[0]['text'], 'default_message')
        channel = get_not_null(messages[0], 'channel')
        eq_(channel['id'], 'sms')
        eq_(channel['name'], 'sms')
        eq_(channel['content_type'], 'text')
        assert len(channel['types']) == 1
        eq_(channel['types'][0], 'sms')

        eq_(messages[1]['text'], 'default_message')
        channel = get_not_null(messages[1], 'channel')
        eq_(channel['id'], 'email')
        eq_(channel['name'], 'email')
        eq_(channel['content_type'], 'html')
        assert len(channel['types']) == 2
        eq_(channel['types'][0], 'web')
        eq_(channel['types'][1], 'email')


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsLineSection(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """
    def test_disruption_on_line_section(self):
        """
        when calling the pt object line:A, at first we have no disruptions,

        then we mock a disruption sent from chaos, and we call again the pt object line:A
        we then must have one disruption
        """
        response = self.query_region('lines/A')

        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        line = lines[0]
        #at first no disruption
        assert 'disruptions' not in line

        self.send_mock("bobette_the_disruption", "A",
                       "line_section", start="stopA", end="stopB", blocking=True)

        #and we call again, we must have the disruption now
        response = self.query_region('lines/A')
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        line = lines[0]

        disruptions = get_disruptions(line, response)

        #at first we got only one disruption
        assert len(disruptions) == 1
        assert any(d['disruption_id'] == 'bobette_the_disruption' for d in disruptions)


@dataset(MAIN_ROUTING_TEST_SETTING)
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
            assert not get_disruptions(b['stop_area'], response)

        #we create a list with every 'to' section to the stop B (the one we added the disruption on)
        self.send_mock("bob_the_disruption", "stopB", "stop_area")

        response = self.query_region(journey_basic_query)

        #the response must be still valid (this test the kraken data reloading)
        is_valid_journey_response(response, self.tester, journey_basic_query)

        stops_b_to = [s['to']['stop_point'] for j in response['journeys'] for s in j['sections']
                      if s['to']['embedded_type'] == 'stop_point' and s['to']['id'] == 'stop_point:stopB']

        assert stops_b_to

        for b in stops_b_to:
            disruptions = get_disruptions(b['stop_area'], response)

            #at first we got only one disruption on A
            assert len(disruptions) == 1

            assert any(d['disruption_id'] == 'bob_the_disruption' for d in disruptions)


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsBlocking(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """
    def get_nb_disruptions(self):
        nb_disruptions_map = defaultdict(int)

        def set_nb_disruptions(l):
            for i in l:
                nb_disruptions_map[i['id']] = len([link for link in i['links'] if link['type'] == 'disruption'])

        response = self.query_region("routes")
        set_nb_disruptions(response['routes'])
        response = self.query_region("lines")
        set_nb_disruptions(response['lines'])
        response = self.query_region('networks')
        set_nb_disruptions(response['networks'])
        response = self.query_region('stop_areas')
        set_nb_disruptions(response['stop_areas'])
        response = self.query_region('stop_points')
        set_nb_disruptions(response['stop_points'])
        return nb_disruptions_map

    def run_check(self, object_id, type_):
        links = []
        journey_query_2_to_format = "journeys?from={from_coord}&to={to_coord}&disruption_active=true".format(from_coord=s_coord, to_coord=r_coord)
        journey_query_2_to_format += "&datetime={datetime}"
        start_period = "20120615T080000"
        disruption_uri = "blocking_{}_disruption".format(type_)

        def get_type_id(k, v):
            if (k != "links" or "type" not in v or "id" not in v or v["type"] != type_) and\
               (k != type_ or "id" not in v):
                return
            links.append(v["id"])

        def check_links_(date, check_existence):
            links[:] = []
            journey_query = journey_query_2_to_format.format(datetime=date)
            response_json = self.query_region(journey_query)

            utils.walk_dict(response_json, get_type_id)
            if check_existence:
                assert any(map(lambda id_: id_ == object_id, links))
            else:
                assert all(map(lambda id_: id_ != object_id, links))


        nb_disruptions_map = self.get_nb_disruptions()
        nb_disruptions = nb_disruptions_map[object_id]
        # We send a blocking disruption on the object
        self.send_mock(disruption_uri, object_id, type_, blocking=True,
                       start_period=start_period)
        nb_disruptions_map = self.get_nb_disruptions()
        assert (nb_disruptions_map[object_id] - nb_disruptions) == 1

        check_links_("20120616T080000", False)
        #We test out of the period
        check_links_("20120614T080000", True)

        #We delete the disruption
        nb_disruptions_map = self.get_nb_disruptions()
        nb_disruptions = nb_disruptions_map[object_id]
        self.send_mock(disruption_uri, object_id, type_,
                       blocking=True, is_deleted=True, start_period=start_period)
        nb_disruptions_map = self.get_nb_disruptions()
        assert (nb_disruptions - nb_disruptions_map[object_id]) == 1
        check_links_("20120616T080000", True)

        #We try to send first the disruption, and then the impacts
        nb_disruptions = nb_disruptions_map[object_id]
        self.send_mock(disruption_uri, None, None, blocking=True,
                       start_period=start_period)
        self.send_mock(disruption_uri, object_id, type_, blocking=True,
                       start_period=start_period)
        nb_disruptions_map = self.get_nb_disruptions()
        assert (nb_disruptions_map[object_id] - nb_disruptions) == 1

        check_links_("20120616T080000", False)
        #We test out of the period
        check_links_("20120614T080000", True)

        #We clean
        self.send_mock(disruption_uri, object_id, type_,
                       blocking=True, is_deleted=True, start_period=start_period)

    def test_blocking_disruption_of_line_route_network_on_journey(self):
        """
        We send blocking disruptions and test if the blocked object is not used
        by the journey.
        Then we delete it and test if use the blocked object
        """
        response = self.query_region(journey_basic_query)

        assert "journeys" in response

        self.run_check('A', 'line')
        self.run_check('A:0', 'route')
        self.run_check('base_network', 'network')

    def test_blocking_disruption_of_stop_area_and_stop_point_on_journey(self):
        """
        We send blocking disruptions on stop area and stop point.

        Then we delete it and test if use the blocked object
        """
        response = self.query_region(journey_basic_query)

        assert "journeys" in response

        self.run_check('stopA', 'stop_area')
        self.run_check('stop_point:stopA', 'stop_point')


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsBlockingOverlapping(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """
    def test_disruption_on_journey(self):
        """
        We block the network and the line A, we test if there's no public_transport
        journey.
        We delete the network disruption and test if there's a journey using
        the line B.
        """
        response = self.query_region(journey_basic_query)

        assert "journeys" in response
        disruptions = self.get_disruptions(response)
        #no disruptions on the journey for the moment
        assert not disruptions

        # some disruption are loaded on the dataset though
        nb_pre_loaded_disruption = len(get_not_null(self.query_region('disruptions'), 'disruptions'))
        assert nb_pre_loaded_disruption == 9

        self.send_mock("blocking_line_disruption", "A",
                       "line", blocking=True)
        self.send_mock("blocking_network_disruption",
                       "base_network", "network", blocking=True)

        # Test disruption API
        response = self.query_region('disruptions')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) - nb_pre_loaded_disruption == 2
        for d in disruptions:
            is_valid_disruption(d)
        assert {"blocking_line_disruption", "blocking_network_disruption"}.\
            issubset(set([d['disruption_uri'] for d in disruptions]))

        response = self.query_region(journey_basic_query + "&disruption_active=true")

        assert all(map(lambda j: len([s for s in j["sections"] if s["type"] == "public_transport"]) == 0,
                       response["journeys"]))

        # we should then not have disruptions (since we don't get any journey)
        disruptions = self.get_disruptions(response)
        assert not disruptions

        # we then query for the same journey but without disruptions,
        # so we'll have a journey (but the disruptions will be displayed
        response = self.query_region(journey_basic_query + "&disruption_active=false")
        disruptions = self.get_disruptions(response)
        assert disruptions
        eq_(len(disruptions), 2)
        assert 'blocking_line_disruption' in disruptions
        assert 'blocking_network_disruption' in disruptions

        self.send_mock("blocking_network_disruption", "base_network", "network", blocking=True, is_deleted=True)
        response = self.query_region(journey_basic_query+ "&disruption_active=true")
        links = []
        def get_line_id(k, v):
            if k != "links" or not "type" in v or not "id" in v or v["type"] != "line":
                return
            links.append(v["id"])
        utils.walk_dict(response, get_line_id)
        assert all(map(lambda id_: id_ != "A", links))

        #we also query without disruption and obviously we should not have the network disruptions
        response = self.query_region(journey_basic_query + "&disruption_active=false")
        assert 'journeys' in response
        disruptions = self.get_disruptions(response)
        assert disruptions
        eq_(len(disruptions), 1)
        assert 'blocking_line_disruption' in disruptions
        #we also check that the journey is tagged as disrupted
        assert any(map(lambda j: j.get('status') == 'NO_SERVICE', response['journeys']))

    def get_disruptions(self, response):
        """
        return a map with the disruption id as key and the list of disruption + impacted object as value
        """
        disruption_by_obj = defaultdict(list)

        all_disruptions = {d['id']: d for d in response['disruptions']}

        def disruptions_filler(_, obj):
            try:
                if 'links' not in obj:
                    return
            except TypeError:
                return

            real_disruptions = [all_disruptions[d['id']] for d in obj['links'] if d['type'] == 'disruption']

            for d in real_disruptions:
                disruption_by_obj[d['disruption_id']].append((d, obj))

        utils.walk_dict(response, disruptions_filler)

        return disruption_by_obj


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsUpdate(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """
    def test_disruption(self):
        """
        test /disruptions and check that an update of a disruption is correctly done
        """
        query = 'traffic_reports?_current_datetime=20120801T000000'
        response = self.query_region(query)

        disrupt = get_disruptions(response['traffic_reports'][0]['network'], response)
        assert disrupt
        eq_(len(disrupt), 2)

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        #we create a disruption on the network
        self.send_mock("test_disruption", "base_network", "network", message='message')

        response = self.query_region(query)

        disrupt = get_disruptions(response['traffic_reports'][0]['network'], response)
        assert disrupt
        eq_(len(disrupt), 3)

        for disruption in disrupt:
            if disruption['id'] == 'test_disruption':
                eq_(disruption['messages'][0]['text'], 'message')

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        #we update the previous disruption
        self.send_mock("test_disruption", "base_network", "network", message='update')

        response = self.query_region(query)

        disrupt = get_disruptions(response['traffic_reports'][0]['network'], response)
        assert disrupt
        eq_(len(disrupt), 3)
        for disruption in disrupt:
            if disruption['id'] == 'test_disruption':
                eq_(disruption['messages'][0]['text'], 'update')

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        #we delete the disruption
        self.send_mock("test_disruption", "base_network", "network", is_deleted=True)

        response = self.query_region(query)

        disrupt = get_disruptions(response['traffic_reports'][0]['network'], response)
        assert disrupt
        eq_(len(disrupt), 2)
        for disruption in disrupt:
            assert disruption['uri'] != 'test_disruption', 'this disruption must have been deleted'


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsStopPoint(ChaosDisruptionsFixture):
    """
    Add disruption on stop point:
        test if the information is raised in response
        test if stop point is blocked
    Then delete disruption:
        test if there is no more disruptions in response
    """
    def test_disruption_on_stop_point(self):
        disruption_id = 'test_disruption_on_stop_pointA'
        disruption_msg = 'stop_point_disruption'
        disruption_target = 'stop_point:stopA'
        disruption_target_type = 'stop_point'
        query = 'stop_points/stop_point:stopA'

        # add disruption on stop point
        self.send_mock(disruption_id,
                       disruption_target,
                       disruption_target_type,
                       blocking=True,
                       message=disruption_msg)
        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert disruptions
        assert len(disruptions) == 1
        assert disruptions[0]['disruption_id'] == disruption_id

        journey_query_adapted = journey_basic_query + "&data_freshness=adapted_schedule"

        response_adapted = self.query_region(journey_query_adapted)

        # As the public transport is blocked by the disruption, we should find only 1 journey here
        assert len(response_adapted['journeys']) == 1
        assert response_adapted['journeys'][0]['type'] == 'non_pt_walk'

        # delete disruption on stop point
        self.send_mock(disruption_id,
                       disruption_target,
                       disruption_target_type,
                       is_deleted=True)

        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert not disruptions

        journey_query = journey_basic_query + "&data_freshness=base_schedule"
        response = self.query_region(journey_query)
        is_valid_journey_response(response, self.tester, journey_query)
        assert len(response['journeys']) == 2

        journey_query = journey_basic_query + "&data_freshness=adapted_schedule"
        response = self.query_region(journey_query)
        is_valid_journey_response(response, self.tester, journey_query)
        assert len(response['journeys']) == 2


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsStopArea(ChaosDisruptionsFixture):
    """
    Add disruption on stop area:
        test if the information is raised in response
        test if stop area is blocked
    Then delete disruption:
        test if there is no more disruptions in response
    """
    def test_disruption_on_stop_point(self):
        disruption_id = 'test_disruption_on_stop_areaA'
        disruption_msg = 'stop_area_disruption'
        disruption_target = 'stopA'
        disruption_target_type = 'stop_area'
        query = 'stop_areas/stopA'

        # add disruption on stop point
        self.send_mock(disruption_id,
                       disruption_target,
                       disruption_target_type,
                       blocking=True,
                       message=disruption_msg)
        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert disruptions
        assert len(disruptions) == 1
        assert disruptions[0]['disruption_id'] == disruption_id

        journey_query_adapted = journey_basic_query + "&data_freshness=adapted_schedule"

        response_adapted = self.query_region(journey_query_adapted)

        # No public transport journey is found, the only way it by foot
        assert len(response_adapted['journeys']) == 1
        assert response_adapted['journeys'][0]['type'] == 'non_pt_walk'

        # delete disruption on stop point
        self.send_mock(disruption_id,
                       disruption_target,
                       disruption_target_type,
                       is_deleted=True)

        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert not disruptions

        journey_query = journey_basic_query + "&data_freshness=base_schedule"
        response = self.query_region(journey_query)
        is_valid_journey_response(response, self.tester, journey_query)
        assert len(response['journeys']) == 2

        journey_query = journey_basic_query + "&data_freshness=adapted_schedule"
        response = self.query_region(journey_query)
        is_valid_journey_response(response, self.tester, journey_query)
        assert len(response['journeys']) == 2




def make_mock_chaos_item(disruption_name, impacted_obj, impacted_obj_type, start, end,
                         message_text='default_message', is_deleted=False, blocking=False,
                         start_period="20100412T165200", end_period="20200412T165200"):
    feed_message = gtfs_realtime_pb2.FeedMessage()
    feed_message.header.gtfs_realtime_version = '1.0'
    feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
    feed_message.header.timestamp = 0

    feed_entity = feed_message.entity.add()
    feed_entity.id = disruption_name
    feed_entity.is_deleted = is_deleted

    disruption = feed_entity.Extensions[chaos_pb2.disruption]

    disruption.id = disruption_name
    disruption.cause.id = "CauseTest"
    disruption.cause.wording = "CauseTest"
    disruption.reference = "DisruptionTest"
    disruption.publication_period.start = utils.str_to_time_stamp(start_period)
    disruption.publication_period.end = utils.str_to_time_stamp(end_period)

    # Tag
    tag = disruption.tags.add()
    tag.name = "rer"
    tag.id = "rer"
    tag = disruption.tags.add()
    tag.name = "metro"
    tag.id = "rer"

    if not impacted_obj or not impacted_obj_type:
        return feed_message.SerializeToString()

    # Impacts
    impact = disruption.impacts.add()
    impact.id = "impact_" + disruption_name + "_1"
    enums_impact = gtfs_realtime_pb2.Alert.DESCRIPTOR.enum_values_by_name
    if blocking:
        impact.severity.effect = enums_impact["NO_SERVICE"].number
        impact.severity.id = 'blocking'
        impact.severity.priority = 10
        impact.severity.wording = "blocking"
        impact.severity.color = "#FFFF00"
    else:
        impact.severity.effect = enums_impact["UNKNOWN_EFFECT"].number
        impact.severity.id = ' not blocking'
        impact.severity.priority = 1
        impact.severity.wording = "not blocking"
        impact.severity.color = "#FFFFF0"

    # ApplicationPeriods
    application_period = impact.application_periods.add()
    application_period.start = utils.str_to_time_stamp(start_period)
    application_period.end = utils.str_to_time_stamp(end_period)

    # PTobject
    type_col = {
        "network": chaos_pb2.PtObject.network,
        "stop_area": chaos_pb2.PtObject.stop_area,
        "line": chaos_pb2.PtObject.line,
        "line_section": chaos_pb2.PtObject.line_section,
        "route": chaos_pb2.PtObject.route,
        "stop_point": chaos_pb2.PtObject.stop_point
    }

    ptobject = impact.informed_entities.add()
    ptobject.uri = impacted_obj
    ptobject.pt_object_type = type_col.get(impacted_obj_type, chaos_pb2.PtObject.unkown_type)
    if ptobject.pt_object_type == chaos_pb2.PtObject.line_section:
        line_section = ptobject.pt_line_section
        line_section.line.uri = impacted_obj
        line_section.line.pt_object_type = chaos_pb2.PtObject.line
        pb_start = line_section.start_point
        pb_start.uri = start
        pb_start.pt_object_type = chaos_pb2.PtObject.stop_area
        pb_end = line_section.end_point
        pb_end.uri = end
        pb_end.pt_object_type = chaos_pb2.PtObject.stop_area

    # Message with one channel and one channel type: sms
    message = impact.messages.add()
    message.text = message_text
    message.channel.id = "sms"
    message.channel.name = "sms"
    message.channel.max_size = 60
    message.channel.content_type = "text"
    message.channel.types.append(chaos_pb2.Channel.sms)

    # Message with one channel and two channel types: web and email
    message = impact.messages.add()
    message.text = message_text
    message.channel.name = "email"
    message.channel.id = "email"
    message.channel.max_size = 250
    message.channel.content_type = "html"
    message.channel.types.append(chaos_pb2.Channel.web)
    message.channel.types.append(chaos_pb2.Channel.email)

    return feed_message.SerializeToString()
