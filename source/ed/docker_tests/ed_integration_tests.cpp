/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ed_integration_tests
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>
#include "utils/logger.h"
#include "utils/functions.h"
#include "type/data.h"
#include "type/pt_data.h"
#include "type/meta_data.h"
#include "type/access_point.h"
#include "type/contributor.h"
#include "type/meta_vehicle_journey.h"
#include "type/dataset.h"
#include "tests/utils_test.h"
#include "routing/raptor_utils.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

namespace pt = boost::posix_time;
namespace nt = navitia::type;
namespace nr = navitia::routing;

struct ArgsFixture {
    ArgsFixture() {
        auto argc = boost::unit_test::framework::master_test_suite().argc;
        auto argv = boost::unit_test::framework::master_test_suite().argv;

        // we should have at least more than one arg, the first is the exe name, the rest is the input file paths
        BOOST_REQUIRE(argc > 1);

        for (int i(1); i < argc; ++i) {
            std::vector<std::string> arg;

            std::string raw_arg = argv[i];
            // dumb method, we erase all '-' not to bother with the '--bob=toto' format
            boost::erase_all(raw_arg, "-");
            boost::split(arg, raw_arg, boost::is_any_of("="));

            BOOST_REQUIRE_EQUAL(arg.size(), 2);
            input_file_paths[arg.at(0)] = arg.at(1);
        }
    }

    std::map<std::string, std::string> input_file_paths;
};

// check headsign value for stop times on given vjs
static void check_headsigns(const nt::Data& data,
                            const std::string& headsign,
                            const std::string& trip_id,
                            size_t first_it_st = 0,
                            size_t last_it_st = std::numeric_limits<size_t>::max()) {
    const nt::HeadsignHandler& headsigns = data.pt_data->headsign_handler;
    const auto vj_from_headsign = headsigns.get_vj_from_headsign(headsign);
    BOOST_REQUIRE_EQUAL(vj_from_headsign.size(), 2);

    for (const auto dst : {"_dst_1", "_dst_2"}) {
        const auto vj_id = "vehicle_journey:" + trip_id + dst;
        const auto* vj = data.pt_data->vehicle_journeys_map.at(vj_id);
        size_t real_last_it_st = vj->stop_time_list.size() - 1;
        if (real_last_it_st > last_it_st) {
            real_last_it_st = last_it_st;
        }
        for (size_t it_st = first_it_st; it_st <= real_last_it_st; ++it_st) {
            BOOST_CHECK_EQUAL(headsigns.get_headsign(vj->stop_time_list[it_st]), headsign);
        }
        BOOST_CHECK(navitia::contains(vj_from_headsign, vj));
    }
}

static void check_unsound_pickup_dropoff(const nt::Data& data) {
    for (const auto* vj : data.pt_data->vehicle_journeys) {
        if (!vj->prev_vj) {
            BOOST_CHECK(!vj->stop_time_list.front().drop_off_allowed());
        }
        if (!vj->next_vj) {
            BOOST_CHECK(!vj->stop_time_list.back().pick_up_allowed());
        }
    }
}

