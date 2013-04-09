#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include "proximity_list/proximity_list.h"

using namespace navitia::type;
using namespace navitia::proximitylist;

BOOST_AUTO_TEST_CASE(distances_grand_cercle)
{
    GeographicalCoord a(0,0);
    GeographicalCoord b(0,0);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(b), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 0, 1e-6);

    GeographicalCoord nantes(-1.57, 47.22);
    GeographicalCoord paris(2.36, 48.85);
    BOOST_CHECK_CLOSE(nantes.distance_to(paris), 340000, 10);
    BOOST_CHECK_CLOSE(paris.distance_to(nantes), 340000, 10);

    a.set_lon(180);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);

    a.set_lon(84); a.set_lat(89.999);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);


    a.set_lon(34); a.set_lat(-89.999);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);


    a.set_lon(0); a.set_lat(45);
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/4,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/4,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
}

BOOST_AUTO_TEST_CASE(approx_distance){
    GeographicalCoord canaltp(2.3921, 48.8296);
    GeographicalCoord tour_eiffel(2.29447,48.85834);
    double coslat = ::cos(canaltp.lat() * 0.0174532925199432958);
    BOOST_CHECK_CLOSE(canaltp.distance_to(tour_eiffel), ::sqrt(canaltp.approx_sqr_distance(tour_eiffel, coslat)), 1);
    BOOST_CHECK_CLOSE(tour_eiffel.distance_to(canaltp), ::sqrt(tour_eiffel.approx_sqr_distance(canaltp, coslat)), 1);
    BOOST_CHECK_CLOSE(canaltp.distance_to(tour_eiffel), ::sqrt(tour_eiffel.approx_sqr_distance(canaltp, coslat)), 1);
    BOOST_CHECK_CLOSE(tour_eiffel.distance_to(canaltp), ::sqrt(canaltp.approx_sqr_distance(tour_eiffel, coslat)), 1);
}

BOOST_AUTO_TEST_CASE(find_nearest){
    constexpr double M_TO_DEG = 1.0/111320.0;
    typedef std::pair<unsigned int, GeographicalCoord> p;
    ProximityList<unsigned int> pl;

    GeographicalCoord c;
    std::vector<GeographicalCoord> coords;

    // Exemple d'illustration issu de wikipedia
    c.set_lon(M_TO_DEG *2); c.set_lat(M_TO_DEG *3); pl.add(c, 1); coords.push_back(c);
    c.set_lon(M_TO_DEG *5); c.set_lat(M_TO_DEG *4); pl.add(c, 2); coords.push_back(c);
    c.set_lon(M_TO_DEG *9); c.set_lat(M_TO_DEG *6); pl.add(c, 3); coords.push_back(c);
    c.set_lon(M_TO_DEG *4); c.set_lat(M_TO_DEG *7); pl.add(c, 4); coords.push_back(c);
    c.set_lon(M_TO_DEG *8); c.set_lat(M_TO_DEG *1); pl.add(c, 5); coords.push_back(c);
    c.set_lon(M_TO_DEG *7); c.set_lat(M_TO_DEG *2); pl.add(c, 6); coords.push_back(c);
    
    pl.build();

    std::vector<unsigned int> expected {1,4,2,6,5,3};
    for(size_t i=0; i < expected.size(); ++i)
        BOOST_CHECK_EQUAL(pl.items[i].element, expected[i]);


    c.set_lon(M_TO_DEG *2); c.set_lat(M_TO_DEG *3);
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 1);

    c.set_lon(M_TO_DEG *7.1); c.set_lat(M_TO_DEG *1.9);
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 6);

    c.set_lon(M_TO_DEG *100); c.set_lat(M_TO_DEG *6);
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 3);

    c.set_lon(M_TO_DEG *2); c.set_lat(M_TO_DEG *4);

    expected = {1};
    auto tmp1 = pl.find_within(c, 1.1);
    std::vector<unsigned int> tmp;
    for(auto p : tmp1) tmp.push_back(p.first);
    BOOST_CHECK_EQUAL(tmp1[0].second, coords[0]);
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2};
    tmp1 = pl.find_within(c, 3.1);
    tmp.clear();
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4};
    tmp.clear();
    tmp1 = pl.find_within(c, 3.7);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 5.4);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4,5,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 6.8);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,3,4,5,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 7.3);
    for(auto p : tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());
}
