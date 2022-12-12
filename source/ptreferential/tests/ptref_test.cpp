/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
#define BOOST_TEST_MODULE ptref_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
#include "ptreferential/ptreferential_api.h"
#include "ptreferential/ptref_graph.h"
#include "ed/build_helper.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include "type/pt_data.h"
#include "tests/utils_test.h"
#include "kraken/apply_disruption.h"
#include <boost/range/adaptors.hpp>

namespace nt = navitia::type;
using namespace navitia::ptref;
using navitia::type::Type_e;

BOOST_AUTO_TEST_CASE(sans_filtre) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250);
        b.vj("B")("stop3", 9000, 9050)("stop4", 9200, 9250);
        b.connection("stop2", "stop3", 10 * 60);
        b.connection("stop3", "stop2", 10 * 60);
    });

    auto indexes = make_query(nt::Type_e::Line, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(nt::Type_e::JourneyPattern, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(nt::Type_e::StopPoint, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 4);

    indexes = make_query(nt::Type_e::StopArea, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 4);

    indexes = make_query(nt::Type_e::Network, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::PhysicalMode, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 8);

    indexes = make_query(nt::Type_e::CommercialMode, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 7);

    indexes = make_query(nt::Type_e::Connection, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(nt::Type_e::JourneyPatternPoint, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 4);

    indexes = make_query(nt::Type_e::VehicleJourney, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(nt::Type_e::Route, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);
}

BOOST_AUTO_TEST_CASE(physical_modes) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        // Physical_mode = Car
        b.vj("A", "11110000", "", true, "", "", "", "physical_mode:Car")("stop1", 8000, 8050)("stop2", 8200, 8250);
        // Physical_mode = Metro
        b.vj("A", "00001111", "", true, "", "", "", "physical_mode:0x1")("stop1", 8000, 8050)("stop2", 8200, 8250)(
            "stop3", 8500, 8500);
        // Physical_mode = Tram
        auto* vj_c = b.vj("C")("stop3", 9000, 9050)("stop4", 9200, 9250).make();
        b.connection("stop2", "stop3", 10 * 60);
        b.connection("stop3", "stop2", 10 * 60);

        auto* contributor = new navitia::type::Contributor();
        contributor->idx = b.data->pt_data->contributors.size();
        contributor->uri = "c1";
        contributor->name = "name-c1";
        b.data->pt_data->contributors.push_back(contributor);

        auto* dataset = new navitia::type::Dataset();
        dataset->idx = b.data->pt_data->datasets.size();
        dataset->uri = "f1";
        dataset->name = "name-f1";
        dataset->contributor = contributor;
        contributor->dataset_list.insert(dataset);
        b.data->pt_data->datasets.push_back(dataset);
        vj_c->dataset = dataset;
    });

    auto indexes = make_query(nt::Type_e::Line, "line.uri=A", *(b.data));
    auto objects = get_objects<nt::Line>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(objects.size(), 1);
    BOOST_REQUIRE_EQUAL((*objects.begin())->physical_mode_list.size(), 2);
    BOOST_CHECK_EQUAL((*objects.begin())->physical_mode_list.at(0)->name, "Car");
    BOOST_CHECK_EQUAL((*objects.begin())->physical_mode_list.at(1)->name, "Metro");

    indexes = make_query(nt::Type_e::Line, "line.uri=C", *(b.data));
    objects = get_objects<nt::Line>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(objects.size(), 1);
    BOOST_REQUIRE_EQUAL((*objects.begin())->physical_mode_list.size(), 1);
    BOOST_CHECK_EQUAL((*objects.begin())->physical_mode_list.at(0)->name, "Tramway");

    indexes = make_query(nt::Type_e::Dataset, "stop_point.uri=stop1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset"}));

    indexes = make_query(nt::Type_e::Dataset, "stop_point.uri=stop2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset"}));

    indexes = make_query(nt::Type_e::Dataset, "stop_point.uri=stop3", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset", "f1"}));

    indexes = make_query(nt::Type_e::Dataset, "dataset.uri=default:dataset", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset"}));

    indexes = make_query(nt::Type_e::Contributor, "dataset.uri=f1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Contributor>(indexes, *b.data), std::set<std::string>({"c1"}));
}

BOOST_AUTO_TEST_CASE(get_indexes_test) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250);
        b.vj("B")("stop3", 9000, 9050)("stop4", 9200, 9250);
        b.connection("stop2", "stop3", 10 * 60);
        b.connection("stop3", "stop2", 10 * 60);
    });

    // On cherche �� retrouver la ligne 1, en passant le stoparea en filtre
    auto indexes = make_query(Type_e::Line, "stop_area.uri = stop1", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, nt::make_indexes({0}));

    // On cherche les stopareas de la ligneA
    indexes = make_query(Type_e::StopArea, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, nt::make_indexes({0, 1}));
}