static void check_ntfs(const nt::Data& data) {
    BOOST_CHECK_EQUAL(data.meta->production_date,
                      boost::gregorian::date_period(boost::gregorian::from_undelimited_string("20150325"),
                                                    boost::gregorian::from_undelimited_string("20150827")));
    const auto& tz_manager = data.pt_data->tz_manager;
    BOOST_CHECK_EQUAL(tz_manager.get_nb_timezones(), 1);
    const auto* tz = tz_manager.get("Europe/Paris");
    BOOST_REQUIRE(tz);
    BOOST_REQUIRE_EQUAL(tz->tz_name, "Europe/Paris");
    BOOST_CHECK_EQUAL(tz->get_utc_offset(boost::gregorian::date(2015, 03, 25)), 60 * 60);
    BOOST_CHECK_EQUAL(tz->get_utc_offset(boost::gregorian::date(2015, 04, 01)), 2 * 60 * 60);

    BOOST_REQUIRE_EQUAL(data.pt_data->networks.size(), 1);
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->name, "ligne flixible");
    BOOST_CHECK_EQUAL(data.pt_data->networks[0]->uri, "network:ligneflexible");

    // check stop_time headsigns
    // check that vj 0 & 1 have headsign N1 from first 3 stop_time, then N2
    check_headsigns(data, "N1", "trip_1", 0, 2);
    check_headsigns(data, "N2", "trip_1", 3, 4);
    // vj 2 & 3 are named vehiclejourney2 with no headsign overload, 4-5 are named vehiclejourney3
    check_headsigns(data, "vehiclejourney2", "trip_2");
    check_headsigns(data, "vehiclejourney3", "trip_3");
    check_headsigns(data, "HS", "trip_4", 0, 3);
    check_headsigns(data, "NULL", "trip_4", 4, 5);

    BOOST_CHECK_EQUAL(data.pt_data->meta_vjs.size(), 5);
    for (auto& mvj : data.pt_data->meta_vjs) {
        BOOST_CHECK_EQUAL(data.pt_data->meta_vjs[mvj->uri], mvj.get());
        BOOST_CHECK_EQUAL(data.pt_data->meta_vjs[nr::MvjIdx(*mvj)], mvj.get());
    }

    // check physical/commercial modes
    // for NTFS we only got the modes defined in the files
    // + a default physical_mode since the trip_4 is not linked to a physical_mode
    BOOST_REQUIRE_EQUAL(data.pt_data->physical_modes.size(), 2);
    BOOST_REQUIRE_EQUAL(data.pt_data->commercial_modes.size(), 1);
    const auto* physical_bus = data.pt_data->physical_modes_map.at("physical_mode:BUS");
    BOOST_CHECK_EQUAL(physical_bus->name, "BUS");
    BOOST_CHECK_EQUAL(physical_bus->uri, "physical_mode:BUS");
    const auto* physical_default = data.pt_data->physical_modes_map.at("physical_mode:default_physical_mode");
    BOOST_CHECK_EQUAL(physical_default->name, "mode physique par défaut");
    BOOST_CHECK_EQUAL(physical_default->uri, "physical_mode:default_physical_mode");
    const auto* commercial_bus = data.pt_data->commercial_modes_map.at("commercial_mode:BUS");
    BOOST_CHECK_EQUAL(commercial_bus->name, "BUS");
    BOOST_CHECK_EQUAL(commercial_bus->uri, "commercial_mode:BUS");

    const auto& vj_map = data.pt_data->vehicle_journeys_map;
    BOOST_CHECK_EQUAL(vj_map.at("vehicle_journey:trip_1_dst_1")->physical_mode, physical_bus);
    BOOST_CHECK_EQUAL(vj_map.at("vehicle_journey:trip_4_dst_1")->physical_mode, physical_default);

    navitia::type::hasVehicleProperties has_vehicle_properties;
    has_vehicle_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(vj_map.at("vehicle_journey:trip_1_dst_1")->accessible(has_vehicle_properties.vehicles()), true);
    BOOST_CHECK_EQUAL(vj_map.at("vehicle_journey:trip_4_dst_1")->accessible(has_vehicle_properties.vehicles()), false);

    auto& routes_map = data.pt_data->routes_map;
    BOOST_REQUIRE_EQUAL(routes_map.size(), 4);

    BOOST_CHECK_EQUAL(routes_map.at("route:route_1")->direction_type, "forward");
    BOOST_CHECK_EQUAL(routes_map.at("route:route_2")->direction_type, "backward");
    BOOST_CHECK_EQUAL(routes_map.at("route:route_3")->direction_type, "");

    check_unsound_pickup_dropoff(data);

    BOOST_CHECK_EQUAL(data.meta->dataset_created_at, boost::posix_time::from_iso_string("20150415T153234"));

    auto& sp_map = data.pt_data->stop_points_map;
    navitia::type::hasProperties has_properties;
    has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(sp_map.at("stop_point:SP:A")->accessible(has_properties.properties()), true);
    BOOST_CHECK_EQUAL(sp_map.at("stop_point:SP:B")->accessible(has_properties.properties()), false);
}

