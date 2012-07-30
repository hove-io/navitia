#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_first_letter

#include "first_letter/first_letter.h"
#include "type/data.h"
#include <boost/test/unit_test.hpp>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include<map>
#include<unordered_map>

namespace pt = boost::posix_time;
using namespace navitia::firstletter;

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

/*
    > Le fonctionnement d'alias :


  */

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

/*
    > Le fonctionnement partiel :> On prends tous les firstletter s'il y au moins un match
      et trie la liste des FirstLetter par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des matchs dans FirstLetter.

    Exemple:
    vector[] = {0,1,2,3,4,5};
    Recherche : "rue jean";
    Vector      nb_found    Quality
        0           1       33
        1           2       50
        2           1       33
        5           2       66

    Result [] = {1,5,0,2}

    Recherche : "jean";
    Vector      nb_found    Quality
        0           1       100
        1           1       100
        2           1       100
        5           1       100

    > Le résultat est trié par qualité
    Result [] = {0,1,2,5}
*/

BOOST_AUTO_TEST_CASE(parse_state_nb_found_test){
        FirstLetter<unsigned int> fl;

        fl.add_string("place jean jaures", 0);
        fl.add_string("rue jeanne d'arc", 1);
        fl.add_string("avenue jean jaures", 2);
        fl.add_string("boulevard poniatowski", 3);
        fl.add_string("pente de Bray", 4);
        fl.add_string("rue jean jaures", 5);

        fl.build();

        auto res = fl.find_partial("rue jean");
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_REQUIRE_EQUAL(res.at(0).idx, 5);
        BOOST_REQUIRE_EQUAL(res.at(1).idx, 1);
        BOOST_REQUIRE_EQUAL(res.at(2).idx, 0);
        BOOST_REQUIRE_EQUAL(res.at(3).idx, 2);

        BOOST_REQUIRE_EQUAL(res.at(0).quality, 66);
        BOOST_REQUIRE_EQUAL(res.at(1).quality, 50);
        BOOST_REQUIRE_EQUAL(res.at(2).quality, 33);
        BOOST_REQUIRE_EQUAL(res.at(3).quality, 33);
    }

/*
    > Le fonctionnement normal :> On prends que les firstletter si tous les mots dans la recherche existent
      et trie la liste des FirstLetter par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des mots dans FirstLetter remonté.
sort
    Exemple:
    vector[] = {0,1,2,3,4,5,6,7,8};
    Recherche : "rue jean";
    Vector      nb_found    FL_wordcount    Quality
        0           2           4               50
        2           2           5               40
        6           2           3               66
        7           2           3               66


    > Le résultat est trié par qualité.
    Result [] = {6,7,0,2}
*/

BOOST_AUTO_TEST_CASE(first_letter_find_quality_test){
        FirstLetter<unsigned int> fl;
        fl.add_string("rue jeanne d'arc", 0);
        fl.add_string("place jean jaures", 1);
        fl.add_string("rue jean paul gaultier paris", 2);
        fl.add_string("avenue jean jaures", 3);
        fl.add_string("boulevard poniatowski", 4);
        fl.add_string("pente de Bray", 5);
        fl.add_string("rue jean jaures", 6);
        fl.add_string("rue jean zay ", 7);
        fl.add_string("place jean paul gaultier ", 8);

        fl.build();

        auto res = fl.find_complete("rue jean");
        std::vector<int> expected = {6,7,0,2};
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_REQUIRE_EQUAL(res.at(0).quality, 66);
        BOOST_REQUIRE_EQUAL(res.at(1).quality, 66);
        BOOST_REQUIRE_EQUAL(res.at(2).quality, 50);
        BOOST_REQUIRE_EQUAL(res.at(3).quality, 40);
        BOOST_REQUIRE_EQUAL(res.at(0).idx, 6);
        BOOST_REQUIRE_EQUAL(res.at(1).idx, 7);
        BOOST_REQUIRE_EQUAL(res.at(2).idx, 0);
        BOOST_REQUIRE_EQUAL(res.at(3).idx, 2);
    }
