/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
#include "ptreferential/ptreferential_api.h"
#include "ptreferential/reflexion.h"
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

struct Moo {
    int bli;
};
DECL_HAS_MEMBER(bli)
DECL_HAS_MEMBER(foo)

BOOST_AUTO_TEST_CASE(reflexion){
    BOOST_CHECK(Reflect_bli<Moo>::value);
    BOOST_CHECK(!Reflect_foo<Moo>::value);

    Moo m;
    m.bli = 42;
    BOOST_CHECK_EQUAL(get_bli(m), 42);
    BOOST_CHECK_THROW(get_foo(m), unknown_member);
}


BOOST_AUTO_TEST_CASE(sans_filtre) {

    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.finish();
    b.data->pt_data->build_uri();

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
    BOOST_CHECK_EQUAL(indexes.size(), 6);

    indexes = make_query(nt::Type_e::CommercialMode, "", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 6);

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

    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    // Physical_mode = Car
    b.vj("A","11110000","",true,"", "","physical_mode:Car")("stop1", 8000,8050)("stop2", 8200,8250);
    // Physical_mode = Metro
    b.vj("A","00001111","",true,"", "","physical_mode:0x1")("stop1", 8000,8050)("stop2", 8200,8250)("stop3", 8500,8500);
    // Physical_mode = Tram
    auto* vj_c = b.vj("C")("stop3", 9000,9050)("stop4", 9200,9250).make();
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);

    navitia::type::Contributor* contributor = new navitia::type::Contributor();
    contributor->idx = b.data->pt_data->contributors.size();
    contributor->uri = "c1";
    contributor->name = "name-c1";
    b.data->pt_data->contributors.push_back(contributor);

    navitia::type::Dataset* dataset = new navitia::type::Dataset();
    dataset->idx = b.data->pt_data->datasets.size();
    dataset->uri = "f1";
    dataset->name = "name-f1";
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);
    b.data->pt_data->datasets.push_back(dataset);
    vj_c->dataset = dataset;

    b.data->build_relations();
    b.finish();
    b.data->pt_data->build_uri();

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
    BOOST_CHECK_EQUAL((*objects.begin())->physical_mode_list.at(0)->name, "Tram");

    indexes = make_query(nt::Type_e::Dataset, "stop_point.uri=stop1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset"}));

    indexes = make_query(nt::Type_e::Dataset, "stop_point.uri=stop2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset"}));

    indexes = make_query(nt::Type_e::Dataset, "stop_point.uri=stop3", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset", "f1"}));

    indexes = make_query(nt::Type_e::Contributor, "dataset.uri=default:dataset", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"default:dataset"}));

    indexes = make_query(nt::Type_e::Contributor, "dataset.uri=f1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Contributor>(indexes, *b.data), std::set<std::string>({"c1"}));
}

BOOST_AUTO_TEST_CASE(get_indexes_test){
    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.finish();
    b.data->pt_data->index();
    b.data->pt_data->build_uri();

    // On cherche à retrouver la ligne 1, en passant le stoparea en filtre
    auto indexes = make_query(Type_e::Line, "stop_area.uri = stop1",*b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, nt::make_indexes({0}));

    // On cherche les stopareas de la ligneA
    indexes = make_query(Type_e::StopArea, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, nt::make_indexes({0, 1}));
}

BOOST_AUTO_TEST_CASE(get_impact_indexes_of_line){
    ed::builder b("201303011T1739");
    b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->pt_data->build_uri();

    using btp = boost::posix_time::time_period;
    const auto& disrup_1 = b.impact(nt::RTLevel::RealTime, "Disruption 1")
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::Line, "A")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();

    navitia::apply_disruption(disrup_1, *b.data->pt_data, *b.data->meta);
    auto indexes = make_query(Type_e::Impact, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>{0});

    navitia::delete_disruption("Disruption 1", *b.data->pt_data, *b.data->meta);
    BOOST_CHECK_THROW(make_query(Type_e::Impact, "line.uri = A", *b.data), ptref_error);

    const auto& disrup_2 = b.impact(nt::RTLevel::RealTime, "Disruption 2")
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::Line, "A")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();

    navitia::apply_disruption(disrup_2, *b.data->pt_data, *b.data->meta);
    indexes = make_query(Type_e::Impact, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>{0});

    const auto& disrup_3 = b.impact(nt::RTLevel::RealTime, "Disruption 3")
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::Line, "A")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();

    navitia::apply_disruption(disrup_3, *b.data->pt_data, *b.data->meta);
    indexes = make_query(Type_e::Impact, "line.uri = A", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, nt::make_indexes({0, 1}));
}

