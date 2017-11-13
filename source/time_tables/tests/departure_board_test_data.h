#pragma once

#include "time_tables/departure_boards.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/pt_data.h"
#include "kraken/realtime.h"
#include "georef/adminref.h"

namespace ntest = navitia::test;
namespace bg = boost::gregorian;

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

        b.vj("M","1111111")("M:s", "10:30"_t)("S11", "11:30"_t, "11:35"_t)("M:e", "12:30"_t);
        b.vj("P", "11111").uri("vjP:1")("stopP1", "23:40"_t)("stopP2", "24:04"_t, "24:06"_t)("stopP3", "24:13"_t);
        b.vj("Q", "11111").uri("vjQ:1")("stopQ1", "23:40"_t)("stopQ2", "23:44"_t, "23:46"_t)("stopQ3", "23:55"_t);

        b.frequency_vj("l:freq", "18:00"_t, "19:00"_t, "00:30"_t).uri("vj:freq")
            ("stopf1", "18:00"_t, "18:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)
            ("stopf2", "18:05"_t, "18:10"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 900)
            ("stopf3", "18:10"_t, "18:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

        navitia::georef::Admin* ad = new navitia::georef::Admin();
        ad->name = "Quimper";
        ad->uri = "Quimper";
        ad->level = 8;
        ad->postal_codes.push_back("29000");
        ad->idx = 0;
        b.data->geo_ref->admins.push_back(ad);
        auto* sp_ptr = b.sps.at("S43");
        sp_ptr->name = "Terminus";
        sp_ptr->stop_area->name = "Terminus";
        sp_ptr->admin_list.push_back(ad);
        sp_ptr->stop_area->admin_list.push_back(ad);

        b.finish();
        b.data->pt_data->index();
        b.data->pt_data->build_uri();
        b.data->complete();
        b.data->compute_labels();

        b.data->meta->production_date = bg::date_period(bg::date(2016,1,1), bg::days(5));

        sp_ptr = b.data->pt_data->stop_points_map["C:S0"];
        b.data->pt_data->codes.add(sp_ptr, "KisioDigital", "KisioDigital_C:S0");
        sp_ptr = b.data->pt_data->stop_points_map["C:S1"];
        b.data->pt_data->codes.add(sp_ptr, "KisioDigital", "KisioDigital_C:S1");
        b.data->pt_data->codes.add(sp_ptr, "AnotherSource", "AnotherSource_C:S1");

        sp_ptr = b.data->pt_data->stop_points_map["S43"];
        b.data->pt_data->codes.add(sp_ptr, "KisioDigital", "KisioDigital_C:S43");

        using ntest::DelayedTimeStop;
        // we delay all A's vjs by 7mn (to be able to test whether it's base schedule or realtime data)
        auto trip_update1 = ntest::make_delay_message("A:vj1", "20160101", {
                DelayedTimeStop("A:s", "20160101T0807"_pts).delay(7_min),
                DelayedTimeStop("S1", "20160101T0907"_pts).delay(7_min),
                DelayedTimeStop("S2", "20160101T1007"_pts).delay(7_min),
                DelayedTimeStop("A:e", "20160101T1107"_pts).delay(7_min),
            });
        navitia::handle_realtime("delay_vj1", "20160101T1337"_dt, trip_update1, *b.data);

        auto trip_update2 = ntest::make_delay_message("A:vj2", "20160101", {
                DelayedTimeStop("A:s", "20160101T0907"_pts).delay(7_min),
                DelayedTimeStop("S1", "20160101T1007"_pts).delay(7_min),
                DelayedTimeStop("S2", "20160101T1107"_pts).delay(7_min),
                DelayedTimeStop("A:e", "20160101T1207"_pts).delay(7_min),
            });
        navitia::handle_realtime("delay_vj2", "20160101T1337"_dt, trip_update2, *b.data);

        auto trip_update3 = ntest::make_delay_message("A:vj3", "20160101", {
                DelayedTimeStop("A:s", "20160101T1007"_pts).delay(7_min),
                DelayedTimeStop("S1", "20160101T1107"_pts).delay(7_min),
                DelayedTimeStop("S2", "20160101T1207"_pts).delay(7_min),
                DelayedTimeStop("A:e", "20160101T1307"_pts).delay(7_min),
            });
        navitia::handle_realtime("delay_vj3", "20160101T1337"_dt, trip_update3, *b.data);


        //
        //      20160103                        |    20160104
        //                                      |
        //      S31                             |         S32             S33
        //      23h40                           |     24h04 24h06        24h13
        //  -----|------------------------------|------|-----|--------------|-          Base_schedule
        //                                      |
        //                                      |
        //       S31                            |              S32               S33
        //      23h40                           |          00h08 00h10          24h17
        //  -----|------------------------------|------------|-----|--------------|-    Realtime
        //                                      |
        //                                      |
        //
        //
        //
        auto trip_update = ntest::make_delay_message("vjP:1", "20160103", {
                DelayedTimeStop("stopP1", "20160103T2340"_pts),
                DelayedTimeStop("stopP2", "20160104T0008"_pts, "20160104T0010"_pts).delay(4_min),
                DelayedTimeStop("stopP3", "20160104T0017"_pts).delay(4_min),
            });
        navitia::handle_realtime("bib", "20160101T1337"_dt, trip_update, *b.data);

        //
        //      20160103                    |    20160104
        //                                  |
        //      Q1            Q2            |
        //      23h40    23h44 23h46        |
        //  -----|--------|-----|----|------|--------------------          Base_schedule
        //                          23h55   |
        //                           Q3     |
        //                                  |   Q1           Q2                Q3
        //                                  | 00h01     00h05 00h06          00h16
        //  --------------------------------|--|----------|-----|--------------|-    Realtime
        //                                  |
        //                                  |
        //
        //
        //
        // 21m
        auto trip_update_q = ntest::make_delay_message("vjQ:1", "20160103", {
                DelayedTimeStop("stopQ1", "20160104T0001"_pts).delay(21_min),
                DelayedTimeStop("stopQ2", "20160104T0005"_pts, "20160104T0006"_pts).delay(21_min),
                DelayedTimeStop("stopQ3", "20160104T0016"_pts).delay(21_min),
            });
        navitia::handle_realtime("Q", "20160101T1337"_dt, trip_update_q, *b.data);
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

    navitia::type::VehicleJourney* r_vj1;
    navitia::type::VehicleJourney* r_vj2;

    calendar_fixture() : b("20120614", "departure board") {
        //2 vj during the week
        b.vj("line:A", "1", "", true, "week")("stop1", "10:00"_t, "10:10"_t)
                                             ("stop2", "12:00"_t, "12:10"_t);
        b.vj("line:A", "101", "", true, "week_bis")("stop1", "11:00"_t, "11:10"_t)
                                                   ("stop2", "14:00"_t, "14:10"_t);
        //NOTE: we give a first random validity pattern because the builder try to factorize them

        //only one on the week end
        b.vj("line:A", "10101", "", true, "weekend")("stop1", "20:00"_t, "20:10"_t)
                                                    ("stop2", "21:00"_t, "21:10"_t);

        // and one everytime
        auto& builder_vj = b.vj("line:A", "1100101", "", true, "all")("stop1", "15:00"_t, "15:10"_t)
                                                                     ("stop2", "16:00"_t, "16:10"_t);
        // Add a comment on the vj
        auto& pt_data =  *(b.data->pt_data);
        pt_data.comments.add(builder_vj.vj, "vj comment");

        // and wednesday that will not be matched to any cal
        b.vj("line:A", "110010011", "", true, "wednesday")("stop1", "17:00"_t, "17:10"_t)
                                                          ("stop2", "18:00"_t, "18:10"_t);

        // Check partial terminus tag
        b.vj("A").vp("10111111").uri("vj1")("Tstop1", "10:00"_t, "10:00"_t)
                                           ("Tstop2", "10:30"_t, "10:30"_t);
        b.vj("A").vp("00000011").uri("vj2")("Tstop1", "10:30"_t, "10:30"_t)
                                           ("Tstop2", "11:00"_t, "11:00"_t)
                                           ("Tstop3", "11:30"_t, "11:30"_t);

        // Check date_time_estimated at stoptime
        b.vj("B", "10111111", "", true, "date_time_estimated", "")
                ("ODTstop1", "10:00"_t, "10:00"_t)
                ("ODTstop2", "10:30"_t, "10:30"_t);
        // Check on_demand_transport at stoptime
        b.vj("B", "10111111", "", true, "on_demand_transport", "")
                ("ODTstop1", "11:00"_t, "11:00"_t)
                ("ODTstop2", "11:30"_t, "11:30"_t);

        // Check line opening in stop schedules
        b.vj("C", "110011000001", "", true, "vj_C", "")
                ("stop1_C", "11:00"_t, "11:00"_t)
                ("stop2_C", "11:30"_t, "11:30"_t);

        b.vj("D", "110000001111", "", true, "vj_D", "")
                ("stop1_D", "00:10"_t, "00:10"_t)
                ("stop2_D", "01:40"_t, "01:40"_t)
                ("stop3_D", "02:50"_t, "02:50"_t);
        /*
                                                   StopR3                            StopR4
                                            ------------------------------------------
                      StopR1               /
            Line R : ---------------------/StopR2
                                          \         StopR5                            StopR6
                                           \ ------------------------------------------

        */

        b.vj("R", "10111111", "", true, "R:vj1", "")("StopR1", "10:00"_t, "10:00"_t)
                                                  ("StopR2", "10:30"_t, "10:30"_t)
                                                  ("StopR3", "11:00"_t, "11:00"_t)
                                                  ("StopR4", "11:30"_t, "11:30"_t);
        b.vj("R", "10111111", "", true, "R:vj2", "")("StopR1", "10:00"_t, "10:00"_t)
                                                  ("StopR2", "10:30"_t, "10:30"_t)
                                                  ("StopR5", "11:00"_t, "11:00"_t)
                                                  ("StopR6", "11:30"_t, "11:30"_t);
        // Add opening and closing time
        b.lines["C"]->opening_time = boost::posix_time::time_duration(9,0,0);
        b.lines["C"]->closing_time = boost::posix_time::time_duration(21,0,0);
        b.lines["D"]->opening_time = boost::posix_time::time_duration(23,30,0);
        b.lines["D"]->closing_time = boost::posix_time::time_duration(6,0,0);
        b.lines["line:A"]->color = "289728";
        b.lines["line:A"]->text_color = "FFD700";
        b.lines["line:A"]->code = "A";

        b.finish();
        b.data->build_uri();

        b.data->pt_data->codes.add(b.get<nt::VehicleJourney>("R:vj1"), "source", "Code-R-vj1");
        b.data->pt_data->codes.add(b.get<nt::StopPoint>("StopR1"), "source", "Code-StopR1");
        b.data->pt_data->codes.add(b.get<nt::StopPoint>("StopR2"), "source", "Code-StopR2");
        b.data->pt_data->codes.add(b.get<nt::StopPoint>("StopR3"), "source", "Code-StopR3");
        b.data->pt_data->codes.add(b.get<nt::StopPoint>("StopR4"), "source", "Code-StopR4");

        beg = b.data->meta->production_date.begin();
        end_of_year = beg + boost::gregorian::years(1);

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

        r_vj1 = pt_data.vehicle_journeys_map["R:vj1"];
        r_vj1->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0010000"});
        r_vj2 = pt_data.vehicle_journeys_map["R:vj2"];
        r_vj2->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0010000"});

        vj_all = pt_data.vehicle_journeys_map["all"];
        vj_all->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"1111111"});
        vj_wednesday = pt_data.vehicle_journeys_map["wednesday"];
        vj_wednesday->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0010000"});

        // Check partial terminus for calendar
        auto cal_partial_terminus = new navitia::type::Calendar(b.data->meta->production_date.begin());
        cal_partial_terminus->uri = "cal_partial_terminus";
        cal_partial_terminus->active_periods.push_back({beg, end_of_year});
        cal_partial_terminus->week_pattern = std::bitset<7>{"1111111"};
        pt_data.calendars.push_back(cal_partial_terminus);
        b.lines["A"]->calendar_list.push_back(cal_partial_terminus);


        //we now add 2 similar calendars
        auto week_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        week_cal->uri = "week_cal";
        week_cal->active_periods.push_back({beg, end_of_year});
        week_cal->week_pattern = std::bitset<7>{"1111100"};
        pt_data.calendars.push_back(week_cal);
        b.lines["A"]->calendar_list.push_back(week_cal);

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

        auto* line = b.lines["A"];
        for(auto r: line->route_list){
            r->destination = b.sas.find("Tstop3")->second;
        }

        line = b.lines["R"];
        for(auto r: line->route_list){
            r->destination = b.sas.find("StopR2")->second;
        }

        // and add a comment on a line
        pt_data.comments.add(b.lines["line:A"], "walk the line");

        auto it_sa = b.sas.find("Tstop3");
        auto it_rt = pt_data.routes_map.find("A:1");
        it_rt->second->destination = it_sa->second;

        // load metavj calendar association from database (association is tested in ed/tests/associated_calendar_test.cpp)
        navitia::type::AssociatedCalendar* associated_calendar_for_week = new navitia::type::AssociatedCalendar();
        navitia::type::AssociatedCalendar* associated_calendar_for_week_end = new navitia::type::AssociatedCalendar();
        navitia::type::AssociatedCalendar* associated_calendar_for_terminus = new navitia::type::AssociatedCalendar();

        navitia::type::AssociatedCalendar* associated_calendar_for_line_r = new navitia::type::AssociatedCalendar();

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

        associated_calendar_for_terminus->calendar = cal_partial_terminus;
        pt_data.associated_calendars.push_back(associated_calendar_for_terminus);
        auto* cal_partial_terminus_mvj = pt_data.meta_vjs.get_mut("vj1");
        cal_partial_terminus_mvj->associated_calendars[cal_partial_terminus->uri] = associated_calendar_for_terminus;
        cal_partial_terminus_mvj = pt_data.meta_vjs.get_mut("vj2");
        cal_partial_terminus_mvj->associated_calendars[weekend_cal->uri] = associated_calendar_for_week;

        associated_calendar_for_line_r->calendar = not_associated_cal;
        auto* r_vj1_mvj = pt_data.meta_vjs.get_mut("R:vj1");
        r_vj1_mvj->associated_calendars[not_associated_cal->uri] = associated_calendar_for_line_r;
        auto* r_vj2_mvj = pt_data.meta_vjs.get_mut("R:vj2");
        r_vj2_mvj->associated_calendars[not_associated_cal->uri] = associated_calendar_for_line_r;

        b.data->build_uri();
        b.data->complete();
        b.data->build_raptor();
        b.data->geo_ref->init();
    }
};



