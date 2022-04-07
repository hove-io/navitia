# encoding: utf-8
# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from six.moves.urllib.parse import quote, quote_plus
from six.moves.urllib.parse import urlencode
from .tests_mechanism import dataset, AbstractTestFixture
from .check_utils import *
from six.moves import range


@dataset({"main_ptref_test": {}})
class TestPtRef(AbstractTestFixture):
    """
    Test the structure of the ptref response
    """

    @staticmethod
    def _test_links(response, pt_obj_name):
        # Test the validity of links of 'previous', 'next', 'last', 'first'
        wanted_links_type = ['previous', 'next', 'last', 'first']
        for l in response['links']:
            if l['type'] in wanted_links_type:
                assert pt_obj_name in l['href']

        # Test the consistency between links
        wanted_links = [l['href'] for l in response['links'] if l['type'] in wanted_links_type]
        if len(wanted_links) <= 1:
            return

        def _get_dict_to_compare(link):
            url_dict = query_from_str(link)
            url_dict.pop('start_page', None)
            url_dict['url'] = link.split('?')[0]
            return url_dict

        url_dict = _get_dict_to_compare(wanted_links[0])

        for l in wanted_links[1:]:
            assert url_dict == _get_dict_to_compare(l)

    def test_pagination_links_with_count(self):
        response = self.query_region("stop_points?count=2&start_page=2", display=True)
        for link in response['links']:
            if link['type'] in ('previous', 'next', 'first', 'last'):
                assert 'count=2' in link['href']

    def test_vj_default_depth(self):
        """default depth is 1"""
        response = self.query_region("vehicle_journeys")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=1)

        assert len(vjs) == 5
        vj = vjs[0]
        assert vj['id'] == 'vehicle_journey:vj1'
        assert vj['name'] == 'vj1'
        assert vj['headsign'] == 'vj1'

        assert len(vj['stop_times']) == 2

        # Local Time
        assert vj['stop_times'][0]['arrival_time'] == '111500'
        assert vj['stop_times'][0]['departure_time'] == '111500'
        assert vj['stop_times'][1]['arrival_time'] == '121000'
        assert vj['stop_times'][1]['departure_time'] == '121000'

        # UTC Time
        # In this case, Local time = UTC Time
        assert vj['stop_times'][0]['utc_arrival_time'] == '101500'
        assert vj['stop_times'][0]['utc_departure_time'] == '101500'
        assert vj['stop_times'][1]['utc_arrival_time'] == '111000'
        assert vj['stop_times'][1]['utc_departure_time'] == '111000'

        # we added some comments on the vj, we should have them
        com = get_not_null(vj, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'information'
        assert com[0]['value'] == 'hello'
        assert "feed_publishers" in response

        feed_publishers = response["feed_publishers"]
        for feed_publisher in feed_publishers:
            is_valid_feed_publisher(feed_publisher)
        feed_publisher = feed_publishers[1]
        assert feed_publisher["id"] == "c1"
        assert feed_publisher["name"] == "name-c1"
        assert feed_publisher["license"] == "ls-c1"
        assert feed_publisher["url"] == "ws-c1"

        feed_publisher = feed_publishers[0]
        assert feed_publisher["id"] == "builder"
        assert feed_publisher["name"] == "canal tp"
        assert feed_publisher["license"] == "ODBL"
        assert feed_publisher["url"] == "www.canaltp.fr"
        self.check_context(response)

    def test_vj_depth_0(self):
        """default depth is 1"""
        response = self.query_region("vehicle_journeys?depth=0")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=0)

    def test_vj_depth_2(self):
        """default depth is 1"""
        response = self.query_region("vehicle_journeys?depth=2")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=2)

    def test_vj_depth_3(self):
        """default depth is 1"""
        response = self.query_region("vehicle_journeys?depth=3")

        vjs = get_not_null(response, 'vehicle_journeys')

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=3)

    def test_frequency_vj(self):
        """
        The response should have 3 optional fields
            start_time
            end_time
            headway_secs
        """
        response = self.query_region('lines/line:freq/vehicle_journeys?depth=3')
        vjs = get_not_null(response, 'vehicle_journeys')

        assert len(vjs) == 1
        assert vjs[0]['start_time'] == '100000'
        assert vjs[0]['end_time'] == '110000'
        assert vjs[0]['headway_secs'] == 600  # a 10 min step

        # with a not frequency VJ
        response = self.query_region('lines/line:A/vehicle_journeys?depth=3')
        vjs = get_not_null(response, 'vehicle_journeys')

        assert len(vjs) == 1
        assert not 'start_time' in vjs[0]
        assert not 'end_time' in vjs[0]
        assert not 'headway_secs' in vjs[0]

    def test_vj_show_codes_propagation(self):
        """stop_area:stop1 has a code, we should be able to find it when accessing it by the vj"""
        response = self.query_region("stop_areas/stop_area:stop1/vehicle_journeys")

        vjs = get_not_null(response, 'vehicle_journeys')

        assert vjs

        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=1)

        stop_points = [get_not_null(st, 'stop_point') for vj in vjs for st in vj['stop_times']]
        stops1 = [s for s in stop_points if s['id'] == 'stop_area:stop1']
        assert stops1

        for stop1 in stops1:
            # all reference to stop1 must have it's codes
            codes = get_not_null(stop1, 'codes')
            code_uic = [c for c in codes if c['type'] == 'code_uic']
            assert len(code_uic) == 1 and code_uic[0]['value'] == 'bobette'

    def test_ptref_without_current_datetime(self):
        """
        stop_area:stop1 without message because _current_datetime is NOW()
        """
        response = self.query_region("stop_areas/stop_area:stop1")

        assert len(response['disruptions']) == 0

    def test_ptref_invalid_type(self):
        response, code = self.query_region("AAAAAA/stop_areas", check=False)
        assert code == 400
        assert response['message'] == 'unknown type: AAAAAA'

        coord = "{lon};{lat}".format(lon=1.2, lat=3.4)
        response, code = self.query_region("{coord}/stop_areas".format(coord=coord), check=False)
        assert code == 400
        assert response['message'] == 'unknown type: {coord}'.format(coord=coord)

    def test_ptref_with_current_datetime(self):
        """
        stop_area:stop1 with _current_datetime
        """

        # In Europe/Paris timezone, the diff between UTC and
        # local time is 2 Hours
        utc_datetime = '20140115T235959'
        local_date_time = '20140116T005959'
        timezone = 'Europe/Paris'

        response = self.query_region("stop_areas/stop_area:stop1?_current_datetime={}".format(utc_datetime))

        disruptions = get_not_null(response, 'disruptions')

        assert len(disruptions) == 1

        messages = get_not_null(disruptions[0], 'messages')

        assert (messages[0]['text']) == 'Disruption on StopArea stop_area:stop1'
        self.check_context(response)
        assert response['context']['timezone'] == timezone
        assert response['context']['current_datetime'] == local_date_time

    def test_contributors(self):
        """test contributor formating"""
        response = self.query_region("contributors")

        contributors = get_not_null(response, 'contributors')

        assert len(contributors) == 1

        ctr = contributors[0]
        assert ctr["id"] == 'c1'
        assert ctr["website"] == 'ws-c1'
        assert ctr["license"] == 'ls-c1'

    def test_datasets(self):
        """test dataset formating"""
        response = self.query_region("datasets")

        datasets = get_not_null(response, 'datasets')

        assert len(datasets) == 1

        ds = datasets[0]
        assert ds["id"] == 'd1'
        assert ds["description"] == 'desc-d1'
        assert ds["system"] == 'sys-d1'

    def test_contributor_by_dataset(self):
        """test contributor by dataset formating"""
        response = self.query_region("datasets/d1/contributors")
        ctrs = get_not_null(response, 'contributors')
        assert len(ctrs) == 1

        ctr = ctrs[0]
        assert ctr["id"] == 'c1'
        assert ctr["website"] == 'ws-c1'
        assert ctr["license"] == 'ls-c1'

    def test_dataset_by_contributor(self):
        """test dataset by contributor formating"""
        response = self.query_region("contributors/c1/datasets")

        frs = get_not_null(response, 'datasets')
        assert len(frs) == 1

        fr = frs[0]
        assert fr["id"] == 'd1'

    def test_line(self):
        """test line formating"""
        response = self.query_region("lines")

        lines = get_not_null(response, 'lines')

        assert len(lines) == 5

        l = lines[0]

        is_valid_line(l, depth_check=1)

        assert l["text_color"] == 'FFD700'
        # we know we have a geojson for this test so we can check it
        geo = get_not_null(l, 'geojson')
        shape(geo)

        com = get_not_null(l, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'information'
        assert com[0]['value'] == "I'm a happy comment"

        physical_modes = get_not_null(l, 'physical_modes')
        assert len(physical_modes) == 1

        is_valid_physical_mode(physical_modes[0], depth_check=1)

        assert physical_modes[0]['id'] == 'physical_mode:Car'
        assert physical_modes[0]['name'] == 'name physical_mode:Car'

        line_group = get_not_null(l, 'line_groups')
        assert len(line_group) == 1
        is_valid_line_group(line_group[0], depth_check=0)
        assert line_group[0]['name'] == 'A group'
        assert line_group[0]['id'] == 'group:A'

        self._test_links(response, 'lines')

    def test_line_without_shape(self):
        """test line formating with shape disabled"""
        response = self.query_region("lines?disable_geojson=true")
        lines = get_not_null(response, 'lines')

        assert len(lines) == 5
        l = lines[0]
        is_valid_line(l, depth_check=1)
        # we don't want a geojson since we have desactivate them
        assert 'geojson' not in l

        response = self.query_region("lines")
        lines = get_not_null(response, 'lines')

        assert len(lines) == 5
        l = lines[0]
        is_valid_line(l, depth_check=1)

        # we check our geojson, just to be safe :)
        assert 'geojson' in l
        geo = get_not_null(l, 'geojson')
        shape(geo)

    def test_line_with_shape(self):
        """test line formating with shape explicitly enabled"""
        response = self.query_region("lines?disable_geojson=false")

        lines = get_not_null(response, 'lines')

        assert len(lines) == 5

        l = lines[0]

        is_valid_line(l, depth_check=1)

        # Test that the geojson is indeed there
        geo = get_not_null(l, 'geojson')
        shape(geo)

    def test_line_groups(self):
        """test line group formating"""
        # Test for each possible range to ensure main_line is always at a depth of 0
        for depth in range(0, 3):
            response = self.query_region("line_groups?depth={0}".format(depth))

            line_groups = get_not_null(response, 'line_groups')

            assert len(line_groups) == 1

            lg = line_groups[0]

            is_valid_line_group(lg, depth_check=depth)

            if depth > 0:
                com = get_not_null(lg, 'comments')
                assert len(com) == 1
                assert com[0]['type'] == 'information'
                assert com[0]['value'] == "I'm a happy comment"

        # test if line_groups are accessible through the ptref graph
        response = self.query_region("routes/line:A:0/line_groups")
        line_groups = get_not_null(response, 'line_groups')
        assert len(line_groups) == 1
        lg = line_groups[0]
        is_valid_line_group(lg)

    def test_line_with_active_disruption(self):
        """test disruption is active"""
        response = self.query_region("lines/line:A?_current_datetime=20140115T235959")

        disruptions = get_not_null(response, 'disruptions')

        assert len(disruptions) == 1
        d = disruptions[0]

        # in pt_ref, the status is always active as the checked
        # period is the validity period
        assert d["status"] == "active"

        messages = get_not_null(d, 'messages')
        assert (messages[0]['text']) == 'Disruption on Line line:A'

    def test_line_codes(self):
        """test line formating"""
        response = self.query_region("lines/line:A?show_codes=true")

        lines = get_not_null(response, 'lines')

        assert len(lines) == 1

        l = lines[0]

        codes = get_not_null(l, 'codes')

        assert len(codes) == 4

        is_valid_codes(codes)

    def test_route(self):
        """test line formating"""
        response = self.query_region("routes")

        routes = get_not_null(response, 'routes')
        assert len(routes) == 5

        r = [r for r in routes if r['id'] == 'line:A:0']
        assert len(r) == 1
        r = r[0]
        is_valid_route(r, depth_check=1)

        # we know we have a geojson for this test so we can check it
        geo = get_not_null(r, 'geojson')
        shape(geo)

        com = get_not_null(r, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'information'
        assert com[0]['value'] == "I'm a happy comment"

        self._test_links(response, 'routes')

    def test_stop_areas(self):
        """test stop_areas formating"""
        response = self.query_region("stop_areas")

        stops = get_not_null(response, 'stop_areas')

        assert len(stops) == 4

        s = next((s for s in stops if s['name'] == 'stop_area:stop1'))
        is_valid_stop_area(s, depth_check=1)

        com = get_not_null(s, 'comments')
        assert len(com) == 2
        assert com[0]['type'] == 'information'
        assert com[0]['value'] == "comment on stop A"
        assert com[1]['type'] == 'information'
        assert com[1]['value'] == "the stop is sad"

        self._test_links(response, 'stop_areas')

    def test_stop_area(self):
        """test stop_areas formating"""
        response = self.query_region("stop_areas/stop_area:stop1?depth=2")

        stops = get_not_null(response, 'stop_areas')

        assert len(stops) == 1

        is_valid_stop_area(stops[0], depth_check=2)
        modes = get_not_null(stops[0], 'physical_modes')
        assert len(modes) == 1
        modes = get_not_null(stops[0], 'commercial_modes')
        assert len(modes) == 1

    def test_stop_points(self):
        """test stop_points formating"""
        response = self.query_region("stop_points?depth=2")

        stops = get_not_null(response, 'stop_points')

        assert len(stops) == 4

        s = next((s for s in stops if s['name'] == 'stop_area:stop2'))  # yes, that's a stop_point
        is_valid_stop_point(s, depth_check=2)

        com = get_not_null(s, 'comments')
        assert len(com) == 1
        assert com[0]['type'] == 'information'
        assert com[0]['value'] == "hello bob"

        modes = get_not_null(s, 'physical_modes')
        assert len(modes) == 1
        is_valid_physical_mode(modes[0], depth_check=1)
        modes = get_not_null(s, 'commercial_modes')
        assert len(modes) == 1
        is_valid_commercial_mode(modes[0], depth_check=1)

        self._test_links(response, 'stop_points')

    def test_simple_crow_fly(self):
        journey_basic_query = "journeys?from=9;9.001&to=stop_area%3Astop2&datetime=20140105T000000"
        response = self.query_region(journey_basic_query)

        # the response must be still valid (this test the kraken data reloading)
        self.is_valid_journey_response(response, journey_basic_query)

    def test_forbidden_uris_on_line(self):
        """test forbidden uri for lines"""
        response = self.query_region("lines")

        lines = get_not_null(response, 'lines')
        assert len(lines) == 5

        assert len(lines[0]['physical_modes']) == 1
        assert lines[0]['physical_modes'][0]['id'] == 'physical_mode:Car'

        # there is only one line, so when we forbid it's physical mode, we find nothing
        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/lines" "?forbidden_uris[]=physical_mode:Car"
        )
        assert code == 404

        # for retrocompatibility purpose forbidden_id[] is the same
        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/lines" "?forbidden_id[]=physical_mode:Car"
        )
        assert code == 404

        # when we forbid another physical_mode, we find again our line
        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/lines" "?forbidden_uris[]=physical_mode:Bus"
        )
        assert code == 200

    def test_simple_pt_objects(self):
        response = self.query_region('pt_objects?q=stop2')

        is_valid_pt_objects_response(response)

        pt_objs = get_not_null(response, 'pt_objects')
        assert len(pt_objs) == 1

        assert get_not_null(pt_objs[0], 'id') == 'stop_area:stop2'

    def test_simple_pt_objects_stop_point(self):
        response = self.query_region('pt_objects?q=stop2&type[]=stop_point')

        is_valid_pt_objects_response(response)

        pt_objs = get_not_null(response, 'pt_objects')
        assert len(pt_objs) == 1

        assert get_not_null(pt_objs[0], 'embedded_type') == 'stop_point'

    def test_simple_pt_objects_stop_point_and_stop_area(self):
        response = self.query_region('pt_objects?q=stop2&type[]=stop_point&type[]=stop_area')

        is_valid_pt_objects_response(response)

        pt_objs = get_not_null(response, 'pt_objects')
        assert len(pt_objs) == 2

        types = [get_not_null(obj, 'embedded_type') for obj in pt_objs]
        assert 'stop_point' in types
        assert 'stop_area' in types

    def test_line_label_pt_objects(self):
        response = self.query_region('pt_objects?q=line:A&type[]=line')
        is_valid_pt_objects_response(response)
        pt_objs = get_not_null(response, 'pt_objects')
        assert len(pt_objs) == 1
        assert get_not_null(pt_objs[0], 'name') == 'network:A Car line:A'
        self.check_context(response)

        response = self.query_region('pt_objects?q=line:Ca roule&type[]=line')
        pt_objs = get_not_null(response, 'pt_objects')
        assert len(pt_objs) == 1
        # not valid as there is no commercial mode (which impacts name)
        assert get_not_null(pt_objs[0], 'name') == 'network:CaRoule line:Ça roule'

    def test_query_with_strange_char(self):
        q = u'stop_points/stop_point:stop_with name bob \" , é'.encode('utf-8')
        encoded_q = quote(q)
        response = self.query_region(encoded_q)

        stops = get_not_null(response, 'stop_points')

        assert len(stops) == 1

        is_valid_stop_point(stops[0], depth_check=1)
        assert stops[0]["id"] == u'stop_point:stop_with name bob \" , é'

    def test_filter_query_with_strange_char(self):
        """test that the ptref mechanism works an object with a weird id"""
        response = self.query_region('stop_points/stop_point:stop_with name bob \" , é/lines')
        lines = get_not_null(response, 'lines')

        assert len(lines) == 1
        for l in lines:
            is_valid_line(l)

    def test_filter_query_with_strange_char_in_filter(self):
        """test that the ptref mechanism works an object with a weird id passed in filter args"""
        response = self.query_region('lines?filter=stop_point.uri="stop_point:stop_with name bob \\\" , é"')
        lines = get_not_null(response, 'lines')

        assert len(lines) == 1
        for l in lines:
            is_valid_line(l)

    def test_journey_with_strange_char(self):
        # we use an encoded url to be able to check the links
        query = 'journeys?from={}&to={}&datetime=20140105T070000'.format(
            quote_plus(u'stop_with name bob \" , é'.encode('utf-8')),
            quote_plus(u'stop_area:stop1'.encode('utf-8')),
        )
        response = self.query_region(query, display=True)

        self.is_valid_journey_response(response, query)

    def test_vj_period_filter(self):
        """with just a since in the middle of the period, we find vj1"""
        response = self.query_region("vehicle_journeys?since=20140105T070000")

        vjs = get_not_null(response, 'vehicle_journeys')
        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=1)

        assert 'vehicle_journey:vj1' in (vj['id'] for vj in vjs)

        # same with an until at the end of the day
        response = self.query_region("vehicle_journeys?since=20140105T000000&until=20140106T0000")
        vjs = get_not_null(response, 'vehicle_journeys')
        assert 'vehicle_journey:vj1' in (vj['id'] for vj in vjs)

        # there is no vj after the 8
        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/vehicle_journeys?since=20140109T070000"
        )

        assert code == 404
        assert get_not_null(response, 'error')['message'] == 'ptref : Filters: Unable to find object'

    def test_vj_period_filter_with_tz(self):
        '''
        testing that using TZ is OK (+0130 means TZ +1h30min)
        Note: This is especially usefull as Kirin now uses it
        '''

        # first check it's OK with no TZ
        params = {'since': '20140105T111500', 'until': '20140105T121000', 'headsign': 'vj1'}
        response = self.query_region('vehicle_journeys?{}'.format(urlencode(params, doseq=True)))
        vjs = get_not_null(response, 'vehicle_journeys')
        assert 'vehicle_journey:vj1' in (vj['id'] for vj in vjs)

        # check using the actual TZ (Europe/Paris so UTC+1)
        params['since'] = '20140105T111500+0100'
        params['until'] = '20140105T121000+0100'
        response = self.query_region('vehicle_journeys?{}'.format(urlencode(params, doseq=True)))
        vjs = get_not_null(response, 'vehicle_journeys')
        assert 'vehicle_journey:vj1' in (vj['id'] for vj in vjs)

        # check converting time correctly to UTC
        params['since'] = '20140105T101500+0000'
        params['until'] = '20140105T111000+0000'
        response = self.query_region('vehicle_journeys?{}'.format(urlencode(params, doseq=True)))
        vjs = get_not_null(response, 'vehicle_journeys')
        assert 'vehicle_journey:vj1' in (vj['id'] for vj in vjs)

        # check converting time correctly to 2 random TZ
        params['since'] = '20140105T034500-0630'
        params['until'] = '20140105T192500+0815'
        response = self.query_region('vehicle_journeys?{}'.format(urlencode(params, doseq=True)))
        vjs = get_not_null(response, 'vehicle_journeys')
        assert 'vehicle_journey:vj1' in (vj['id'] for vj in vjs)

        # check putting time in UTC with a mistake returns no VJ as it's then out of VJ's period
        params['since'] = '20140105T111500+0000'
        params['until'] = '20140105T121000-0000'
        response, code = self.query_no_assert(
            'v1/coverage/main_ptref_test/vehicle_journeys?{}'.format(urlencode(params, doseq=True))
        )
        assert code == 404
        assert get_not_null(response, 'error')['message'] == 'ptref : Filters: Unable to find object'

    def test_line_by_code(self):
        """test the filter=type.has_code(key, value)"""
        response = self.query_region("lines?filter=line.has_code(codeB, B)&show_codes=true")
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert 'B' in [code['value'] for code in lines[0]['codes'] if code['type'] == 'codeB']

        response = self.query_region("lines?filter=line.has_code(codeB, Bise)&show_codes=true")
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert 'B' in [code['value'] for code in lines[0]['codes'] if code['type'] == 'codeB']

        response = self.query_region("lines?filter=line.has_code(codeC, C)&show_codes=true")
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert 'B' in [code['value'] for code in lines[0]['codes'] if code['type'] == 'codeB']

        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/lines?filter=line.has_code(codeB, rien)&show_codes=true"
        )
        assert code == 400
        assert get_not_null(response, 'error')['message'] == 'ptref : Filters: Unable to find object'

        response, code = self.query_no_assert(
            "v1/coverage/main_ptref_test/lines?filter=line.has_code(codeC, rien)&show_codes=true"
        )
        assert code == 400
        assert get_not_null(response, 'error')['message'] == 'ptref : Filters: Unable to find object'

    def test_pt_ref_internal_method(self):
        from jormungandr import i_manager
        from navitiacommon import type_pb2

        i = i_manager.instances['main_ptref_test']

        assert len([r for r in i.ptref.get_objs(type_pb2.ROUTE)]) == 5

    def test_ptref_on_stop_areas_with_disable_disruption(self):
        """
        stop_area:stop1 with and without disable_disruption
        """
        current_datetime = '20140115T235959'
        response = self.query_region("stop_areas/stop_area:stop1?_current_datetime={}".format(current_datetime))

        disruptions = get_not_null(response, 'disruptions')

        assert len(disruptions) == 1

        response = self.query_region(
            "stop_areas/stop_area:stop1?_current_datetime={}&disable_disruption=true".format(current_datetime)
        )
        assert len(response['disruptions']) == 0

        response = self.query_region(
            "stop_areas/stop_area:stop1?_current_datetime={}&disable_disruption=false".format(current_datetime)
        )
        assert len(response['disruptions']) == 1

    def test_ptref_on_lines_with_disable_disruption(self):
        """
        line:A with and without disable_disruption
        """
        response = self.query_region("lines/line:A?_current_datetime=20140115T235959")
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 1

        response = self.query_region("lines/line:A?_current_datetime=20140115T235959&disable_disruption=true")
        assert len(response['disruptions']) == 0

        response = self.query_region("lines/line:A?_current_datetime=20140115T235959&disable_disruption=false")
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 1

    def test_pt_objects_with_disable_disruption(self):
        response = self.query_region('pt_objects?q=line:A&type[]=line&_current_datetime=20140115T235959')
        is_valid_pt_objects_response(response)
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 1
        assert disruptions[0]['disruption_id'] == 'Disruption On line:A'

        response = self.query_region(
            'pt_objects?q=line:A&type[]=line&_current_datetime=20140115T235959' '&disable_disruption=true'
        )
        assert len(response['disruptions']) == 0

        response = self.query_region(
            'pt_objects?q=line:A&type[]=line&_current_datetime=20140115T235959' '&disable_disruption=false'
        )
        assert len(response['disruptions']) == 1

    def test_networks_with_external_code(self):
        """test networks with external_code parameter"""
        response = self.query_region("networks")
        networks = get_not_null(response, 'networks')
        assert len(networks) == 5
        assert networks[0]['id'] == 'network:A'
        assert networks[1]['id'] == 'network:B'
        assert networks[2]['id'] == 'network:C'
        assert networks[3]['id'] == 'network:CaRoule'
        assert networks[4]['id'] == 'network:freq'

        response = self.query_region("networks?external_code=A")
        networks = get_not_null(response, 'networks')
        assert len(networks) == 1
        assert networks[0]['id'] == 'network:A'

        response = self.query_region("networks?external_code=B")
        networks = get_not_null(response, 'networks')
        assert len(networks) == 1
        assert networks[0]['id'] == 'network:B'

        response, code = self.query_no_assert("v1/coverage/main_ptref_test/networks?external_code=wrong_code")
        assert code == 404
        message = get_not_null(response, 'message')
        assert 'Unable to find an object for the external_code wrong_code' in message

        # without coverage
        response, code = self.query_no_assert("v1/networks?external_code=A")
        assert code == 200
        networks = get_not_null(response, 'networks')
        assert len(networks) == 1
        assert networks[0]['id'] == 'network:A'

        response, code = self.query_no_assert("v1/networks?external_code=wrong_code")
        assert code == 404
        message = get_not_null(response, 'message')
        assert 'Unable to find an object for the external_code wrong_code' in message

        # Without coverage, networks have to work with external_code
        response, code = self.query_no_assert("v1/networks")
        assert code == 400
        message = get_not_null(response, 'message')
        assert 'parameter \"external_code\" invalid: Missing required parameter' in message

    def test_lines_with_external_code(self):
        """test lines with external_code parameter"""
        response = self.query_region("lines")
        lines = get_not_null(response, 'lines')
        assert len(lines) == 5
        assert lines[0]['id'] == 'line:A'
        assert lines[1]['id'] == 'line:B'
        assert lines[2]['id'] == 'line:C'
        assert lines[3]['id'] == 'line:Ça roule'
        assert lines[4]['id'] == 'line:freq'

        response = self.query_region("lines?external_code=A")
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert lines[0]['id'] == 'line:A'

        response, code = self.query_no_assert("v1/coverage/main_ptref_test/lines?external_code=wrong_code")
        assert code == 404
        message = get_not_null(response, 'message')
        assert 'Unable to find an object for the external_code wrong_code' in message

        # without coverage
        response, code = self.query_no_assert("v1/lines?external_code=A")
        assert code == 200
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert lines[0]['id'] == 'line:A'

        response, code = self.query_no_assert("v1/lines?external_code=wrong_code")
        assert code == 404
        message = get_not_null(response, 'message')
        assert 'Unable to find an object for the external_code wrong_code' in message

        # Without coverage, networks have to work with external_code
        response, code = self.query_no_assert("v1/lines")
        assert code == 400
        message = get_not_null(response, 'message')
        assert 'parameter \"external_code\" invalid: Missing required parameter' in message

    def test_stop_points_with_external_code(self):
        """test stop_points with external_code parameter"""
        response = self.query_region("stop_points")
        stop_points = get_not_null(response, 'stop_points')
        assert len(stop_points) == 4
        assert stop_points[0]['id'] == 'stop_area:stop1'
        assert stop_points[1]['id'] == 'stop_area:stop2'
        assert stop_points[2]['id'] == 'stop_area:stop3'
        assert stop_points[3]['id'] == 'stop_point:stop_with name bob " , é'

        response = self.query_region("stop_points?external_code=stop1_code")
        stop_points = get_not_null(response, 'stop_points')
        assert len(stop_points) == 1
        assert stop_points[0]['id'] == 'stop_area:stop1'

        response, code = self.query_no_assert("v1/coverage/main_ptref_test/stop_points?external_code=wrong_code")
        assert code == 404
        message = get_not_null(response, 'message')
        assert 'Unable to find an object for the external_code wrong_code' in message

        # without coverage
        response, code = self.query_no_assert("v1/stop_points?external_code=stop1_code")
        assert code == 200
        stop_points = get_not_null(response, 'stop_points')
        assert len(stop_points) == 1
        assert stop_points[0]['id'] == 'stop_area:stop1'

        response, code = self.query_no_assert("v1/stop_points?external_code=wrong_code")
        assert code == 404
        message = get_not_null(response, 'message')
        assert 'Unable to find an object for the external_code wrong_code' in message

        # Without coverage, networks have to work with external_code
        response, code = self.query_no_assert("v1/stop_points")
        assert code == 400
        message = get_not_null(response, 'message')
        assert 'parameter \"external_code\" invalid: Missing required parameter' in message

    def test_stop_times_properties(self):
        response = self.query_region("vehicle_journeys")
        vjs = get_not_null(response, 'vehicle_journeys')
        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=1)

        assert len(vjs) == 5

        # Vehicle_journey with two stop_times
        vj = vjs[0]
        assert len(vj['stop_times']) == 2
        assert vj['stop_times'][0]
        assert vj['stop_times'][0]['pickup_allowed'] is True
        assert vj['stop_times'][0]['drop_off_allowed'] is False
        assert vj['stop_times'][0]['skipped_stop'] is False

        assert vj['stop_times'][1]['pickup_allowed'] is False
        assert vj['stop_times'][1]['drop_off_allowed'] is True
        assert vj['stop_times'][1]['skipped_stop'] is False

        # Vehicle_journey with three stop_times having an intermediate skipped_stop
        vj = vjs[2]
        assert len(vj['stop_times']) == 3
        assert vj['stop_times'][0]['pickup_allowed'] is True
        assert vj['stop_times'][0]['drop_off_allowed'] is False
        assert vj['stop_times'][0]['skipped_stop'] is False

        assert vj['stop_times'][1]['pickup_allowed'] is False
        assert vj['stop_times'][1]['drop_off_allowed'] is False
        assert vj['stop_times'][1]['skipped_stop'] is True

        assert vj['stop_times'][2]['pickup_allowed'] is False
        assert vj['stop_times'][2]['drop_off_allowed'] is True
        assert vj['stop_times'][2]['skipped_stop'] is False