BOOST_AUTO_TEST_CASE(get_impact_indexes_of_stop_point){
    ed::builder b("201303011T1739");
    b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->pt_data->build_uri();

    using btp = boost::posix_time::time_period;
    const auto& disrup_1 = b.impact(nt::RTLevel::RealTime, "Disruption 1")
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::StopPoint, "stop1")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();

    navitia::apply_disruption(disrup_1, *b.data->pt_data, *b.data->meta);
    auto indexes = make_query(Type_e::Impact, "stop_point.uri = stop1", *b.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>{0});
}

BOOST_AUTO_TEST_CASE(ptref_on_vj_impacted){
    ed::builder b("201303011T1739");
    b.vj("A", "000001", "", true, "vj:A-1")("stop1", "08:00"_t)("stop2", "09:00"_t);
    b.vj("A", "000001", "", true, "vj:A-2")("stop1", "08:20"_t)("stop2", "09:20"_t);
    b.vj("A", "000001", "", true, "vj:A-3")("stop1", "08:30"_t)("stop2", "09:30"_t);
    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->pt_data->build_uri();

    // no disruptions
    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "vehicle_journey.has_disruption()", *b.data),
                      ptref_error);
    // disruption with Effect OTHER_EFFECT on vj:A-1
    using btp = boost::posix_time::time_period;
    const auto& disrup_1 = b.impact(nt::RTLevel::RealTime, "Disruption 1")
                     .severity(nt::disruption::Effect::OTHER_EFFECT)
                     .on(nt::Type_e::MetaVehicleJourney, "vj:A-1")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();
    navitia::apply_disruption(disrup_1, *b.data->pt_data, *b.data->meta);

    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "vehicle_journey.has_disruption()", *b.data),
                      ptref_error);
    // disruption with Effect NO_SERVICE on vj:A-2
    const auto& disrup_2 = b.impact(nt::RTLevel::RealTime, "Disruption 2")
                     .severity(nt::disruption::Effect::NO_SERVICE)
                     .on(nt::Type_e::MetaVehicleJourney, "vj:A-2")
                     .application_periods(btp("20150928T000000"_dt, "20150928T240000"_dt))
                     .get_disruption();
    navitia::apply_disruption(disrup_2, *b.data->pt_data, *b.data->meta);
    navitia::type::Indexes vj_idx = make_query(nt::Type_e::VehicleJourney, "vehicle_journey.has_disruption()",
                                               *b.data);

    BOOST_CHECK_EQUAL(vj_idx.size(), 1);
    BOOST_CHECK_EQUAL(get_uris<nt::VehicleJourney>(vj_idx, *b.data), std::set<std::string>({"vj:A-2"}));
}

BOOST_AUTO_TEST_CASE(make_query_filtre_direct) {

    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.finish();
    b.data->pt_data->build_uri();

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

    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000, 8050);
    b.lines["A"]->code = "line_A";
    b.vj("C")("stop2", 8000, 8050);
    b.lines["C"]->code = "line C";
    b.data->build_relations();
    b.finish();
    b.data->pt_data->build_uri();

    //find line by code
    auto indexes = make_query(nt::Type_e::Line, "line.code=line_A", *(b.data));
    auto objects = get_objects<nt::Line>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(objects.size(), 1);
    BOOST_CHECK_EQUAL((*objects.begin())->code, "line_A");

    indexes = make_query(nt::Type_e::Line, "line.code=\"line C\"", *(b.data));
    objects = get_objects<nt::Line>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(objects.size(), 1);
    BOOST_CHECK_EQUAL((*objects.begin())->code, "line C");

    //no line B
    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, "line.code=\"line B\"", *(b.data)),
                      ptref_error);

    //find route by line code
    indexes = make_query(nt::Type_e::Route, "line.code=line_A", *(b.data));
    auto routes = get_objects<nt::Route>(indexes, *b.data);
    BOOST_REQUIRE_EQUAL(routes.size(), 1);
    BOOST_CHECK_EQUAL((*routes.begin())->line->code, "line_A");

    //route has no code attribute, no filter can be used
    indexes = make_query(nt::Type_e::Line, "route.code=test", *(b.data));
    BOOST_CHECK_EQUAL(indexes.size(), 2);
}

