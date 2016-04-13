#pragma once

#include "time_tables/departure_boards.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/pt_data.h"
#include "kraken/realtime.h"

namespace ntest = navitia::test;

/**
 * basic data set for departure board
 *
 * Also used with realtime proxy
 *
 *          A:s
 *           v RouteA
 *           v
 *           v
 *         |-----------|      RouteB
 *B:s > > >| S1> > > > |> > > > B:e
 *         | v         |
 *         | v         |
 *         | S2        |
 *         |-----------|
 *           v         SA1
 *           v
 *           v
 *          A:e
 * Route C:
 * C:S0 > C:S1 > C:S2
 *
 * Routes J, K and L pass at stop point S42
 */
struct departure_board_fixture {
    ed::builder b;
    departure_board_fixture(): b("20160101") {
        b.sa("SA1")("S1")("S2");

        b.vj("A").uri("A:vj1")("A:s", "08:00"_t)("S1", "09:00"_t)("S2", "10:00"_t)("A:e", "11:00"_t);
        b.vj("A").uri("A:vj2")("A:s", "09:00"_t)("S1", "10:00"_t)("S2", "11:00"_t)("A:e", "12:00"_t);
        b.vj("A").uri("A:vj3")("A:s", "10:00"_t)("S1", "11:00"_t)("S2", "12:00"_t)("A:e", "13:00"_t);

        b.vj("B").uri("B:vj1")("B:s", "10:30"_t)("S1", "11:30"_t)("B:e", "12:30"_t);

        b.vj("C").uri("C:vj1")("C:S0", "11:30"_t)("C:S1", "12:30"_t)("C:S2", "13:30"_t);
        b.lines.find("C")->second->properties["realtime_system"] = "KisioDigital";

        // J is late
        b.vj("J")("S40", "11:00"_t)("S42", "12:00"_t)("S43", "13:00"_t);
        b.lines.find("J")->second->properties["realtime_system"] = "KisioDigital";

        b.vj("K")("S41", "08:59"_t)("S42", "09:59"_t)("S43", "10:59"_t);
        b.vj("K")("S41", "09:03"_t)("S42", "10:03"_t)("S43", "11:03"_t);
        b.vj("K")("S41", "09:06"_t)("S42", "10:06"_t)("S43", "11:06"_t);
        b.vj("K")("S41", "09:09"_t)("S42", "10:09"_t)("S43", "11:09"_t);
        b.vj("K")("S41", "09:19"_t)("S42", "10:19"_t)("S43", "11:19"_t);
        b.lines.find("K")->second->properties["realtime_system"] = "KisioDigital";

        b.vj("L")("S39", "09:02"_t)("S42", "10:02"_t)("S43", "11:02"_t);
        b.vj("L")("S39", "09:07"_t)("S42", "10:07"_t)("S43", "11:07"_t);
        b.vj("L")("S39", "09:11"_t)("S42", "10:11"_t)("S43", "11:11"_t);
        b.lines.find("L")->second->properties["realtime_system"] = "KisioDigital";

        b.finish();
        b.data->pt_data->index();
        b.data->pt_data->build_uri();
        b.data->complete();

        auto* sp_ptr = b.data->pt_data->stop_points_map["C:S0"];
        b.data->pt_data->codes.add(sp_ptr, "KisioDigital", "KisioDigital_C:S0");
        sp_ptr = b.data->pt_data->stop_points_map["C:S1"];
        b.data->pt_data->codes.add(sp_ptr, "KisioDigital", "KisioDigital_C:S1");
        b.data->pt_data->codes.add(sp_ptr, "AnotherSource", "AnotherSource_C:S1");

        // we delay all A's vjs by 7mn (to be able to test whether it's base schedule or realtime data)
        auto trip_update1 = ntest::make_delay_message("A:vj1", "20160101", {
                std::make_tuple("A:s", "20160101T0807"_pts, "20160101T0807"_pts),
                std::make_tuple("S1", "20160101T0907"_pts, "20160101T0907"_pts),
                std::make_tuple("S2", "20160101T1007"_pts, "20160101T1007"_pts),
                std::make_tuple("A:e", "20160101T1107"_pts, "20160101T1107"_pts),
            });
        navitia::handle_realtime("delay_vj1", "20160101T1337"_dt, trip_update1, *b.data);

        auto trip_update2 = ntest::make_delay_message("A:vj2", "20160101", {
                std::make_tuple("A:s", "20160101T0907"_pts, "20160101T0907"_pts),
                std::make_tuple("S1", "20160101T1007"_pts, "20160101T1007"_pts),
                std::make_tuple("S2", "20160101T1107"_pts, "20160101T1107"_pts),
                std::make_tuple("A:e", "20160101T1207"_pts, "20160101T1207"_pts),
            });
        navitia::handle_realtime("delay_vj2", "20160101T1337"_dt, trip_update2, *b.data);

        auto trip_update3 = ntest::make_delay_message("A:vj3", "20160101", {
                std::make_tuple("A:s", "20160101T1007"_pts, "20160101T1007"_pts),
                std::make_tuple("S1", "20160101T1107"_pts, "20160101T1107"_pts),
                std::make_tuple("S2", "20160101T1207"_pts, "20160101T1207"_pts),
                std::make_tuple("A:e", "20160101T1307"_pts, "20160101T1307"_pts),
            });
        navitia::handle_realtime("delay_vj3", "20160101T1337"_dt, trip_update3, *b.data);

        b.data->build_raptor();
        b.data->build_uri();
    }
};

