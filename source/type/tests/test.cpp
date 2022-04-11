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
#define BOOST_TEST_MODULE test_proximity_list

#include <algorithm>
#include "type/message.h"
#include "type/data.h"
#include "type/datetime.h"
#include "tests/utils_test.h"
#include "type/meta_data.h"
#include "ed/build_helper.h"

#include <boost/geometry.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/test/unit_test.hpp>

namespace pt = boost::posix_time;
namespace bg = boost::gregorian;
using namespace navitia;
using navitia::type::disruption::Disruption;
using navitia::type::disruption::Impact;

#define BOOST_CHECK_NOT(value) BOOST_CHECK(!value)

using namespace navitia::type;
BOOST_AUTO_TEST_CASE(boost_geometry) {
    GeographicalCoord c(2, 48);
    GeographicalCoord b(3, 48);
    std::vector<GeographicalCoord> line{c, b};
    GeographicalCoord centroid;
    boost::geometry::centroid(line, centroid);
    BOOST_CHECK_EQUAL(centroid, GeographicalCoord(2.5, 48));
    // spherical_point s(2,48);
}

BOOST_AUTO_TEST_CASE(uri_coord) {
    std::string uri("coord:2.2:4.42");
    EntryPoint ep(Type_e::Coord, uri);

    BOOST_CHECK(ep.type == Type_e::Coord);
    BOOST_CHECK_CLOSE(ep.coordinates.lon(), 2.2, 1e-6);
    BOOST_CHECK_CLOSE(ep.coordinates.lat(), 4.42, 1e-6);

    EntryPoint ep2(Type_e::Coord, "coord:2.1");
    BOOST_CHECK_CLOSE(ep2.coordinates.lon(), 0, 1e-6);
    BOOST_CHECK_CLOSE(ep2.coordinates.lat(), 0, 1e-6);

    EntryPoint ep3(Type_e::Coord, "coord:2.1:bli");
    BOOST_CHECK_CLOSE(ep3.coordinates.lon(), 0, 1e-6);
    BOOST_CHECK_CLOSE(ep3.coordinates.lat(), 0, 1e-6);
}

BOOST_AUTO_TEST_CASE(projection) {
    using namespace navitia::type;
    using boost::tie;

    GeographicalCoord a(0, 0);   // Début de segment
    GeographicalCoord b(10, 0);  // Fin de segment
    GeographicalCoord p(5, 5);   // Point à projeter
    GeographicalCoord pp;        // Point projeté
    float d;                     // Distance du projeté
    tie(pp, d) = p.project(a, b);

    BOOST_CHECK_EQUAL(pp.lon(), 5);
    BOOST_CHECK_EQUAL(pp.lat(), 0);
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1e-3);

    // Si on est à l'extérieur à gauche
    p.set_lon(-10);
    p.set_lat(0);
    tie(pp, d) = p.project(a, b);
    BOOST_CHECK_EQUAL(pp.lon(), a.lon());
    BOOST_CHECK_EQUAL(pp.lat(), a.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1);

    // Si on est à l'extérieur à droite
    p.set_lon(20);
    p.set_lat(0);
    tie(pp, d) = p.project(a, b);
    BOOST_CHECK_EQUAL(pp.lon(), b.lon());
    BOOST_CHECK_EQUAL(pp.lat(), b.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1);

    // On refait la même en permutant A et B

    // Si on est à l'extérieur à gauche
    p.set_lon(-10);
    p.set_lat(0);
    tie(pp, d) = p.project(b, a);
    BOOST_CHECK_EQUAL(pp.lon(), a.lon());
    BOOST_CHECK_EQUAL(pp.lat(), a.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1);

    // Si on est à l'extérieur à droite
    p.set_lon(20);
    p.set_lat(0);
    tie(pp, d) = p.project(b, a);
    BOOST_CHECK_EQUAL(pp.lon(), b.lon());
    BOOST_CHECK_EQUAL(pp.lat(), b.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1);

    // On essaye avec des trucs plus pentus ;)
    a.set_lon(-3);
    a.set_lat(-3);
    b.set_lon(5);
    b.set_lat(5);
    p.set_lon(-2);
    p.set_lat(2);
    tie(pp, d) = p.project(a, b);
    BOOST_CHECK_SMALL(pp.lon(), 1e-3);
    BOOST_CHECK_SMALL(pp.lat(), 1e-3);
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1e-3);
}