BOOST_FIXTURE_TEST_CASE(fusio_test, ArgsFixture) {
    const auto input_file = input_file_paths.at("ntfs_file");
    nt::Data data;

    bool failed = false;
    try {
        data.load_nav(input_file);
    } catch (const navitia::data::data_loading_error&) {
        failed = true;
    }
    data.build_raptor();
    BOOST_REQUIRE_EQUAL(failed, false);

    check_ntfs(data);

    BOOST_REQUIRE_EQUAL(data.pt_data->contributors.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->contributors[0]->license, "LICENSE");
    BOOST_REQUIRE_EQUAL(data.pt_data->contributors[0]->website, "http://www.canaltp.fr");

    // Here we check trip_short_name as well as headsign of some vjs
    const nt::HeadsignHandler& headsigns = data.pt_data->headsign_handler;
    // vjs with headsign="vehiclejourney1" and trip_short_name="trip_1_short_name"
    const auto vj_from_headsign_1 = headsigns.get_vj_from_headsign("vehiclejourney1");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_1.size(), 2);
    BOOST_REQUIRE_EQUAL(vj_from_headsign_1[0]->headsign, "vehiclejourney1");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_1[0]->name, "trip_1_short_name");
    // vjs with headsign="vehiclejourney3" but without trip_short_name
    const auto vj_from_headsign_2 = headsigns.get_vj_from_headsign("vehiclejourney3");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_2.size(), 2);
    BOOST_REQUIRE_EQUAL(vj_from_headsign_2[0]->headsign, "vehiclejourney3");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_2[0]->name, "vehiclejourney3");
    // vjs without headsign but with trip_short_name="trip_5_short_name"
    const auto vj_from_headsign_3 = headsigns.get_vj_from_headsign("trip_5_dst_1");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_3.size(), 1);
    BOOST_REQUIRE_EQUAL(vj_from_headsign_3[0]->headsign, "trip_5_dst_1");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_3[0]->name, "trip_5_short_name");
    const auto vj_from_headsign_4 = headsigns.get_vj_from_headsign("trip_5_dst_2");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_4.size(), 1);
    BOOST_REQUIRE_EQUAL(vj_from_headsign_4[0]->headsign, "trip_5_dst_2");
    BOOST_REQUIRE_EQUAL(vj_from_headsign_4[0]->name, "trip_5_short_name");
}

