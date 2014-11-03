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
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include "type/type.h"
#include "type/message.h"
#include "type/data.h"

#include <boost/geometry.hpp>
#include <boost/make_shared.hpp>

namespace pt = boost::posix_time;
namespace bg = boost::gregorian;
using navitia::type::new_disruption::Impact;
using navitia::type::new_disruption::Disruption;

#define BOOST_CHECK_NOT(value) BOOST_CHECK(!value)

using namespace navitia::type;
BOOST_AUTO_TEST_CASE(boost_geometry){
    GeographicalCoord c(2, 48);
    GeographicalCoord b(3, 48);
    std::vector<GeographicalCoord> line{c, b};
    GeographicalCoord centroid;
    boost::geometry::centroid(line, centroid);
    BOOST_CHECK_EQUAL(centroid, GeographicalCoord(2.5, 48));
    //spherical_point s(2,48);
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

    GeographicalCoord a(0,0); // Début de segment
    GeographicalCoord b(10, 0); // Fin de segment
    GeographicalCoord p(5, 5); // Point à projeter
    GeographicalCoord pp; //Point projeté
    float d; // Distance du projeté
    tie(pp,d) = p.project(a, b);

    BOOST_CHECK_EQUAL(pp.lon(), 5);
    BOOST_CHECK_EQUAL(pp.lat(), 0);
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1e-3);

    // Si on est à l'extérieur à gauche
    p.set_lon(-10); p.set_lat(0);
    tie(pp,d) = p.project(a, b);
    BOOST_CHECK_EQUAL(pp.lon(), a.lon());
    BOOST_CHECK_EQUAL(pp.lat(), a.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1);

    // Si on est à l'extérieur à droite
    p.set_lon(20); p.set_lat(0);
    tie(pp,d) = p.project(a, b);
    BOOST_CHECK_EQUAL(pp.lon(), b.lon());
    BOOST_CHECK_EQUAL(pp.lat(), b.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1);

    // On refait la même en permutant A et B

    // Si on est à l'extérieur à gauche
    p.set_lon(-10); p.set_lat(0);
    tie(pp,d) = p.project(b, a);
    BOOST_CHECK_EQUAL(pp.lon(), a.lon());
    BOOST_CHECK_EQUAL(pp.lat(), a.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1);

    // Si on est à l'extérieur à droite
    p.set_lon(20); p.set_lat(0);
    tie(pp,d) = p.project(b, a);
    BOOST_CHECK_EQUAL(pp.lon(), b.lon());
    BOOST_CHECK_EQUAL(pp.lat(), b.lat());
    BOOST_CHECK_CLOSE(d, pp.distance_to(p),1);

    // On essaye avec des trucs plus pentus ;)
    a.set_lon(-3); a.set_lat(-3);
    b.set_lon(5); b.set_lat(5);
    p.set_lon(-2);  p.set_lat(2);
    tie(pp,d) = p.project(a, b);
    BOOST_CHECK_SMALL(pp.lon(), 1e-3);
    BOOST_CHECK_SMALL(pp.lat(), 1e-3);
    BOOST_CHECK_CLOSE(d, pp.distance_to(p), 1e-3);
}


struct disruption_fixture {
    disruption_fixture() : disruption(std::make_unique<Disruption>()) {
        disruption->impacts.push_back(boost::make_shared<Impact>());

        disruption->publication_period = pt::time_period(
                    pt::time_from_string("2013-02-22 12:32:00"),
                    pt::time_from_string("2013-02-23 12:32:00"));

        _impact = disruption->impacts.back();

        auto impact_acquired = _impact.lock();
        impact_acquired->disruption = disruption.get();

        //the impact can be applied every day from 6to in the publication period
        auto publication_period = boost::gregorian::date_period(boost::gregorian::from_undelimited_string("20130301"),
                                                              boost::gregorian::from_undelimited_string("20130401"));
        for(boost::gregorian::day_iterator it(publication_period.begin()); it < publication_period.end(); ++it) {
            impact_acquired->application_periods.push_back(pt::time_period(
                                                               pt::ptime(*it, pt::duration_from_string("06:00")),
                                                               pt::ptime(*it, pt::duration_from_string("20:00"))));
        }
    }

    std::unique_ptr<Disruption> disruption;
    boost::weak_ptr<Impact> _impact;
    pt::ptime time_in_publication_period = pt::time_from_string("2013-02-22 20:00:00");
};


BOOST_FIXTURE_TEST_CASE(message_publishable, disruption_fixture) {
    auto impact = _impact.lock();
    auto p = pt::time_period(pt::time_from_string("2013-03-01 07:00:00"), pt::hours(1)); //in publication period

    BOOST_CHECK(impact->is_valid(time_in_publication_period, p));
    BOOST_CHECK_NOT(impact->is_valid(pt::time_from_string("2013-02-22 06:00:00"), p)); //before publication period
    BOOST_CHECK_NOT(impact->is_valid(pt::time_from_string("2013-02-24 01:00:00"), p)); //after in publication period
}

BOOST_FIXTURE_TEST_CASE(message_is_applicable_simple, disruption_fixture) {
    auto impact = _impact.lock();
    //it is applicable if there is at least one intersection not null with a period
    BOOST_CHECK(impact->is_valid(time_in_publication_period, pt::time_period(pt::time_from_string("2013-03-23 00:00:00"), pt::hours(24))));
    BOOST_CHECK(impact->is_valid(time_in_publication_period, pt::time_period(pt::time_from_string("2013-03-22 10:00:00"), pt::hours(1))));
    BOOST_CHECK_NOT(impact->is_valid(time_in_publication_period, pt::time_period(pt::time_from_string("2013-03-23 04:00:00"), pt::hours(1))));

    BOOST_CHECK(impact->is_valid(time_in_publication_period, pt::time_period(pt::time_from_string("2013-03-22 12:30:00"), pt::hours(10))));
    BOOST_CHECK(impact->is_valid(time_in_publication_period, pt::time_period(pt::time_from_string("2013-03-23 12:30:00"), pt::hours(10))));
}


BOOST_AUTO_TEST_CASE(vj_is_odt) {
    navitia::type::VehicleJourney vj_odt, vj_regular;

    vj_regular.vehicle_journey_type = VehicleJourneyType::regular;
    vj_regular.has_landing();
    BOOST_CHECK_NOT(vj_regular.is_odt());

    vj_odt.vehicle_journey_type = VehicleJourneyType::virtual_with_stop_time;
    BOOST_CHECK(vj_odt.is_odt());

    vj_odt.vehicle_journey_type = VehicleJourneyType::virtual_without_stop_time;
    BOOST_CHECK(vj_odt.is_odt());

    vj_odt.vehicle_journey_type = VehicleJourneyType::stop_point_to_stop_point;
    BOOST_CHECK(vj_odt.is_odt());

    vj_odt.vehicle_journey_type = VehicleJourneyType::adress_to_stop_point;
    BOOST_CHECK(vj_odt.is_odt());

    vj_odt.vehicle_journey_type = VehicleJourneyType::odt_point_to_point;
    BOOST_CHECK(vj_odt.is_odt());
}