/**
 * Data set for departure board with calendar
 */
struct calendar_fixture {
    ed::builder b;
    boost::gregorian::date beg;
    boost::gregorian::date end_of_year;

    navitia::type::VehicleJourney* vj_week;
    navitia::type::VehicleJourney* vj_week_bis;
    navitia::type::VehicleJourney* vj_weekend;
    navitia::type::VehicleJourney* vj_all;
    navitia::type::VehicleJourney* vj_wednesday;
    calendar_fixture() : b("20120614", "departure board") {
        //2 vj during the week
        b.vj("line:A", "1", "", true, "week")("stop1", 10 * 3600, 10 * 3600 + 10 * 60)("stop2", 12 * 3600, 12 * 3600 + 10 * 60);
        b.vj("line:A", "101", "", true, "week_bis")("stop1", 11 * 3600, 11 * 3600 + 10 * 60)("stop2", 14 * 3600, 14 * 3600 + 10 * 60);
        //NOTE: we give a first random validity pattern because the builder try to factorize them

        //only one on the week end
        b.vj("line:A", "10101", "", true, "weekend")("stop1", 20 * 3600, 20 * 3600 + 10 * 60)("stop2", 21 * 3600, 21 * 3600 + 10 * 60);

        // and one everytime
        auto& builder_vj = b.vj("line:A", "1100101", "", true, "all")("stop1", "15:00"_t, "15:10"_t)
                                                                     ("stop2", "16:00"_t, "16:10"_t);
        // Add a comment on the vj
        auto& pt_data =  *(b.data->pt_data);
        pt_data.comments.add(builder_vj.vj, "vj comment");

        // and wednesday that will not be matched to any cal
        b.vj("line:A", "110010011", "", true, "wednesday")("stop1", 17 * 3600, 17 * 3600 + 10 * 60)("stop2", 18 * 3600, 18 * 3600 + 10 * 60);

        // Check partial terminus tag
        b.vj("A", "10111111", "", true, "vj1", "")("Tstop1", 10*3600, 10*3600)
                                                  ("Tstop2", 10*3600 + 30*60, 10*3600 + 30*60);
        b.vj("A", "10111111", "", true, "vj2", "")("Tstop1", 10*3600 + 30*60, 10*3600 + 30*60)
                                                  ("Tstop2", 11*3600,11*3600)
                                                  ("Tstop3", 11*3600 + 30*60,36300 + 30*60);
        // Check date_time_estimated at stoptime
        b.vj("B", "10111111", "", true, "date_time_estimated", "")
            ("ODTstop1", 10*3600, 10*3600)
            ("ODTstop2", 10*3600 + 30*60, 10*3600 + 30*60);
        // Check on_demand_transport at stoptime
        b.vj("B", "10111111", "", true, "on_demand_transport", "")
            ("ODTstop1", 11*3600, 11*3600)
            ("ODTstop2", 11*3600 + 30*60, 11*3600 + 30*60);

        // Check line opening in stop schedules
        b.vj("C", "110011000001", "", true, "vj_C", "")
                ("stop1_C", "11:00"_t, "11:00"_t)
                ("stop2_C", "11:30"_t, "11:30"_t);
        b.vj("D", "110000001111", "", true, "vj_D", "")
                ("stop1_D", "00:10"_t, "00:10"_t)
                ("stop2_D", "01:40"_t, "01:40"_t)
                ("stop3_D", "02:50"_t, "02:50"_t);
        // Add opening and closing time
        b.lines["C"]->opening_time = boost::posix_time::time_duration(9,0,0);
        b.lines["C"]->closing_time = boost::posix_time::time_duration(21,0,0);
        b.lines["D"]->opening_time = boost::posix_time::time_duration(23,30,0);
        b.lines["D"]->closing_time = boost::posix_time::time_duration(6,0,0);
        b.finish();
        b.data->build_uri();
        beg = b.data->meta->production_date.begin();
        end_of_year = beg + boost::gregorian::years(1) + boost::gregorian::days(1);

        navitia::type::VehicleJourney* vj = pt_data.vehicle_journeys_map["on_demand_transport"];
        vj->stop_time_list[0].set_odt(true);

        vj = pt_data.vehicle_journeys_map["date_time_estimated"];
        vj->stop_time_list[0].set_date_time_estimated(true);

        vj_week = pt_data.vehicle_journeys_map["week"];
        vj_week->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"1111100"});
        vj_week_bis = pt_data.vehicle_journeys_map["week_bis"];
        vj_week_bis->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"1111100"});
        vj_weekend = pt_data.vehicle_journeys_map["weekend"];
        vj_weekend->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0000011"});
        vj_all = pt_data.vehicle_journeys_map["all"];
        vj_all->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"1111111"});
        vj_wednesday = pt_data.vehicle_journeys_map["wednesday"];
        vj_wednesday->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0010000"});

        //we now add 2 similar calendars
        auto week_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        week_cal->uri = "week_cal";
        week_cal->active_periods.push_back({beg, end_of_year});
        week_cal->week_pattern = std::bitset<7>{"1111100"};
        pt_data.calendars.push_back(week_cal);

        auto weekend_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        weekend_cal->uri = "weekend_cal";
        weekend_cal->active_periods.push_back({beg, end_of_year});
        weekend_cal->week_pattern = std::bitset<7>{"0000011"};
        pt_data.calendars.push_back(weekend_cal);

        auto not_associated_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        not_associated_cal->uri = "not_associated_cal";
        not_associated_cal->active_periods.push_back({beg, end_of_year});
        not_associated_cal->week_pattern = std::bitset<7>{"0010000"};
        pt_data.calendars.push_back(not_associated_cal); //not associated to the line

        //both calendars are associated to the line
        b.lines["line:A"]->calendar_list.push_back(week_cal);
        b.lines["line:A"]->calendar_list.push_back(weekend_cal);
        for(auto r: pt_data.routes){
            r->destination = b.sas.find("stop2")->second;
        }

        // and add a comment on a line
        pt_data.comments.add(b.lines["line:A"], "walk the line");

        auto it_sa = b.sas.find("Tstop3");
        auto it_rt = pt_data.routes_map.find("A:1");
        it_rt->second->destination = it_sa->second;

        // load metavj calendar association from database (association is tested in ed/tests/associated_calendar_test.cpp)
        navitia::type::AssociatedCalendar* associated_calendar_for_week = new navitia::type::AssociatedCalendar();
        navitia::type::AssociatedCalendar* associated_calendar_for_week_end = new navitia::type::AssociatedCalendar();
        associated_calendar_for_week->calendar = week_cal;
        associated_calendar_for_week_end->calendar = weekend_cal;
        pt_data.associated_calendars.push_back(associated_calendar_for_week);
        pt_data.associated_calendars.push_back(associated_calendar_for_week_end);
        auto* week_mvj = pt_data.meta_vjs.get_mut("week");
        week_mvj->associated_calendars[week_cal->uri] = associated_calendar_for_week;
        auto* week_bis_mvj = pt_data.meta_vjs.get_mut("week_bis");
        week_bis_mvj->associated_calendars[week_cal->uri] = associated_calendar_for_week;
        auto* weekend_mvj = pt_data.meta_vjs.get_mut("weekend");
        weekend_mvj->associated_calendars[weekend_cal->uri] = associated_calendar_for_week_end;
        auto* all_mvj = pt_data.meta_vjs.get_mut("all");
        all_mvj->associated_calendars[week_cal->uri] = associated_calendar_for_week;
        all_mvj->associated_calendars[weekend_cal->uri] = associated_calendar_for_week_end;

        b.data->build_uri();
        b.data->complete();
        b.data->build_raptor();
        b.data->geo_ref->init();
    }
};