BOOST_AUTO_TEST_CASE(forbidden_uri) {

    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.connection("stop2", "stop3", 10*60);
    b.connection("stop3", "stop2", 10*60);
    b.finish();
    b.data->pt_data->build_uri();

    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, "stop_point.uri=stop1", {"A"}, *(b.data)), ptref_error);
}
BOOST_AUTO_TEST_CASE(after_filter) {
    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250)("stop3", 8500)("stop4", 9000);
    b.vj("B")("stop5", 9000,9050)("stop2", 9200,9250);
    b.vj("C")("stop6", 9000,9050)("stop2", 9200,9250)("stop7", 10000);
    b.vj("D")("stop5", 9000,9050)("stop2", 9200,9250)("stop3", 10000);
    b.finish();
    b.data->pt_data->build_uri();

    auto indexes = make_query(nt::Type_e::StopArea, "AFTER(stop_area.uri=stop2)", *(b.data));
    BOOST_REQUIRE_EQUAL(indexes.size(), 3);
    auto expected_uris = {"stop3", "stop4", "stop7"};
    for(auto stop_area_idx : indexes) {
        auto stop_area = b.data->pt_data->stop_areas[stop_area_idx];
        BOOST_REQUIRE(std::find(expected_uris.begin(), expected_uris.end(),
                            stop_area->uri) != expected_uris.end());
    }
}

// TODO: reactivate when this code is moved
#if 0
BOOST_AUTO_TEST_CASE(find_path_test){
    auto res = find_path(nt::Type_e::Route);
    // Cas où on veut partir et arriver au même point
    BOOST_CHECK(res[nt::Type_e::Route] == nt::Type_e::Route);
    // On regarde qu'il y a un semblant d'itinéraire pour chaque type : il faut un prédécesseur
    for(auto node_pred : res){
        // Route c'est lui même puisque c'est la source, et validity pattern est puni, donc il est seul dans son coin, de même pour POI et POIType
        if(node_pred.first != Type_e::Route && node_pred.first != Type_e::ValidityPattern &&
                node_pred.first != Type_e::POI && node_pred.first != Type_e::POIType &&
                node_pred.first != Type_e::Contributor && node_pred.first != Type_e::Impact &&
                node_pred.first != Type_e::Dataset)
            BOOST_CHECK(node_pred.first != node_pred.second);
    }

    // On vérifie qu'il n'y ait pas de nœud qui ne soit pas atteignable depuis un autre nœud
    // En connexité non forte, il y a un seul ensemble : validity pattern est relié (en un seul sens) au gros paté
    // Avec les composantes fortement connexes : there must be only two : validity pattern est isolé car on ne peut y aller
    Jointures j;
    std::vector<Jointures::vertex_t> component(boost::num_vertices(j.g));
    int num = boost::connected_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 3);
    num =  boost::strong_components(j.g, &component[0]);
    BOOST_CHECK_EQUAL(num, 4);

    // Type qui n'existe pas dans le graph : il n'y a pas de chemin
    BOOST_CHECK_THROW(find_path(nt::Type_e::Unknown), ptref_error);
}
#endif

//helper to get default values
static nt::Indexes query(nt::Type_e requested_type, std::string request,
                                    const nt::Data& data,
                                    const boost::optional<boost::posix_time::ptime>& since = boost::none,
                                    const boost::optional<boost::posix_time::ptime>& until = boost::none,
                                    bool fail = false) {
    try {
       return navitia::ptref::make_query(requested_type, request, {}, nt::OdtLevel_e::all, since, until, data);
    } catch (const navitia::ptref::ptref_error& e) {
        if (fail) {
            throw e;
        }
        BOOST_CHECK_MESSAGE(false, " error on ptref request " + std::string(e.what()));
        return nt::Indexes{};
    }
}

/*
 * Test the meta-vj filtering on vj
 */
