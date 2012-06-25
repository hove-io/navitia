#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_first_letter

#include "first_letter.h"
#include "type/data.h"
#include <boost/test/unit_test.hpp>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include<map>

namespace pt = boost::posix_time;
BOOST_AUTO_TEST_CASE(parse_state_test){
    FirstLetter<unsigned int> fl;
    fl.add_string("rue jean jaures", 0);
    fl.add_string("place jean jaures", 1);
    fl.add_string("rue jeanne d'arc", 2);
    fl.add_string("avenue jean jaures", 3);
    fl.add_string("boulevard poniatowski", 4);
    fl.add_string("pente de Bray", 5);
    fl.build();    

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

    res = fl.find("r jean");
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = fl.find("jean r");
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = fl.find("jEaN r");
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = fl.find("ponia");
    expected = {4};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = fl.find("ru je jau");
    expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(alias_test){
    FirstLetter<unsigned int> fl;
    std::map <std::string,std::string> map;
    map["st"]="saint";
    std::string str = fl.get_alias("st", map);
    BOOST_CHECK_EQUAL("saint",str);

    map["de"]="";
    str = fl.get_alias("de", map);
    BOOST_CHECK_EQUAL("", str);

}
