#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include "proximity_list.h"

using namespace navitia::type;

BOOST_AUTO_TEST_CASE(distances_euclidiennes) {
    GeographicalCoord a(0,0, false);
    GeographicalCoord b(1,0, false);

    BOOST_CHECK_CLOSE(a.distance_to(b), 1, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 1, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    b.y = 1;
    BOOST_CHECK_CLOSE(a.distance_to(b), 1.4142, 0.01);
    BOOST_CHECK_CLOSE(b.distance_to(a), 1.4142, 0.01);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    b.x = -1;
    b.y = 0;
    BOOST_CHECK_CLOSE(a.distance_to(b), 1, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 1, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    b.x = 0; b.y = 1;
    BOOST_CHECK_CLOSE(a.distance_to(b), 1, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 1, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    b.y = - 100;
    BOOST_CHECK_CLOSE(a.distance_to(b), 100, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 100, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    a.y = 100;
    BOOST_CHECK_CLOSE(a.distance_to(b), 200, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 200, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
}

BOOST_AUTO_TEST_CASE(distances_grand_cercle)
{
    GeographicalCoord a(0,0, true);
    GeographicalCoord b(0,0, true);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
    BOOST_CHECK_CLOSE(a.distance_to(b), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(a), 0, 1e-6);

    GeographicalCoord nantes(-1.57, 47.22);
    GeographicalCoord paris(2.36, 48.85);
    BOOST_CHECK_CLOSE(nantes.distance_to(paris), 340000, 10);
    BOOST_CHECK_CLOSE(paris.distance_to(nantes), 340000, 10);

    a.x = 180;
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);

    a.x=84; a.y=89.999;
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);


    a.x=34; a.y=-89.999;
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/2,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);


    a.x=0; a.y=45;
    BOOST_CHECK_CLOSE(a.distance_to(b), 6371*3.14*1000/4,1);
    BOOST_CHECK_CLOSE(b.distance_to(a), 6371*3.14*1000/4,1);
    BOOST_CHECK_CLOSE(a.distance_to(a), 0, 1e-6);
    BOOST_CHECK_CLOSE(b.distance_to(b), 0, 1e-6);
}
BOOST_AUTO_TEST_CASE(find_nearest){
    typedef std::pair<unsigned int, GeographicalCoord> p;
    ProximityList<unsigned int> pl;

    GeographicalCoord c;
    std::vector<GeographicalCoord> coords;
    c.degrees = false;

    // Exemple d'illustration issu de wikipedia
    c.x = 2; c.y = 3; pl.add(c, 1); coords.push_back(c);
    c.x = 5; c.y = 4; pl.add(c, 2); coords.push_back(c);
    c.x = 9; c.y = 6; pl.add(c, 3); coords.push_back(c);
    c.x = 4; c.y = 7; pl.add(c, 4); coords.push_back(c);
    c.x = 8; c.y = 1; pl.add(c, 5); coords.push_back(c);
    c.x = 7; c.y = 2; pl.add(c, 6); coords.push_back(c);
    
    pl.build();

    std::vector<unsigned int> expected {1,2,4,6,5,3};
    for(size_t i=0; i < expected.size(); ++i)
        BOOST_CHECK_EQUAL(pl.items[i].element, expected[i]);


    c.x = 2; c.y = 3;
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 1);

    c.x = 7.1; c.y = 1.9;
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 6);

    c.x = 100; c.y = 6;
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 3);

    c.x = 2; c.y=4;

    expected = {1};
    auto tmp1 = pl.find_within(c, 1.1);
    std::vector<unsigned int> tmp;
    BOOST_FOREACH(auto p, tmp1) tmp.push_back(p.first);
    BOOST_CHECK_EQUAL(tmp1[0].second, coords[0]);
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2};
    tmp1 = pl.find_within(c, 3.1);
    tmp.clear();
    BOOST_FOREACH(auto p, tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4};
    tmp.clear();
    tmp1 = pl.find_within(c, 3.7);
    BOOST_FOREACH(auto p, tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 5.4);
    BOOST_FOREACH(auto p, tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,4,5,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 6.8);
    BOOST_FOREACH(auto p, tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());

    expected={1,2,3,4,5,6};
    tmp.clear();
    tmp1 = pl.find_within(c, 7.3);
    BOOST_FOREACH(auto p, tmp1) tmp.push_back(p.first);
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(tmp.begin(), tmp.end(), expected.begin(), expected.end());
}