BOOST_AUTO_TEST_CASE(mvj_filtering) {
    ed::builder builder("20130311");
    builder.generate_dummy_basis();
    // Date  11    12    13    14
    // A      -   08:00 08:00   -
    // B    10:00 10:00   -   10:00
    // C      -     -     -   10:00
    builder.vj("A", "0110")("stop1", "08:00"_t);
    builder.vj("B", "1011")("stop3", "10:00"_t);
    builder.vj("C", "1000")("stop3", "10:00"_t);
    builder.finish();
    builder.data->pt_data->build_uri();
    nt::idx_t a = 0;
    nt::idx_t b = 1;
    nt::idx_t c = 2;

    //not limited, we get 3 mvj
    auto indexes = query(nt::Type_e::MetaVehicleJourney, "", *(builder.data));
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({0, 1, 2}));

    // looking for MetaVJ 0
    indexes = make_query(nt::Type_e::MetaVehicleJourney,
                         R"(trip.uri="vehicle_journey 0")",
                         *(builder.data));
    BOOST_REQUIRE_EQUAL(indexes.size(), 1);
    const auto mvj_idx = navitia::Idx<nt::MetaVehicleJourney>(*indexes.begin());
    BOOST_CHECK_EQUAL(builder.data->pt_data->meta_vjs[mvj_idx]->uri, "vehicle_journey 0");

    // looking for MetaVJ A through VJ A
    indexes = make_query(Type_e::MetaVehicleJourney, "vehicle_journey.uri = vj:A:0", *builder.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, {0})

    //not limited, we get 3 vj
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data));
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c}));

    // looking for VJ B through MetaVJ B
    indexes = make_query(Type_e::VehicleJourney, R"(trip.uri="vehicle_journey 1")", *builder.data);
    BOOST_CHECK_EQUAL_RANGE(indexes, {b})
}

/*
 * Test the filtering on the period
 */
BOOST_AUTO_TEST_CASE(vj_filtering) {
    ed::builder builder("20130311");
    builder.generate_dummy_basis();
    // Date  11    12    13    14
    // A      -   08:00 08:00   -
    // B    10:00 10:00   -   10:00
    // C      -     -     -   10:00
    builder.vj("A", "0110")("stop1", "08:00"_t)("stop2", "09:00"_t);
    builder.vj("B", "1011")("stop3", "10:00"_t)("stop2", "11:00"_t);
    builder.vj("C", "1000")("stop3", "10:00"_t)("stop2", "11:00"_t);
    builder.finish();
    nt::idx_t a = 0;
    nt::idx_t b = 1;
    nt::idx_t c = 2;

    //not limited, we get 3 vj
    auto indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data));
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c}));

    // the second day A and B are activ
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data),
                    {"20130312T0700"_dt}, {"20130313T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b}));

    // but the third only A works
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data),
                    {"20130313T0700"_dt}, {"20130314T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, {a});

    // on day 3 and 4, A, B and C are valid only one day, so it's ok
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data),
                    {"20130313T0700"_dt}, {"20130315T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c}));

    //with just a since, we check til the end of the period
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130313T0000"_dt}, {});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b, c})); // all are valid from the 3rd day

    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {"20130314T0000"_dt}, {});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({b, c})); // only B and C are valid from the 4th day

    // with just an until, we check from the begining of the validity period
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data), {}, {"20130313T0000"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, std::vector<size_t>({a, b}));

    //tests with the time
    // we want the vj valid from 11pm only the first day, B is valid at 10, so not it the period
    BOOST_CHECK_THROW(query(nt::Type_e::VehicleJourney, "", *(builder.data),
                            {"20130311T1100"_dt}, {"20130312T0000"_dt}, true),
                      ptref_error);

    // we want the vj valid from 11pm the first day to 08:00 (excluded) the second, we still have no vj
    BOOST_CHECK_THROW(query(nt::Type_e::VehicleJourney, "", *(builder.data),
                            {"20130311T1100"_dt}, {"20130312T0759"_dt}, true),
                      ptref_error);

    // we want the vj valid from 11pm the first day to 08::00 the second, we now have A in the results
    indexes = query(nt::Type_e::VehicleJourney, "", *(builder.data),
                    {"20130311T1100"_dt}, {"20130312T080001"_dt});
    BOOST_CHECK_EQUAL_RANGE(indexes, {a});

    // we give a period after or before the validitity period, it's KO
    BOOST_CHECK_THROW(query(nt::Type_e::VehicleJourney, "", *(builder.data),
                            {"20110311T1100"_dt}, {"20110312T0800"_dt}, true),
                      ptref_error);
    BOOST_CHECK_THROW(query(nt::Type_e::VehicleJourney, "", *(builder.data),
                            {"20160311T1100"_dt}, {"20160312T0800"_dt}, true),
                      ptref_error);

    // we give a since > until, it's an error
    BOOST_CHECK_THROW(query(nt::Type_e::VehicleJourney, "", *(builder.data),
                            {"20130316T1100"_dt}, {"20130311T0800"_dt}, true),
                      ptref_error);
}