BOOST_FIXTURE_TEST_CASE(gtfs_test, ArgsFixture) {
    const auto input_file = input_file_paths.at("gtfs_google_example_file");
    navitia::type::Data data;

    bool failed = false;
    try {
        data.load_nav(input_file);
    } catch (const navitia::data::data_loading_error&) {
        failed = true;
    }
    data.build_raptor();
    BOOST_REQUIRE_EQUAL(failed, false);

    const auto& pt_data = *data.pt_data;

    BOOST_CHECK_EQUAL(data.meta->production_date,
                      boost::gregorian::date_period(boost::gregorian::from_undelimited_string("20070101"),
                                                    boost::gregorian::from_undelimited_string("20080102")));

    const auto& tz_manager = data.pt_data->tz_manager;
    BOOST_CHECK_EQUAL(tz_manager.get_nb_timezones(), 1);
    const auto* tz = tz_manager.get("America/Los_Angeles");
    BOOST_REQUIRE(tz);
    BOOST_REQUIRE_EQUAL(tz->tz_name, "America/Los_Angeles");
    BOOST_CHECK_EQUAL(tz->get_utc_offset(boost::gregorian::date(2007, 04, 01)), -420 * 60);
    BOOST_CHECK_EQUAL(tz->get_utc_offset(boost::gregorian::date(2007, 12, 01)), -480 * 60);

    // TODO add generic check on collections
    // check map/vec same size + uri + idx + ...

    BOOST_REQUIRE_EQUAL(pt_data.networks.size(), 1);
    BOOST_CHECK_EQUAL(pt_data.networks[0]->name, "Demo Transit Authority");
    BOOST_CHECK_EQUAL(pt_data.networks[0]->uri, "network:DTA");

    // check physical/commercial modes
    // for GTFS we got all the default ones
    BOOST_REQUIRE_EQUAL(pt_data.physical_modes.size(), 7);  // less physical mode, some are aggregated
    BOOST_REQUIRE_EQUAL(pt_data.commercial_modes.size(), 8);
    // we check one of each
    const auto* physical_bus = pt_data.physical_modes_map.at("physical_mode:Bus");
    BOOST_REQUIRE_EQUAL(physical_bus->name, "Bus");
    BOOST_REQUIRE_EQUAL(physical_bus->uri, "physical_mode:Bus");
    const auto* physical_tram = pt_data.physical_modes_map.at("physical_mode:Tramway");
    BOOST_REQUIRE_EQUAL(physical_tram->name, "Tramway");
    BOOST_REQUIRE_EQUAL(physical_tram->uri, "physical_mode:Tramway");
    const auto* physical_suspended = pt_data.physical_modes_map.at("physical_mode:SuspendedCableCar");
    BOOST_REQUIRE_EQUAL(physical_suspended->name, "Suspended Cable Car");
    BOOST_REQUIRE_EQUAL(physical_suspended->uri, "physical_mode:SuspendedCableCar");
    const auto* commercial_bus = pt_data.commercial_modes_map.at("commercial_mode:Bus");
    BOOST_REQUIRE_EQUAL(commercial_bus->name, "Bus");
    BOOST_REQUIRE_EQUAL(commercial_bus->uri, "commercial_mode:Bus");
    const auto* commercial_tram = pt_data.commercial_modes_map.at("commercial_mode:Tramway");
    BOOST_REQUIRE_EQUAL(commercial_tram->name, "Tram, Streetcar, Light rail");
    BOOST_REQUIRE_EQUAL(commercial_tram->uri, "commercial_mode:Tramway");
    const auto* commercial_metro = pt_data.commercial_modes_map.at("commercial_mode:Metro");
    BOOST_REQUIRE_EQUAL(commercial_metro->name, "Subway, Metro");
    BOOST_REQUIRE_EQUAL(commercial_metro->uri, "commercial_mode:Metro");

    BOOST_REQUIRE_EQUAL(pt_data.stop_areas.size(), 9);
    const auto* fur_creek = pt_data.stop_areas_map.at("stop_area:Navitia:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek->uri, "stop_area:Navitia:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(fur_creek->coord.lat(), 36.425288, 0.001);
    BOOST_CHECK_CLOSE(fur_creek->coord.lon(), -117.133162, 0.001);

    for (auto sa : pt_data.stop_areas) {
        BOOST_CHECK_EQUAL(sa->timezone, "America/Los_Angeles");
    }

    // stop point, and lines should be equals too
    BOOST_REQUIRE_EQUAL(pt_data.stop_points.size(), 9);
    const auto* fur_creek_sp = pt_data.stop_points_map.at("stop_point:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek_sp->uri, "stop_point:FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(fur_creek_sp->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(fur_creek_sp->coord.lat(), 36.425288, 0.001);
    BOOST_CHECK_CLOSE(fur_creek_sp->coord.lon(), -117.133162, 0.001);
    BOOST_CHECK_EQUAL(fur_creek_sp->stop_area, fur_creek);

    // no connection in the gtfs provided, so we only create only cnx
    // for each stop_point
    // Note: the dataset is kind of useless without cnx but for
    // the moment we need them to be explicitly provided
    BOOST_REQUIRE_EQUAL(pt_data.stop_point_connections.size(), 9);

    BOOST_REQUIRE_EQUAL(pt_data.lines.size(), 5);
    const auto* ab_line = pt_data.lines_map.at("line:AB");
    BOOST_CHECK_EQUAL(ab_line->uri, "line:AB");
    BOOST_CHECK_EQUAL(ab_line->name, "Airport - Bullfrog");
    BOOST_REQUIRE(ab_line->network);
    BOOST_CHECK_EQUAL(ab_line->network->uri, "network:DTA");
    BOOST_REQUIRE(ab_line->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(ab_line->commercial_mode->uri, "commercial_mode:Bus");
    BOOST_REQUIRE_EQUAL(ab_line->route_list.size(), 2);
    BOOST_CHECK(boost::find_if(ab_line->route_list, [](nt::Route* r) { return r->uri == "route:AB:0"; })
                != std::end(ab_line->route_list));
    BOOST_CHECK(boost::find_if(ab_line->route_list, [](nt::Route* r) { return r->uri == "route:AB:1"; })
                != std::end(ab_line->route_list));

    // we need to also check the number of routes created
    BOOST_REQUIRE_EQUAL(pt_data.routes.size(), 9);
    for (const auto& r : pt_data.routes) {
        BOOST_REQUIRE(r->line);
        BOOST_CHECK_EQUAL(r->name, r->line->name);  // route's name is it's line's name
    }

    BOOST_CHECK_EQUAL(data.meta->production_date, boost::gregorian::date_period("20070101"_d, "20080102"_d));

    BOOST_REQUIRE_EQUAL(pt_data.vehicle_journeys.size(), 11 * 2);
    const auto* vj_ab1_dst1 = pt_data.vehicle_journeys_map.at("vehicle_journey:AB1_dst_1");
    BOOST_CHECK_EQUAL(vj_ab1_dst1->uri, "vehicle_journey:AB1_dst_1");
    // TODO check all vp
    BOOST_CHECK_EQUAL(vj_ab1_dst1->name, "to Bullfrog");
    BOOST_REQUIRE_EQUAL(vj_ab1_dst1->stop_time_list.size(), 2);
    // UTC offset is 8h in winter for LA
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[0].arrival_time, "08:00"_t + "08:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[0].departure_time, "08:00"_t + "08:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[1].arrival_time, "08:10"_t + "08:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->stop_time_list[1].departure_time, "08:15"_t + "08:00"_t);
    BOOST_REQUIRE(vj_ab1_dst1->next_vj);
    BOOST_CHECK_EQUAL(vj_ab1_dst1->next_vj->uri, "vehicle_journey:BFC1_dst_1");
    // and ab1 should be the previous vj of it
    BOOST_CHECK_EQUAL(vj_ab1_dst1->next_vj->prev_vj, vj_ab1_dst1);

    // we also check the other AB1 split for the summer DST
    const auto* vj_ab1_dst2 = pt_data.vehicle_journeys_map.at("vehicle_journey:AB1_dst_2");
    // they should have the same meta vj
    BOOST_CHECK_EQUAL(vj_ab1_dst2->meta_vj, vj_ab1_dst1->meta_vj);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->uri, "vehicle_journey:AB1_dst_2");
    BOOST_CHECK_EQUAL(vj_ab1_dst2->name, "to Bullfrog");
    BOOST_REQUIRE_EQUAL(vj_ab1_dst2->stop_time_list.size(), 2);
    // UTC offset is 7h in summer for LA
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[0].arrival_time, "08:00"_t + "07:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[0].departure_time, "08:00"_t + "07:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[1].arrival_time, "08:10"_t + "07:00"_t);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->stop_time_list[1].departure_time, "08:15"_t + "07:00"_t);
    BOOST_REQUIRE(vj_ab1_dst2->next_vj);
    BOOST_CHECK_EQUAL(vj_ab1_dst2->next_vj->uri, "vehicle_journey:BFC1_dst_2");
    // and ab1 should be the previous vj of it
    BOOST_CHECK_EQUAL(vj_ab1_dst2->next_vj->prev_vj, vj_ab1_dst2);

    // check that STBA is a frequency vj
    const auto* vj_stba = pt_data.vehicle_journeys_map.at("vehicle_journey:STBA_dst_1");
    const auto* vj_stba_freq = dynamic_cast<const nt::FrequencyVehicleJourney*>(vj_stba);
    BOOST_REQUIRE(vj_stba_freq);
    BOOST_CHECK_EQUAL(vj_stba_freq->start_time, "14:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->end_time, "30:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->headway_secs, 1800);
    for (const auto& st : vj_stba->stop_time_list) {
        BOOST_CHECK(st.is_frequency());
    }
    vj_stba = pt_data.vehicle_journeys_map.at("vehicle_journey:STBA_dst_2");
    vj_stba_freq = dynamic_cast<const nt::FrequencyVehicleJourney*>(vj_stba);
    BOOST_REQUIRE(vj_stba_freq);
    BOOST_CHECK_EQUAL(vj_stba_freq->start_time, "13:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->end_time, "29:00"_t);
    BOOST_CHECK_EQUAL(vj_stba_freq->headway_secs, 1800);
    for (const auto& st : vj_stba->stop_time_list) {
        BOOST_CHECK(st.is_frequency());
    }

    check_unsound_pickup_dropoff(data);
}

BOOST_FIXTURE_TEST_CASE(ntfs_v5_test, ArgsFixture) {
    const auto input_file = input_file_paths.at("ntfs_v5_file");
    nt::Data data;

    bool failed = false;
    try {
        data.load_nav(input_file);
    } catch (const navitia::data::data_loading_error&) {
        failed = true;
    }
    data.build_raptor();
    BOOST_REQUIRE_EQUAL(failed, false);

    BOOST_REQUIRE_EQUAL(data.pt_data->lines.size(), 4);
    BOOST_REQUIRE_EQUAL(data.pt_data->routes.size(), 4);

    BOOST_REQUIRE_EQUAL(data.pt_data->datasets.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->contributors.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->contributors[0], data.pt_data->datasets[0]->contributor);
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->desc, "dataset_test");
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->uri, "d1");
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->validation_period,
                      boost::gregorian::date_period("20150826"_d, "20150926"_d));
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->realtime_level == nt::RTLevel::Base, true);
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->system, "obiti");

    // accepted side-effect (no link) as ntfs_v5 fixture does not contain datasets.txt, which is now required
    BOOST_CHECK(data.pt_data->vehicle_journeys[0]->dataset == nullptr);

    BOOST_REQUIRE_EQUAL(data.pt_data->networks.size(), 1);
    BOOST_CHECK_EQUAL(data.pt_data->codes.get_codes(data.pt_data->networks[0]),
                      (nt::CodeContainer::Codes{{"external_code", {"FilvertTAD", "FilbleuTAD"}}}));
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points.size(), 8);
    BOOST_CHECK_EQUAL(data.pt_data->codes.get_codes(data.pt_data->stop_points_map["stop_point:SP:A"]),
                      (nt::CodeContainer::Codes{{"external_code", {"A"}}, {"source", {"A", "Ahah", "Aïe"}}}));

    // SP E
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:E"]->address_id, "SP:E:ADD_ID");
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:E"]->address->number, 0);
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:E"]->address->way->name, "SP:E STREET_NAME");

    // SP A - This is a edge case, address number is empty and street name too, within the data
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:A"]->address->number, 0);
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:A"]->address->way->name, "");

    // SP D - the classical case
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:D"]->address->number, 25);
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:D"]->address->way->name, "SP:D STREET_NAME");

    //// SP F - without the number
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:F"]->address->number, 0);
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:F"]->address->way->name, "SP:F STREET_NAME");

    // SP J - without the number
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:J"]->address->number, 0);
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:J"]->address->way->name,
                        "FACE AU 23 SP:J STREET_NAME");

    // Access Point
    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:A"]->access_points.size(), 1);
    for (const auto& ap : data.pt_data->stop_points_map["stop_point:SP:A"]->access_points) {
        BOOST_REQUIRE_EQUAL(ap->uri, "IO:1");
        BOOST_REQUIRE_EQUAL(ap->stop_code, "stop_code_io_1");
        BOOST_REQUIRE_EQUAL(ap->length, 68);
        BOOST_REQUIRE_EQUAL(ap->traversal_time, 87);
        BOOST_REQUIRE_EQUAL(ap->is_entrance, true);
        BOOST_REQUIRE_EQUAL(ap->is_exit, true);
        BOOST_REQUIRE_EQUAL(ap->stair_count, -1);             // no value
        BOOST_REQUIRE_EQUAL(ap->max_slope, -1);               // no value
        BOOST_REQUIRE_EQUAL(ap->min_width, -1);               // no value
        BOOST_REQUIRE_EQUAL(ap->signposted_as, "");           // no value
        BOOST_REQUIRE_EQUAL(ap->reversed_signposted_as, "");  // no value
        BOOST_CHECK_CLOSE(ap->coord.lat(), 45.0614, 0.001);
        BOOST_CHECK_CLOSE(ap->coord.lon(), 0.6155, 0.001);
    }

    BOOST_REQUIRE_EQUAL(data.pt_data->stop_points_map["stop_point:SP:B"]->access_points.size(), 1);
    for (const auto& ap : data.pt_data->stop_points_map["stop_point:SP:B"]->access_points) {
        BOOST_REQUIRE_EQUAL(ap->uri, "IO:2");
        BOOST_REQUIRE_EQUAL(ap->stop_code, "stop_code_io_2");
        BOOST_REQUIRE_EQUAL(ap->length, 68);
        BOOST_REQUIRE_EQUAL(ap->traversal_time, 87);
        BOOST_REQUIRE_EQUAL(ap->is_entrance, false);
        BOOST_REQUIRE_EQUAL(ap->is_exit, true);
        BOOST_REQUIRE_EQUAL(ap->stair_count, 3);
        BOOST_REQUIRE_EQUAL(ap->max_slope, 30);
        BOOST_REQUIRE_EQUAL(ap->min_width, 2);
        BOOST_REQUIRE_EQUAL(ap->signposted_as, "");           // no value
        BOOST_REQUIRE_EQUAL(ap->reversed_signposted_as, "");  // no value
        BOOST_CHECK_CLOSE(ap->coord.lat(), 45.0614, 0.001);
        BOOST_CHECK_CLOSE(ap->coord.lon(), 0.6155, 0.001);
    }

    check_ntfs(data);
}