template <typename T>
std::shared_ptr<T> create_obj_with_name(std::string const& name) {
    auto obj = std::make_shared<T>();
    obj->name = name;
    return obj;
}

BOOST_AUTO_TEST_CASE(human_sort_on_stop_areas_should_not_throw) {
    std::vector<std::shared_ptr<StopArea>> stop_areas{
        create_obj_with_name<StopArea>("AVENUE DU NID (Sarcelles)"),
        create_obj_with_name<StopArea>("Alésia (Paris)"),
        create_obj_with_name<StopArea>("Asnières — Gennevilliers Les Courtilles (Asnières-sur-Seine)"),
        create_obj_with_name<StopArea>("Aéroport de Marseille (Marseille)"),
        create_obj_with_name<StopArea>("ZURICH (Rueil-Malmaison)"),
        create_obj_with_name<StopArea>("Église de Pantin (Pantin)"),
    };

    boost::sort(stop_areas, Less());

    std::vector<std::string> stop_areas_name;
    boost::transform(stop_areas, std::back_inserter(stop_areas_name), [](const auto& sa) { return sa->name; });

    std::vector<std::string> stop_areas_sorted{
        "Aéroport de Marseille (Marseille)",
        "Alésia (Paris)",
        "Asnières — Gennevilliers Les Courtilles (Asnières-sur-Seine)",
        "AVENUE DU NID (Sarcelles)",
        "Église de Pantin (Pantin)",
        "ZURICH (Rueil-Malmaison)",
    };

    BOOST_CHECK_EQUAL_RANGE(stop_areas_name, stop_areas_sorted);
}

BOOST_AUTO_TEST_CASE(human_sort_on_stop_points_should_not_throw) {
    std::vector<std::shared_ptr<StopPoint>> stop_points{
        create_obj_with_name<StopPoint>("AVENUE DU NID (Sarcelles)"),
        create_obj_with_name<StopPoint>("Alésia (Paris)"),
        create_obj_with_name<StopPoint>("Asnières — Gennevilliers Les Courtilles (Asnières-sur-Seine)"),
        create_obj_with_name<StopPoint>("Aéroport de Marseille (Marseille)"),
        create_obj_with_name<StopPoint>("ZURICH (Rueil-Malmaison)"),
        create_obj_with_name<StopPoint>("Église de Pantin (Pantin)"),
    };

    boost::sort(stop_points, Less());

    std::vector<std::string> stop_points_name;
    boost::transform(stop_points, std::back_inserter(stop_points_name), [](const auto& sp) { return sp->name; });

    std::vector<std::string> stop_points_sorted{
        "Aéroport de Marseille (Marseille)",
        "Alésia (Paris)",
        "Asnières — Gennevilliers Les Courtilles (Asnières-sur-Seine)",
        "AVENUE DU NID (Sarcelles)",
        "Église de Pantin (Pantin)",
        "ZURICH (Rueil-Malmaison)",
    };

    BOOST_CHECK_EQUAL_RANGE(stop_points_name, stop_points_sorted);
}

