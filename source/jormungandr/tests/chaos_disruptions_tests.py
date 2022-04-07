# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from . import gtfs_realtime_pb2
from .tests_mechanism import dataset
from .check_utils import *
from . import chaos_pb2
from jormungandr import utils
from .rabbitmq_utils import RabbitMQCnxFixture, rt_topic


class ChaosDisruptionsFixture(RabbitMQCnxFixture):
    """
    Mock a chaos disruption message, in order to check the api
    """

    def _make_mock_item(
        self,
        disruption_name,
        impacted_obj,
        impacted_obj_type,
        start=None,
        end=None,
        message='default_message',
        is_deleted=False,
        blocking=False,
        start_period="20100412T165200",
        end_period="20300412T165200",
        cause='CauseTest',
        category=None,
        routes=None,
        properties=None,
        tags=None,
    ):
        return make_mock_chaos_item(
            disruption_name,
            impacted_obj,
            impacted_obj_type,
            start,
            end,
            message,
            is_deleted,
            blocking,
            start_period,
            end_period,
            cause=cause,
            category=category,
            routes=routes,
            properties=properties,
            tags=tags,
        )


MAIN_ROUTING_TEST_SETTING = {
    "main_routing_test": {
        'kraken_args': ['--BROKER.sleeptime=0', '--BROKER.rt_topics=' + rt_topic, 'spawn_maintenance_worker']
    }
}


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
        # at first no disruption
        assert 'disruptions' not in stop

        properties = [{'key': 'foo', 'type': 'bar', 'value': '42'}]
        tags = [{'id': 're', 'name': 'rer'}, {'id': 'met', 'name': 'metro'}]
        self.send_mock("bob_the_disruption", "stopB", "stop_area", properties=properties, tags=tags)

        # and we call again, we must have the disruption now
        response = self.query_region('stop_areas/stopB?_current_datetime=20160314T104600')
        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        disruptions = get_disruptions(stop, response)

        # at first we got only one disruption on B
        assert len(disruptions) == 1

        assert any(d['disruption_id'] == 'bob_the_disruption' for d in disruptions)
        assert disruptions[0]['cause'] == 'CauseTest'
        assert 'category' not in disruptions[0]
        assert len(disruptions[0]['tags']) == 2
        assert 'metro' in disruptions[0]['tags']
        assert 'rer' in disruptions[0]['tags']

        # here we test messages in disruption: message, channel and types
        messages = get_not_null(disruptions[0], 'messages')
        assert len(messages) == 3
        assert messages[0]['text'] == 'default_message'
        channel = get_not_null(messages[0], 'channel')
        assert channel['id'] == 'sms'
        assert channel['name'] == 'sms'
        assert channel['content_type'] == 'text'
        assert len(channel['types']) == 1
        assert channel['types'][0] == 'sms'

        assert messages[1]['text'] == 'default_message'
        channel = get_not_null(messages[1], 'channel')
        assert channel['id'] == 'email'
        assert channel['name'] == 'email'
        assert channel['content_type'] == 'html'
        assert len(channel['types']) == 2
        assert channel['types'][0] == 'web'
        assert channel['types'][1] == 'email'

        assert messages[2]['text'] == 'default_message'
        channel = get_not_null(messages[2], 'channel')
        assert channel['id'] == 'beacon'
        assert channel['name'] == 'beacon'
        assert channel['content_type'] == 'text'
        assert len(channel['types']) == 4
        assert channel['types'][0] == 'web'
        assert channel['types'][1] == 'mobile'
        assert channel['types'][2] == 'title'
        assert channel['types'][3] == 'beacon'

        properties = get_not_null(disruptions[0], 'properties')
        assert len(properties) == 1
        assert properties[0]['key'] == 'foo'
        assert properties[0]['type'] == 'bar'
        assert properties[0]['value'] == '42'

    def test_current_datetime_out_of_bounds(self):
        """
        current_datetime out of bounds
        publication date of disruption          20100412T165200                        20300412T165200
        current_datetime    20080412T165200
        """
        response = self.query_region('stop_areas/stopB')

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]
        # at first no disruption
        assert 'disruptions' not in stop

        self.send_mock("bob_the_disruption", "stopB", "stop_area")

        response = self.query_region('stop_areas/stopB?_current_datetime=20080412T165200')
        assert len(response['disruptions']) == 0


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestPlacesWithDisruptions(ChaosDisruptionsFixture):
    def test_places_with_disruptions(self):
        """
        /places?q=stopB
        """
        response = self.query_region('places?q=stopB')
        assert len(response['disruptions']) == 0

        self.send_mock("bob_the_disruption", "stopB", "stop_area")

        response = self.query_region('places?q=stopB')
        assert len(response['places']) == 1
        assert response['places'][0]['id'] == 'stopB'

        # we shouldn't have any disruption in api /places
        assert len(response['disruptions']) == 0


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsLineSection(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """

    def test_disruption_on_line_section(self):
        """
        the test blocking a line section
        we check that before we have not our disruption and we have it after sending it
        TODO add more checks when all the line section features will be implemented
        """

        def query_on_date(q, **kwargs):
            """We do the query on a dt inside the disruption application period"""
            return self.query_region('{q}?_current_datetime=20120614T120000'.format(q=q), **kwargs)

        def has_dis(obj_get, dis):
            r = query_on_date('{col}/{uri}'.format(col=obj_get.collection, uri=obj_get.uri))
            return has_disruption(r, obj_get, dis)

        response = self.query_region('disruptions')
        assert 'bobette_the_disruption' not in [d['disruption_id'] for d in response['disruptions']]
        assert not has_dis(ObjGetter('stop_points', 'stop_point:stopA'), 'bobette_the_disruption')
        assert not has_dis(ObjGetter('vehicle_journeys', 'vehicle_journey:vjA'), 'bobette_the_disruption')

        self.send_mock(
            "bobette_the_disruption",
            "A",
            "line_section",
            start="stopB",
            end="stopA",
            start_period="20120614T100000",
            end_period="20120624T170000",
            blocking=True,
        )

        response = self.query_region('disruptions')
        # and we call again, we must have the disruption now
        dis_by_name = {d['disruption_id']: d for d in response['disruptions']}

        assert 'bobette_the_disruption' in dis_by_name
        is_valid_line_section_disruption(dis_by_name['bobette_the_disruption'])

        # the disruption is linked to the trips of the line and to the stoppoints
        assert has_dis(ObjGetter('stop_points', 'stop_point:stopA'), 'bobette_the_disruption')
        assert has_dis(ObjGetter('vehicle_journeys', 'vehicle_journey:vjA'), 'bobette_the_disruption')


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsLineSectionOnOneStop(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order not the get previous disruptions
    """

    def test_disruption_on_line_section_on_one_stop(self):
        """
        the test blocking a line section
        the line section is only from B to B, it should block only B
        """

        def query_on_date(q, **kwargs):
            """We do the query on a dt inside the disruption application period"""
            return self.query_region('{q}?_current_datetime=20120614T120000'.format(q=q), **kwargs)

        def has_dis(obj_get, dis):
            r = query_on_date('{col}/{uri}'.format(col=obj_get.collection, uri=obj_get.uri))
            return has_disruption(r, obj_get, dis)

        response = self.query_region('disruptions')
        assert 'bobette_the_disruption' not in [d['disruption_id'] for d in response['disruptions']]
        assert not has_dis(ObjGetter('stop_points', 'stop_point:stopA'), 'bobette_the_disruption')
        assert not has_dis(ObjGetter('stop_points', 'stop_point:stopB'), 'bobette_the_disruption')
        assert not has_dis(ObjGetter('vehicle_journeys', 'vehicle_journey:vjA'), 'bobette_the_disruption')

        self.send_mock(
            "bobette_the_disruption",
            "A",
            "line_section",
            start="stopB",
            end="stopB",
            start_period="20120614T100000",
            end_period="20120624T170000",
            blocking=True,
        )

        response = self.query_region('disruptions')
        # and we call again, we must have the disruption now
        dis_by_name = {d['disruption_id']: d for d in response['disruptions']}

        bobette = dis_by_name.get('bobette_the_disruption')
        assert bobette
        is_valid_line_section_disruption(bobette)

        section = get_not_null(bobette, 'impacted_objects')[0].get('impacted_section', {})
        assert section.get('from', {}).get('id') == 'stopB'
        assert section.get('to', {}).get('id') == 'stopB'
        assert 'routes' in section
        assert len(section['routes']) == 2

        # the disruption is linked to the trips of the line and to the stoppoints
        assert not has_dis(ObjGetter('stop_points', 'stop_point:stopA'), 'bobette_the_disruption')
        assert has_dis(ObjGetter('stop_points', 'stop_point:stopB'), 'bobette_the_disruption')
        assert has_dis(ObjGetter('vehicle_journeys', 'vehicle_journey:vjA'), 'bobette_the_disruption')


@dataset(MAIN_ROUTING_TEST_SETTING)
class TestChaosDisruptionsLineSectionOnOneStopWithRoute(ChaosDisruptionsFixture):
    """
    Note: it is done as a new fixture, to spawn a new kraken, in order to not get the previous disruptions
    """

    def test_disruption_on_line_section_on_one_stop_with_route(self):
        """
        the test blocking a line section
        the line section is only from B to B, it should block only B
        """

        def query_on_date(q, **kwargs):
            """We do the query on a dt inside the disruption application period"""
            return self.query_region('{q}?_current_datetime=20120614T120000'.format(q=q), **kwargs)

        def has_dis(obj_get, dis):
            r = query_on_date('{col}/{uri}'.format(col=obj_get.collection, uri=obj_get.uri))
            return has_disruption(r, obj_get, dis)

        response = self.query_region('disruptions')
        assert 'bobette_the_disruption' not in [d['disruption_id'] for d in response['disruptions']]
        assert not has_dis(ObjGetter('stop_points', 'stop_point:stopA'), 'bobette_the_disruption')
        assert not has_dis(ObjGetter('stop_points', 'stop_point:stopB'), 'bobette_the_disruption')
        assert not has_dis(ObjGetter('vehicle_journeys', 'vehicle_journey:vjA'), 'bobette_the_disruption')

        self.send_mock(
            "bobette_the_disruption_with_route",
            "A",
            "line_section",
            start="stopB",
            end="stopB",
            start_period="20120614T100000",
            end_period="20120624T170000",
            blocking=True,
            routes=['A:0'],
        )

        response = self.query_region('disruptions')
        # and we call again, we must have the disruption now
        dis_by_name = {d['disruption_id']: d for d in response['disruptions']}

        bobette = dis_by_name.get('bobette_the_disruption_with_route')
        assert bobette
        is_valid_line_section_disruption(bobette)

        section = get_not_null(bobette, 'impacted_objects')[0].get('impacted_section', {})
        assert section.get('from', {}).get('id') == 'stopB'
        assert section.get('to', {}).get('id') == 'stopB'
        assert 'routes' in section
        assert len(section['routes']) == 1
        assert section['routes'][0].get('id') == 'A:0'

        # the disruption is linked to the trips of the line and to the stoppoints
        assert not has_dis(ObjGetter('stop_points', 'stop_point:stopA'), 'bobette_the_disruption_with_route')
        assert has_dis(ObjGetter('stop_points', 'stop_point:stopB'), 'bobette_the_disruption_with_route')
        assert has_dis(ObjGetter('vehicle_journeys', 'vehicle_journey:vjA'), 'bobette_the_disruption_with_route')


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

        stops_b_to = [
            s['to']['stop_point']
            for j in response['journeys']
            for s in j['sections']
            if s['to']['embedded_type'] == 'stop_point' and s['to']['id'] == 'stop_point:stopB'
        ]

        assert stops_b_to

        for b in stops_b_to:
            assert not get_disruptions(b['stop_area'], response)

        # We should not have a bypass_disruption link
        # in case there is no disruption
        links = get_links_dict(response)
        assert 'bypass_disruptions' not in links

        # we create a list with every 'to' section to the stop B (the one we added the disruption on)
        self.send_mock("bob_the_disruption", "stopB", "stop_area")
        query = journey_basic_query + '&_current_datetime=20160314T144100'
        response = self.query_region(query)

        # the response must be still valid (this tests the kraken data reloading)
        self.is_valid_journey_response(response, query)

        stops_b_to = [
            s['to']['stop_point']
            for j in response['journeys']
            for s in j['sections']
            if s['to']['embedded_type'] == 'stop_point' and s['to']['id'] == 'stop_point:stopB'
        ]

        assert stops_b_to

        for b in stops_b_to:
            disruptions = get_disruptions(b['stop_area'], response)

            # at first we got only one disruption on A
            assert len(disruptions) == 1

            assert any(d['disruption_id'] == 'bob_the_disruption' for d in disruptions)

        # We should have a bypass_disruption link
        # in case there is disruptions but without NO_SERVICE severity
        links = get_links_dict(response)
        assert 'bypass_disruptions' not in links

    def test_journey_with_current_datetime_out_of_bounds(self):
        """
        current_datetime out of bounds
        publication date of disruption          20100412T165200                        20300412T165200
        current_datetime    20090314T144100
        """
        query = journey_basic_query + '&_current_datetime=20090314T144100'
        response = self.query_region(query)

        # the response must be still valid (this tests the kraken data reloading)
        self.is_valid_journey_response(response, query)

        stops_b_to = [
            s['to']['stop_point']
            for j in response['journeys']
            for s in j['sections']
            if s['to']['embedded_type'] == 'stop_point' and s['to']['id'] == 'stop_point:stopB'
        ]

        assert stops_b_to

        assert len(response['disruptions']) == 0

    def test_disruption_on_journey_display_informations(self):
        """
        same kind of test with a call on journeys
        we add one disruption on stop_area of the section and we should get it in display_informations.links[]
        we add one on line and we should also get it in display_informations.links[]
        we add one disruption on stop_point of the section and we should get it in display_informations.links[]
        """

        # we already have created a disruption on stopA (stop_area stopB)
        query = journey_basic_query + '&_current_datetime=20160314T144100'
        response = self.query_region(query)

        # the response must be still valid (this tests the kraken data reloading)
        self.is_valid_journey_response(response, query)

        # we search for the disruption on stop_area in section.display_informations.links[]
        display_infos = [
            s['display_informations']
            for j in response['journeys']
            for s in j['sections']
            if s['type'] == 'public_transport'
        ]
        links = [link for di in display_infos for link in di['links'] if link['type'] == 'disruption']
        assert len(links) == 1
        assert links[0]['id'] == 'impact_bob_the_disruption_1'

        links = [link for di in display_infos for link in di['links'] if link['type'] == 'stop_area']
        assert len(links) == 1
        assert links[0]["id"] == "stopA"

        assert len(response["terminus"]) == 1
        assert response["terminus"][0]["id"] == "stopA"

        # we create a disruption on line
        self.send_mock("bob_the_disruption_on_line", "A", "line")
        query = journey_basic_query + '&_current_datetime=20160314T144100'
        response = self.query_region(query)

        # the response must be still valid (this tests the kraken data reloading)
        self.is_valid_journey_response(response, query)

        # we search for the disruption on stop_area and line in section.display_informations.links[]
        display_infos = [
            s['display_informations']
            for j in response['journeys']
            for s in j['sections']
            if s['type'] == 'public_transport'
        ]
        links = [link for di in display_infos for link in di['links'] if link['type'] == 'disruption']
        assert len(links) == 2
        assert any(link['id'] == 'impact_bob_the_disruption_1' for link in links)
        assert any(link['id'] == 'impact_bob_the_disruption_on_line_1' for link in links)

        # we create a disruption on a stop_point A
        self.send_mock("bob_the_disruption_on_stop_point", "stop_point:stopA", "stop_point")
        query = journey_basic_query + '&_current_datetime=20160314T144100'
        response = self.query_region(query)

        # the response must be still valid (this tests the kraken data reloading)
        self.is_valid_journey_response(response, query)

        # we search for the disruption on stop_area, stop_point and line in section.disdisplay_informations.links[]
        display_infos = [
            s['display_informations']
            for j in response['journeys']
            for s in j['sections']
            if s['type'] == 'public_transport'
        ]
        links = [link for di in display_infos for link in di['links'] if link['type'] == 'disruption']
        assert len(links) == 3
        assert any(link['id'] == 'impact_bob_the_disruption_1' for link in links)
        assert any(link['id'] == 'impact_bob_the_disruption_on_line_1' for link in links)
        assert any(link['id'] == 'impact_bob_the_disruption_on_stop_point_1' for link in links)

        links = [link for di in display_infos for link in di['links'] if link['type'] == 'stop_area']
        assert len(links) == 1
        assert links[0]["id"] == "stopA"

        assert len(response["terminus"]) == 1
        assert response["terminus"][0]["id"] == "stopA"

    def test_disruption_on_journey_with_blocking_disruption(self):
        """
        same kind of test with a call on journeys

        at first no disruptions, we add one with NO_SERVICE severity
        and we should get it
        """
        # we create a list with every 'to' section to the stop B (the one we added the disruption on)
        self.send_mock("bob_the_disruption", "stopB", "stop_area", blocking=True)
        query = journey_basic_query + '&_current_datetime=20160314T144100'
        response = self.query_region(query)

        self.is_valid_journey_response(response, query)

        # We should have a bypass_disruption link
        # in case there is disruptions with NO_SERVICE severity
        links = get_links_dict(response)
        assert 'bypass_disruptions' in links
        assert '&data_freshness=realtime' in links['bypass_disruptions']['href']

        # We call journey with realtime data_freshness
        # We should not receive bypass_disruptions link
        # even if we have a disruption with NO_SERVICE severity
        query = query + "&data_freshness=realtime"
        response = self.query_region(query)

        self.is_valid_journey_response(response, query)
        links = get_links_dict(response)
        assert 'bypass_disruptions' not in links


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
        journey_query_2_to_format = "journeys?from={from_coord}&to={to_coord}&data_freshness=realtime".format(
            from_coord=s_coord, to_coord=r_coord
        )
        journey_query_2_to_format += "&datetime={datetime}"
        start_period = "20120615T080000"
        disruption_uri = "blocking_{}_disruption".format(type_)

        def get_type_id(k, v):
            if (k != "links" or "type" not in v or "id" not in v or v["type"] != type_) and (
                k != type_ or "id" not in v
            ):
                return
            links.append(v["id"])

        def check_links_(date, check_existence):
            links[:] = []
            journey_query = journey_query_2_to_format.format(datetime=date)
            response_json = self.query_region(journey_query)

            utils.walk_dict(response_json, get_type_id)
            if check_existence:
                assert any([id_ == object_id for id_ in links])
            else:
                assert all([id_ != object_id for id_ in links])

        nb_disruptions_map = self.get_nb_disruptions()
        nb_disruptions = nb_disruptions_map[object_id]
        # We send a blocking disruption on the object
        self.send_mock(disruption_uri, object_id, type_, blocking=True, start_period=start_period)
        nb_disruptions_map = self.get_nb_disruptions()
        assert (nb_disruptions_map[object_id] - nb_disruptions) == 1

        check_links_("20120616T080000", False)
        # We test out of the period
        check_links_("20120614T080000", True)

        # We delete the disruption
        nb_disruptions_map = self.get_nb_disruptions()
        nb_disruptions = nb_disruptions_map[object_id]
        self.send_mock(
            disruption_uri, object_id, type_, blocking=True, is_deleted=True, start_period=start_period
        )
        nb_disruptions_map = self.get_nb_disruptions()
        assert (nb_disruptions - nb_disruptions_map[object_id]) == 1
        check_links_("20120616T080000", True)

        # We try to send first the disruption, and then the impacts
        nb_disruptions = nb_disruptions_map[object_id]
        self.send_mock(disruption_uri, None, None, blocking=True, start_period=start_period)
        self.send_mock(disruption_uri, object_id, type_, blocking=True, start_period=start_period)
        nb_disruptions_map = self.get_nb_disruptions()
        assert (nb_disruptions_map[object_id] - nb_disruptions) == 1

        check_links_("20120616T080000", False)
        # We test out of the period
        check_links_("20120614T080000", True)

        # We clean
        self.send_mock(
            disruption_uri, object_id, type_, blocking=True, is_deleted=True, start_period=start_period
        )

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

        def contains_pt_journey(resp):
            return any(
                [
                    len([s for s in j["sections"] if s["type"] == "public_transport"]) == 1
                    for j in resp["journeys"]
                ]
            )

        def get_pt_sections(resp):
            return [s for j in response['journeys'] for s in j['sections'] if s['type'] == 'public_transport']

        response = self.query_region(journey_basic_query)
        assert "journeys" in response
        disruptions = self.get_disruptions(response)
        # no disruptions on the journey for the moment
        assert not disruptions

        # Check which line is used in PT journey
        pt_sections = get_pt_sections(response)
        assert len(pt_sections) == 1
        assert pt_sections[0]['departure_date_time'] == '20120614T080100'
        assert pt_sections[0]['arrival_date_time'] == '20120614T080102'
        line_id = next(o['id'] for o in pt_sections[0]['links'] if o['type'] == 'line')
        assert line_id == 'A'

        # some disruptions are loaded on the dataset though
        nb_pre_loaded_disruption = len(get_not_null(self.query_region('disruptions'), 'disruptions'))
        assert nb_pre_loaded_disruption == 12

        # Check that there is one non-blocking disruption on some lines existing in the dataset:
        response = self.query_region(
            journey_basic_query + "&_current_datetime=20120614T080000&data_freshness=base_schedule"
        )
        assert len(response['disruptions']) == 1
        assert response['disruptions'][0]['severity']['effect'] == 'UNKNOWN_EFFECT'
        impacted_lines = [
            obj['pt_object']['id']
            for obj in response['disruptions'][0]['impacted_objects']
            if obj['pt_object']['embedded_type'] == 'line'
        ]
        assert 'A' in impacted_lines
        assert 'B' in impacted_lines
        assert 'C' in impacted_lines

        # Step 1 : Send an impact on the line 'A' with end date same as PT departure datetime
        # Information on PT section: departure = 20120614T080100 / arrival = 20120614T080102
        # Proposed application period = (20120614T080000, 20120614T080100)
        self.send_mock(
            "blocking_line_disruption_ending_at_pt_departure",
            "A",
            "line",
            blocking=True,
            start_period="20120614T080000",
            end_period="20120614T080100",
        )

        # Check disruptions
        response = self.query_region('disruptions')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) - nb_pre_loaded_disruption == 1
        for d in disruptions:
            is_valid_disruption(d)
        assert "blocking_line_disruption_ending_at_pt_departure" in [d['disruption_uri'] for d in disruptions]
        nb_pre_loaded_disruption = len(disruptions)

        # Check that the journey with PT exists and line 'A' is used in PT
        response = self.query_region(
            journey_basic_query + "&_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert contains_pt_journey(response) is True
        pt_sections = get_pt_sections(response)
        assert len(pt_sections) == 1
        assert pt_sections[0]['departure_date_time'] == '20120614T080100'
        assert pt_sections[0]['arrival_date_time'] == '20120614T080102'
        line_id = next(o['id'] for o in pt_sections[0]['links'] if o['type'] == 'line')
        assert line_id == 'A'

        # One disruption on line 'A' with severity 'UNKNOWN_EFFECT' exists (ignored disruption)
        assert len(response['disruptions']) == 1
        assert response['disruptions'][0]['severity']['effect'] == 'UNKNOWN_EFFECT'
        impacted_lines = [
            obj['pt_object']['id']
            for obj in response['disruptions'][0]['impacted_objects']
            if obj['pt_object']['embedded_type'] == 'line'
        ]
        assert 'A' in impacted_lines

        # Step 2 : Send an impact on the line 'A' with start date same as PT arrival datetime
        # Proposed application period = (20120614T080102, 20120614T090000)
        self.send_mock(
            "blocking_line_disruption_starting_from_pt_arrival",
            "A",
            "line",
            blocking=True,
            start_period="20120614T080102",
            end_period="20120614T090000",
        )
        # Check disruptions
        response = self.query_region('disruptions')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) - nb_pre_loaded_disruption == 1
        for d in disruptions:
            is_valid_disruption(d)
        assert "blocking_line_disruption_starting_from_pt_arrival" in [d['disruption_uri'] for d in disruptions]
        nb_pre_loaded_disruption = len(disruptions)

        # Check that the journey with PT exists and line 'A' is used in PT
        response = self.query_region(
            journey_basic_query + "&_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert contains_pt_journey(response) is True
        pt_sections = get_pt_sections(response)
        assert len(pt_sections) == 1
        assert pt_sections[0]['departure_date_time'] == '20120614T080100'
        assert pt_sections[0]['arrival_date_time'] == '20120614T080102'
        line_id = next(o['id'] for o in pt_sections[0]['links'] if o['type'] == 'line')
        assert line_id == 'A'

        # One disruption on line 'A' with severity 'UNKNOWN_EFFECT' exists
        assert len(response['disruptions']) == 1
        assert response['disruptions'][0]['severity']['effect'] == 'UNKNOWN_EFFECT'
        impacted_lines = [
            obj['pt_object']['id']
            for obj in response['disruptions'][0]['impacted_objects']
            if obj['pt_object']['embedded_type'] == 'line'
        ]
        assert 'A' in impacted_lines

        # Step 3 : Send an impact on the line 'A' with start date 1 second before PT arrival datetime
        # Proposed application period = (20120614T080101, 20120614T090000)
        self.send_mock(
            "blocking_line_disruption_starting_1s_before_pt_arrival",
            "A",
            "line",
            blocking=True,
            start_period="20120614T080101",
            end_period="20120614T090000",
        )
        # Check disruptions
        response = self.query_region('disruptions')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) - nb_pre_loaded_disruption == 1
        for d in disruptions:
            is_valid_disruption(d)
        assert "blocking_line_disruption_starting_1s_before_pt_arrival" in [
            d['disruption_uri'] for d in disruptions
        ]
        nb_pre_loaded_disruption = len(disruptions)

        # Check that line 'A' is not used any more in the journey with PT instead it's line 'M'
        response = self.query_region(
            journey_basic_query + "&_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert contains_pt_journey(response) is True
        pt_sections = get_pt_sections(response)
        assert len(pt_sections) == 1
        assert pt_sections[0]['departure_date_time'] == '20120614T080101'
        assert pt_sections[0]['arrival_date_time'] == '20120614T080103'
        line_id = next(o['id'] for o in pt_sections[0]['links'] if o['type'] == 'line')
        assert line_id == 'M'
        # No disruption on line 'M'
        assert len(response['disruptions']) == 0

        # Step 4 : Send an impact on the line 'M' with end date 1 second after PT departure datetime
        # Information on PT section: departure = 20120614T080101 / arrival = 20120614T080103
        # Proposed application period = (20120614T080000, 20120614T080102)
        self.send_mock(
            "blocking_line_disruption_ending_1s_after_pt_departure",
            "M",
            "line",
            blocking=True,
            start_period="20120614T080000",
            end_period="20120614T080102",
        )
        # Check disruptions
        response = self.query_region('disruptions')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) - nb_pre_loaded_disruption == 1
        for d in disruptions:
            is_valid_disruption(d)
        assert "blocking_line_disruption_ending_1s_after_pt_departure" in [
            d['disruption_uri'] for d in disruptions
        ]
        nb_pre_loaded_disruption = len(disruptions)

        # Check that line 'M' is not used any more in the journey with PT instead it's line 'B'
        response = self.query_region(
            journey_basic_query + "&_current_datetime=20120614T080000&data_freshness=realtime"
        )
        assert contains_pt_journey(response) is True
        pt_sections = get_pt_sections(response)
        assert len(pt_sections) == 1
        assert pt_sections[0]['departure_date_time'] == '20120614T180100'
        assert pt_sections[0]['arrival_date_time'] == '20120614T180102'
        line_id = next(o['id'] for o in pt_sections[0]['links'] if o['type'] == 'line')
        assert line_id == 'B'
        # No disruption on line 'B'
        assert len(response['disruptions']) == 0

        # Send blocking impacts on line A and base_network with very wide application period
        self.send_mock("blocking_line_disruption", "A", "line", blocking=True)
        self.send_mock("blocking_network_disruption", "base_network", "network", blocking=True)

        # Test disruption API
        response = self.query_region('disruptions')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) - nb_pre_loaded_disruption == 2
        for d in disruptions:
            is_valid_disruption(d)
        assert {"blocking_line_disruption", "blocking_network_disruption"}.issubset(
            set([d['disruption_uri'] for d in disruptions])
        )
        # Check that there is no more PT journey in the response
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        assert contains_pt_journey(response) is False

        # we should then not have disruptions (since we don't get any journey)
        disruptions = self.get_disruptions(response)
        assert not disruptions

        # we then query for the same journey but without disruptions,
        # so we'll have a journey (but the disruptions will be displayed
        response = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        disruptions = self.get_disruptions(response)
        assert disruptions
        assert len(disruptions) == 2
        assert 'blocking_line_disruption' in disruptions
        assert 'blocking_network_disruption' in disruptions

        self.send_mock("blocking_network_disruption", "base_network", "network", blocking=True, is_deleted=True)
        response = self.query_region(journey_basic_query + "&data_freshness=realtime")
        links = []

        def get_line_id(k, v):
            if k != "links" or not "type" in v or not "id" in v or v["type"] != "line":
                return
            links.append(v["id"])

        utils.walk_dict(response, get_line_id)
        assert all([id_ != "A" for id_ in links])

        # we also query without disruption and obviously we should not have the network disruptions
        response = self.query_region(journey_basic_query + "&data_freshness=base_schedule")
        assert 'journeys' in response
        disruptions = self.get_disruptions(response)
        assert disruptions
        assert len(disruptions) == 1
        assert 'blocking_line_disruption' in disruptions
        # we also check that the journey is tagged as disrupted
        assert any([j.get('status') == 'NO_SERVICE' for j in response['journeys']])

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
        assert len(disrupt) == 2

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        # we create a disruption on the network
        self.send_mock("test_disruption", "base_network", "network", message='message')

        response = self.query_region(query)

        disrupt = get_disruptions(response['traffic_reports'][0]['network'], response)
        assert disrupt
        assert len(disrupt) == 3

        for disruption in disrupt:
            if disruption['id'] == 'test_disruption':
                assert disruption['messages'][0]['text'] == 'message'

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        # we update the previous disruption
        self.send_mock("test_disruption", "base_network", "network", message='update')

        response = self.query_region(query)

        disrupt = get_disruptions(response['traffic_reports'][0]['network'], response)
        assert disrupt
        assert len(disrupt) == 3
        for disruption in disrupt:
            if disruption['id'] == 'test_disruption':
                assert disruption['messages'][0]['text'] == 'update'

        status = self.query_region('status')
        last_loaded_data = get_not_null(status['status'], 'last_rt_data_loaded')

        # we delete the disruption
        self.send_mock("test_disruption", "base_network", "network", is_deleted=True)

        response = self.query_region(query)

        disrupt = get_disruptions(response['traffic_reports'][0]['network'], response)
        assert disrupt
        assert len(disrupt) == 2
        for disruption in disrupt:
            assert disruption['uri'] != 'test_disruption', 'this disruption must have been deleted'

    def test_traffic_reports_with_parameters(self):
        """
        Here we test api /traffic_reports with parameters
        """

        def get_stop_area(sas, sa_id):
            for sa in sas:
                if sa['id'] == sa_id:
                    return sa
            return None

        mode_tramway = 'physical_mode:0x0'

        # Call traffic_reports and verify all the objects
        response = self.query_region('traffic_reports?_current_datetime=20120801T000000&depth=3')
        assert len(response['traffic_reports']) == 1
        assert len(response['disruptions']) == 7
        assert response['traffic_reports'][0]['network']['id'] == 'base_network'
        assert len(response['traffic_reports'][0]['lines']) == 1
        assert response['traffic_reports'][0]['lines'][0]['id'] == 'A'
        assert len(response['traffic_reports'][0]['lines'][0]['physical_modes']) == 1
        assert response['traffic_reports'][0]['lines'][0]['physical_modes'][0]['id'] == mode_tramway
        assert len(response['traffic_reports'][0]['stop_areas']) == 2

        stop_area = get_stop_area(response['traffic_reports'][0]['stop_areas'], 'stopA')
        assert stop_area['id'] == 'stopA'
        assert len(stop_area['physical_modes']) == 2
        assert any(pm['id'] == mode_tramway for pm in stop_area['physical_modes'])
        assert any(pm['id'] == 'physical_mode:0x1' for pm in stop_area['physical_modes'])

        stop_area = get_stop_area(response['traffic_reports'][0]['stop_areas'], 'stopB')
        assert stop_area['id'] == 'stopB'
        assert len(stop_area['physical_modes']) == 1
        assert stop_area['physical_modes'][0]['id'] == mode_tramway

        # traffic_reports with forbidden_uris[]=physical_mode:0x0 (Tramway) gives empty result
        # as all the pt_objects impacted are related to physical_mode Tramway
        response = self.query_region(
            'traffic_reports?_current_datetime=20120801T000000&depth=3&forbidden_uris[]=physical_mode:0x0'
        )
        assert len(response['traffic_reports']) == 0
        assert len(response['disruptions']) == 0

        # traffic_reports with forbidden_uris[]=physical_mode:0x1 (Metro) filters all the objects impacted
        # and related to physical_mode Metro. stop_area 'stopA' is hence excluded
        response = self.query_region(
            'traffic_reports?_current_datetime=20120801T000000&depth=3&forbidden_uris[]=physical_mode:0x1'
        )
        assert len(response['traffic_reports']) == 1
        assert len(response['disruptions']) == 3
        assert response['traffic_reports'][0]['network']['id'] == 'base_network'
        assert len(response['traffic_reports'][0]['lines']) == 1
        assert response['traffic_reports'][0]['lines'][0]['id'] == 'A'
        assert len(response['traffic_reports'][0]['lines'][0]['physical_modes']) == 1
        assert response['traffic_reports'][0]['lines'][0]['physical_modes'][0]['id'] == mode_tramway
        assert len(response['traffic_reports'][0]['stop_areas']) == 1
        assert response['traffic_reports'][0]['stop_areas'][0]['id'] == 'stopB'
        assert len(response['traffic_reports'][0]['stop_areas'][0]['physical_modes']) == 1
        assert response['traffic_reports'][0]['stop_areas'][0]['physical_modes'][0]['id'] == mode_tramway

    def test_disruption_with_and_without_tags(self):
        """
        test api /disruptions with and without parameter tags[] and check the result
        """
        query = 'disruptions?since=20120801T000000'
        disruptions = self.query_region(query)['disruptions']
        assert len(disruptions) == 11

        # query with parameter tags[]
        resp, code = self.query_region('disruptions?since=20120801T000000&tags[]=tag_name', check=False)
        assert code == 404
        assert resp['error']['message'] == 'ptref : Filters: Unable to find object'

        # we create some disruptions with tags
        tags = [{'id': 're', 'name': 'rer'}, {'id': 'met', 'name': 'metro'}]
        self.send_mock("disruption_on_network", "base_network", "network", message='message', tags=tags)
        tags = [{'id': 'bu', 'name': 'bus'}, {'id': 'met', 'name': 'metro'}]
        self.send_mock("disruption_on_stopA", "stopA", "stop_area", message='message', tags=tags)
        tags = [{'id': 'met', 'name': 'metro'}]
        self.send_mock("disruption_on_stopB", "stopB", "stop_area", message='message', tags=tags)

        disruptions = self.query_region(query)['disruptions']
        assert len(disruptions) == 14

        disruptions = self.query_region('disruptions?since=20120801T000000&tags[]=rer')['disruptions']
        assert len(disruptions) == 1

        disruptions = self.query_region('disruptions?since=20120801T000000&tags[]=rer&tags[]=bus')['disruptions']
        assert len(disruptions) == 2

        disruptions = self.query_region('disruptions?since=20120801T000000&tags[]=metro')['disruptions']
        assert len(disruptions) == 3

        # query with parameter filter
        disruptions = self.query_region(
            'disruptions?since=20120801T000000' '&filter=stop_area.id="stopA" or stop_area.id="stopB"'
        )['disruptions']
        assert len(disruptions) == 4

        # query with parameter filter and tags with one value
        disruptions = self.query_region(
            'disruptions?since=20120801T000000'
            '&filter=stop_area.id="stopA" or stop_area.id="stopB"'
            '&tags[]=bus'
        )['disruptions']
        assert len(disruptions) == 1

        # query with parameter filter and tags with two values
        disruptions = self.query_region(
            'disruptions?since=20120801T000000'
            '&filter=stop_area.id="stopA" or stop_area.id="stopB"'
            '&tags[]=bus&tags[]=metro'
        )['disruptions']
        assert len(disruptions) == 2


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
        self.send_mock(
            disruption_id, disruption_target, disruption_target_type, blocking=True, message=disruption_msg
        )
        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert disruptions
        assert len(disruptions) == 1
        assert disruptions[0]['disruption_id'] == disruption_id

        journey_query_adapted = journey_basic_query + "&data_freshness=adapted_schedule"

        response_adapted = self.query_region(journey_query_adapted)

        # As the public transport is blocked by the disruption, we should find only 1 journey here
        assert len(response_adapted['journeys']) == 1
        assert response_adapted['journeys'][0]['type'] == 'best'

        # delete disruption on stop point
        self.send_mock(disruption_id, disruption_target, disruption_target_type, is_deleted=True)

        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert not disruptions

        journey_query = journey_basic_query + "&data_freshness=base_schedule"
        response = self.query_region(journey_query)
        self.is_valid_journey_response(response, journey_query)
        assert len(response['journeys']) == 2

        journey_query = journey_basic_query + "&data_freshness=adapted_schedule"
        response = self.query_region(journey_query)
        self.is_valid_journey_response(response, journey_query)
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
        self.send_mock(
            disruption_id,
            disruption_target,
            disruption_target_type,
            blocking=True,
            message=disruption_msg,
            cause='foo',
            category='CategoryTest',
        )
        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert disruptions
        assert len(disruptions) == 1
        assert disruptions[0]['disruption_id'] == disruption_id
        assert disruptions[0]['cause'] == 'foo'
        assert disruptions[0]['category'] == 'CategoryTest'

        journey_query_adapted = journey_basic_query + "&data_freshness=adapted_schedule"

        response_adapted = self.query_region(journey_query_adapted)

        # No public transport journey is found, the only way it by foot
        assert len(response_adapted['journeys']) == 1
        assert response_adapted['journeys'][0]['type'] == 'best'

        # delete disruption on stop point
        self.send_mock(disruption_id, disruption_target, disruption_target_type, is_deleted=True)

        response = self.query_region(query)
        disruptions = response.get('disruptions')
        assert not disruptions

        journey_query = journey_basic_query + "&data_freshness=base_schedule"
        response = self.query_region(journey_query)
        self.is_valid_journey_response(response, journey_query)
        assert len(response['journeys']) == 2

        journey_query = journey_basic_query + "&data_freshness=adapted_schedule"
        response = self.query_region(journey_query)
        self.is_valid_journey_response(response, journey_query)
        assert len(response['journeys']) == 2