BOOST_FIXTURE_TEST_CASE(osm_pois_id_should_match_mimir_naming, ArgsFixture) {
    navitia::type::Data data;
    BOOST_REQUIRE_NO_THROW(data.load_nav(input_file_paths.at("osm_and_gtfs_file")););
    data.build_raptor();

    const auto& pois = data.geo_ref->pois;
    BOOST_REQUIRE_GT(pois.size(), 0);

    BOOST_CHECK(
        navitia::contains_if(pois, [&](const navitia::georef::POI* p) { return p->uri == "poi:osm:way:551462554"; }));
    BOOST_CHECK(
        navitia::contains_if(pois, [&](const navitia::georef::POI* p) { return p->uri == "poi:osm:node:5414687331"; }));
}

BOOST_FIXTURE_TEST_CASE(poi2ed_pois_uri_should_match_mimir_naming, ArgsFixture) {
    navitia::type::Data data;
    BOOST_REQUIRE_NO_THROW(data.load_nav(input_file_paths.at("poi_file")););
    data.build_raptor();

    const auto& pois = data.geo_ref->pois;
    BOOST_REQUIRE_GT(pois.size(), 0);

    BOOST_CHECK(navitia::contains_if(pois, [&](const navitia::georef::POI* p) { return p->uri == "poi:ADM44_100"; }));
    BOOST_CHECK(
        navitia::contains_if(pois, [&](const navitia::georef::POI* p) { return p->uri == "poi:CULT44_30021"; }));
    BOOST_CHECK(navitia::contains_if(
        pois, [&](const navitia::georef::POI* p) { return p->uri == "poi:PAIHABIT0000000028850973"; }));
}