struct disruption_fixture {
    disruption_fixture() : disruption(std::make_unique<Disruption>("bob", navitia::type::RTLevel::Adapted)) {
        navitia::type::disruption::DisruptionHolder holder;
        disruption->add_impact(boost::make_shared<Impact>(), holder);

        disruption->publication_period =
            pt::time_period(pt::time_from_string("2013-02-22 12:32:00"), pt::time_from_string("2013-02-23 12:32:00"));

        _impact = disruption->get_impacts().back();

        auto impact_acquired = _impact.lock();
        impact_acquired->disruption = disruption.get();

        // the impact can be applied every day from 6to in the publication period
        auto publication_period = boost::gregorian::date_period(boost::gregorian::from_undelimited_string("20130301"),
                                                                boost::gregorian::from_undelimited_string("20130401"));
        for (boost::gregorian::day_iterator it(publication_period.begin()); it < publication_period.end(); ++it) {
            impact_acquired->application_periods.emplace_back(pt::ptime(*it, pt::duration_from_string("06:00")),
                                                              pt::ptime(*it, pt::duration_from_string("20:00")));
        }
        navitia::type::disruption::ApplicationPattern application_pattern;
        application_pattern.add_time_slot("06:00"_t, "20:00"_t);
        application_pattern.week_pattern = std::bitset<7>{"1111111"};
        application_pattern.application_period =
            boost::gregorian::date_period(boost::gregorian::from_undelimited_string("20140301"),
                                          boost::gregorian::from_undelimited_string("20140305"));
        impact_acquired->application_patterns.insert(application_pattern);
    }

    std::unique_ptr<Disruption> disruption;
    boost::weak_ptr<Impact> _impact;
    pt::ptime time_in_publication_period = pt::time_from_string("2013-02-22 20:00:00");
};

BOOST_FIXTURE_TEST_CASE(application_patterns, disruption_fixture) {
    auto impact = _impact.lock();

    std::set<nt::disruption::ApplicationPattern, std::less<nt::disruption::ApplicationPattern>>
        tmp_application_patterns = impact->application_patterns;

    BOOST_CHECK_EQUAL(tmp_application_patterns.size(), 1);

    auto application_pattern = *(tmp_application_patterns.begin());
    BOOST_CHECK_EQUAL(application_pattern.application_period,
                      boost::gregorian::date_period(boost::gregorian::from_undelimited_string("20140301"),
                                                    boost::gregorian::from_undelimited_string("20140305")));
    BOOST_CHECK_EQUAL(application_pattern.week_pattern, std::bitset<7>{"1111111"});
    BOOST_CHECK_EQUAL(application_pattern.time_slots.size(), 1);

    auto time_slot = *(application_pattern.time_slots.begin());
    BOOST_CHECK_EQUAL(time_slot.begin, "06:00"_t);
    BOOST_CHECK_EQUAL(time_slot.end, "20:00"_t);
}

BOOST_FIXTURE_TEST_CASE(message_publishable, disruption_fixture) {
    auto impact = _impact.lock();
    auto p = pt::time_period(pt::time_from_string("2013-03-01 07:00:00"), pt::hours(1));  // in publication period
    BOOST_CHECK(impact->is_valid(time_in_publication_period, p));
    BOOST_CHECK_NOT(impact->is_valid(pt::time_from_string("2013-02-22 06:00:00"), p));  // before publication period
    BOOST_CHECK_NOT(impact->is_valid(pt::time_from_string("2013-02-24 01:00:00"), p));  // after in publication period
}

BOOST_FIXTURE_TEST_CASE(message_is_applicable_simple, disruption_fixture) {
    auto impact = _impact.lock();
    // it is applicable if there is at least one intersection not null with a period
    BOOST_CHECK(impact->is_valid(time_in_publication_period,
                                 pt::time_period(pt::time_from_string("2013-03-23 00:00:00"), pt::hours(24))));
    BOOST_CHECK(impact->is_valid(time_in_publication_period,
                                 pt::time_period(pt::time_from_string("2013-03-22 10:00:00"), pt::hours(1))));
    BOOST_CHECK_NOT(impact->is_valid(time_in_publication_period,
                                     pt::time_period(pt::time_from_string("2013-03-23 04:00:00"), pt::hours(1))));

    BOOST_CHECK(impact->is_valid(time_in_publication_period,
                                 pt::time_period(pt::time_from_string("2013-03-22 12:30:00"), pt::hours(10))));
    BOOST_CHECK(impact->is_valid(time_in_publication_period,
                                 pt::time_period(pt::time_from_string("2013-03-23 12:30:00"), pt::hours(10))));
}

/*
 * Test that the construction of a tz handler from a list a dst then it's conversion to this list of dst
 * leads to the same initial period
 */