BOOST_AUTO_TEST_CASE(get_impact_indexes_of_line) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });

    using btp = boost::posix_time::time_period;
    const auto& disrup_1 = b.impact(nt::RTLevel::RealTime, "Disruption 1")
                               .severity(nt::disruption::Effect::NO_SERVICE)
                               .on(nt::Type_e::Line, "A", *b.data->pt_data)
                               .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                               .get_disruption();

    navitia::apply_disruption(disrup_1, *b.data->pt_data, *b.data->meta);
    auto indexes = make_query(Type_e::Impact, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>{0});

    navitia::delete_disruption("Disruption 1", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_THROW(make_query(Type_e::Impact, "line.uri = A", *b.data), ptref_error);

    const auto& disrup_2 = b.impact(nt::RTLevel::RealTime, "Disruption 2")
                               .severity(nt::disruption::Effect::NO_SERVICE)
                               .on(nt::Type_e::Line, "A", *b.data->pt_data)
                               .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                               .get_disruption();

    navitia::apply_disruption(disrup_2, *b.data->pt_data, *b.data->meta);
    indexes = make_query(Type_e::Impact, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>{0});

    const auto& disrup_3 = b.impact(nt::RTLevel::RealTime, "Disruption 3")
                               .severity(nt::disruption::Effect::NO_SERVICE)
                               .on(nt::Type_e::Line, "A", *b.data->pt_data)
                               .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                               .get_disruption();

    navitia::apply_disruption(disrup_3, *b.data->pt_data, *b.data->meta);
    indexes = make_query(Type_e::Impact, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, nt::make_indexes({0, 1}));
}

BOOST_AUTO_TEST_CASE(get_impact_indexes_of_stop_point) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    });

    using btp = boost::posix_time::time_period;
    const auto& disrup_1 = b.impact(nt::RTLevel::RealTime, "Disruption 1")
                               .severity(nt::disruption::Effect::NO_SERVICE)
                               .on(nt::Type_e::StopPoint, "stop1", *b.data->pt_data)
                               .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                               .get_disruption();

    navitia::apply_disruption(disrup_1, *b.data->pt_data, *b.data->meta);
    auto indexes = make_query(Type_e::Impact, "stop_point.uri = stop1", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>{0});
}

BOOST_AUTO_TEST_CASE(ptref_on_vj_impacted) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj("A", "000001", "", true, "vj:A-2")("stop1", "08:20"_t)("stop2", "09:20"_t);
        b.vj("A", "000001", "", true, "vj:A-3")("stop1", "08:30"_t)("stop2", "09:30"_t);
    });

    // no disruptions
    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "vehicle_journey.has_disruption()", *b.data), ptref_error);
    // disruption with Effect OTHER_EFFECT on vj:A-1
    using btp = boost::posix_time::time_period;
    const auto& disrup_1 = b.impact(nt::RTLevel::RealTime, "Disruption 1")
                               .severity(nt::disruption::Effect::OTHER_EFFECT)
                               .on(nt::Type_e::MetaVehicleJourney, "vj:A-1", *b.data->pt_data)
                               .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                               .get_disruption();
    navitia::apply_disruption(disrup_1, *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "vehicle_journey.has_disruption()", *b.data), ptref_error);
    // disruption with Effect NO_SERVICE on vj:A-2
    const auto& disrup_2 = b.impact(nt::RTLevel::RealTime, "Disruption 2")
                               .severity(nt::disruption::Effect::NO_SERVICE)
                               .on(nt::Type_e::MetaVehicleJourney, "vj:A-2", *b.data->pt_data)
                               .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                               .get_disruption();
    navitia::apply_disruption(disrup_2, *b.data->pt_data, *b.data->meta);
    navitia::type::Indexes vj_idx = make_query(nt::Type_e::VehicleJourney, "vehicle_journey.has_disruption()", *b.data);

    BOOST_CHECK_EQUAL(vj_idx.size(), 1);
    BOOST_CHECK_EQUAL(get_uris<nt::VehicleJourney>(vj_idx, *b.data), std::set<std::string>({"vehicle_journey:vj:A-2"}));
}

BOOST_AUTO_TEST_CASE(make_query_filtre_direct) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250);
        b.vj("B")("stop3", 9000, 9050)("stop4", 9200, 9250);
        b.connection("stop2", "stop3", 10 * 60);
        b.connection("stop3", "stop2", 10 * 60);
    });

    auto indexes = make_query(nt::Type_e::Line, "line.uri=A", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::JourneyPattern, "journey_pattern.uri=journey_pattern:0", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::StopPoint, "stop_point.uri=stop1", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::StopArea, "stop_area.uri=stop1", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::Network, "network.uri=base_network", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::PhysicalMode, "physical_mode.uri=physical_mode:0x1", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::CommercialMode, "commercial_mode.uri=0x1", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);

    indexes = make_query(nt::Type_e::Connection, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(nt::Type_e::VehicleJourney, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(nt::Type_e::Route, "route.uri=A:0", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 1);
}

BOOST_AUTO_TEST_CASE(line_code) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050);
        b.lines["A"]->code = "line_A";
        b.vj("C")("stop2", 8000, 8050);
        b.lines["C"]->code = "line C";
    });

    // find line by code
    auto indexes = make_query(nt::Type_e::Line, "line.code=line_A", *(b.data));
    auto objects = get_objects<nt::Line>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(objects.size(), 1);
    BOOST_CHECK_EQUAL((*objects.begin())->code, "line_A");

    indexes = make_query(nt::Type_e::Line, "line.code=\"line C\"", *(b.data));
    objects = get_objects<nt::Line>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(objects.size(), 1);
    BOOST_CHECK_EQUAL((*objects.begin())->code, "line C");

    // no line B
    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, "line.code=\"line B\"", *(b.data)), ptref_error);

    // find route by line code
    indexes = make_query(nt::Type_e::Route, "line.code=line_A", *(b.data));
    auto routes = get_objects<nt::Route>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(routes.size(), 1);
    BOOST_CHECK_EQUAL((*routes.begin())->line->code, "line_A");

    // route has no code attribute, error
    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, "route.code=test", *(b.data)), ptref_error);
}

BOOST_AUTO_TEST_CASE(forbidden_uri) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250);
        b.vj("B")("stop3", 9000, 9050)("stop4", 9200, 9250);
        b.connection("stop2", "stop3", 10 * 60);
        b.connection("stop3", "stop2", 10 * 60);
    });

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, "stop_point.uri=stop1", {"A"}, *(b.data)), ptref_error);
}

