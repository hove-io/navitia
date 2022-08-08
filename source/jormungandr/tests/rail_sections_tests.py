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
    def default_query(self, q, data_freshness=None, count=None, **kwargs):
        """
        query navitia with a current date in the publication period of the impacts
        """
        query = '{}?_current_datetime=20170101T100000'.format(q)
        if data_freshness:
            query += "&data_freshness={}".format(data_freshness)
        if count:
            query += "&count={}".format(count)

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
        assert len(get_not_null(r, 'disruptions')) == 9
        assert r['disruptions'][0]['id'] == 'rail_section_on_line1-1'
        assert r['disruptions'][1]['id'] == 'rail_section_on_line1-2'
        assert r['disruptions'][2]['id'] == 'rail_section_on_line1-3'
        assert r['disruptions'][3]['id'] == 'rail_section_on_line11'
        assert r['disruptions'][4]['id'] == 'rail_section_bis_on_line11'

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
            'stopF': True,
            'stopG': True,
            'stopH': True,
            'stopI': True,
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
            'stopG': True,
            'stopH': True,
            'stopI': True,
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
        assert len(get_not_null(r, 'disruptions')) == 9
        is_valid_rail_section_disruption(r['disruptions'][1])
        is_valid_rail_section_disruption(r['disruptions'][2])
        is_valid_rail_section_disruption(r['disruptions'][3])
        is_valid_rail_section_disruption(r['disruptions'][4])
        is_valid_rail_section_disruption(r['disruptions'][5])

    def test_terminus_schedules_impacted_by_rail_section(self):
        # terminus_schedules on line:1 gives us 3 rail_section impacts
        r = self.default_query('lines/line:1/terminus_schedules', data_freshness="base_schedule", count=100)
        assert len(get_not_null(r, 'disruptions')) == 3
        is_valid_rail_section_disruption(r['disruptions'][0])
        is_valid_rail_section_disruption(r['disruptions'][1])
        is_valid_rail_section_disruption(r['disruptions'][2])

        r = self.default_query('lines/line:2/terminus_schedules', data_freshness="base_schedule")
        assert len(r['disruptions']) == 0

        # terminus_schedules on line:100  with different stop_areas
        r = self.default_query('lines/line:100/terminus_schedules', data_freshness="base_schedule")
        assert len(get_not_null(r, 'disruptions')) == 1

        # No impact for stop_area from the start to rail_section.start_point
        r = self.default_query(
            'lines/line:100/stop_areas/stopAreaA1/terminus_schedules', data_freshness="base_schedule"
        )
        assert len(r['disruptions']) == 0

        r = self.default_query(
            'lines/line:100/stop_areas/stopAreaC1/terminus_schedules', data_freshness="base_schedule"
        )
        assert len(r['disruptions']) == 0

        # Impacts for all stop_area after rail_section.start_point
        r = self.default_query(
            'lines/line:100/stop_areas/stopAreaD1/terminus_schedules', data_freshness="base_schedule"
        )
        assert len(get_not_null(r, 'disruptions')) == 1

        r = self.default_query(
            'lines/line:100/stop_areas/stopAreaG1/terminus_schedules', data_freshness="base_schedule"
        )
        assert len(get_not_null(r, 'disruptions')) == 1
        r = self.default_query(
            'lines/line:100/stop_areas/stopAreaH1/terminus_schedules', data_freshness="base_schedule"
        )
        assert len(get_not_null(r, 'disruptions')) == 1

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

        # Test on line 'line:1' impacted with more than on rail_section with severity=NO_SERVICE
        # stopI -> stopG / base_schedule
        # There is no impact on stopI->stopG with vj:2
        scenario = {
            'rail_section_on_line1-1': False,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': False,
        }

        r = journeys(_from='stopI', to='stopG')
        assert get_used_vj(r) == [['vehicle_journey:vj:2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopE -> stopF / base_schedule
        # Disruption on rail_section_on_line1-1 should be present
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

        # stopF-> stopH / base_schedule
        # vehicle_journey used in the journey is impacted totally by the rail_section rail_section_on_line1-1 from stopC
        scenario = {
            'rail_section_on_line1-1': True,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': False,
        }
        r = journeys(_from='stopF', to='stopH')
        assert get_used_vj(r) == [['vehicle_journey:vj:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopF-> stopH / realtime
        # No journey as the section is completely impacted
        scenario = {
            'rail_section_on_line1-1': False,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': False,
        }
        q = journey_query(_from='stopF', to='stopH', data_freshness="realtime")
        r = self.query_region(q)
        assert get_used_vj(r) == [['vehicle_journey:vj:3']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopA -> stopB / base_schedule
        # No disruption should be informed on rail_section as drop_off_allowed on stopB
        scenario = {
            'rail_section_on_line1-1': False,
            'rail_section_on_line1-2': False,
            'rail_section_on_line1-3': False,
        }

        r = journeys(_from='stopA', to='stopB')
        assert get_used_vj(r) == [['vehicle_journey:vj:1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # END stopM -> stopO / Base_schedule
        # Section M to O impacted with rail_section rail_section_on_line1-2
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

        # stopA -> stopR / base_schedule
        # We should have a disruption on rail_section_on_line1-3
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

        # Test on line 'line:11' impacted with a rail_section with severity=REDUCED_SERVICE
        # since 2017-01-01 until 2017-01-05
        # Impacted from CC to FF with blocked stops DD and EE (vehicle_journey:vj:11-1)
        # Impacted from FF to CC with blocked stops EE and DD (vehicle_journey:vj:11-2)
        # stopAA-> stopBB / base_schedule: No disruption
        scenario = {
            'rail_section_on_line11': False,
        }

        r = journeys(_from='stopAA', to='stopBB')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopBB-> stopAA / base_schedule: No disruption
        r = journeys(_from='stopBB', to='stopAA')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopBB / realtime: No disruption
        r = journeys(_from='stopAA', to='stopBB', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:0:rail_section_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopCC / base_schedule: No disruption
        scenario = {
            'rail_section_on_line11': False,
        }

        r = journeys(_from='stopAA', to='stopCC')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopEE / base_schedule: A disruption to display
        scenario = {
            'rail_section_on_line11': True,
        }

        r = journeys(_from='stopAA', to='stopEE')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:11-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopEE-> stopAA / base_schedule: A disruption to display
        r = journeys(_from='stopEE', to='stopAA')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:11-2'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopEE / realtime: No journey as the stop stopEE is in the section completely impacted
        q = journey_query(_from='stopAA', to='stopEE', data_freshness="realtime")
        r = self.query_region(q)
        assert "journeys" not in r

        # stopAA-> stopFF / base_schedule: No disruption
        # We should not have disruption on rail_section
        scenario = {
            'rail_section_on_line11': False,
        }

        r = journeys(_from='stopAA', to='stopFF')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopFF-> stopAA / base_schedule: No disruption
        r = journeys(_from='stopFF', to='stopAA')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopFF / realtime: No disruption to display
        r = journeys(_from='stopAA', to='stopGG', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:0:rail_section_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopCC-> stopFF / base_schedule: No disruption
        scenario = {
            'rail_section_on_line11': False,
        }

        r = journeys(_from='stopCC', to='stopFF')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopCC-> stopFF / realtime: No disruption
        r = journeys(_from='stopCC', to='stopFF', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:0:rail_section_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopDD-> stopFF / base_schedule: A disruption to display
        scenario = {
            'rail_section_on_line11': True,
        }

        r = journeys(_from='stopDD', to='stopFF')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:11-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopDD-> stopFF / realtime: No journey as the stop stopEE is in the section completely impacted
        q = journey_query(_from='stopDD', to='stopFF', data_freshness="realtime")
        r = self.query_region(q)
        assert "journeys" not in r

        # stopGG-> stopII / Base_schedule: No disruption
        scenario = {
            'rail_section_on_line11': False,
        }

        r = journeys(_from='stopGG', to='stopII')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopGG-> stopII / realtime: No disruption
        r = journeys(_from='stopGG', to='stopII', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:0:rail_section_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # Test on line 'line:100' impacted with a rail_section with severity=NO_SERVICE
        # Impacted from C1 to E1 with blocked stops D1 and E1
        # All the stops from stopDD to stopII are impacted
        # stopA1-> stopB1 / Base_schedule: No disruption
        scenario = {
            'rail_section_on_line100': False,
        }

        r = journeys(_from='stopA1', to='stopB1')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:100-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopA1-> stopC1 / base_schedule: No disruption as we can get off at the station stopC1
        r = journeys(_from='stopA1', to='stopC1')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:100-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopA1-> stopE1 / base_schedule: A disruption to display as all the stops from stopD1 are impacted
        scenario = {
            'rail_section_on_line100': True,
        }
        r = journeys(_from='stopA1', to='stopE1')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:100-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:100-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopG1-> stopI1 / base_schedule: A disruption to display as all the stops from stopD1 are impacted
        r = journeys(_from='stopG1', to='stopI1')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:100-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:100-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopG1-> stopI1 / realtime: There is no_solution as all the stops from stopD1 are impacted
        q = journey_query(_from='stopG1', to='stopI1', data_freshness="realtime")
        r = self.query_region(q)
        assert "journeys" not in r

        # Test on line 'line:101' impacted with two rail_sections and severity=NO_SERVICE
        # Stops: P1, Q1, R1, S1, T1, U1, V1
        # Impacted from S1 to V1 with blocked stops T1 and U1
        # Impacted from V1 to S1 with blocked stops U1 and T1
        # All the stops from stop to stopU1 toward stopP1 are impacted
        # stopP1-> stopR1 / Base_schedule: No disruption
        scenario = {
            'rail_section_on_line101': False,
        }

        r = journeys(_from='stopP1', to='stopR1')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:101-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopP1-> stopT1 / Base_schedule: A disruption to display on vj:101-1
        scenario = {
            'rail_section_on_line101': True,
        }

        r = journeys(_from='stopP1', to='stopT1')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:101-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:101-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopR1-> stopP1 / Base_schedule: A disruption to display on vj:101-2
        scenario = {
            'rail_section_on_line101': True,
        }

        r = journeys(_from='stopR1', to='stopP1')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:101-2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:101-2'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # denfert-> cdg / Base_schedule: A disruption to display on vj:rer_b_nord
        scenario = {
            'rail_section_on_rer_b': True,
            'line_section_on_rer_b_port_royal': False,
        }
        r = journeys(_from='denfert_area', to='cdg_area')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:rer_b_nord']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # denfert-> port_royal / Base_schedule: A disruption to display on vj:rer_b_nord
        scenario = {
            'rail_section_on_rer_b': False,
            'line_section_on_rer_b_port_royal': True,
        }
        r = journeys(_from='denfert_area', to='port_royal_area')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:rer_b_nord']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # cdg-> denfert / Base_schedule: A disruption to display on vj:rer_b_sud
        scenario = {
            'rail_section_on_rer_b': True,
            'line_section_on_rer_b_port_royal': False,
        }
        r = journeys(_from='cdg_area', to='denfert_area')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:rer_b_sud']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # cdg-> port_royal / Base_schedule: Two disruption to display on vj:rer_b_sud
        scenario = {
            'rail_section_on_rer_b': True,
            'line_section_on_rer_b_port_royal': True,
        }
        r = journeys(_from='cdg_area', to='port_royal_area')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:rer_b_sud']]
        d = get_all_element_disruptions(r['journeys'], r)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

    def test_journeys_with_rail_section_detour(self):
        """
        for /journeys, we should display a rail section disruption only if we use an impacted section

        We do all the calls as base_schedule to see the disruption but not be really impacted by them
        """
        # with this departure time we should take 'vj:1:1' that is impacted
        date = '20170107T080000'
        current = '_current_datetime=20170107T080000'

        def journey_query(_from, to, data_freshness="base_schedule"):
            return 'journeys?from={fr}&to={to}&datetime={dt}&{cur}&data_freshness={data_freshness}'.format(
                fr=_from, to=to, dt=date, cur=current, data_freshness=data_freshness
            )

        def journeys(_from, to, data_freshness="base_schedule"):
            q = journey_query(_from=_from, to=to, data_freshness=data_freshness)
            r = self.query_region(q)
            self.is_valid_journey_response(r, q)
            return r

        # Test on line 'line:11' impacted with a rail_section with severity=DETOUR
        # since 2017-01-06 until 2017-01-10
        # Impacted from CC to FF with blocked stops DD and EE (vehicle_journey:vj:11-1)
        # Impacted from FF to CC with blocked stops EE and DD (vehicle_journey:vj:11-2)
        # stopAA-> stopBB / base_schedule: No disruption
        scenario = {
            'rail_section_bis_on_line11': False,
        }

        r = journeys(_from='stopAA', to='stopBB')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopBB-> stopAA / base_schedule: No disruption
        r = journeys(_from='stopBB', to='stopAA')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopBB / realtime: No disruption
        r = journeys(_from='stopAA', to='stopBB', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:1:rail_section_bis_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopCC / base_schedule: No disruption
        scenario = {
            'rail_section_bis_on_line11': False,
        }

        r = journeys(_from='stopAA', to='stopCC')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopEE / base_schedule: A disruption to display
        scenario = {
            'rail_section_bis_on_line11': True,
        }

        r = journeys(_from='stopAA', to='stopEE')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:11-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopEE-> stopAA / base_schedule: A disruption to display
        r = journeys(_from='stopEE', to='stopAA')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:11-2'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopEE / realtime: No journey as the stop stopEE is in the section completely impacted
        q = journey_query(_from='stopAA', to='stopEE', data_freshness="realtime")
        r = self.query_region(q)
        assert "journeys" not in r

        # stopAA-> stopFF / base_schedule: No disruption
        # We should not have disruption on rail_section
        scenario = {
            'rail_section_on_line11': False,
        }

        r = journeys(_from='stopAA', to='stopFF')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopFF-> stopAA / base_schedule: No disruption
        r = journeys(_from='stopFF', to='stopAA')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-2']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopAA-> stopFF / realtime: No disruption to display
        r = journeys(_from='stopAA', to='stopGG', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:1:rail_section_bis_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopCC-> stopFF / base_schedule: No disruption
        scenario = {
            'rail_section_bis_on_line11': False,
        }

        r = journeys(_from='stopCC', to='stopFF')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopCC-> stopFF / realtime: No disruption
        r = journeys(_from='stopCC', to='stopFF', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:1:rail_section_bis_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopDD-> stopFF / base_schedule: A disruption to display
        scenario = {
            'rail_section_bis_on_line11': True,
        }

        r = journeys(_from='stopDD', to='stopFF')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert impacted_headsigns(d) == {'vj:11-1'}
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopDD-> stopFF / realtime: No journey as the stop stopEE is in the section completely impacted
        q = journey_query(_from='stopDD', to='stopFF', data_freshness="realtime")
        r = self.query_region(q)
        assert "journeys" not in r

        # stopGG-> stopII / Base_schedule: No disruption
        scenario = {
            'rail_section_bis_on_line11': False,
        }

        r = journeys(_from='stopGG', to='stopII')
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

        # stopGG-> stopII / realtime: No disruption
        r = journeys(_from='stopGG', to='stopII', data_freshness="realtime")
        assert len(r["journeys"]) == 1
        assert get_used_vj(r) == [['vehicle_journey:vj:11-1:Adapted:1:rail_section_bis_on_line11']]
        d = get_all_element_disruptions(r['journeys'], r)
        assert not impacted_headsigns(d)
        for disruption, result in scenario.items():
            assert result == (disruption in d)

    def test_traffic_reports_on_stop_areas(self):
        """
        we should be able to find the related rail section disruption with /traffic_reports
        """

        # rail_section_on_line1-1
        scenario = {
            'stopAreaA': False,
            'stopAreaB': True,
            'stopAreaC': True,
            'stopAreaD': True,
            'stopAreaE': True,
            'stopAreaF': True,
            'stopAreaG': True,
            'stopAreaH': True,
            'stopAreaI': True,
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
            'stopAreaG': True,
            'stopAreaH': True,
            'stopAreaI': True,
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

        # rail_section_on_line100
        scenario = {
            'stopAreaA1': False,
            'stopAreaB1': False,
            'stopAreaC1': True,
            'stopAreaD1': True,
            'stopAreaE1': True,
            'stopAreaF1': True,
            'stopAreaG1': True,
            'stopAreaH1': True,
            'stopAreaI1': True,
        }

        for sa, result in scenario.items():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa), 'rail_section_on_line100'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'rail_section_on_line100',
                ObjGetter('stop_areas', sa),
            )

    def test_traffic_reports_on_networks(self):
        for disruption_label in [
            'rail_section_on_line1-1',
            'rail_section_on_line1-2',
            'rail_section_on_line1-3',
            'rail_section_on_line11',
            'rail_section_bis_on_line11',
            'rail_section_on_line100',
        ]:
            assert self.has_dis('networks/base_network/traffic_reports', disruption_label)

        r = self.default_query('traffic_reports')
        # only one network (base_network) is disrupted
        assert len(r['traffic_reports']) == 1
        assert len(r['traffic_reports'][0]['stop_areas']) == 41

        for sa in r['traffic_reports'][0]['stop_areas']:
            if sa['id'] in [
                'stopAreaB',
                'stopAreaC',
                'stopAreaD',
                'stopAreaCC',
                'stopAreaDD',
                'stopAreaEE',
                'stopAreaFF',
            ]:
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
                'stopAreaGG',
                'stopAreaHH',
                'stopAreaII',
                'stopAreaC1',
                'stopAreaD1',
                'stopAreaE1',
                'stopAreaF1',
                'stopAreaG1',
                'stopAreaH1',
                'stopAreaI1',
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
            'stopF': True,
            'stopG': True,
            'stopH': True,
            'stopI': True,
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
            'stopG': True,
            'stopH': True,
            'stopI': True,
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

        # Impact rail_section_on_line100 with severity NO_SERVICE impacts all the stop_points from
        # rail_section.start_point
        scenario = {
            'stopA1': False,
            'stopB1': False,
            'stopC1': True,
            'stopD1': True,
            'stopE1': True,
            'stopF1': True,
            'stopG1': True,
            'stopH1': True,
            'stopI1': True,
        }
        for sp, result in scenario.items():
            sa = sp[0:4] + 'Area' + sp[-2:]
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'rail_section_on_line100'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'rail_section_on_line100',
                ObjGetter('stop_areas', sa),
            )

        # Impact rail_section_on_line11 with severity REDUCED_SERVICE impacts all the stop_points from
        # rail_section.start_point
        scenario = {
            'stopAA': False,
            'stopBB': False,
            'stopCC': True,
            'stopDD': True,
            'stopEE': True,
            'stopFF': True,
            'stopGG': False,
            'stopHH': False,
            'stopII': False,
        }
        for sp, result in scenario.items():
            sa = sp[0:4] + 'Area' + sp[-2:]
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp), 'rail_section_on_line11'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'rail_section_on_line11',
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

    def test_stops_schedules_and_departures_impacted_by_rail_section(self):
        """
        For /departures, arrivals and stop_schedules we display rail section impact
        if the stop_point is part of a rail section impact
        """
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        query = 'stop_areas/{sa}/{q}?{cur}&{d}&{f}'

        for q in ['departures', 'arrivals', 'stop_schedules']:
            # stopAreaA
            scenario = {
                'rail_section_on_line1-1': False,
                'rail_section_on_line1-2': False,
                'rail_section_on_line1-3': False,
            }
            r = self.query_region(query.format(sa='stopAreaA', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            assert not impacted_headsigns(d)

            # stopAreaC
            scenario = {
                'rail_section_on_line1-1': True,
                'rail_section_on_line1-2': True,
                'rail_section_on_line1-3': False,
            }
            r = self.query_region(query.format(sa='stopAreaC', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert impacted_headsigns(d) == {'vj:1', 'vj:4'}
            if q == 'stop_schedules':
                assert impacted_ids(d) == {
                    'vehicle_journey:vj:1',
                    'vehicle_journey:vj:4',
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
            if q in ['departures', 'arrivals']:
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

            # Test on line 'line:11' impacted with a rail_section with severity=REDUCED_SERVICE
            # Impacted from CC to EE with blocked stops DD and EE
            # stopBB: No disruption
            scenario = {
                'rail_section_on_line11': False,
            }
            r = self.query_region(query.format(sa='stopAreaBB', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert not impacted_headsigns(d)
            if q == 'stop_schedules':
                assert not impacted_ids(d)

            # stopCC: No disruption
            scenario = {
                'rail_section_on_line11': False,
            }
            r = self.query_region(query.format(sa='stopAreaCC', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert not impacted_headsigns(d)
            if q == 'stop_schedules':
                assert not impacted_ids(d)

            # stopDD: as stop_areas stopAreaDD and stopAreaEE are impacted a disruption to display
            scenario = {
                'rail_section_on_line11': True,
            }
            r = self.query_region(query.format(sa='stopAreaDD', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert impacted_headsigns(d) == {'vj:11-1', 'vj:11-2'}
            if q == 'stop_schedules':
                assert impacted_ids(d) == {'vehicle_journey:vj:11-1', 'vehicle_journey:vj:11-2'}

            # stopFF: No disruption
            scenario = {
                'rail_section_on_line11': False,
            }
            r = self.query_region(query.format(sa='stopAreaFF', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert not impacted_headsigns(d)
            if q == 'stop_schedules':
                assert not impacted_ids(d)

            # Test on line 'line:100' impacted with a rail_section with severity=NO_SERVICE
            # Impacted from C1 to E1 with blocked stops D1 and E1
            # All the stops from stopDD to stopII are impacted
            # stopB1 / base_schedule: No disruption
            scenario = {
                'rail_section_on_line100': False,
            }
            r = self.query_region(query.format(sa='stopAreaB1', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert not impacted_headsigns(d)
            if q == 'stop_schedules':
                assert not impacted_ids(d)

            # stopC1: No disruption
            scenario = {
                'rail_section_on_line100': False,
            }
            r = self.query_region(query.format(sa='stopAreaC1', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert not impacted_headsigns(d)
            if q == 'stop_schedules':
                assert not impacted_ids(d)

            # stopD1: as all stop_areas from stopAreaD1 are impacted a disruption to display
            scenario = {
                'rail_section_on_line100': True,
            }
            r = self.query_region(query.format(sa='stopAreaD1', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert impacted_headsigns(d) == {'vj:100-1'}
            if q == 'stop_schedules':
                assert impacted_ids(d) == {'vehicle_journey:vj:100-1'}

            # stopG1: as all stop_areas from stopAreaD1 are impacted a disruption to display
            scenario = {
                'rail_section_on_line100': True,
            }
            r = self.query_region(query.format(sa='stopAreaG1', cur=cur, d=dt, f=fresh, q=q))
            d = get_all_element_disruptions(r[q], r)
            for disruption, result in scenario.items():
                assert result == (disruption in d)
            if q in ['departures', 'arrivals']:
                assert impacted_headsigns(d) == {'vj:100-1'}
            if q == 'stop_schedules':
                assert impacted_ids(d) == {'vehicle_journey:vj:100-1'}