@dataset({"main_ptref_test": {}, "main_routing_test": {}})
class TestPtRefRoutingAndPtrefCov(AbstractTestFixture):
    def test_external_code(self):
        """test the strange and ugly external code api"""
        response = self.query("v1/lines?external_code=A&show_codes=true")
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert 'A' in [code['value'] for code in lines[0]['codes'] if code['type'] == 'external_code']

    def test_external_code_no_code(self):
        """the external_code is a mandatory parameter for collection without coverage"""
        r, status = self.query_no_assert("v1/lines")
        assert status == 400
        assert (
            "parameter \"external_code\" invalid: "
            "Missing required parameter in the post body or the query string"
            "\nexternal_code description: An external code to query" == r.get('message')
        )

    def test_parameter_error_message(self):
        """test the parameter validation error message"""
        r, status = self.query_no_assert("v1/coverage/lines?disable_geojson=12")
        assert status == 400
        assert (
            "parameter \"disable_geojson\" invalid: Invalid literal for boolean(): 12\n"
            "disable_geojson description: hide the coverage geojson to reduce response size" == r.get('message')
        )

    def test_invalid_url(self):
        """the following bad url was causing internal errors, it should only be a 404"""
        _, status = self.query_no_assert("v1/coverage/lines/bob")
        assert status == 404