BOOST_AUTO_TEST_CASE(find_path_test) {
    auto res = find_path(nt::Type_e::Route);
    // Cas o�� on veut partir et arriver au m��me point
    BOOST_CHECK(res[nt::Type_e::Route] == nt::Type_e::Route);
    // On regarde qu'il y a un semblant d'itin��raire pour chaque type : il faut un pr��d��cesseur
    for (auto node_pred : res) {
        // Route c'est lui m��me puisque c'est la source, et validity pattern est puni, donc il est seul dans son coin,
        // de m��me pour POI et POIType
        if (node_pred.first != Type_e::Route && node_pred.first != Type_e::ValidityPattern
            && node_pred.first != Type_e::POI && node_pred.first != Type_e::POIType
            && node_pred.first != Type_e::Contributor && node_pred.first != Type_e::Impact
            && node_pred.first != Type_e::Dataset)
            BOOST_CHECK(node_pred.first != node_pred.second);
    }

    // On v��rifie qu'il n'y ait pas de n��ud qui ne soit pas atteignable depuis un autre n��ud
    // En connexit�� non forte, il y a un seul ensemble : validity pattern est reli�� (en un seul sens) au gros pat��
    // Avec les composantes fortement connexes : there must be only two : validity pattern est isol�� car on ne peut y
    // aller
    Jointures j;
    std::vector<Jointures::vertex_t> component(boost::num_vertices(j.g));
    int num = boost::connected_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 2);
    num = boost::strong_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 3);

    // Type qui n'existe pas dans le graph : il n'y a pas de chemin
    BOOST_CHECK_THROW(find_path(nt::Type_e::Unknown), ptref_error);
}

// helper to get default values
static nt::Indexes query(nt::Type_e requested_type,
                         std::string request,
                         const nt::Data& data,
                         const boost::optional<boost::posix_time::ptime>& since = boost::none,
                         const boost::optional<boost::posix_time::ptime>& until = boost::none,
                         bool fail = false) {
    try {
        return navitia::ptref::make_query(requested_type, request, {}, nt::OdtLevel_e::all, since, until, data);
    } catch (const navitia::ptref::ptref_error& e) {
        if (fail) {
            throw;  // rethrow (the right way)
        }
        BOOST_CHECK_MESSAGE(false, " error on ptref request " + std::string(e.what()));
        return nt::Indexes{};
    }
}

/*
 * Test the meta-vj filtering on vj
 */
BOOST_AUTO_TEST_CASE(mvj_filtering) {
    ed::builder builder("20130311", [](ed::builder& b) {
        // Date  11    12    13    14
        // A      -   08:00 08:00   -
        // B    10:00 10:00   -   10:00
        // C      -     -     -   10:00
        b.vj("A", "0110")("stop1", "08:00"_t);
        b.vj("B", "1011")("stop3", "10:00"_t);
        b.vj("C", "1000")("stop3", "10:00"_t);
    });

    nt::idx_t a = 0;
    nt::idx_t b = 1;
    nt::idx_t c = 2;

    // not limited, we get 3 mvj
    auto indexes = query(nt::Type_e::MetaVehicleJourney, "", *(builder.data));
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({0, 1, 2}));

    // looking for MetaVJ 0
    indexes = make_query(nt::Type_e::MetaVehicleJourney, R"(trip.uri="vj 0")", *(builder.data));
    BOOST_REQUIRE_EQUAL(indexes.size(), 1);
    const auto mvj_idx = navitia::Idx<nt::MetaVehicleJourney>(*indexes.begin());
    BOOST_CHECK_EQUAL(builder.data->pt_data->meta_vjs[mvj_idx]->uri, "vj 0");

    // looking for MetaVJ A through VJ A
    indexes = make_query(Type_e::MetaVehicleJourney, "vehicle_journey.uri = vehicle_journey:A:0", *builder.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, {0})

    // not limited, we get 3 vj
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data));
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c}));

    // looking for VJ B through MetaVJ B
    indexes = make_query(Type_e::VehicleJourney, R"(trip.uri="vj 1")", *builder.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, {b})
}

/*
 * Test the filtering on the period
 */
BOOST_AUTO_TEST_CASE(vj_filtering) {
    ed::builder builder("20130311", [](ed::builder& b) {
        // Date  11    12    13    14
        // A      -   08:00 08:00   -
        // B    10:00 10:00   -   10:00
        // C      -     -     -   10:00
        b.vj("A", "0110")("stop1", "08:00"_t)("stop2", "09:00"_t);
        b.vj("B", "1011")("stop3", "10:00"_t)("stop2", "11:00"_t);
        b.vj("C", "1000")("stop3", "10:00"_t)("stop2", "11:00"_t);
    });
    nt::idx_t a = 0;
    nt::idx_t b = 1;
    nt::idx_t c = 2;

    // not limited, we get 3 vj
    auto indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data));
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c}));

    // the second day A and B are activ
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130312T0700"_dt}, {"20130313T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b}));

    // but the third only A works
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130313T0700"_dt}, {"20130314T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, {a});

    // on day 3 and 4, A, B and C are valid only one day, so it's ok
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130313T0700"_dt}, {"20130315T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c}));

    // with just a since, we check til the end of the period
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130313T0000"_dt}, {});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c}));  // all are valid from the 3rd day

    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130314T0000"_dt}, {});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({b, c}));  // only B and C are valid from the 4th day

    // with just an until, we check from the begining of the validity period
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {}, {"20130313T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b}));

    // tests with the time
    // we want the vj valid from 11pm only the first day, B is valid at 10, so not it the period
    BOOST_CHECK_THROW(
        query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130311T1100"_dt}, {"20130312T0000"_dt}, true),
        ptref_error);

    // we want the vj valid from 11pm the first day to 08:00 (excluded) the second, we still have no vj
    BOOST_CHECK_THROW(
        query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130311T1100"_dt}, {"20130312T0759"_dt}, true),
        ptref_error);

    // we want the vj valid from 11pm the first day to 08::00 the second, we now have A in the results
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130311T1100"_dt}, {"20130312T080001"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, {a});

    // we give a period after or before the validitity period, it's KO
    BOOST_CHECK_THROW(
        query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20110311T1100"_dt}, {"20110312T0800"_dt}, true),
        ptref_error);
    BOOST_CHECK_THROW(
        query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20160311T1100"_dt}, {"20160312T0800"_dt}, true),
        ptref_error);

    // we give a since > until, it's an error
    BOOST_CHECK_THROW(
        query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130316T1100"_dt}, {"20130311T0800"_dt}, true),
        ptref_error);
}