BOOST_AUTO_TEST_CASE(headsign_request) {
    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.vj("C")("stop3", 9000,9050)("stop5", 9200,9250);
    b.finish();
    b.data->pt_data->build_uri();

    const auto res = make_query(nt::Type_e::VehicleJourney,
                                R"(vehicle_journey.has_headsign("vehicle_journey 1"))",
                                *(b.data));
    BOOST_CHECK_EQUAL_RANGE(res, nt::make_indexes({1}));
}

BOOST_AUTO_TEST_CASE(headsign_sa_request) {
    ed::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);
    b.vj("C")("stop3", 9000,9050)("stop5", 9200,9250);
    b.finish();
    b.data->pt_data->build_uri();

    const auto res = make_query(nt::Type_e::StopArea,
                                R"(vehicle_journey.has_headsign("vehicle_journey 1"))",
                                *(b.data));
    BOOST_REQUIRE_EQUAL(res.size(), 2);
    std::set<std::string> sas;
    for (const auto& idx: res) {
        sas.insert(b.data->pt_data->stop_areas.at(idx)->name);
    }
    BOOST_CHECK_EQUAL(sas, (std::set<std::string>{"stop3", "stop4"}));
}

BOOST_AUTO_TEST_CASE(code_request) {
    ed::builder b("20150101");
    b.generate_dummy_basis();
    const auto* a = b.sa("A").sa;
    b.sa("B");
    b.data->pt_data->codes.add(a, "UIC", "8727100");
    b.finish();
    b.data->pt_data->build_uri();

    const auto res = make_query(nt::Type_e::StopArea,
                                R"(stop_area.has_code(UIC, 8727100))",
                                *(b.data));

    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::StopArea>(res, *b.data), std::set<std::string>({"A"}));
}