BOOST_AUTO_TEST_CASE(tz_handler_test) {
    namespace nt = navitia::type;
    const nt::TimeZoneHandler::dst_periods periods{
        {"02:00"_t, {{"20160101"_d, "20160601"_d}, {"20160901"_d, "20170101"_d}}},
        {"03:00"_t, {{"20160601"_d, "20160901"_d}}}};
    nt::TimeZoneHandler tz_handler{"paris", "20160101"_d, periods};

    auto build_dst_periods = tz_handler.get_periods_and_shift();

    BOOST_CHECK_EQUAL_RANGE(periods, build_dst_periods);
}

/*
 * Test that the tz handler doesn't overflow
 */
BOOST_AUTO_TEST_CASE(tz_handler_overflow_test) {
    namespace nt = navitia::type;
    const nt::TimeZoneHandler::dst_periods periods{
        {"12:00"_t, {{"20160101"_d, "20160601"_d}, {"20160901"_d, "20170101"_d}}}};
    nt::TimeZoneHandler tz_handler{"paris", "20160101"_d, periods};

    auto build_dst_periods = tz_handler.get_periods_and_shift();

    BOOST_CHECK_EQUAL_RANGE(periods, build_dst_periods);
    BOOST_CHECK_EQUAL(build_dst_periods.begin()->first, 60 * 60 * 12);
}

BOOST_AUTO_TEST_CASE(get_sections_ranks) {
    ed::builder b("20120614");

    b.sa("0", 0, 0, false, true)("0a")("0b")("0c");
    // 0a 0b 1 0a 2 3 0a 0b 0c 4 2 5 2
    const auto* vj = b.vj("A")("0a", "09:00"_t)("0b", "09:00"_t)("1", "09:00"_t)("0a", "09:00"_t)("2", "09:00"_t)(
                          "3", "09:00"_t)("0a", "09:00"_t)("0b", "09:00"_t)("0c", "09:00"_t)("4", "09:00"_t)(
                          "2", "09:00"_t)("5", "09:00"_t)("2", "09:00"_t)
                         .make();
    b.data->pt_data->sort_and_index();
    b.data->complete();
    b.finish();

    auto sa = [&](const std::string& id) { return b.get<type::StopArea>(id); };

    using rst = RankStopTime;
    // 0a 0b 1 0a 2 3 0a 0b 0c 4 2 5 2
    //       *
    BOOST_CHECK_EQUAL(vj->get_sections_ranks(sa("1"), sa("1")), std::set<rst>({rst(2)}));
    // 0a 0b 1 0a 2 3 0a 0b 0c 4 2 5 2
    //       ********
    BOOST_CHECK_EQUAL(vj->get_sections_ranks(sa("1"), sa("3")), std::set<rst>({rst(2), rst(3), rst(4), rst(5)}));
    // 0a 0b 1 0a 2 3 0a 0b 0c 4 2 5 2
    //
    // 4 is after 0 -> empty
    BOOST_CHECK_EQUAL(vj->get_sections_ranks(sa("4"), sa("0")), std::set<rst>());
    // 0a 0b 1 0a 2 3 0a 0b 0c 4 2 5 2
    // ** **   **     ** ** **
    // route point, only the corresponding stop point
    BOOST_CHECK_EQUAL(vj->get_sections_ranks(sa("0"), sa("0")),
                      std::set<rst>({rst(0), rst(1), rst(3), rst(6), rst(7), rst(8)}));
    // 0a 0b 1 0a 2 3 0a 0b 0c 4 2 5 2
    //         ******
    // shortest sections, thus we don't have 1
    BOOST_CHECK_EQUAL(vj->get_sections_ranks(sa("0"), sa("3")), std::set<rst>({rst(3), rst(4), rst(5)}));
    // 0a 0b 1 0a 2 3 0a 0b 0c 4 2 5 2
    //         ****   ************
    // shortest sections, thus we don't have 1, 3 and 5
    // We still have 0a 0b 0c in the second section since they are in the same area
    BOOST_CHECK_EQUAL(vj->get_sections_ranks(sa("0"), sa("2")),
                      std::set<rst>({rst(3), rst(4), rst(6), rst(7), rst(8), rst(9), rst(10)}));
}
