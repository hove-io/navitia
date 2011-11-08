#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_proximity_list

#include <boost/test/unit_test.hpp>
#include "proximity_list.h"

#include <vector>
#include <iostream>

namespace pt = boost::posix_time;
BOOST_AUTO_TEST_CASE(parse_state_test){
using namespace navitia::type;
    ProximityList<unsigned int> pl;

    GeographicalCoord c;

    // Exemple d'illustration issu de wikipedia
    c.x = 2; c.y = 3; pl.add(c, 1);
    c.x = 5; c.y = 4; pl.add(c, 2);
    c.x = 9; c.y = 6; pl.add(c, 3);
    c.x = 4; c.y = 7; pl.add(c, 4);
    c.x = 8; c.y = 1; pl.add(c, 5);
    c.x = 7; c.y = 2; pl.add(c, 6);
    
    pl.build();

    std::vector<unsigned int> expected {1,2,4,6,5,3};
    for(size_t i=0; i < expected.size(); ++i)
        BOOST_CHECK_EQUAL(pl.items[i].element, expected[i]);

    auto res = pl.find_within(c, 100);
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = pl.find_k_nearest(c, 10);
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    c.x = 2; c.y = 3;
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 1);

    c.x = 7.1; c.y = 1.9;
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 6);

    c.x = 100; c.y = 6;
    BOOST_CHECK_EQUAL(pl.find_nearest(c), 3);
}
