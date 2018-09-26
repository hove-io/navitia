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
from __future__ import absolute_import, print_function, unicode_literals, division
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import (
    ObjGetter,
    has_disruption,
    get_not_null,
    is_valid_line_section_disruption,
    get_used_vj,
    get_all_element_disruptions,
)
import pytest


# Looking for (identifiable) objects impacted by 'disruptions' inputed
def impacted_ids(disruptions):
    # for the impacted object returns:
    #  * id
    #  * or the headsign (for the display_information field)
    #  * or the id of the vehicle_journey for stop_schedule.date_time
    def get_id(obj):
        id = obj.impacted_object.get('id')
        if id is None:
            # display_information case
            id = obj.impacted_object.get('headsign')
        if id is None:
            # stop_schedule.date_time case
            assert obj.impacted_object['links'][0]['type'] == 'vehicle_journey'
            id = obj.impacted_object['links'][0]['id']
        return id

    ids = set()
    for d in disruptions.values():
        ids.update(set(get_id(o) for o in d))

    return ids


@dataset({"line_sections_test": {}})
class TestLineSections(AbstractTestFixture):
    def default_query(self, q, **kwargs):
        """
        query navitia with a current date in the publication period of the impacts
        """
        return self.query_region('{}?_current_datetime=20170101T100000'.format(q), **kwargs)

    def has_disruption(self, object_get, disruption_uri):
        """
        Little helper calling the detail of an object and checking it's disruptions
        """
        r = self.default_query('{col}/{uri}'.format(col=object_get.collection, uri=object_get.uri))
        return has_disruption(r, object_get, disruption_uri)

    def has_tf_disruption(self, q, disruption):
        """
        checks a disruption is present in traffic report response to the request provided
        """
        r = self.default_query(q)

        return any(disruption == d['disruption_id'] for d in r.get('disruptions', []))

    def has_tf_linked_disruption(self, q, disruption, object_get):
        """
        checks a disruption is present in traffic report response to
        the request provided
        also check that the disruption is linked to the specified object
        """
        r = self.default_query(q)

        if not (
            r.get('disruptions')
            and r.get('traffic_reports')
            and r['traffic_reports'][0].get(object_get.collection)
        ):
            return False

        if disruption not in [d['disruption_id'] for d in r['disruptions']]:
            return False

        for obj in r['traffic_reports'][0][object_get.collection]:
            if obj['id'] == object_get.uri:
                return any(disruption == link['id'] for link in obj.get('links', []))

        return False

    def has_dis(self, q, disruption_label):
        r = self.default_query(q)
        return disruption_label in (d['disruption_id'] for d in r['disruptions'])

    def test_line_section_structure(self):
        r = self.default_query('stop_points/C_1')

        assert len(get_not_null(r, 'disruptions')) == 1
        is_valid_line_section_disruption(r['disruptions'][0])

    def test_on_stop_areas(self):
        """
        the line section disruption is not linked to a stop area, we cannot directly find our disruption
        """
        for disruption_label in [
            'line_section_on_line_1',
            'line_section_on_line_1_other_effect',
            'line_section_on_line_2',
        ]:
            for stop_area in ['A', 'B', 'C', 'D', 'E', 'F']:
                assert not self.has_disruption(ObjGetter('stop_areas', stop_area), disruption_label)

    def test_on_stop_points(self):
        """
        the line section disruption should be linked to the impacted stop_points
        """
        # line_section_on_line_1
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': False,
            'B_2': False,
            'C_1': True,
            'C_2': False,
            'C_3': False,
            'D_1': True,
            'D_2': False,
            'D_3': True,
            'E_1': True,
            'E_2': False,
            'F_1': False,
            'F_2': False,
        }
        for stop_point, result in scenario.iteritems():
            assert result == self.has_disruption(ObjGetter('stop_points', stop_point), 'line_section_on_line_1')

        # line_section_on_line_1_other_effect
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': False,
            'B_2': False,
            'C_1': False,
            'C_2': False,
            'C_3': False,
            'D_1': False,
            'D_2': False,
            'D_3': False,
            'E_1': True,
            'E_2': False,
            'F_1': True,
            'F_2': False,
        }
        for stop_point, result in scenario.iteritems():
            assert result == self.has_disruption(
                ObjGetter('stop_points', stop_point), 'line_section_on_line_1_other_effect'
            )

        # line_section_on_line_2
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': True,
            'B_2': False,
            'C_1': False,
            'C_2': False,
            'C_3': False,
            'D_1': False,
            'D_2': False,
            'D_3': False,
            'E_1': False,
            'E_2': False,
            'F_1': True,
            'F_2': False,
        }
        for stop_point, result in scenario.iteritems():
            assert result == self.has_disruption(ObjGetter('stop_points', stop_point), 'line_section_on_line_2')

    def test_on_vehicle_journeys(self):
        """
        the line section disruption should be linked to the impacted vehicle journeys
        """

        # line_section_on_line_1
        scenario = {'vj:1:1': True, 'vj:1:2': False, 'vj:1:3': False, 'vj:2': False, 'vj:3': False}
        for vj, result in scenario.iteritems():
            assert result == self.has_disruption(ObjGetter('vehicle_journeys', vj), 'line_section_on_line_1')

        # line_section_on_line_1_other_effect
        scenario = {'vj:1:1': True, 'vj:1:2': False, 'vj:1:3': False, 'vj:2': False, 'vj:3': False}
        for vj, result in scenario.iteritems():
            assert result == self.has_disruption(
                ObjGetter('vehicle_journeys', vj), 'line_section_on_line_1_other_effect'
            )

        # line_section_on_line_2
        scenario = {'vj:1:1': False, 'vj:1:2': False, 'vj:1:3': False, 'vj:2': True, 'vj:3': False}
        for vj, result in scenario.iteritems():
            assert result == self.has_disruption(ObjGetter('vehicle_journeys', vj), 'line_section_on_line_2')

    def test_traffic_reports_on_stop_areas(self):
        """
        we should be able to find the related line section disruption with /traffic_report
        """

        # line_section_on_line_1
        scenario = {'A': False, 'B': False, 'C': True, 'D': True, 'E': True, 'F': False}

        for sa, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'line_section_on_line_1'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'line_section_on_line_1', ObjGetter('stop_areas', sa)
            )

        # line_section_on_line_1_other_effect
        scenario = {'A': False, 'B': False, 'C': False, 'D': False, 'E': True, 'F': True}

        for sa, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'line_section_on_line_1_other_effect'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'line_section_on_line_1_other_effect',
                ObjGetter('stop_areas', sa),
            )

        # line_section_on_line_2
        scenario = {'A': False, 'B': True, 'C': False, 'D': False, 'E': False, 'F': True}

        for sa, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'line_section_on_line_2'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'line_section_on_line_2', ObjGetter('stop_areas', sa)
            )

    def test_traffic_reports_on_networks(self):
        for disruption_label in [
            'line_section_on_line_1',
            'line_section_on_line_1_other_effect',
            'line_section_on_line_2',
        ]:
            assert self.has_dis('networks/base_network/traffic_reports', disruption_label)
            assert not self.has_dis('networks/network:other/traffic_reports', disruption_label)

        r = self.default_query('traffic_reports')
        # only one network (base_network) is disrupted
        assert len(r['traffic_reports']) == 1
        assert len(r['traffic_reports'][0]['stop_areas']) == 5
        for sa in r['traffic_reports'][0]['stop_areas']:
            assert len(sa['links']) == (2 if sa['id'] in ['A', 'E', 'F'] else 1)

    def test_traffic_reports_on_lines(self):
        """
        we should be able to find the related line section disruption with /traffic_report
        """

        # some of the tests are failing and are runned in the next function named
        # test_failing_traffic_reports_on_lines

        # line_section_on_line_1
        scenario = {
            'line:1': True,
            'line:2': False,
            # 'line:3': False,
        }

        for line, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'lines/{}/traffic_reports'.format(line), 'line_section_on_line_1'
            )
            assert result == self.has_tf_linked_disruption(
                'lines/{}/traffic_reports'.format(line), 'line_section_on_line_1', ObjGetter('lines', line)
            )

        # there is no link between line:3 and the disruption
        assert not self.has_tf_linked_disruption(
            'lines/line:3/traffic_reports', 'line_section_on_line_1', ObjGetter('lines', 'line:3')
        )

        # line_section_on_line_1_other_effect
        scenario = {
            'line:1': True,
            # 'line:2': False,
            'line:3': False,
        }

        for line, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'lines/{}/traffic_reports'.format(line), 'line_section_on_line_1_other_effect'
            )
            assert result == self.has_tf_linked_disruption(
                'lines/{}/traffic_reports'.format(line),
                'line_section_on_line_1_other_effect',
                ObjGetter('lines', line),
            )

        # there is no link between line:2 and the disruption
        assert not self.has_tf_linked_disruption(
            'lines/line:2/traffic_reports', 'line_section_on_line_1_other_effect', ObjGetter('lines', 'line:2')
        )

        # line_section_on_line_2
        scenario = {
            # 'line:1': False,
            'line:2': True,
            'line:3': False,
        }

        for line, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'lines/{}/traffic_reports'.format(line), 'line_section_on_line_2'
            )
            assert result == self.has_tf_linked_disruption(
                'lines/{}/traffic_reports'.format(line), 'line_section_on_line_2', ObjGetter('lines', line)
            )

        # there is no link between line:1 and the disruption
        assert not self.has_tf_linked_disruption(
            'lines/line:1/traffic_reports', 'line_section_on_line_2', ObjGetter('lines', 'line:1')
        )

    @pytest.mark.xfail(
        strict=True,
        reason="the disruption affects at least 1 stop of the line whereas the line itself is not impacted",
        raises=AssertionError,
    )
    def test_failing_traffic_reports_on_lines(self):
        """
            specific cases for lines where the actual behavior is undesirable
        """

        assert not self.has_tf_disruption(
            'lines/line:3/traffic_reports',
            'line_section_on_line_1'
        )

        assert not self.has_tf_disruption(
            'lines/line:2/traffic_reports',
            'line_section_on_line_1_other_effect'
        )

        assert not self.has_tf_disruption(
            'lines/line:3/traffic_reports',
            'line_section_on_line_2'
        )

    def test_traffic_reports_on_routes(self):
        """
        for routes since we display the impacts on all the stops (but we do not display a route object)
        we display the disruption even if the route has not been directly impacted
        """

        # some of the tests are failing and are runned in the next function named
        # test_failing_traffic_reports_on_routes

        # line_section_on_line_1
        scenario = {
            'route:line:1:1': True,
            'route:line:1:2': False,
            'route:line:1:3': False,
            'route:line:2:1': False,
            # 'route:line:3:1': False,
        }

        for route, result in scenario.iteritems():
            assert result == self.has_dis('routes/{}/traffic_reports'.format(route), 'line_section_on_line_1')

        # line_section_on_line_1_other_effect
        scenario = {
            'route:line:1:1': True,
            'route:line:1:2': False,
            'route:line:1:3': True,
            # 'route:line:2:1': False,
            'route:line:3:1': False,
        }

        for route, result in scenario.iteritems():
            assert result == self.has_dis(
                'routes/{}/traffic_reports'.format(route), 'line_section_on_line_1_other_effect'
            )

        # line_section_on_line_2
        scenario = {
            # 'route:line:1:1': False,
            'route:line:1:2': False,
            # 'route:line:1:3': False,
            'route:line:2:1': True,
            'route:line:3:1': False,
        }

        for route, result in scenario.iteritems():
            assert result == self.has_dis('routes/{}/traffic_reports'.format(route), 'line_section_on_line_2')

    @pytest.mark.xfail(
        strict=True,
        reason="The disruptions shouldn't be displayed if only the stop point is impacted but not the route point",
        raises=AssertionError,
    )
    def test_failing_traffic_reports_on_routes(self):
        """
            specific cases for routes where the actual behavior is undesirable
        """

        assert not self.has_dis(
            'routes/route:line:3:1/traffic_reports',
            'line_section_on_line_1'
        )

        assert not self.has_dis(
            'routes/route:line:2:1/traffic_reports',
            'line_section_on_line_1_other_effect'
        )

        assert not self.has_dis(
            'routes/route:line:1:1/traffic_reports',
            'line_section_on_line_2'
        )

        assert not self.has_dis(
            'routes/route:line:1:3/traffic_reports',
            'line_section_on_line_2'
        )

    def test_traffic_reports_on_vjs(self):
        """
        for /traffic_reports on vjs it's a bit the same as the lines
        we display a line section disruption if it impacts the stops of the vj
        """
        # some of the tests are failing and are runned in the next function named
        # test_failing_traffic_reports_on_vjs

        # line_section_on_line_1
        scenario = {
            'vj:1:1': True,
            'vj:1:2': False,
            'vj:1:3': False,
            'vj:2': False,
            # 'vj:3': False,
        }

        for vj, result in scenario.iteritems():
            assert result == self.has_dis(
                'vehicle_journeys/{}/traffic_reports'.format(vj), 'line_section_on_line_1'
            )

        # line_section_on_line_1_other_effect
        scenario = {
            'vj:1:1': True,
            'vj:1:2': False,
            'vj:1:3': True,
            # 'vj:2': False,
            'vj:3': False,
        }

        for vj, result in scenario.iteritems():
            assert result == self.has_dis(
                'vehicle_journeys/{}/traffic_reports'.format(vj), 'line_section_on_line_1_other_effect'
            )

        # line_section_on_line_2
        scenario = {
            # 'vj:1:1': False,
            'vj:1:2': False,
            # 'vj:1:3': False,
            'vj:2': True,
            'vj:3': False,
        }

        for vj, result in scenario.iteritems():
            assert result == self.has_dis(
                'vehicle_journeys/{}/traffic_reports'.format(vj), 'line_section_on_line_2'
            )

    @pytest.mark.xfail(
        strict=True,
        reason="The disruptions shouldn't be displayed if only the stop point is impacted but not the route point",
        raises=AssertionError,
    )
    def test_failing_traffic_reports_on_vjs(self):
        """
            specific cases for vjs where the actual behavior is undesirable
        """

        assert not self.has_dis(
            'vehicle_journeys/vj:3/traffic_reports',
            'line_section_on_line_1'
        )
        assert not self.has_dis(
            'vehicle_journeys/vj:2/traffic_reports',
            'line_section_on_line_1_other_effect'
        )
        assert not self.has_dis(
            'vehicle_journeys/vj:1:1/traffic_reports',
            'line_section_on_line_2'
        )
        assert not self.has_dis(
            'vehicle_journeys/vj:1:3/traffic_reports',
            'line_section_on_line_2'
        )

    def test_traffic_reports_on_stop_points(self):
        """
        for /traffic_reports on stop_points there is a line section disruption link if it impacts the stop_point
        """

        # line_section_on_line_1
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': False,
            'B_2': False,
            'C_1': True,
            'C_2': False,
            'C_3': False,
            'D_1': True,
            'D_2': False,
            'D_3': True,
            'E_1': True,
            'E_2': False,
            'F_1': False,
            'F_2': False,
        }

        for sp, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'line_section_on_line_1'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'line_section_on_line_1',
                ObjGetter('stop_areas', sp[0]),
            )

        # line_section_on_line_1_other_effect
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': False,
            'B_2': False,
            'C_1': False,
            'C_2': False,
            'C_3': False,
            'D_1': False,
            'D_2': False,
            'D_3': False,
            'E_1': True,
            'E_2': False,
            'F_1': True,
            'F_2': False,
        }

        for sp, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'line_section_on_line_1_other_effect'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'line_section_on_line_1_other_effect',
                ObjGetter('stop_areas', sp[0]),
            )

        # line_section_on_line_2
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': True,
            'B_2': False,
            'C_1': False,
            'C_2': False,
            'C_3': False,
            'D_1': False,
            'D_2': False,
            'D_3': False,
            'E_1': False,
            'E_2': False,
            'F_1': True,
            'F_2': False,
        }

        for sp, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'line_section_on_line_2'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'line_section_on_line_2',
                ObjGetter('stop_areas', sp[0]),
            )

    def test_journeys_use_vj_impacted_by_line_section(self):
        """
        for /journeys, we should display a line section disruption only if we use an impacted section
        We do all the calls as base_schedule to see the disruption but not be really impacted by them
        In this test we leave at:
            - 08:00 using vj:1:1
            - 10:00 using vj:1:3
            - 11:00 using vj:2
        """
        # with this departure time we should take 'vj:1:1' that is impacted
        date = '20170102T080000'
        current = '_current_datetime=20170101T080000'

        def journey_query(_from, to):
            return 'journeys?from={fr}&to={to}&datetime={dt}&{cur}&data_freshness=base_schedule'.format(
                fr=_from, to=to, dt=date, cur=current
            )

        def journeys(_from, to):
            q = journey_query(_from=_from, to=to)
            r = self.query_region(q)
            self.is_valid_journey_response(r, q)
            return r

        # If the journey contains stop points or lines impacted in a line section,
        # the disruption is displayed and linked to them even if this
        # line section is not used in the path

        # A -> B
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='A', to='B')
        # we check the used vj to be certain we took an impacted vj
        assert get_used_vj(r) == [['vj:1:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

        # A -> C
        scenario = {
            'line_section_on_line_1': True,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        # 'C' is part of the impacted section, it's the first stop,
        # so A->C use an impacted section
        r = journeys(_from='A', to='C')
        assert get_used_vj(r) == [['vj:1:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:1:1', 'C_1'}

        # B -> C
        scenario = {
            'line_section_on_line_1': True,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        # same for B->C
        r = journeys(_from='B', to='C')
        assert get_used_vj(r) == [['vj:1:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:1:1', 'C_1'}

        # C -> D
        scenario = {
            'line_section_on_line_1': True,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='C', to='D')
        assert get_used_vj(r) == [['vj:1:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:1:1', 'C_1', 'D_1'}

        # D -> F
        scenario = {
            'line_section_on_line_1': True,
            'line_section_on_line_1_other_effect': True,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='D', to='F')
        assert get_used_vj(r) == [['vj:1:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:1:1', 'D_3', 'E_1', 'F_1'}

        # A -> F
        scenario = {
            'line_section_on_line_1': True,
            'line_section_on_line_1_other_effect': True,
            'line_section_on_line_2': False,
        }
        # this journey passes over the impacted section but does not stop (or transfer) on it
        # as such it isn't impacted
        r = journeys(_from='A', to='F')
        assert get_used_vj(r) == [['vj:1:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:1:1', 'C_1', 'D_1', 'D_3', 'E_1', 'F_1'}

        # with this departure time we should take 'vj:2'
        date = '20170102T110000'
        current = '_current_datetime=20170101T080000'

        # A -> B
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': True,
        }
        r = journeys(_from='A', to='B')
        # we check the used vj to be certain we took an impacted vj
        assert get_used_vj(r) == [['vj:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:2', 'B_1'}

        # B -> F
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': True,
        }
        r = journeys(_from='B', to='F')
        # we check the used vj to be certain we took an impacted vj
        assert get_used_vj(r) == [['vj:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:2', 'B_1', 'F_1'}

        # A -> F
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': True,
        }
        r = journeys(_from='A', to='F')
        # we check the used vj to be certain we took an impacted vj
        assert get_used_vj(r) == [['vj:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:2', 'B_1', 'F_1'}

    def test_journeys_use_vj_not_impacted_by_line_section(self):
        """
        for /journeys, we should display a line section disruption only if we use an impacted section

        We do all the calls as base_schedule to see the disruption but not be really impacted by them

        In this test we leave at 09:00 so we'll use vj:1:2 (that is not impacted because not part of an
        impacted route) during the application period of the impact
        """
        # with this departure time we should take 'vj:1:2'
        date = '20170102T090000'
        current = '_current_datetime=20170101T080000'

        def journey_query(_from, to):
            return 'journeys?from={fr}&to={to}&datetime={dt}&{cur}&data_freshness=base_schedule'.format(
                fr=_from, to=to, dt=date, cur=current
            )

        def journeys(_from, to):
            q = journey_query(_from=_from, to=to)
            r = self.query_region(q)
            self.is_valid_journey_response(r, q)
            return r

        # A -> F
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='A', to='F')
        # we check the used vj to be certain we took the right vj
        assert get_used_vj(r) == [['vj:1:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

        # C -> D
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='C', to='D')
        assert get_used_vj(r) == [['vj:1:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

        # B -> E
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='B', to='E')
        assert get_used_vj(r) == [['vj:1:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

        # C -> A
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='C', to='A')
        assert get_used_vj(r) == [['vj:3']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

        # with this departure time we should take 'vj:1:3'
        date = '20170102T100000'
        current = '_current_datetime=20170101T080000'

        # A -> B
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='A', to='B')
        # we check the used vj to be certain we took an impacted vj
        assert get_used_vj(r) == [['vj:1:3']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

    @pytest.mark.xfail(
        strict=True,
        reason="The disruption is declared on route:1:3 whereas it shouldn't be applied because it concerns E -> F stops",
        raises=AssertionError,
    )
    def test_failing_journeys_use_vj_not_impacted_by_line_section(self):
        def journey_query(_from, to):
            return 'journeys?from={fr}&to={to}&datetime={dt}&{cur}&data_freshness=base_schedule'.format(
                fr=_from, to=to, dt=date, cur=current
            )

        def journeys(_from, to):
            q = journey_query(_from=_from, to=to)
            r = self.query_region(q)
            self.is_valid_journey_response(r, q)
            return r

        # with this departure time we should take 'vj:1:3'
        date = '20170102T100000'
        current = '_current_datetime=20170101T080000'

        # Here, the disruption matches the route stop "F_1|route:1:3" when we add messages
        # B -> F
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='B', to='F')
        # we check the used vj to be certain we took an impacted vj
        assert get_used_vj(r) == [['vj:1:3']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

        # Here, the disruption matches the route stop "F_1|route:1:3" when we add messages
        # A -> F
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = journeys(_from='A', to='F')
        # we check the used vj to be certain we took an impacted vj
        assert get_used_vj(r) == [['vj:1:3']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

    def test_stops_schedules_and_departures_impacted_by_line_section(self):
        """
        For /departures we display a line section impact if the stoppoint is part of a line section impact
        """
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        query = 'stop_areas/{sa}/{q}?{cur}&{d}&{f}'

        for q in ['departures', 'stop_schedules']:
            # A
            scenario = {
                'line_section_on_line_1': True,
                'line_section_on_line_1_other_effect': True,
                'line_section_on_line_2': True,
            }
            r = self.query_region(query.format(sa='A', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.iteritems():
                assert result == (disruption in d)
            assert impacted_ids(d) == {'line:1', 'line:2'}

            # B
            scenario = {
                'line_section_on_line_1': True,
                'line_section_on_line_1_other_effect': True,
                'line_section_on_line_2': True,
            }
            r = self.query_region(query.format(sa='B', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.iteritems():
                assert result == (disruption in d)
            assert impacted_ids(d) == {'vj:2', 'line:1', 'line:2', 'B_1'}

            # C
            scenario = {
                'line_section_on_line_1': True,
                'line_section_on_line_1_other_effect': True,
                'line_section_on_line_2': False,
            }
            r = self.query_region(query.format(sa='C', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.iteritems():
                assert result == (disruption in d)
            # the impact is linked in the response to the stop point and the vj
            assert impacted_ids(d) == {'vj:1:1', 'line:1', 'C_1'}

            # D
            scenario = {
                'line_section_on_line_1': True,
                'line_section_on_line_1_other_effect': True,
                'line_section_on_line_2': False,
            }
            r = self.query_region(query.format(sa='D', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.iteritems():
                assert result == (disruption in d)
            assert impacted_ids(d) == {'vj:1:1', 'line:1', 'D_1', 'D_3'}

            # E
            scenario = {
                'line_section_on_line_1': True,
                'line_section_on_line_1_other_effect': True,
                'line_section_on_line_2': False,
            }
            r = self.query_region(query.format(sa='E', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.iteritems():
                assert result == (disruption in d)
            assert impacted_ids(d) == {'vj:1:1', 'line:1', 'E_1'}

        # F
        query = 'stop_areas/{sa}/departures?{cur}&{d}&{f}'
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = self.query_region(query.format(sa='F', cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

        query = 'stop_areas/{sa}/stop_schedules?{cur}&{d}&{f}'
        scenario = {
            'line_section_on_line_1': True,
            'line_section_on_line_1_other_effect': True,
            'line_section_on_line_2': True,
        }
        r = self.query_region(query.format(sa='F', cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['stop_schedules'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'line:1', 'line:2', 'F_1'}

    def test_route_schedule_impacted_by_line_section(self):
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        query = 'lines/{l}/departures?{cur}&{d}&{f}'

        # line:1
        scenario = {
            'line_section_on_line_1': True,
            'line_section_on_line_1_other_effect': True,
            'line_section_on_line_2': False,
        }
        r = self.query_region(query.format(l='line:1', cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:1:1', 'line:1', 'C_1', 'D_1', 'D_3', 'E_1'}

        # line:2
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': True,
        }
        r = self.query_region(query.format(l='line:2', cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert impacted_ids(d) == {'vj:2', 'line:2', 'B_1'}

        # line:3
        scenario = {
            'line_section_on_line_1': False,
            'line_section_on_line_1_other_effect': False,
            'line_section_on_line_2': False,
        }
        r = self.query_region(query.format(l='line:3', cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        for disruption, result in scenario.iteritems():
            assert result == (disruption in d)
        assert not impacted_ids(d)

    def test_line_impacted_by_line_section(self):
        # line_section_on_line_1
        scenario = {'line:1': True, 'line:2': False, 'line:3': False}

        for line, result in scenario.iteritems():
            assert result == self.has_disruption(ObjGetter('lines', line), 'line_section_on_line_1')

        # line_section_on_line_1_other_effect
        scenario = {'line:1': True, 'line:2': False, 'line:3': False}

        for line, result in scenario.iteritems():
            assert result == self.has_disruption(ObjGetter('lines', line), 'line_section_on_line_1_other_effect')

        # line_section_on_line_2
        scenario = {'line:1': False, 'line:2': True, 'line:3': False}

        for line, result in scenario.iteritems():
            assert result == self.has_disruption(ObjGetter('lines', line), 'line_section_on_line_2')
