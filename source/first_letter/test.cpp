#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_first_letter

#include "first_letter.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_CASE(parse_state_test){
    FirstLetter fl;
    fl.add_string("rue jean jaures", 0);
    fl.add_string("place jean jaures", 1);
    fl.add_string("rue jeanne d'arc", 2);
    fl.add_string("avenue jean jaures", 3);

    auto res = fl.find("rue jean jaures");

    std::vector<int> expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = fl.find("jaures");
    expected = {0, 1, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = fl.find("avenue");
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = fl.find("av");
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());
}
