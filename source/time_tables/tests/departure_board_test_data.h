#pragma once

#include "time_tables/departure_boards.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "type/pt_data.h"
#include "kraken/realtime.h"
#include "georef/adminref.h"
#include "kraken/apply_disruption.h"

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
    navitia::type::StopPoint* sp_ptr;
    departure_board_fixture()
        : b(
            "20160101",
            [&](ed::builder& b) {
                b.sa("SA1")("S1")("S2");

                b.vj("A")
                    .route("A:0", "forward")
                    .name("A:vj1")("A:s", "08:00"_t)("S1", "09:00"_t)("S2", "10:00"_t)("A:e", "11:00"_t);
                b.vj("A").name("A:vj2")("A:s", "09:00"_t)("S1", "10:00"_t)("S2", "11:00"_t)("A:e", "12:00"_t);
                b.vj("A").name("A:vj3")("A:s", "10:00"_t)("S1", "11:00"_t)("S2", "12:00"_t)("A:e", "13:00"_t);

                b.vj("B")
                    .route("B:1", "anticlockwise")
                    .name("B:vj1")("B:s", "10:30"_t)("S1", "11:30"_t)("B:e", "12:30"_t);

                b.vj("C").name("C:vj1")("C:S0", "11:30"_t)("C:S1", "12:30"_t)("C:S2", "13:30"_t);
                b.lines.find("C")->second->properties["realtime_system"] = "Kisio数字";

                // J is late
                b.vj("J")("S40", "11:00"_t)("S42", "12:00"_t)("S43", "13:00"_t);
                b.lines.find("J")->second->properties["realtime_system"] = "Kisio数字";

                b.vj("K")("S41", "08:59"_t)("S42", "09:59"_t)("S43", "10:59"_t);
                b.vj("K")("S41", "09:03"_t)("S42", "10:03"_t)("S43", "11:03"_t);
                b.vj("K")("S41", "09:06"_t)("S42", "10:06"_t)("S43", "11:06"_t);
                b.vj("K")("S41", "09:09"_t)("S42", "10:09"_t)("S43", "11:09"_t);
                b.vj("K")("S41", "09:19"_t)("S42", "10:19"_t)("S43", "11:19"_t);
                b.lines.find("K")->second->properties["realtime_system"] = "Kisio数字";

                b.vj("L")("S39", "09:02"_t)("S42", "10:02"_t)("S43", "11:02"_t);
                b.vj("L")("S39", "09:07"_t)("S42", "10:07"_t)("S43", "11:07"_t);
                b.vj("L")("S39", "09:11"_t)("S42", "10:11"_t)("S43", "11:11"_t);
                b.lines.find("L")->second->properties["realtime_system"] = "Kisio数字";

                b.vj("M", "1111111")("M:s", "10:30"_t)("S11", "11:30"_t, "11:35"_t)("M:e", "12:30"_t);
                b.vj("P", "11111")
                    .name("vjP:1")("stopP1", "23:40"_t)("stopP2", "24:04"_t, "24:06"_t)("stopP3", "24:13"_t);
                b.vj("Q", "11111")
                    .name("vjQ:1")("stopQ1", "23:40"_t)("stopQ2", "23:44"_t, "23:46"_t)("stopQ3", "23:55"_t);

                b.frequency_vj("l:freq", "18:00"_t, "19:00"_t, "00:30"_t)
                    .name("vj:freq")("stopf1", "18:00"_t, "18:00"_t, std::numeric_limits<uint16_t>::max(), false, true,
                                     0, 300)("stopf2", "18:05"_t, "18:10"_t, std::numeric_limits<uint16_t>::max(), true,
                                             true, 900, 900)("stopf3", "18:10"_t, "18:10"_t,
                                                             std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

                b.vj("T")("TS39", "09:02"_t)("TS42", "10:02"_t)("S43", "11:02"_t);
                b.vj("T")("TS39", "09:07"_t)("TS42", "10:07"_t)("S43", "11:07"_t);
                b.vj("T")("TS39", "09:11"_t)("TS42", "10:11"_t)("S43", "11:11"_t);
                b.lines.find("T")->second->properties["realtime_system"] = "Kisio数字";

                // Terminus_schedule
                // 1 line, 1 routes and 2 VJs
                // * VJ1 : E->D->C->B->A1->AVj1
                // * VJ2 : D->C->B->A2->AVj2

                // * VJ3 : E<-D<-C->B<-A1<-AVj1
                // * VJ4 : D<-C<-B<-A2<-AVj2

                // * Route1       Avj2 <------A2---
                // *                               |
                // * Route1       Avj1 <------A1-------- B <- C <- D <- E
                // *
                //                  01  02  03  04  05  06  07  08  09      Direction
                // Active VJ1 :         *   *   *       *   *       *       AVj1
                // Active VJ2 :         *   *   *   *   *                   AVj2
                // Active VJ3 :         *   *   *   *               *       E
                // Active VJ4 :     *   *   *                               E

                b.sa("Avj1", 0., 0., false)("Avj1");
                b.sa("Avj2", 0., 0., false)("Avj2");
                b.sa("B", 0., 0., false)("B:sp");
                b.sa("C", 0., 0., false)("C:sp");
                b.sa("D", 0., 0., false)("D:sp");
                b.sa("E", 0., 0., false)("E:sp");
                b.sa("A1", 0., 0., false)("A1:sp");
                b.sa("A2", 0., 0., false)("A2:sp");

                b.vj("Line1", "101101110")
                    .route("Route1")
                    .name("AVJ1")("E:sp", "07:45"_t)("D:sp", "08:00"_t)("C:sp", "09:00"_t)("B:sp", "10:00"_t)(
                        "A1", "11:00"_t)("Avj1", "11:15"_t);
                b.vj("Line1", "100011110")
                    .route("Route2_R")
                    .name("AVJ3")("Avj1", "07:45"_t)("A1", "08:00"_t)("B:sp", "09:00"_t)("C:sp", "10:00"_t)(
                        "D:sp", "11:00"_t)("E:sp", "11:15"_t);

                b.vj("Line1", "000111110")
                    .route("Route1")
                    .name("AVJ2")("D:sp", "09:00"_t)("C:sp", "09:10"_t)("B:sp", "10:10"_t)("A2", "11:10"_t)("Avj2",
                                                                                                            "11:25"_t);
                b.vj("Line1", "000000111")
                    .route("Route2_R")
                    .name("AVJ4")("Avj2", "09:00"_t)("A2", "09:10"_t)("B:sp", "10:10"_t)("C:sp", "11:10"_t)("D:sp",
                                                                                                            "11:25"_t);

                b.lines.find("Line1")->second->properties["realtime_system"] = "Kisio数字";

                // 1 line, 1 routes and 2 VJs
                // * VJ1 : AA->BB->CC->DD->EE
                // * VJ2 : AA->BB->CC->DD

                b.sa("AA", 1., 1., false)("AA:sp");
                b.sa("BB", 2., 2., false)("BB:sp");
                b.sa("CC", 3., 3., false)("CC:sp");
                b.sa("DD", 4., 4., false)("DD:sp");
                b.sa("EE", 5., 5., false)("EE:sp");

                b.vj("Line2").route("Route11").name("AA:VJ1")("AA:sp", "07:45"_t)("BB:sp", "08:00"_t)(
                    "CC:sp", "09:00"_t)("DD:sp", "10:00"_t)("EE:sp", "11:00"_t);
                b.vj("Line2").route("Route12").name("BB:VJ2")("AA:sp", "07:55"_t)("BB:sp", "08:55"_t)(
                    "CC:sp", "09:30"_t)("DD:sp", "10:10"_t);
                b.lines.find("Line2")->second->properties["realtime_system"] = "Kisio数字";

                // Terminus_schedule
                // 1 line, 2 routes and 2 VJs
                // * Route1, VJ1 : TS_A->TS_B->TS_C->TS_D->TS_E
                // * Route2, VJ2 : TS_A<-TS_B<-TS_C<-TS_D<-TS_E

                // Active only, VJ1 : 04/01/2016, VJ2: 03/01/2016 and VJ1, VJ2: 07/01/2016

                b.sa("TS_A", 0., 0., false)("TS_A:sp");
                b.sa("TS_B", 0., 0., false)("TS_B:sp");
                b.sa("TS_C", 0., 0., false)("TS_C:sp");
                b.sa("TS_D", 0., 0., false)("TS_D:sp");
                b.sa("TS_E", 0., 0., false)("TS_E:sp");

                b.vj("Line3", "001001000")
                    .route("TS_Route1")
                    .name("TS_AVJ1")("TS_A:sp", "07:45"_t)("TS_B:sp", "08:00"_t)("TS_C:sp", "09:00"_t)(
                        "TS_D:sp", "10:00"_t)("TS_E:sp", "10:10"_t);
                b.vj("Line3", "001000100")
                    .route("TS_Route2")
                    .name("TS_AVJ2")("TS_D:sp", "08:45"_t)("TS_D:sp", "09:00"_t)("TS_C:sp", "09:10"_t)(
                        "TS_B:sp", "10:10"_t)("TS_A:sp", "11:10"_t);

                b.lines.find("Line3")->second->properties["realtime_system"] = "Kisio数字";
                b.lines.find("Line3")->second->opening_time =
                    boost::make_optional(boost::posix_time::duration_from_string("06:00"));
                b.lines.find("Line3")->second->closing_time =
                    boost::make_optional(boost::posix_time::duration_from_string("14:00"));
                // Timeo R
                auto it_rt = b.data->pt_data->routes_map.find("Route2_R");
                it_rt->second->direction_type = "backward";
                // Timeo A
                it_rt = b.data->pt_data->routes_map.find("Route1");
                it_rt->second->direction_type = "forward";

                // Timeo R
                it_rt = b.data->pt_data->routes_map.find("TS_Route2");
                it_rt->second->direction_type = "backward";
                // Timeo A
                it_rt = b.data->pt_data->routes_map.find("TS_Route1");
                it_rt->second->direction_type = "forward";

                auto& vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:L:11");
                b.data->pt_data->codes.add(vj, "source", "vj:l:11");
                vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:L:12");
                b.data->pt_data->codes.add(vj, "source", "vj:l:12");
                vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:L:13");
                b.data->pt_data->codes.add(vj, "source", "vj:l:13");
                vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:M:14");
                b.data->pt_data->codes.add(vj, "source", "vehicle_journey:M:14");
                vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:C:vj1");
                b.data->pt_data->codes.add(vj, "source", "vehicle_journey:C:vj1");
                vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vj:freq");
                b.data->pt_data->codes.add(vj, "source", "vehicle_journey:vj:freq");

                auto* ad = new navitia::georef::Admin();
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

                b.data->complete();
                b.data->compute_labels();
                b.make();

                // Disruption NO_SERVICE on StopArea
                using btp = boost::posix_time::time_period;

                navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption 1")
                                              .severity(nt::disruption::Effect::NO_SERVICE)
                                              .on(nt::Type_e::StopArea, "D", *b.data->pt_data)
                                              .application_periods(btp("20160103T000000"_dt, "20160103T235900"_dt))
                                              .publish(btp("20160103T000000"_dt, "20160103T235900"_dt))
                                              .msg("Disruption on stop area D")
                                              .get_disruption(),
                                          *b.data->pt_data, *b.data->meta);
            },
            true) {
        b.data->meta->production_date = bg::date_period(bg::date(2016, 1, 1), bg::days(10));

        sp_ptr = b.data->pt_data->stop_points_map["BB:sp"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Kisio数字_BB:sp");

        sp_ptr = b.data->pt_data->stop_points_map["C:sp"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Kisio数字_C:sp");

        sp_ptr = b.data->pt_data->stop_points_map["C:S0"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Kisio数字_C:S0");
        sp_ptr = b.data->pt_data->stop_points_map["C:S1"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Kisio数字_C:S1");
        b.data->pt_data->codes.add(sp_ptr, "AnotherSource", "AnotherSource_C:S1");

        sp_ptr = b.data->pt_data->stop_points_map["S43"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Kisio数字_C:S43");

        sp_ptr = b.data->pt_data->stop_points_map["Avj1"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Avj1");

        sp_ptr = b.data->pt_data->stop_points_map["Avj2"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Avj2");

        sp_ptr = b.data->pt_data->stop_points_map["E:sp"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "E:sp");

        sp_ptr = b.data->pt_data->stop_points_map["TS_D:sp"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "TS_D:sp");
        sp_ptr = b.data->pt_data->stop_points_map["C:S2"];
        b.data->pt_data->codes.add(sp_ptr, "Kisio数字", "Kisio数字_C:S2");

        sp_ptr = b.data->pt_data->stop_points_map.at("C:S0");
        b.data->pt_data->codes.add(sp_ptr, "source", "C:S0");

        sp_ptr = b.data->pt_data->stop_points_map.at("S11");
        b.data->pt_data->codes.add(sp_ptr, "source", "S11");

        sp_ptr = b.data->pt_data->stop_points_map.at("stopP2");
        b.data->pt_data->codes.add(sp_ptr, "source", "stopP2");

        sp_ptr = b.data->pt_data->stop_points_map.at("stopQ2");
        b.data->pt_data->codes.add(sp_ptr, "source", "stopQ2");

        sp_ptr = b.data->pt_data->stop_points_map.at("stopf1");
        b.data->pt_data->codes.add(sp_ptr, "source", "stopf1");
        b.data->pt_data->codes.add(sp_ptr, "source", "C:S0");

        using ntest::RTStopTime;
        // we delay all A's vjs by 7mn (to be able to test whether it's base schedule or realtime data)
        auto trip_update1 = ntest::make_trip_update_message("A:vj1", "20160101",
                                                            {
                                                                RTStopTime("A:s", "20160101T0807"_pts).delay(7_min),
                                                                RTStopTime("S1", "20160101T0907"_pts).delay(7_min),
                                                                RTStopTime("S2", "20160101T1007"_pts).delay(7_min),
                                                                RTStopTime("A:e", "20160101T1107"_pts).delay(7_min),
                                                            });
        navitia::handle_realtime("delay_vj1", "20160101T1337"_dt, trip_update1, *b.data, true, true);

        auto trip_update2 = ntest::make_trip_update_message("A:vj2", "20160101",
                                                            {
                                                                RTStopTime("A:s", "20160101T0907"_pts).delay(7_min),
                                                                RTStopTime("S1", "20160101T1007"_pts).delay(7_min),
                                                                RTStopTime("S2", "20160101T1107"_pts).delay(7_min),
                                                                RTStopTime("A:e", "20160101T1207"_pts).delay(7_min),
                                                            });
        navitia::handle_realtime("delay_vj2", "20160101T1337"_dt, trip_update2, *b.data, true, true);

        auto trip_update3 = ntest::make_trip_update_message("A:vj3", "20160101",
                                                            {
                                                                RTStopTime("A:s", "20160101T1007"_pts).delay(7_min),
                                                                RTStopTime("S1", "20160101T1107"_pts).delay(7_min),
                                                                RTStopTime("S2", "20160101T1207"_pts).delay(7_min),
                                                                RTStopTime("A:e", "20160101T1307"_pts).delay(7_min),
                                                            });
        navitia::handle_realtime("delay_vj3", "20160101T1337"_dt, trip_update3, *b.data, true, true);

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
        auto trip_update = ntest::make_trip_update_message(
            "vjP:1", "20160103",
            {
                RTStopTime("stopP1", "20160103T2340"_pts),
                RTStopTime("stopP2", "20160104T0008"_pts, "20160104T0010"_pts).delay(4_min),
                RTStopTime("stopP3", "20160104T0017"_pts).delay(4_min),
            });
        navitia::handle_realtime("bib", "20160101T1337"_dt, trip_update, *b.data, true, true);

        auto* vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vjP:1:modified:0:bib");
        b.data->pt_data->codes.add(vj, "source", "vehicle_journey:vjP:1:modified:0:bib");

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
        auto trip_update_q = ntest::make_trip_update_message(
            "vjQ:1", "20160103",
            {
                RTStopTime("stopQ1", "20160104T0001"_pts).delay(21_min),
                RTStopTime("stopQ2", "20160104T0005"_pts, "20160104T0006"_pts).delay(21_min),
                RTStopTime("stopQ3", "20160104T0016"_pts).delay(21_min),
            });
        navitia::handle_realtime("Q", "20160101T1337"_dt, trip_update_q, *b.data, true, true);
        vj = b.data->pt_data->vehicle_journeys_map.at("vehicle_journey:vjQ:1:modified:0:Q");
        b.data->pt_data->codes.add(vj, "source", "vehicle_journey:vjQ:1:modified:0:Q");

        b.data->build_raptor();
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

    calendar_fixture()
        : b(
            "20120614",
            [&](ed::builder& b) {
                // Stop areas
                b.sa("SA1", 0., 1.)("stop1_D", 0., 1.);
                b.sa("SA2", 0., 2.)("stop2_D", 0., 2.);
                b.sa("SA3", 0., 3.)("stop3_D", 0., 3.);

                // 2 vj during the week
                b.vj("line:A", "1", "", true, "week")("stop1", "10:00"_t, "10:10"_t)("stop2", "12:00"_t, "12:10"_t);
                b.vj("line:A", "101", "", true, "week_bis")("stop1", "11:00"_t, "11:10"_t)("stop2", "14:00"_t,
                                                                                           "14:10"_t);
                // NOTE: we give a first random validity pattern because the builder try to factorize them

                // only one on the week end
                b.vj("line:A", "10101", "", true, "weekend")("stop1", "20:00"_t, "20:10"_t)("stop2", "21:00"_t,
                                                                                            "21:10"_t);

                // and one everytime
                auto& builder_vj = b.vj("line:A", "1100101", "", true, "all")("stop1", "15:00"_t, "15:10"_t)(
                    "stop2", "16:00"_t, "16:10"_t);
                // Add a comment on the vj
                auto& pt_data = *(b.data->pt_data);
                pt_data.comments.add(builder_vj.vj, nt::Comment("vj comment", "on_demand_transport"));

                // and wednesday that will not be matched to any cal
                b.vj("line:A", "110010011", "", true, "wednesday")("stop1", "17:00"_t, "17:10"_t)("stop2", "18:00"_t,
                                                                                                  "18:10"_t);

                // Check partial terminus tag
                b.vj("A").vp("10111111").name("vj1")("Tstop1", "10:00"_t, "10:00"_t)("Tstop2", "10:30"_t, "10:30"_t);
                b.vj("A")
                    .vp("00000011")
                    .name("vj2")("Tstop1", "10:30"_t, "10:30"_t)("Tstop2", "11:00"_t, "11:00"_t)("Tstop3", "11:30"_t,
                                                                                                 "11:30"_t);

                // Check date_time_estimated at stoptime
                b.vj("B", "10111111", "", true, "date_time_estimated", "")("ODTstop1", "10:00"_t, "10:00"_t)(
                    "ODTstop2", "10:30"_t, "10:30"_t);
                // Check on_demand_transport at stoptime
                b.vj("B", "10111111", "", true, "on_demand_transport", "")("ODTstop1", "11:00"_t, "11:00"_t)(
                    "ODTstop2", "11:30"_t, "11:30"_t);

                // Check line opening in stop schedules
                b.vj("C", "110011000001", "", true, "vj_C", "")("stop1_C", "11:00"_t, "11:00"_t)("stop2_C", "11:30"_t,
                                                                                                 "11:30"_t);

                b.vj("D", "110000001111", "", true, "vj_D", "")("stop1_D", "00:10"_t, "00:10"_t)(
                    "stop2_D", "01:40"_t, "01:40"_t, std::numeric_limits<uint16_t>::max(), false, false, 0, 0, true)(
                    "stop3_D", "02:50"_t, "02:50"_t);
                /*
                                                           StopR3                            StopR4
                                                    ------------------------------------------
                              StopR1               /
                    Line R : ---------------------/StopR2
                                                  \         StopR5                            StopR6
                                                   \ ------------------------------------------

                */

                b.vj("R", "10111111", "", true, "R:vj1", "")("StopR1", "10:00"_t, "10:00"_t)(
                    "StopR2", "10:30"_t, "10:30"_t)("StopR3", "11:00"_t, "11:00"_t)("StopR4", "11:30"_t, "11:30"_t);
                b.vj("R", "10111111", "", true, "R:vj2", "")("StopR1", "10:00"_t, "10:00"_t)(
                    "StopR2", "10:30"_t, "10:30"_t)("StopR5", "11:00"_t, "11:00"_t)("StopR6", "11:30"_t, "11:30"_t);
                // Add opening and closing time
                b.lines["C"]->opening_time = boost::posix_time::time_duration(9, 0, 0);
                b.lines["C"]->closing_time = boost::posix_time::time_duration(21, 0, 0);
                b.lines["D"]->opening_time = boost::posix_time::time_duration(23, 30, 0);
                b.lines["D"]->closing_time = boost::posix_time::time_duration(6, 0, 0);
                b.lines["line:A"]->color = "289728";
                b.lines["line:A"]->text_color = "FFD700";
                b.lines["line:A"]->code = "A";

                // Add admins to test display_informations.direction
                auto* ad = new navitia::georef::Admin();
                ad->name = "admin_name";
                ad->uri = "admin_uri";
                ad->level = 8;
                ad->postal_codes.push_back("29000");
                ad->idx = 0;
                b.data->geo_ref->admins.push_back(ad);
                auto* sp_ptr = b.sps.at("Tstop3");
                sp_ptr->stop_area->admin_list.push_back(ad);

                // b.finish();
                b.data->build_uri();

                pt_data.codes.add(b.get<nt::VehicleJourney>("vehicle_journey:R:vj1"), "source", "Code-R-vj1");
                pt_data.codes.add(b.get<nt::StopPoint>("StopR1"), "source", "Code-StopR1");
                pt_data.codes.add(b.get<nt::StopPoint>("StopR2"), "source", "Code-StopR2");
                pt_data.codes.add(b.get<nt::StopPoint>("StopR3"), "source", "Code-StopR3");
                pt_data.codes.add(b.get<nt::StopPoint>("StopR4"), "source", "Code-StopR4");
                pt_data.codes.add(b.get<nt::Network>("base_network"), "app_code", "ilevia");

                beg = b.data->meta->production_date.begin();
                end_of_year = beg + boost::gregorian::years(1);

                navitia::type::VehicleJourney* vj = pt_data.vehicle_journeys_map["vehicle_journey:on_demand_transport"];
                vj->stop_time_list[0].set_odt(true);

                vj = pt_data.vehicle_journeys_map["vehicle_journey:date_time_estimated"];
                vj->stop_time_list[0].set_date_time_estimated(true);

                vj_week = pt_data.vehicle_journeys_map["vehicle_journey:week"];
                vj_week->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"1111100"});
                vj_week_bis = pt_data.vehicle_journeys_map["vehicle_journey:week_bis"];
                vj_week_bis->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"1111100"});
                vj_weekend = pt_data.vehicle_journeys_map["vehicle_journey:weekend"];
                vj_weekend->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0000011"});

                r_vj1 = pt_data.vehicle_journeys_map["vehicle_journey:R:vj1"];
                r_vj1->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0010000"});
                r_vj2 = pt_data.vehicle_journeys_map["vehicle_journey:R:vj2"];
                r_vj2->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0010000"});

                vj_all = pt_data.vehicle_journeys_map["vehicle_journey:all"];
                vj_all->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"1111111"});
                vj_wednesday = pt_data.vehicle_journeys_map["vehicle_journey:wednesday"];
                vj_wednesday->base_validity_pattern()->add(beg, end_of_year, std::bitset<7>{"0010000"});

                // Check partial terminus for calendar
                auto cal_partial_terminus = new navitia::type::Calendar(b.data->meta->production_date.begin());
                cal_partial_terminus->uri = "cal_partial_terminus";
                cal_partial_terminus->active_periods.emplace_back(beg, end_of_year);
                cal_partial_terminus->week_pattern = std::bitset<7>{"1111111"};
                pt_data.calendars.push_back(cal_partial_terminus);
                b.lines["A"]->calendar_list.push_back(cal_partial_terminus);

                // we now add 2 similar calendars
                auto week_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
                week_cal->uri = "week_cal";
                week_cal->active_periods.emplace_back(beg, end_of_year);
                week_cal->week_pattern = std::bitset<7>{"1111100"};
                pt_data.calendars.push_back(week_cal);
                b.lines["A"]->calendar_list.push_back(week_cal);

                auto weekend_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
                weekend_cal->uri = "weekend_cal";
                weekend_cal->active_periods.emplace_back(beg, end_of_year);
                weekend_cal->week_pattern = std::bitset<7>{"0000011"};
                pt_data.calendars.push_back(weekend_cal);

                auto not_associated_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
                not_associated_cal->uri = "not_associated_cal";
                not_associated_cal->active_periods.emplace_back(beg, end_of_year);
                not_associated_cal->week_pattern = std::bitset<7>{"0010000"};
                pt_data.calendars.push_back(not_associated_cal);  // not associated to the line

                // both calendars are associated to the line
                b.lines["line:A"]->calendar_list.push_back(week_cal);
                b.lines["line:A"]->calendar_list.push_back(weekend_cal);
                for (auto r : pt_data.routes) {
                    r->destination = b.sas.find("stop2")->second;
                }

                auto* line = b.lines["A"];
                for (auto r : line->route_list) {
                    r->destination = b.sas.find("Tstop3")->second;
                }

                line = b.lines["R"];
                for (auto r : line->route_list) {
                    r->destination = b.sas.find("StopR2")->second;
                }

                // and add a comment on a line
                pt_data.comments.add(b.lines["line:A"], nt::Comment("walk the line", "information"));

                auto it_sa = b.sas.find("Tstop3");
                auto it_rt = pt_data.routes_map.find("A:1");
                it_rt->second->destination = it_sa->second;

                // load metavj calendar association from database (association is tested in
                // ed/tests/associated_calendar_test.cpp)
                auto* associated_calendar_for_week = new navitia::type::AssociatedCalendar();
                auto* associated_calendar_for_week_end = new navitia::type::AssociatedCalendar();
                auto* associated_calendar_for_terminus = new navitia::type::AssociatedCalendar();

                auto* associated_calendar_for_line_r = new navitia::type::AssociatedCalendar();

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
                cal_partial_terminus_mvj->associated_calendars[cal_partial_terminus->uri] =
                    associated_calendar_for_terminus;
                cal_partial_terminus_mvj = pt_data.meta_vjs.get_mut("vj2");
                cal_partial_terminus_mvj->associated_calendars[weekend_cal->uri] = associated_calendar_for_week;

                associated_calendar_for_line_r->calendar = not_associated_cal;
                auto* r_vj1_mvj = pt_data.meta_vjs.get_mut("R:vj1");
                r_vj1_mvj->associated_calendars[not_associated_cal->uri] = associated_calendar_for_line_r;
                auto* r_vj2_mvj = pt_data.meta_vjs.get_mut("R:vj2");
                r_vj2_mvj->associated_calendars[not_associated_cal->uri] = associated_calendar_for_line_r;

                b.data->complete();
            },
            true,
            "departure board") {
        b.data->geo_ref->init();
    }
};