BOOST_AUTO_TEST_CASE(headsign_request) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250);
        b.vj("B")("stop3", 9000, 9050)("stop4", 9200, 9250);
        b.vj("C")("stop3", 9000, 9050)("stop5", 9200, 9250);
    });

    const auto res = make_query(nt::Type_e::VehicleJourney, R"(vehicle_journey.has_headsign("vj 1"))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(res, nt::make_indexes({1}));
}

BOOST_AUTO_TEST_CASE(headsign_sa_request) {
    ed::builder b("201303011T1739", [](ed::builder& b) {
        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250);
        b.vj("B")("stop3", 9000, 9050)("stop4", 9200, 9250);
        b.vj("C")("stop3", 9000, 9050)("stop5", 9200, 9250);
    });

    const auto res = make_query(nt::Type_e::StopArea, R"(vehicle_journey.has_headsign("vj 1"))", *(b.data));
    BOOST_REQUIRE_EQUAL(res.size(), 2);
    std::set<std::string> sas;
    for (const auto& idx : res) {
        sas.insert(b.data->pt_data->stop_areas.at(idx)->name);
    }
    BOOST_CHECK_EQUAL(sas, (std::set<std::string>{"stop3", "stop4"}));
}

BOOST_AUTO_TEST_CASE(code_request) {
    ed::builder b("20150101", [](ed::builder& b) {
        const auto* a = b.sa("A").sa;
        b.sa("B");
        b.data->pt_data->codes.add(a, "UIC", "8727100");
    });

    const auto res = make_query(nt::Type_e::StopArea, R"(stop_area.has_code(UIC, 8727100))", *(b.data));

    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::StopArea>(res, *b.data), std::set<std::string>({"A"}));
}

BOOST_AUTO_TEST_CASE(code_type_request) {
    ed::builder b("20190101", [](ed::builder& b) {
        b.sa("sa_1")("stop1", {{"code_type_1", {"0", "1"}}});
        b.sa("sa_1")("stop2", {{"code_type_2", {"0"}}});
        b.sa("sa_1")("stop3", {{"code_type_1", {"0", "1"}}});
        b.sa("sa_1")("stop4", {{"code_type_2", {"1"}}});

        b.vj("A")("stop1", 8000, 8050)("stop2", 8200, 8250);
        b.vj("B")("stop3", 9000, 9050)("stop4", 9200, 9250);
    });

    // Add codes on stop area
    const auto* sa = b.get<nt::StopArea>("sa_1");
    b.data->pt_data->codes.add(sa, "code_type_1", "0");

    auto res = make_query(nt::Type_e::StopPoint, R"(stop_point.has_code_type(code_type_1))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::StopPoint>(res, *b.data), std::set<std::string>({"stop1", "stop3"}));

    res = make_query(nt::Type_e::StopPoint, R"(stop_point.has_code_type(code_type_2))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::StopPoint>(res, *b.data), std::set<std::string>({"stop2", "stop4"}));

    res = make_query(nt::Type_e::StopArea, R"(stop_area.has_code_type(code_type_1))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::StopArea>(res, *b.data), std::set<std::string>({"sa_1"}));

    BOOST_CHECK_THROW(make_query(nt::Type_e::StopArea, R"(stop_area.has_code_type(code_type_doesnt_exist))", *(b.data)),
                      ptref_error);
}

BOOST_AUTO_TEST_CASE(contributor_and_dataset) {
    ed::builder b("201601011T1739");
    auto* vj_a = b.vj("A")("stop1", 8000, 8050).make();
    b.lines["A"]->code = "line_A";
    auto* vj_c = b.vj("C")("stop2", 8000, 8050).make();
    b.lines["C"]->code = "line C";

    // contributor "c1" contains dataset "d1" and "d2"
    // contributor "c2" contains dataset "d3"
    // Here contributor c1 contains datasets d1 and d2
    auto* contributor = b.add<nt::Contributor>("c1", "name-c1");

    auto* dataset = b.add<nt::Dataset>("d1", "name-d1");
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);

    // dataset "d1" is assigned to vehicle_journey "vehicle_journey:A:0"
    vj_a->dataset = dataset;

    dataset = b.add<nt::Dataset>("d2", "name-d2");
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);

    // dataset "d2" is assigned to vehicle_journey "vehicle_journey:C:1"
    vj_c->dataset = dataset;

    // Here contributor c2 contains datasets d3
    contributor = b.add<nt::Contributor>("c2", "name-c2");

    dataset = b.add<nt::Dataset>("d3", "name-d3");
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);

    b.make();

    auto indexes = make_query(nt::Type_e::Contributor, "contributor.uri=c1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Contributor>(indexes, *b.data), std::set<std::string>({"c1"}));

    indexes = make_query(nt::Type_e::Dataset, "dataset.uri=d1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"d1"}));

    indexes = make_query(nt::Type_e::Dataset, "contributor.uri=c1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"d1", "d2"}));

    indexes = make_query(nt::Type_e::VehicleJourney, "contributor.uri=c1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::VehicleJourney>(indexes, *b.data),
                            std::set<std::string>({"vehicle_journey:A:0", "vehicle_journey:C:1"}));

    indexes = make_query(nt::Type_e::VehicleJourney, "dataset.uri=d1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::VehicleJourney>(indexes, *b.data),
                            std::set<std::string>({"vehicle_journey:A:0"}));

    indexes = make_query(nt::Type_e::VehicleJourney, "dataset.uri=d2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::VehicleJourney>(indexes, *b.data),
                            std::set<std::string>({"vehicle_journey:C:1"}));

    indexes = make_query(nt::Type_e::Route, "dataset.uri=d1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(indexes, *b.data), std::set<std::string>({"A:0"}));

    indexes = make_query(nt::Type_e::Line, "dataset.uri=d2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(indexes, *b.data), std::set<std::string>({"C:1"}));

    indexes = make_query(nt::Type_e::Dataset, "contributor.uri=c2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"d3"}));

    // no vehicle_journey for dataset "d3"
    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "dataset.uri=d3", *(b.data)), ptref_error);

    // no vehicle_journey for contributor "c2"
    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "contributor.uri=c2", *(b.data)), ptref_error);

    // no route for contributor "c2"
    BOOST_CHECK_THROW(make_query(nt::Type_e::Route, "contributor.uri=c2", *(b.data)), ptref_error);

    // no line for contributor "c2"
    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, "contributor.uri=c2", *(b.data)), ptref_error);
}