def make_mock_chaos_item(
    disruption_name,
    impacted_obj,
    impacted_obj_type,
    start,
    end,
    message_text='default_message',
    is_deleted=False,
    blocking=False,
    start_period="20100412T165200",
    end_period="20300412T165200",
    cause='CauseTest',
    category=None,
    routes=None,
    properties=None,
    tags=None,
):
    feed_message = gtfs_realtime_pb2.FeedMessage()
    feed_message.header.gtfs_realtime_version = '1.0'
    feed_message.header.incrementality = gtfs_realtime_pb2.FeedHeader.DIFFERENTIAL
    feed_message.header.timestamp = 0

    feed_entity = feed_message.entity.add()
    feed_entity.id = disruption_name
    feed_entity.is_deleted = is_deleted

    disruption = feed_entity.Extensions[chaos_pb2.disruption]

    disruption.id = disruption_name
    disruption.cause.id = cause
    disruption.cause.wording = cause
    if category:
        disruption.cause.category.id = category
        disruption.cause.category.name = category
    disruption.reference = "DisruptionTest"
    disruption.publication_period.start = utils.str_to_time_stamp(start_period)
    disruption.publication_period.end = utils.str_to_time_stamp(end_period)

    # disruption tags
    for t in tags or []:
        tag = disruption.tags.add()
        tag.id = t['id']
        tag.name = t['name']

    if not impacted_obj or not impacted_obj_type:
        return feed_message.SerializeToString()

    # Impacts
    impact = disruption.impacts.add()
    impact.id = "impact_" + disruption_name + "_1"
    enums_impact = gtfs_realtime_pb2.Alert.DESCRIPTOR.enum_values_by_name
    impact.created_at = utils.str_to_time_stamp(u'20160405T150623')
    impact.updated_at = utils.str_to_time_stamp(u'20160405T150733')
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
        "stop_point": chaos_pb2.PtObject.stop_point,
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
        for route in routes or []:
            pb_route = line_section.routes.add()
            pb_route.pt_object_type = chaos_pb2.PtObject.route
            pb_route.uri = route

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

    # message with one channel and four channel types: web, mobile, title and beacon
    message = impact.messages.add()
    message.text = message_text
    message.channel.name = "beacon"
    message.channel.id = "beacon"
    message.channel.max_size = 60
    message.channel.content_type = "text"
    message.channel.types.append(chaos_pb2.Channel.web)
    message.channel.types.append(chaos_pb2.Channel.mobile)
    message.channel.types.append(chaos_pb2.Channel.title)
    message.channel.types.append(chaos_pb2.Channel.beacon)

    # properties linked to the disruption
    for p in properties or []:
        prop = disruption.properties.add()
        prop.key = p['key']
        prop.type = p['type']
        prop.value = p['value']

    return feed_message.SerializeToString()
