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
from __future__ import absolute_import, print_function, unicode_literals, division
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import (
    ObjGetter,
    has_disruption,
    get_not_null,
    is_valid_line_section_disruption,
    is_valid_rail_section_disruption,
    get_used_vj,
    get_all_element_disruptions,
    impacted_ids,
    impacted_headsigns,
)
import pytest


@dataset({"rail_sections_test": {}})
class TestRailSections(AbstractTestFixture):
    def default_query(self, q, data_freshness=None, **kwargs):
        """
        query navitia with a current date in the publication period of the impacts
        """
        query = '{}?_current_datetime=20170101T100000'.format(q)
        if data_freshness:
            query += "&data_freshness={}".format(data_freshness)
        return self.query_region(query, **kwargs)

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

    def test_disruption_api_with_rail_section(self):
        r = self.default_query('disruptions')
        assert len(get_not_null(r, 'disruptions')) == 4
        assert r['disruptions'][0]['id'] == 'rail_section_on_line1-1'
        assert r['disruptions'][1]['id'] == 'rail_section_on_line1-2'
        assert r['disruptions'][2]['id'] == 'rail_section_on_line1-3'
        assert r['disruptions'][3]['id'] == 'rail_section_on_line11'

    def test_stop_points_api_with_rail_section(self):
        r = self.default_query('stop_points/stopC')
        assert len(get_not_null(r, 'disruptions')) == 2
        is_valid_rail_section_disruption(r['disruptions'][0])
        is_valid_rail_section_disruption(r['disruptions'][1])

    def test_on_stop_areas(self):
        """
        the rail section disruption is not linked to a stop area, we cannot directly find our disruption
        """
        for disruption_label in [
            'rail_section_on_line1-1',
            'rail_section_on_line1-2',
            'rail_section_on_line1-3',
        ]:
            for stop_area in [
                'stopAreaA',
                'stopAreaB',
                'stopAreaC',
                'stopAreaD',
                'stopAreaE',
                'stopAreaF',
                'stopAreaG',
                'stopAreaH',
                'stopAreaI',
                'stopAreaJ',
                'stopAreaK',
                'stopAreaL',
                'stopAreaM',
                'stopAreaN',
                'stopAreaO',
                'stopAreaP',
                'stopAreaQ',
                'stopAreaR',
            ]:
                assert not self.has_disruption(ObjGetter('stop_areas', stop_area), disruption_label)

    def test_on_stop_points(self):
        """
        the rail section disruption should be linked to the impacted stop_points
        """
        scenario = {
            'stopA': False,
            'stopB': True,
            'stopC': True,
            'stopD': True,
            'stopE': True,
            'stopF': False,
            'stopG': False,
            'stopH': False,
            'stopI': False,
            'stopJ': False,
            'stopK': False,
            'stopL': False,
            'stopM': False,
            'stopN': False,
            'stopO': False,
            'stopP': False,
            'stopQ': False,
            'stopR': False,
        }
        for stop_point, result in scenario.items():
            assert result == self.has_disruption(ObjGetter('stop_points', stop_point), 'rail_section_on_line1-1')

        scenario = {
            'stopA': False,
            'stopB': True,
            'stopC': True,
            'stopD': True,
            'stopE': False,
            'stopF': False,
            'stopG': False,
            'stopH': False,
            'stopI': False,
            'stopJ': False,
            'stopK': False,
            'stopL': False,
            'stopM': True,
            'stopN': True,
            'stopO': True,
            'stopP': False,
            'stopQ': False,
            'stopR': False,
        }
        for stop_point, result in scenario.items():
            assert result == self.has_disruption(ObjGetter('stop_points', stop_point), 'rail_section_on_line1-2')

        scenario = {
            'stopA': False,
            'stopB': False,
            'stopC': False,
            'stopD': False,
            'stopE': False,
            'stopF': False,
            'stopG': False,
            'stopH': False,
            'stopI': False,
            'stopJ': False,
            'stopK': False,
            'stopL': False,
            'stopM': False,
            'stopN': False,
            'stopO': False,
            'stopP': True,
            'stopQ': True,
            'stopR': True,
        }
        for stop_point, result in scenario.items():
            assert result == self.has_disruption(ObjGetter('stop_points', stop_point), 'rail_section_on_line1-3')

    def test_on_vehicle_journeys(self):
        """
        the rail section disruption should be linked to the impacted vehicle journeys
        """

        scenario = {
            'vehicle_journey:vj:1': True,
            'vehicle_journey:vj:2': False,
            'vehicle_journey:vj:3': False,
            'vehicle_journey:vj:4': False,
            'vehicle_journey:vj:5': False,
            'vehicle_journey:vj:6': False,
        }
        for vj, result in scenario.items():
            assert result == self.has_disruption(ObjGetter('vehicle_journeys', vj), 'rail_section_on_line1-1')

        scenario = {
            'vehicle_journey:vj:1': False,
            'vehicle_journey:vj:2': False,
            'vehicle_journey:vj:3': False,
            'vehicle_journey:vj:4': True,
            'vehicle_journey:vj:5': False,
            'vehicle_journey:vj:6': False,
        }
        for vj, result in scenario.items():
            assert result == self.has_disruption(ObjGetter('vehicle_journeys', vj), 'rail_section_on_line1-2')

        scenario = {
            'vehicle_journey:vj:1': False,
            'vehicle_journey:vj:2': False,
            'vehicle_journey:vj:3': False,
            'vehicle_journey:vj:4': False,
            'vehicle_journey:vj:5': True,
            'vehicle_journey:vj:6': False,
        }
        for vj, result in scenario.items():
            assert result == self.has_disruption(ObjGetter('vehicle_journeys', vj), 'rail_section_on_line1-3')

    def test_line_impacted_by_rail_section(self):
        assert True == self.has_disruption(ObjGetter('lines', 'line:1'), 'rail_section_on_line1-1')
        assert True == self.has_disruption(ObjGetter('lines', 'line:1'), 'rail_section_on_line1-2')
        assert True == self.has_disruption(ObjGetter('lines', 'line:1'), 'rail_section_on_line1-3')
        assert False == self.has_disruption(ObjGetter('lines', 'line:2'), 'rail_section_on_line1-1')
        assert False == self.has_disruption(ObjGetter('lines', 'line:2'), 'rail_section_on_line1-2')
        assert False == self.has_disruption(ObjGetter('lines', 'line:2'), 'rail_section_on_line1-3')

    def test_line_reports_impacted_by_rail_section(self):
        r = self.default_query('line_reports')
        assert len(get_not_null(r, 'disruptions')) == 4
        is_valid_rail_section_disruption(r['disruptions'][0])
        is_valid_rail_section_disruption(r['disruptions'][1])
        is_valid_rail_section_disruption(r['disruptions'][2])
        is_valid_rail_section_disruption(r['disruptions'][3])

    def test_terminus_schedules_impacted_by_rail_section(self):

        r = self.default_query('lines/line:1/terminus_schedules', data_freshness="base_schedule")
        assert len(get_not_null(r, 'disruptions')) == 3
        is_valid_rail_section_disruption(r['disruptions'][0])
        is_valid_rail_section_disruption(r['disruptions'][1])
        is_valid_rail_section_disruption(r['disruptions'][2])

        r = self.default_query('lines/line:2/terminus_schedules', data_freshness="base_schedule")
        assert len(r['disruptions']) == 0

    def test_journeys_with_rail_section(self):
        """
        for /journeys, we should display a rail section disruption only if we use an impacted section

        We do all the calls as base_schedule to see the disruption but not be really impacted by them

        In this test we leave at 08:00 so we'll use vj:1:1 (that is impacted) during the application
        period of the impact
        """
        # with this departure time we should take 'vj:1:1' that is impacted
        date = '20170102T080000'
        current = '_current_datetime=20170101T080000'

        def journey_query(_from, to, data_freshness="base_schedule"):
            return 'journeys?from={fr}&to={to}&datetime={dt}&{cur}&data_freshness={data_freshness}'.format(
                fr=_from, to=to, dt=date, cur=current, data_freshness=data_freshness
            )

        def journeys(_from, to, data_freshness="base_schedule"):
            q = journey_query(_from=_from, to=to, data_freshness=data_freshness)
            r = self.query_region(q)
            self.is_valid_journey_response(r, q)
            return r

        # stopI -> stopG
        scenario = {
            'rail_section_on_line1-1': False,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': False,
        }

        # There is no impact on stopI->stopG with vj:2
        r = journeys(_from='stopI', to='stopG')
        assert get_used_vj(r) == [['vehicle_journey:vj:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopE -> stopF
        scenario = {
            'rail_section_on_line1-1': True,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': False,
        }

        r = journeys(_from='stopE', to='stopF')
        assert get_used_vj(r) == [['vehicle_journey:vj:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopM -> stopO
        scenario = {
            'rail_section_on_line1-1': False,
            'rail_section_on_line1-2': True,
            'rail_section_on_line1-3': False,
        }

        r = journeys(_from='stopM', to='stopO')
        assert get_used_vj(r) == [['vehicle_journey:vj:4']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:4'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopA -> stopR
        scenario = {
            'rail_section_on_line1-1': False,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': True,
        }

        r = journeys(_from='stopA', to='stopR')
        assert get_used_vj(r) == [['vehicle_journey:vj:5']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:5'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopGG
        # Base_schedule
        scenario = {
            'rail_section_on_line11': True,
        }

        r = journeys(_from='stopAA', to='stopGG')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:11-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

    def test_traffic_reports_on_stop_areas(self):
        """
        we should be able to find the related rail section disruption with /traffic_report
        """

        # rail_section_on_line1-1
        scenario = {
            'stopAreaA': False,
            'stopAreaB': True,
            'stopAreaC': True,
            'stopAreaD': True,
            'stopAreaE': True,
            'stopAreaF': False,
            'stopAreaG': False,
            'stopAreaH': False,
            'stopAreaI': False,
        }

        for sa, result in scenario.items():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'rail_section_on_line1-1'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'rail_section_on_line1-1',
                ObjGetter('stop_areas', sa),
            )

        # rail_section_on_line1-2
        scenario = {
            'stopAreaA': False,
            'stopAreaB': True,
            'stopAreaC': True,
            'stopAreaD': True,
            'stopAreaM': True,
            'stopAreaN': True,
            'stopAreaO': True,
            'stopAreaG': False,
            'stopAreaH': False,
            'stopAreaI': False,
        }

        for sa, result in scenario.items():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'rail_section_on_line1-2'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'rail_section_on_line1-2',
                ObjGetter('stop_areas', sa),
            )

        # rail_section_on_line1-3
        scenario = {
            'stopAreaA': False,
            'stopAreaB': False,
            'stopAreaC': False,
            'stopAreaP': True,
            'stopAreaQ': True,
            'stopAreaR': True,
        }

        for sa, result in scenario.items():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'rail_section_on_line1-3'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'rail_section_on_line1-3',
                ObjGetter('stop_areas', sa),
            )

    def test_traffic_reports_on_networks(self):
        for disruption_label in [
            'rail_section_on_line1-1',
            'rail_section_on_line1-2',
            'rail_section_on_line1-3',
        ]:
            assert self.has_dis('networks/base_network/traffic_reports', disruption_label)

        r = self.default_query('traffic_reports')
        # only one network (base_network) is disrupted
        assert len(r['traffic_reports']) == 1
        assert len(r['traffic_reports'][0]['stop_areas']) == 13

        for sa in r['traffic_reports'][0]['stop_areas']:
            if sa['id'] in ['stopAreaB', 'stopAreaC', 'stopAreaD']:
                assert len(sa['links']) == 2
        for sa in r['traffic_reports'][0]['stop_areas']:
            if sa['id'] in [
                'stopAreaE',
                'stopAreaM',
                'stopAreaN',
                'stopAreaO',
                'stopAreaP',
                'stopAreaQ',
                'stopAreaR',
                'stopAreaDD',
                'stopAreaEE',
                'stopAreaFF',
            ]:
                assert len(sa['links']) == 1

    def test_traffic_reports_on_vjs(self):
        """
        for /traffic_reports on vjs it's a bit the same as the lines
        we display a rail section disruption if it impacts the stops of the vj
        """
        # all VJ are imapcted because they all have stopB with an impact
        scenario = {
            'vehicle_journey:vj:1': True,
            'vehicle_journey:vj:2': True,
            'vehicle_journey:vj:3': True,
            'vehicle_journey:vj:4': True,
            'vehicle_journey:vj:5': True,
            'vehicle_journey:vj:6': True,
        }

        for disruption_label in ['rail_section_on_line1-1', 'rail_section_on_line1-2']:
            for vj, result in scenario.items():
                assert result == self.has_dis('vehicle_journeys/{}/traffic_reports'.format(vj), disruption_label)

        scenario = {
            'vehicle_journey:vj:1': False,
            'vehicle_journey:vj:2': False,
            'vehicle_journey:vj:3': False,
            'vehicle_journey:vj:4': False,
            'vehicle_journey:vj:5': True,
            'vehicle_journey:vj:6': True,
        }
        for vj, result in scenario.items():
            assert result == self.has_dis(
                'vehicle_journeys/{}/traffic_reports'.format(vj), 'rail_section_on_line1-3'
            )

    def test_traffic_reports_on_stop_points(self):
        """
        for /traffic_reports on stop_points there is a rail section disruption link if it impacts the stop_point
        """

        # rail_section_on_line1-1
        scenario = {
            'stopA': False,
            'stopB': True,
            'stopC': True,
            'stopD': True,
            'stopE': True,
            'stopF': False,
            'stopG': False,
            'stopH': False,
            'stopI': False,
            'stopJ': False,
            'stopK': False,
            'stopL': False,
            'stopM': False,
            'stopN': False,
            'stopO': False,
            'stopP': False,
            'stopQ': False,
            'stopR': False,
        }
        for sp, result in scenario.items():
            sa = sp[0:4] + 'Area' + sp[-1]
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'rail_section_on_line1-1'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'rail_section_on_line1-1',
                ObjGetter('stop_areas', sa),
            )

        # rail_section_on_line1-2
        scenario = {
            'stopA': False,
            'stopB': True,
            'stopC': True,
            'stopD': True,
            'stopE': False,
            'stopF': False,
            'stopG': False,
            'stopH': False,
            'stopI': False,
            'stopJ': False,
            'stopK': False,
            'stopL': False,
            'stopM': True,
            'stopN': True,
            'stopO': True,
            'stopP': False,
            'stopQ': False,
            'stopR': False,
        }
        for sp, result in scenario.items():
            sa = sp[0:4] + 'Area' + sp[-1]
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'rail_section_on_line1-2'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'rail_section_on_line1-2',
                ObjGetter('stop_areas', sa),
            )

        # rail_section_on_line1-3
        scenario = {
            'stopA': False,
            'stopB': False,
            'stopC': False,
            'stopD': False,
            'stopE': False,
            'stopF': False,
            'stopG': False,
            'stopH': False,
            'stopI': False,
            'stopJ': False,
            'stopK': False,
            'stopL': False,
            'stopM': False,
            'stopN': False,
            'stopO': False,
            'stopP': True,
            'stopQ': True,
            'stopR': True,
        }
        for sp, result in scenario.items():
            sa = sp[0:4] + 'Area' + sp[-1]
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'rail_section_on_line1-3'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'rail_section_on_line1-3',
                ObjGetter('stop_areas', sa),
            )

    def test_traffic_reports_on_routes(self):
        """
        for routes since we display the impacts on all the stops (but we do not display a route object)
        we display the disruption even if the route has not been directly impacted
        """
        scenario = {
            'route1': True,
            'route2': True,
            'route3': True,
            'route4': True,
            'route5': True,
            'route6': True,
            'route2-1': False,
        }
        for route, result in scenario.items():
            assert result == self.has_dis('routes/{}/traffic_reports'.format(route), 'rail_section_on_line1-1')

        scenario = {
            'route1': True,
            'route2': True,
            'route3': True,
            'route4': True,
            'route5': True,
            'route6': True,
            'route2-1': False,
        }
        for route, result in scenario.items():
            assert result == self.has_dis('routes/{}/traffic_reports'.format(route), 'rail_section_on_line1-2')

        scenario = {
            'route1': False,
            'route2': False,
            'route3': False,
            'route4': False,
            'route5': True,
            'route6': True,
            'route2-1': False,
        }
        for route, result in scenario.items():
            assert result == self.has_dis('routes/{}/traffic_reports'.format(route), 'rail_section_on_line1-3')

    def test_route_schedule_impacted_by_rail_section(self):
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        query = 'lines/{l}/departures?{cur}&{d}&{f}'

        # line:1
        scenario = {
            'rail_section_on_line1-1': True,
            'rail_section_on_line1-2': True,
            'rail_section_on_line1-3': True,
        }
        r = self.query_region(query.format(l='line:1', cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        for disruption, result in scenario.items():
            assert result == (disruption in d)
        assert impacted_headsigns(d) == {'vj:1', 'vj:4', 'vj:5'}

        # line:2
        scenario = {
            'rail_section_on_line1-1': False,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': False,
        }
        r = self.query_region(query.format(l='line:2', cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

    def test_stops_schedules_and_departures_impacted_by_line_section(self):
        """
        For /departures we display rail section impact if the stoppoint is part of a line section impact
        """
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        query = 'stop_areas/{sa}/{q}?{cur}&{d}&{f}'

        for q in ['departures', 'stop_schedules']:
            # stopAreaA
            scenario = {
                'rail_section_on_line1-1': True,
                'rail_section_on_line1-2': True,
                'rail_section_on_line1-3': True,
            }
            r = self.query_region(query.format(sa='stopAreaA', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q == 'departures':
                assert impacted_headsigns(d) == {'vj:1', 'vj:4', 'vj:5'}
            if q == 'stop_schedules':
                assert impacted_ids(d) == {
                    'vehicle_journey:vj:1',
                    'vehicle_journey:vj:4',
                    'vehicle_journey:vj:5',
                }

            # stopAreaM
            scenario = {
                'rail_section_on_line1-1': False,
                'rail_section_on_line1-2': True,
                'rail_section_on_line1-3': False,
            }
            r = self.query_region(query.format(sa='stopAreaM', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q == 'departures':
                assert impacted_headsigns(d) == {'vj:4'}
            if q == 'stop_schedules':
                assert impacted_ids(d) == {'vehicle_journey:vj:4'}

            # stopAreaQ
            scenario = {
                'rail_section_on_line1-1': False,
                'rail_section_on_line1-2': False,
                'rail_section_on_line1-3': True,
            }
            r = self.query_region(query.format(sa='stopAreaQ', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q == 'departures':
                assert impacted_headsigns(d) == {'vj:5'}
            if q == 'stop_schedules':
                assert impacted_ids(d) == {'vehicle_journey:vj:5'}