BOOST_AUTO_TEST_CASE(contributor_and_dataset) {

    ed::builder b("201601011T1739");
    b.generate_dummy_basis();
    auto* vj_a = b.vj("A")("stop1", 8000, 8050).make();
    b.lines["A"]->code = "line_A";
    auto* vj_c = b.vj("C")("stop2", 8000, 8050).make();
    b.lines["C"]->code = "line C";

    //contributor "c1" contains dataset "d1" and "d2"
    //contributor "c2" contains dataset "d3"
    //Here contributor c1 contains datasets d1 and d2
    auto* contributor = b.add<nt::Contributor>("c1", "name-c1");

    auto* dataset = b.add<nt::Dataset>("d1", "name-d1");
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);

    //dataset "d1" is assigned to vehicle_journey "vj:A:0"
    vj_a->dataset = dataset;

    dataset = b.add<nt::Dataset>("d2", "name-d2");
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);

    //dataset "d2" is assigned to vehicle_journey "vj:C:1"
    vj_c->dataset = dataset;

    //Here contributor c2 contains datasets d3
    contributor = b.add<nt::Contributor>("c2", "name-c2");

    dataset = b.add<nt::Dataset>("d3", "name-d3");
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);

    b.data->build_relations();
    b.finish();
    b.data->pt_data->build_uri();

    auto indexes = make_query(nt::Type_e::Contributor, "contributor.uri=c1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Contributor>(indexes, *b.data), std::set<std::string>({"c1"}));

    indexes = make_query(nt::Type_e::Dataset, "dataset.uri=d1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"d1"}));

    indexes = make_query(nt::Type_e::Dataset, "contributor.uri=c1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"d1", "d2"}));

    indexes = make_query(nt::Type_e::VehicleJourney, "contributor.uri=c1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::VehicleJourney>(indexes, *b.data), std::set<std::string>({"vj:A:0", "vj:C:1"}));

    indexes = make_query(nt::Type_e::VehicleJourney, "dataset.uri=d1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::VehicleJourney>(indexes, *b.data), std::set<std::string>({"vj:A:0"}));

    indexes = make_query(nt::Type_e::VehicleJourney, "dataset.uri=d2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::VehicleJourney>(indexes, *b.data), std::set<std::string>({"vj:C:1"}));

    indexes = make_query(nt::Type_e::Route, "dataset.uri=d1", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(indexes, *b.data), std::set<std::string>({"A:0"}));

    indexes = make_query(nt::Type_e::Line, "dataset.uri=d2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(indexes, *b.data), std::set<std::string>({"C:1"}));

    indexes = make_query(nt::Type_e::Dataset, "contributor.uri=c2", *(b.data));
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Dataset>(indexes, *b.data), std::set<std::string>({"d3"}));

    //no vehicle_journey for dataset "d3"
    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "dataset.uri=d3", *(b.data)),
                      ptref_error);

    //no vehicle_journey for contributor "c2"
    BOOST_CHECK_THROW(make_query(nt::Type_e::VehicleJourney, "contributor.uri=c2", *(b.data)),
                      ptref_error);

    //no route for contributor "c2"
    BOOST_CHECK_THROW(make_query(nt::Type_e::Route, "contributor.uri=c2", *(b.data)),
                      ptref_error);

    //no line for contributor "c2"
    BOOST_CHECK_THROW(make_query(nt::Type_e::Line, "contributor.uri=c2", *(b.data)),
                      ptref_error);
}


BOOST_AUTO_TEST_CASE(get_potential_routes_test) {
    ed::builder b("201601011T1739");
    b.generate_dummy_basis();
    b.vj("A").route("r1")("stop1", "09:00"_t)("stop2", "10:00"_t)("stop3", "11:00"_t);
    b.vj("A").route("r1")("stop1", "09:10"_t)("stop2", "10:10"_t)("stop3", "11:10"_t);
    b.vj("A").route("r2")("stop1", "09:10"_t)("stop2", "10:10"_t)("stop3", "11:10"_t);
    b.vj("A").route("r3")("stop2", "09:10"_t)("stop3", "10:10"_t)("stop4", "11:10"_t);
    b.vj("B").route("r3")("stop1", "09:10"_t)("stop2", "10:10"_t)("stop3", "11:10"_t);

    b.finish();
    b.data->build_uri();

    b.data->pt_data->codes.add(b.get<nt::StopPoint>("stop1"), "code_key", "stop1 code");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("stop2"), "code_key", "stop2 code");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("stop3"), "code_key", "stop3 code");
    b.data->pt_data->codes.add(b.get<nt::StopArea>("stop3"), "code_key", "stoparea3 code");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("stop4"), "code_key", "stop4 code");

    auto routes_names = [](const std::vector<const nt::Route*> routes) {
        std::set<std::string> res;
        for (const auto* r: routes) {
            res.insert(r->uri);
        }
        return res;
    };

    // only r1 and r2 go first on stop1 then stop3
    auto routes = navitia::ptref::get_matching_routes(b.data.get(),
                                                      b.get<nt::Line>("A"),
                                                      b.get<nt::StopPoint>("stop1"),
                                                      {"code_key", "stop3 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({"r1", "r2"}));

    // r1, r2 and r3 go first on stop2 then stop3 (even if s3 is not the final destination of r3)
    routes = navitia::ptref::get_matching_routes(b.data.get(),
                                                 b.get<nt::Line>("A"),
                                                 b.get<nt::StopPoint>("stop2"),
                                                {"code_key", "stop3 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({"r1", "r2", "r3"}));
    // same as above, but we do the lookup with the stoparea's code, it should work too
    routes = navitia::ptref::get_matching_routes(b.data.get(),
                                                 b.get<nt::Line>("A"),
                                                 b.get<nt::StopPoint>("stop2"),
                                                {"code_key", "stoparea3 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({"r1", "r2", "r3"}));

    // there is no route that first goes to stop2 then stop1
    routes = navitia::ptref::get_matching_routes(b.data.get(),
                                                 b.get<nt::Line>("A"),
                                                 b.get<nt::StopPoint>("stop2"),
                                                {"code_key", "stop1 code"});
    BOOST_CHECK_EQUAL_RANGE(routes_names(routes), std::set<std::string>({}));
}
