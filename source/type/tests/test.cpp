#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include "type/type.h"

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

BOOST_AUTO_TEST_CASE(uri_sa) {
    std::string uri("stop_area:moo:ext_code");
    EntryPoint ep(uri);

    BOOST_CHECK(ep.type == Type_e::eStopArea);

    // On garde le prefixe de type
    BOOST_CHECK_EQUAL(ep.external_code, "stop_area:moo:ext_code");
}

BOOST_AUTO_TEST_CASE(uri_coord) {
    std::string uri("coord:2.2:4.42");
    EntryPoint ep(uri);

    BOOST_CHECK(ep.type == Type_e::eCoord);
    BOOST_CHECK_CLOSE(ep.coordinates.lon(), 2.2, 1e-6);
    BOOST_CHECK_CLOSE(ep.coordinates.lat(), 4.42, 1e-6);

    EntryPoint ep2("coord:2.1");
    BOOST_CHECK_CLOSE(ep2.coordinates.lon(), 0, 1e-6);
    BOOST_CHECK_CLOSE(ep2.coordinates.lat(), 0, 1e-6);

    EntryPoint ep3("coord:2.1:bli");
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