BOOST_FIXTURE_TEST_CASE(ntfs_dst_test, ArgsFixture) {
    const auto input_file = input_file_paths.at("ntfs_dst_file");
    nt::Data data;

    bool failed = false;
    try {
        data.load_nav(input_file);
    } catch (const navitia::data::data_loading_error&) {
        failed = true;
    }
    data.build_raptor();
    BOOST_REQUIRE_EQUAL(failed, false);

    BOOST_CHECK_EQUAL(data.pt_data->lines.size(), 2);
    BOOST_CHECK_EQUAL(data.pt_data->routes.size(), 1);

    BOOST_REQUIRE_EQUAL(data.pt_data->datasets.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->contributors.size(), 1);
    BOOST_REQUIRE_EQUAL(data.pt_data->contributors[0], data.pt_data->datasets[0]->contributor);
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->desc, "centre-sncf");
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->uri, "SCF:23");
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->validation_period,
                      boost::gregorian::date_period("20180319"_d, "20180617"_d));
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->realtime_level == nt::RTLevel::Base, true);
    BOOST_CHECK_EQUAL(data.pt_data->datasets[0]->system, "ChouetteV2");
    BOOST_REQUIRE_EQUAL(data.pt_data->vehicle_journeys.size(), 2);
    BOOST_CHECK_EQUAL(data.pt_data->vehicle_journeys[0]->dataset->uri, "SCF:23");

    const auto* vj_dst1 = data.pt_data->vehicle_journeys_map.at("vehicle_journey:SCF:OCESN010410R01001-1_dst_1");
    BOOST_CHECK_EQUAL(vj_dst1->uri, "vehicle_journey:SCF:OCESN010410R01001-1_dst_1");

    BOOST_REQUIRE_EQUAL(vj_dst1->stop_time_list.size(), 12);

    // we also check the other AB1 split for the summer DST
    const auto* vj_dst2 = data.pt_data->vehicle_journeys_map.at("vehicle_journey:SCF:OCESN010410R01001-1_dst_2");
    // they should have the same meta vj
    BOOST_CHECK_EQUAL(vj_dst2->meta_vj, vj_dst1->meta_vj);
}
