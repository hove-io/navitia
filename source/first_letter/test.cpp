#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_first_letter

#include "first_letter.h"
#include <boost/test/unit_test.hpp>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>

namespace pt = boost::posix_time;
BOOST_AUTO_TEST_CASE(parse_state_test){
    FirstLetter<unsigned int> fl;
    fl.add_string("rue jean jaures", 0);
    fl.add_string("place jean jaures", 1);
    fl.add_string("rue jeanne d'arc", 2);
    fl.add_string("avenue jean jaures", 3);
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

    // Partie permettant de faire des benchmarks
    std::fstream ifile("/home/tristram/adresses_uniq.txt");
    std::string line;
    int idx = 0;
    while(getline(ifile, line)) {
        fl.add_string(boost::to_lower_copy(line), idx++);
    }
    std::cout << "Nombre d'adresses : " << fl.map.size() << std::endl;

    fl.build();
    std::cout << "Serious stuff now !" << std::endl;
    pt::ptime start, end;
    start = pt::microsec_clock::local_time();
    for(int i=0; i <100; i++){
        fl.find("rue jaur");
        fl.find("av char de gau");
        fl.find("poniat");
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Il y a " /*<< indexes.size()*/ << " rue jean jaures en France, et il a fallu "
              << (end - start).total_milliseconds() << " ms pour le calculer" << std::endl;
}