BOOST_AUTO_TEST_CASE(get_potential_routes_test) {
    ed::builder b("201601011T1739", [](ed::builder& b) {
        b.sa("sa1")("stop1", {{"code_key", {"stop1 code"}}});
        b.sa("sa2")("stop2", {{"code_key", {"stop2 code"}}});
        b.sa("sa3")("stop3", {{"code_key", {"stop3 code", "stoparea3 code"}}});
        b.sa("sa4")("stop4", {{"code_key", {"stop4 code"}}});

        b.vj("A").route("r1")("stop1", "09:00"_t)("stop2", "10:00"_t)("stop3", "11:00"_t);
        b.vj("A").route("r1")("stop1", "09:10"_t)("stop2", "10:10"_t)("stop3", "11:10"_t);
        b.vj("A").route("r2")("stop1", "09:10"_t)("stop2", "10:10"_t)("stop3", "11:10"_t);
        b.vj("A").route("r3")("stop2", "09:10"_t)("stop3", "10:10"_t)("stop4", "11:10"_t);
        b.vj("B").route("r4")("stop1", "09:10"_t)("stop2", "10:10"_t)("stop3", "11:10"_t);
    });

    auto routes_names = [](const std::vector<const nt::Route*> routes) {
        std::set<std::string> res;
        for (const auto* r : routes) {
            res.insert(r->uri);
        }
        return res;
    };

    // only r1 and r2 go first on stop1 then stop3
    auto routes = navitia::ptref::get_matching_routes(b.data.get(), b.get<nt::Line>("A"), b.get<nt::StopPoint>("stop1"),
                                                      {"code_key", "stop3 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({"r1", "r2"}));

    // r1, r2 and r3 go first on stop2 then stop3 (even if s3 is not the final destination of r3)
    routes = navitia::ptref::get_matching_routes(b.data.get(), b.get<nt::Line>("A"), b.get<nt::StopPoint>("stop2"),
                                                 {"code_key", "stop3 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({"r1", "r2", "r3"}));
    // same as above, but we do the lookup with the stoparea's code, it should work too
    routes = navitia::ptref::get_matching_routes(b.data.get(), b.get<nt::Line>("A"), b.get<nt::StopPoint>("stop2"),
                                                 {"code_key", "stoparea3 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({"r1", "r2", "r3"}));

    // there is no route that first goes to stop2 then stop1
    routes = navitia::ptref::get_matching_routes(b.data.get(), b.get<nt::Line>("A"), b.get<nt::StopPoint>("stop2"),
                                                 {"code_key", "stop1 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({}));
}

namespace navitia {
namespace type {
static std::ostream& operator<<(std::ostream& os, const Type_e& type) {
    return os << navitia::type::static_data::get()->captionByType(type);
}
}  // namespace type
}  // namespace navitia

static std::vector<Type_e> get_path(Type_e source, Type_e dest) {
    std::vector<Type_e> res;
    auto succ = find_path(dest);
    for (auto cur = source; succ.at(cur) != cur; cur = succ.at(cur)) {
        res.push_back(succ.at(cur));
    }
    return res;
}

BOOST_AUTO_TEST_CASE(test_ptref_complete_pathes) {
    // Test that the paths in PTRef are corrects (not corrupted by shortcuts, etc.)
    // Those paths may change as shortcuts are added, but they should stay consistent
    // The goal of these tests is to check no unwanted side-effect is done by modifications on PTRef-graph
    // TODO: make it exhaustive, so keep it alphabetically stored by (source, dest)
    const auto CommercialMode = Type_e::CommercialMode;
    const auto Dataset = Type_e::Dataset;
    const auto JourneyPattern = Type_e::JourneyPattern;
    const auto JourneyPatternPoint = Type_e::JourneyPatternPoint;
    const auto Line = Type_e::Line;
    const auto Network = Type_e::Network;
    const auto POI = Type_e::POI;
    const auto POIType = Type_e::POIType;
    const auto Route = Type_e::Route;
    const auto StopArea = Type_e::StopArea;
    const auto StopPoint = Type_e::StopPoint;
    const auto ValidityPattern = Type_e::ValidityPattern;
    const auto VehicleJourney = Type_e::VehicleJourney;
    const auto Company = Type_e::Company;
    const auto PhysicalMode = Type_e::PhysicalMode;
    const auto MetaVehicleJourney = Type_e::MetaVehicleJourney;
    const auto Impact = Type_e::Impact;
    const auto LineGroup = Type_e::LineGroup;
    using p = std::vector<Type_e>;  // A path in the graph is a vector of PTRef types (only "source" is missing)

    BOOST_CHECK_EQUAL_RANGE(get_path(CommercialMode, StopArea), p({Line, Route, StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(CommercialMode, StopPoint), p({Line, Route, StopPoint}));

    BOOST_CHECK_EQUAL_RANGE(get_path(POI, POIType), p({POIType}));  // Only buddy
    BOOST_CHECK_EQUAL_RANGE(get_path(POI, VehicleJourney), p({}));  // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(POI, POI), p({}));  // If source==dest: doesn't crash (handled before find_path())
    BOOST_CHECK_EQUAL_RANGE(get_path(POIType, POI), p({POI}));  // Only buddy

    BOOST_CHECK_EQUAL_RANGE(get_path(Route, CommercialMode), p({Line, CommercialMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, StopArea), p({StopArea}));  // 2-way shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, CommercialMode), p({Route, Line, CommercialMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, CommercialMode), p({Route, Line, CommercialMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, POI), p({}));  // No path exists

    // extra checks
    BOOST_CHECK_THROW(get_path(Type_e::Address, Type_e::Admin), ptref_error);  // Non-PTRef types are rejected

    BOOST_CHECK_EQUAL_RANGE(
        get_path(Company, Dataset),
        p({Line, Route, Dataset}));  // shortcut only 1-way  // 2 paths with Line, Route, VehicleJourney, Dataset
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, Impact), p({Line, Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, JourneyPattern), p({Line, Route, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, JourneyPatternPoint),
                            p({Line, Route, JourneyPattern, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, Line), p({Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, MetaVehicleJourney),
                            p({Line, Route, VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, Network), p({Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, PhysicalMode), p({Line, Route, JourneyPattern, PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, Route), p({Line, Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, StopArea), p({Line, Route, StopArea}));    // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, StopPoint), p({Line, Route, StopPoint}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, ValidityPattern), p({Line, Route, VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Company, VehicleJourney), p({Line, Route, VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, Company), p({VehicleJourney, Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, Impact), p({VehicleJourney, Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, JourneyPattern), p({VehicleJourney, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, JourneyPatternPoint),
                            p({VehicleJourney, JourneyPattern, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, Line), p({VehicleJourney, Route, Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, MetaVehicleJourney), p({VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, Network), p({VehicleJourney, Route, Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, PhysicalMode), p({VehicleJourney, PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, Route), p({VehicleJourney, Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, StopArea),
                            p({VehicleJourney, JourneyPattern, JourneyPatternPoint, StopPoint, StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, StopPoint),
                            p({VehicleJourney, JourneyPattern, JourneyPatternPoint, StopPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, ValidityPattern), p({VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Dataset, VehicleJourney), p({VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, Company), p({Line, Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, JourneyPattern), p({Route, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, JourneyPatternPoint), p({StopPoint, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, Line), p({Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, MetaVehicleJourney), p({MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, Network), p({Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, PhysicalMode), p({Route, JourneyPattern, PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, Route), p({Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, StopArea), p({StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, StopPoint), p({StopPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, ValidityPattern),
                            p({MetaVehicleJourney, VehicleJourney,
                               ValidityPattern}));  // 2 paths with worst Route, VehicleJourney, ValidityPattern
    BOOST_CHECK_EQUAL_RANGE(get_path(Impact, VehicleJourney),
                            p({MetaVehicleJourney, VehicleJourney}));  // 2 paths with worst Route, VehicleJourney

    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, Company), p({Route, Line, Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, Dataset), p({VehicleJourney, Dataset}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, Impact),
                            p({VehicleJourney, Impact}));  // 2 paths with (worst?) Route, Impact
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, JourneyPatternPoint), p({JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, Line), p({Route, Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, MetaVehicleJourney), p({VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, Network), p({Route, Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, PhysicalMode), p({PhysicalMode}));  // 2-way shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, Route), p({Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, StopArea), p({JourneyPatternPoint, StopPoint, StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, StopPoint), p({JourneyPatternPoint, StopPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, ValidityPattern), p({VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPattern, VehicleJourney), p({VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, Company), p({JourneyPattern, Route, Line, Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, Dataset), p({JourneyPattern, VehicleJourney, Dataset}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, Impact), p({StopPoint, Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, JourneyPattern), p({JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, Line), p({JourneyPattern, Route, Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, MetaVehicleJourney),
                            p({JourneyPattern, VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, Network), p({JourneyPattern, Route, Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, PhysicalMode), p({JourneyPattern, PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, Route), p({JourneyPattern, Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, StopArea), p({StopPoint, StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, StopPoint), p({StopPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, ValidityPattern),
                            p({JourneyPattern, VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(JourneyPatternPoint, VehicleJourney), p({JourneyPattern, VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(Line, Company), p({Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, Dataset),
                            p({Route, Dataset}));  // indirect shortcut  // 2 paths with Route, VehicleJourney, Dataset
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, Impact), p({Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, JourneyPattern), p({Route, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, JourneyPatternPoint), p({Route, JourneyPattern, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, MetaVehicleJourney), p({Route, VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, Network), p({Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, PhysicalMode),
                            p({Route, JourneyPattern, PhysicalMode}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, Route), p({Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, StopArea), p({Route, StopArea}));    // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, StopPoint), p({Route, StopPoint}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, ValidityPattern), p({Route, VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, VehicleJourney), p({Route, VehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, LineGroup), p({LineGroup}));
    BOOST_CHECK_EQUAL_RANGE(get_path(LineGroup, Line), p({Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Line, CommercialMode), p({CommercialMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(CommercialMode, Line), p({Line}));

    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, Company), p({VehicleJourney, Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, Dataset), p({VehicleJourney, Dataset}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, Impact), p({Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, JourneyPattern), p({VehicleJourney, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, JourneyPatternPoint),
                            p({VehicleJourney, JourneyPattern, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, Line), p({VehicleJourney, Route, Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, Network), p({VehicleJourney, Route, Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, PhysicalMode), p({VehicleJourney, PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, Route), p({VehicleJourney, Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, StopArea),
                            p({VehicleJourney, JourneyPattern, JourneyPatternPoint, StopPoint, StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, StopPoint),
                            p({VehicleJourney, JourneyPattern, JourneyPatternPoint, StopPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, ValidityPattern), p({VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(MetaVehicleJourney, VehicleJourney), p({VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(Network, Company), p({Line, Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, Dataset), p({Dataset}));  // shortcut only 1-way
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, Impact), p({Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, JourneyPattern), p({Line, Route, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, JourneyPatternPoint),
                            p({Line, Route, JourneyPattern, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, Line), p({Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, MetaVehicleJourney),
                            p({Line, Route, VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, PhysicalMode),
                            p({Line, Route, JourneyPattern, PhysicalMode}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, Route), p({Line, Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, StopArea), p({Line, Route, StopArea}));    // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, StopPoint), p({Line, Route, StopPoint}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, ValidityPattern), p({Line, Route, VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Network, VehicleJourney), p({Line, Route, VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, Company), p({VehicleJourney, Company}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, Dataset), p({VehicleJourney, Dataset}));
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, Impact), p({VehicleJourney, Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, JourneyPattern), p({JourneyPattern}));            // 2-way shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, JourneyPatternPoint), p({JourneyPatternPoint}));  // only 1-way
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, Line), p({JourneyPattern, Route, Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, MetaVehicleJourney), p({VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, Network),
                            p({JourneyPattern, Route, Line, Network}));                  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, Route), p({JourneyPattern, Route}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, StopArea),
                            p({JourneyPatternPoint, StopPoint, StopArea}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, StopPoint),
                            p({JourneyPatternPoint, StopPoint}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, ValidityPattern), p({VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(PhysicalMode, VehicleJourney), p({VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(Route, Company), p({Line, Company}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, Dataset),
                            p({Dataset}));  // only 1-way  // 2 paths with VehicleJourney, Dataset
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, Impact), p({Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, JourneyPattern), p({JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, JourneyPatternPoint), p({JourneyPattern, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, Line), p({Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, MetaVehicleJourney), p({VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, Network), p({Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, PhysicalMode), p({JourneyPattern, PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, StopArea), p({StopArea}));    // 2-way shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, StopPoint), p({StopPoint}));  // 2-way shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, ValidityPattern), p({VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(Route, VehicleJourney), p({VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, Company), p({Route, Line, Company}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, Dataset), p({StopPoint, Dataset}));    // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, Impact), p({Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, JourneyPattern), p({StopPoint, JourneyPatternPoint, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, JourneyPatternPoint), p({StopPoint, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, Line), p({Route, Line}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, MetaVehicleJourney),
                            p({StopPoint, JourneyPatternPoint, JourneyPattern, VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, Network), p({Route, Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, PhysicalMode),
                            p({StopPoint, JourneyPatternPoint, JourneyPattern, PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, Route), p({Route}));  // 2-way shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, StopPoint), p({StopPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, ValidityPattern),
                            p({StopPoint, JourneyPatternPoint, JourneyPattern, VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopArea, VehicleJourney),
                            p({StopPoint, JourneyPatternPoint, JourneyPattern, VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, Company), p({Route, Line, Company}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, Dataset), p({Dataset}));               // only 1-way
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, Impact), p({Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, JourneyPattern), p({JourneyPatternPoint, JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, JourneyPatternPoint), p({JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, Line), p({Route, Line}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, MetaVehicleJourney),
                            p({JourneyPatternPoint, JourneyPattern, VehicleJourney, MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, Network), p({Route, Line, Network}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, PhysicalMode),
                            p({JourneyPatternPoint, JourneyPattern, PhysicalMode}));  // indirect shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, Route), p({Route}));                  // 2-way shortcut
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, StopArea), p({StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, ValidityPattern),
                            p({JourneyPatternPoint, JourneyPattern, VehicleJourney, ValidityPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(StopPoint, VehicleJourney),
                            p({JourneyPatternPoint, JourneyPattern, VehicleJourney}));

    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, Company), p({}));              // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, Dataset), p({}));              // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, Impact), p({}));               // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, JourneyPattern), p({}));       // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, JourneyPatternPoint), p({}));  // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, Line), p({}));                 // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, MetaVehicleJourney), p({}));   // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, Network), p({}));              // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, PhysicalMode), p({}));         // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, Route), p({}));                // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, StopArea), p({}));             // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, StopPoint), p({}));            // No path exists
    BOOST_CHECK_EQUAL_RANGE(get_path(ValidityPattern, VehicleJourney), p({}));       // Only the other way

    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, Company), p({Company}));  // only 1-way
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, Dataset), p({Dataset}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, Impact), p({Impact}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, JourneyPattern), p({JourneyPattern}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, JourneyPatternPoint), p({JourneyPattern, JourneyPatternPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, Line), p({Route, Line}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, MetaVehicleJourney), p({MetaVehicleJourney}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, Network), p({Route, Line, Network}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, PhysicalMode), p({PhysicalMode}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, Route), p({Route}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, StopArea),
                            p({JourneyPattern, JourneyPatternPoint, StopPoint, StopArea}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, StopPoint), p({JourneyPattern, JourneyPatternPoint, StopPoint}));
    BOOST_CHECK_EQUAL_RANGE(get_path(VehicleJourney, ValidityPattern), p({ValidityPattern}));  // only 1-way
}

BOOST_AUTO_TEST_CASE(find_path_coord) {
    ed::builder b("201601011T1739");

    BOOST_CHECK_THROW(make_query(nt::Type_e::JourneyPatternPoint, "coord.uri=\"42\"", *(b.data)), ptref_error);
    BOOST_CHECK_THROW(make_query(nt::Type_e::JourneyPatternPoint, "way.uri=\"42\"", *(b.data)), ptref_error);
    BOOST_CHECK_THROW(make_query(nt::Type_e::JourneyPatternPoint, "address.uri=\"42\"", *(b.data)), ptref_error);
    BOOST_CHECK_THROW(make_query(nt::Type_e::JourneyPatternPoint, "admin.uri=\"42\"", *(b.data)), ptref_error);
}

BOOST_AUTO_TEST_CASE(has_code_type_should_take_multiple_values) {
    ed::builder b("201601011T1739", [](ed::builder& b) {
        b.sa("sa1")("stop1", {{"code_1", {"value 1", "value 2"}}});
        b.sa("sa2")("stop2", {{"code_2", {"value 3", "value 4"}}});
        b.sa("sa3")("stop3", {{"code_3", {"value 5", "value 6"}}});
    });

    auto indexes = make_query(nt::Type_e::StopArea, "stop_point.has_code_type(code_1, other_code, code_3)", *(b.data));
    auto stop_areas_uris = get_uris<nt::StopArea>(indexes, *b.data);
    BOOST_CHECK_EQUAL_RANGE(stop_areas_uris, std::set<std::string>({"sa1", "sa3"}));
}

BOOST_AUTO_TEST_CASE(direction_type_request) {
    ed::builder b("20190101", [](ed::builder& b) {
        b.vj("L1").route("route1", "forward").name("vj:0")("stop1", "8:05"_t, "8:06"_t)("stop2", "8:10"_t, "8:11"_t);
        b.vj("L2").route("route2", "forward").name("vj:0")("stop3", "8:05"_t, "8:06"_t)("stop4", "8:10"_t, "8:11"_t);
        b.vj("L3").route("route3", "clockwise").name("vj:0")("stop1", "8:05"_t, "8:06"_t)("stop2", "8:10"_t, "8:11"_t);
        b.vj("L4").route("route4", "inbound").name("vj:0")("stop1", "8:05"_t, "8:06"_t)("stop2", "8:10"_t, "8:11"_t);
        b.vj("L5").route("route5", "backward").name("vj:0")("stop1", "8:05"_t, "8:06"_t)("stop2", "8:10"_t, "8:11"_t);
        b.vj("L6")
            .route("route6", "anticlockwise")
            .name("vj:0")("stop1", "8:05"_t, "8:06"_t)("stop2", "8:10"_t, "8:11"_t);
        b.vj("L7").route("route7", "outbound").name("vj:0")("stop1", "8:05"_t, "8:06"_t)("stop2", "8:10"_t, "8:11"_t);
    });

    auto res = make_query(nt::Type_e::Route, R"(route.has_direction_type(forward))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(res, *b.data), std::set<std::string>({"route1", "route2"}));

    res = make_query(nt::Type_e::Route, R"(route.has_direction_type(backward))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(res, *b.data), std::set<std::string>({"route5"}));

    res = make_query(nt::Type_e::Route, R"(route.has_direction_type(forward, clockwise, inbound))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(res, *b.data),
                            std::set<std::string>({"route1", "route2", "route3", "route4"}));

    res = make_query(nt::Type_e::Line, R"(route.has_direction_type(clockwise, backward, outbound))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(res, *b.data), std::set<std::string>({"L3", "L5", "L7"}));

    BOOST_CHECK_THROW(
        make_query(nt::Type_e::Route, R"(route.has_direction_type(direction_type_doesnt_exist))", *(b.data)),
        ptref_error);

    res = make_query(nt::Type_e::Line, R"(route.has_direction_type(forward))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(res, *b.data), std::set<std::string>({"L1", "L2"}));

    res = make_query(nt::Type_e::StopPoint, R"(route.has_direction_type(forward))", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::StopPoint>(res, *b.data),
                            std::set<std::string>({"stop1", "stop2", "stop3", "stop4"}));

    res = make_query(nt::Type_e::JourneyPatternPoint, R"(route.has_direction_type(forward))", *(b.data));
    BOOST_CHECK_EQUAL(res.size(), 4);

    // Only route has direction type
    BOOST_CHECK_THROW(make_query(nt::Type_e::Route, R"(line.has_direction_type(forward))", *(b.data)), ptref_error);
    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, R"(stop_point.has_direction_type(forward))", *(b.data)),
                      ptref_error);
}