@dataset({"main_routing_test": {}})
class TestPtRefRoutingCov(AbstractTestFixture):
    def test_with_coords(self):
        """test with a coord in the pt call, so a place nearby is actually called"""
        response = self.query_region("coords/{coord}/stop_areas".format(coord=r_coord))

        stops = get_not_null(response, 'stop_areas')

        for s in stops:
            is_valid_stop_area(s)

        # the default is the search for all stops within 200m, so we should have A and C
        assert len(stops) == 2

        assert set(["stopA", "stopC"]) == set([s['name'] for s in stops])

    def test_with_coord(self):
        """some but with coord and not coords"""
        response = self.query_region("coord/{coord}/stop_areas".format(coord=r_coord))

        stops = get_not_null(response, 'stop_areas')

        for s in stops:
            is_valid_stop_area(s)

        # the default is the search for all stops within 200m, so we should have A and C
        assert len(stops) == 2

        assert set(["stopA", "stopC"]) == set([s['name'] for s in stops])

    def test_with_coord_distance_different(self):
        """same as test_with_coord, but with 300m radius. so we find all stops"""
        response = self.query_region("coords/{coord}/stop_areas?distance=300".format(coord=r_coord))

        stops = get_not_null(response, 'stop_areas')

        for s in stops:
            is_valid_stop_area(s)

        assert len(stops) == 3
        assert set(["stopA", "stopB", "stopC"]) == set([s['name'] for s in stops])

    def test_with_coord_and_filter(self):
        """
        we now test with a more complex query, we want all stops with a metro within 300m of r

        only A and C have a metro line
        Note: the metro is physical_mode:0x1
        """
        response = self.query_region(
            "physical_modes/physical_mode:0x1/coords/{coord}/stop_areas" "?distance=300".format(coord=r_coord),
            display=True,
        )

        stops = get_not_null(response, 'stop_areas')

        for s in stops:
            is_valid_stop_area(s)

        # the default is the search for all stops within 200m, so we should have all 3 stops
        # we should have 3 stops
        assert len(stops) == 2
        assert set(["stopA", "stopC"]) == set([s['name'] for s in stops])

    def test_address_in_stop_points(self):
        """test presence of address node in stop_points"""
        response = self.query_region("stop_points")
        stops = get_not_null(response, 'stop_points')
        for s in stops:
            address = get_not_null(s, 'address')
            is_valid_address(address, depth_check=0)

    def test_all_lines(self):
        """test with all lines in the pt call"""
        response = self.query_region('lines')
        assert 'error' not in response
        lines = get_not_null(response, 'lines')
        assert len(lines) == 7
        assert {"1PM", "1A", "1B", "1C", "1D", "1K", "1M"} == {l['code'] for l in lines}

    def test_line_filter_line_code(self):
        """test filtering lines from line code 1A in the pt call"""
        response = self.query_region('lines?filter=line.code=1A')
        assert 'error' not in response
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert "1A" == lines[0]['code']

    def test_line_filter_line_code_with_resource_uri(self):
        """test filtering lines from line code 1A in the pt call with a resource uri"""
        response = self.query_region('physical_modes/physical_mode:0x1/lines?filter=line.code=1D')
        assert 'error' not in response
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        assert "1D" == lines[0]['code']

    def test_line_filter_line_code_empty_response(self):
        """test filtering lines from line code bob in the pt call
        as no line has the code "bob" response returns no object"""
        url = 'v1/coverage/main_routing_test/lines?filter=line.code=bob'
        response, status = self.query_no_assert(url)
        assert status == 400
        assert 'error' in response
        assert 'bad_filter' in response['error']['id']

    def test_line_filter_route_code_invalid(self):
        """test filtering lines from route code bob in the pt call
        as there is no attribute "code" for route, filter is invalid"""
        response_all_lines = self.query_region('lines')
        all_lines = get_not_null(response_all_lines, 'lines')
        response, status = self.query_no_assert('v1/coverage/main_routing_test/lines?filter=route.code=bob')
        assert status == 400
        assert 'error' in response
        assert 'unable_to_parse' in response['error']['id']

    def test_route_filter_line_code(self):
        """test filtering routes from line code 1B in the pt call"""
        response = self.query_region('routes?filter=line.code=1B')
        assert 'error' not in response
        routes = get_not_null(response, 'routes')
        assert len(routes) == 1
        assert "1B" == routes[0]['line']['code']

    def test_headsign(self):
        """test basic usage of headsign"""
        response = self.query_region('vehicle_journeys?headsign=vjA_hs')
        assert 'error' not in response
        vjs = get_not_null(response, 'vehicle_journeys')
        assert len(vjs) == 1
        assert vjs[0]['name'] == 'vjA'
        assert vjs[0]['headsign'] == 'vjA_hs'

    def test_headsign_with_resource_uri(self):
        """test usage of headsign with resource uri"""
        response = self.query_region('physical_modes/physical_mode:0x0/vehicle_journeys' '?headsign=vjA_hs')
        assert 'error' not in response
        vjs = get_not_null(response, 'vehicle_journeys')
        assert len(vjs) == 1

    def test_headsign_with_code_filter_and_resource_uri(self):
        """test usage of headsign with code filter and resource uri"""
        response = self.query_region(
            'physical_modes/physical_mode:0x0/vehicle_journeys' '?headsign=vjA_hs&filter=line.code=1A'
        )
        assert 'error' not in response
        vjs = get_not_null(response, 'vehicle_journeys')
        assert len(vjs) == 1

    def test_multiple_resource_uri_no_final_collection_uri(self):
        """test usage of multiple resource uris with line and physical mode giving result,
        then with multiple resource uris giving no result as nothing matches"""
        response = self.query_region('physical_modes/physical_mode:0x0/lines/A')
        assert 'error' not in response
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        response = self.query_region('lines/D')
        assert 'error' not in response
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1
        response = self.query_region('physical_modes/physical_mode:0x1/lines/D')
        assert 'error' not in response
        lines = get_not_null(response, 'lines')
        assert len(lines) == 1

        response, status = self.query_region('physical_modes/physical_mode:0x0/lines/D', False)
        assert status == 404
        assert 'error' in response
        assert 'unknown_object' in response['error']['id']

    def test_multiple_resource_uri_with_final_collection_uri(self):
        """test usage of multiple resource uris with line and physical mode giving result,
        as we match it with a final collection, so the intersection is what we want"""
        response = self.query_region('physical_modes/physical_mode:0x1/lines/D/stop_areas')
        assert 'error' not in response
        stop_areas = get_not_null(response, 'stop_areas')
        assert len(stop_areas) == 2
        response = self.query_region('physical_modes/physical_mode:0x0/lines/D/stop_areas')
        assert 'error' not in response
        stop_areas = get_not_null(response, 'stop_areas')
        assert len(stop_areas) == 1

    def test_headsign_stop_time_vj(self):
        """test basic print of headsign in stop_times for vj"""
        response = self.query_region('vehicle_journeys?filter=vehicle_journey.name="vjA"')
        assert 'error' not in response
        vjs = get_not_null(response, 'vehicle_journeys')
        assert len(vjs) == 1
        assert vjs[0]['name'] == "vjA"
        assert vjs[0]['headsign'] == "vjA_hs"
        assert len(vjs[0]['stop_times']) == 2
        assert vjs[0]['stop_times'][0]['headsign'] == "A00"
        assert vjs[0]['stop_times'][1]['headsign'] == "vjA_hs"

    def test_headsign_display_info_journeys(self):
        """test basic print of headsign in section for journeys"""
        response = self.query_region(
            'journeys?from=stop_point:stopB&to=stop_point:stopA&datetime=20120615T000000&max_duration_to_pt=0'
        )
        assert 'error' not in response
        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 1
        assert len(journeys[0]['sections']) == 1
        assert journeys[0]['sections'][0]['display_informations']['headsign'] == "A00"

    def test_headsign_display_info_departures(self):
        """test basic print of headsign in display informations for departures"""
        response = self.query_region('stop_points/stop_point:stopB/departures?from_datetime=20120615T000000')
        assert 'error' not in response
        departures = get_not_null(response, 'departures')
        assert len(departures) == 4
        assert {"A00", "vjM", "vjB", "vjPM"} == {d['display_informations']['headsign'] for d in departures}

    def test_headsign_display_info_arrivals(self):
        """test basic print of headsign in display informations for arrivals"""
        response = self.query_region('stop_points/stop_point:stopB/arrivals?from_datetime=20120615T000000')
        assert 'error' not in response
        arrivals = get_not_null(response, 'arrivals')
        assert len(arrivals) == 2
        assert arrivals[0]['display_informations']['headsign'] == "vj 5"

    def test_headsign_display_info_route_schedules(self):
        """test basic print of headsign in display informations for route schedules"""
        response = self.query_region('routes/A:0/route_schedules?from_datetime=20120615T000000')
        assert 'error' not in response
        route_schedules = get_not_null(response, 'route_schedules')
        assert len(route_schedules) == 1
        assert len(route_schedules[0]['table']['headers']) == 1
        display_info = route_schedules[0]['table']['headers'][0]['display_informations']
        assert display_info['headsign'] == "vjA_hs"
        assert {"A00", "vjA_hs"} == set(display_info['headsigns'])

    def test_trip_id_vj(self):
        """test basic print of trip and its id in vehicle_journeys"""
        response = self.query_region('vehicle_journeys')
        assert 'error' not in response
        vjs = get_not_null(response, 'vehicle_journeys')
        for vj in vjs:
            is_valid_vehicle_journey(vj, depth_check=1)
        assert any(vj['name'] == "vjB" and vj['headsign'] == "vjB" and vj['trip']['id'] == "vjB" for vj in vjs)

    def test_disruptions(self):
        """test the /disruptions api"""
        response = self.query_region('disruptions')

        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 12
        for d in disruptions:
            is_valid_disruption(d)

        # we test that we can access a specific disruption
        response = self.query_region('disruptions/too_bad_line_C')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 1

        # we can also display all disruptions of an object
        response = self.query_region('lines/C/disruptions')
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 2
        disruptions_uris = set([d['uri'] for d in disruptions])
        assert {"too_bad_line_C", "too_bad_all_lines"} == disruptions_uris

        # we can access line object from the disruption
        response, status = self.query_region('disruptions/too_bad_line_C/lines', check=False)
        assert status == 200

    def test_trips(self):
        """test the /trips api"""
        response = self.query_region('trips')

        trips = get_not_null(response, 'trips')
        assert len(trips) == 9
        for t in trips:
            is_valid_trip(t)

        # we test that we can access a specific trip
        response = self.query_region('trips/vjA')
        trips = get_not_null(response, 'trips')
        assert len(trips) == 1
        assert get_not_null(trips[0], 'id') == "vjA"

        # we can also display trip of a vj
        response = self.query_region('vehicle_journeys/vehicle_journey:vjB/trips')
        trips = get_not_null(response, 'trips')
        assert len(trips) == 1
        assert get_not_null(trips[0], 'id') == "vjB"

    def test_attributs_in_display_info_journeys(self):
        """test some attributs in  display_information of a section for journeys"""
        response = self.query_region(
            'journeys?from=stop_point:stopB&to=stop_point:stopA&datetime=20120615T000000&max_duration_to_pt=0'
        )
        assert 'error' not in response
        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 1
        assert len(journeys[0]['sections']) == 1
        assert journeys[0]['sections'][0]['display_informations']['headsign'] == "A00"
        assert journeys[0]['sections'][0]['display_informations']['color'] == "289728"
        assert journeys[0]['sections'][0]['display_informations']['text_color'] == "FFD700"
        assert journeys[0]['sections'][0]['display_informations']['label'] == "1A"
        assert journeys[0]['sections'][0]['display_informations']['code'] == "1A"
        assert journeys[0]['sections'][0]['display_informations']['name'] == "A"

    def test_stop_points_depth_3(self):
        """
        test stop_points formating in depth 3
        Note: done in main_routing_test because we need a routing graph to have all the attributes
        """
        response = self.query_region("stop_points?depth=3")
        for s in get_not_null(response, 'stop_points'):
            is_valid_stop_point(s, depth_check=3)

    def test_pois_uri_poi_types(self):
        response = self.query_region("pois/poi:station_1/poi_types")
        assert len(response["poi_types"]) == 1
        assert response["poi_types"][0]["id"] == "poi_type:amenity:bicycle_rental"

    def test_pt_objects_with_filter(self):
        response = self.query_region('pt_objects?q=stop&type[]=stop_point')
        is_valid_pt_objects_response(response)
        assert len(get_not_null(response, 'pt_objects')) == 4

        # Add filter on StopArea
        response = self.query_region('pt_objects?q=stop&type[]=stop_point&filter=stop_area.id=stopB&depth=3')
        pt_objects = get_not_null(response, 'pt_objects')
        assert len(pt_objects) == 1
        assert get_not_null(pt_objects[0], 'id') == 'stop_point:stopB'
        assert get_not_null(pt_objects[0]['stop_point']['stop_area'], 'id') == 'stopB'

        # Filter on physical_mode
        response = self.query_region(
            'pt_objects?q=stop&type[]=stop_point&filter=physical_mode.id=physical_mode:0x1&depth=3'
        )
        pt_objects = get_not_null(response, 'pt_objects')
        assert len(pt_objects) == 2
        sp1 = pt_objects[0]['stop_point']
        assert sp1['id'] == 'stop_point:stopA'
        assert 'physical_mode:0x1' in [mode['id'] for mode in sp1['physical_modes']]
        sp2 = pt_objects[1]['stop_point']
        assert sp2['id'] == 'stop_point:stopC'
        assert 'physical_mode:0x1' in [mode['id'] for mode in sp2['physical_modes']]

        # request on stop_area without filter
        response = self.query_region('pt_objects?q=stop&type[]=stop_area')
        pt_objects = get_not_null(response, 'pt_objects')
        assert len(pt_objects) == 3
        stop_area_ids = [sa['id'] for sa in pt_objects]
        assert 'stopA' in stop_area_ids
        assert 'stopB' in stop_area_ids
        assert 'stopC' in stop_area_ids

        # Filter on line
        response = self.query_region('pt_objects?q=stop&type[]=stop_area&filter=line.id=PM&depth=3')
        pt_objects = get_not_null(response, 'pt_objects')
        assert len(pt_objects) == 2
        stop_area_ids = [sa['id'] for sa in pt_objects]
        assert 'stopA' in stop_area_ids
        assert 'stopB' in stop_area_ids
        sa1_line_ids = [line['id'] for line in pt_objects[0]['stop_area']['lines']]
        assert 'PM' in sa1_line_ids
        sa2_line_ids = [line['id'] for line in pt_objects[1]['stop_area']['lines']]
        assert 'PM' in sa2_line_ids

    def test_max_count_values_with_pagination(self):
        """
        Verify use of parameter count in ptref
        Default value is fixed to 1000
        """
        response = self.query_region("vehicle_journeys")
        assert response['pagination']['items_per_page'] == 25
        assert response['pagination']['items_on_page'] == 9
        assert response['pagination']['total_result'] == 9

        response = self.query_region("vehicle_journeys?count=2")
        assert response['pagination']['items_per_page'] == 2
        assert response['pagination']['items_on_page'] == 2
        assert response['pagination']['total_result'] == 9

        response = self.query_region("vehicle_journeys?count=1200")
        assert response['pagination']['items_per_page'] == 1000
        assert response['pagination']['items_on_page'] == 9
        assert response['pagination']['total_result'] == 9
