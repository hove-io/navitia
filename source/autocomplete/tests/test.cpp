#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_autocomplete

#include "autocomplete/autocomplete.h"
#include "type/data.h"
#include <boost/test/unit_test.hpp>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include<map>
#include<unordered_map>

namespace pt = boost::posix_time;
using namespace navitia::autocomplete;

BOOST_AUTO_TEST_CASE(parse_state_test){
    Autocomplete<unsigned int> ac;
    ac.add_string("rue jean jaures", 0);
    ac.add_string("place jean jaures", 1);
    ac.add_string("rue jeanne d'arc", 2);
    ac.add_string("avenue jean jaures", 3);
    ac.add_string("boulevard poniatowski", 4);
    ac.add_string("pente de Bray", 5);
    ac.build();    

    auto res = ac.find("rue jean jaures");
    std::vector<int> expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("jaures");
    expected = {0, 1, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("avenue");
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("av");
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("r jean");
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("jean r");
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("jEaN r");
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("ponia");
    expected = {4};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    res = ac.find("ru je jau");
    expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());    
}

/*
    > Le fonctionnement d'alias :


  */

BOOST_AUTO_TEST_CASE(alias_test){
    Autocomplete<unsigned int> ac;
    std::map <std::string,std::string> map;
    map["st"]="saint";
    std::string str = ac.get_alias("st", map);
    BOOST_CHECK_EQUAL("saint",str);

    map["de"]="";
    str = ac.get_alias("de", map);
    BOOST_CHECK_EQUAL("", str);
}

/*
    > Le fonctionnement partiel :> On prends tous les autocomplete s'il y au moins un match
      et trie la liste des Autocomplete par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des matchs dans Autocomplete.

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
        Autocomplete<unsigned int> ac;

        ac.add_string("place jean jaures", 0);
        ac.add_string("rue jeanne d'arc", 1);
        ac.add_string("avenue jean jaures", 2);
        ac.add_string("boulevard poniatowski", 3);
        ac.add_string("pente de Bray", 4);
        ac.add_string("rue jean jaures", 5);

        ac.build();

        auto res = ac.find_partial("rue jean");
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_CHECK_EQUAL(res.at(0).idx, 5);
        BOOST_CHECK_EQUAL(res.at(1).idx, 1);
        //Ils on la même qualité, l'ordre est donc indéterminé
        BOOST_CHECK(res.at(2).idx == 0 || res.at(2).idx == 2);
        BOOST_CHECK(res.at(3).idx == 0 || res.at(3).idx == 2);

        BOOST_CHECK_EQUAL(res.at(0).quality, 66);
        BOOST_CHECK_EQUAL(res.at(1).quality, 50);
        BOOST_CHECK_EQUAL(res.at(2).quality, 33);
        BOOST_CHECK_EQUAL(res.at(3).quality, 33);
    }

/*
    > Le fonctionnement normal :> On prends que les autocomplete si tous les mots dans la recherche existent
      et trie la liste des Autocomplete par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des mots dans autocomplete remonté.
sort
    Exemple:
    vector[] = {0,1,2,3,4,5,6,7,8};
    Recherche : "rue jean";
    Vector      nb_found    AC_wordcount    Quality
        0           2           4               50
        2           2           5               40
        6           2           3               66
        7           2           3               66


    > Le résultat est trié par qualité.
    Result [] = {6,7,0,2}
*/

BOOST_AUTO_TEST_CASE(autocomplete_find_quality_test){
        Autocomplete<unsigned int> ac;
        ac.add_string("rue jeanne d'arc", 0);
        ac.add_string("place jean jaures", 1);
        ac.add_string("rue jean paul gaultier paris", 2);
        ac.add_string("avenue jean jaures", 3);
        ac.add_string("boulevard poniatowski", 4);
        ac.add_string("pente de Bray", 5);
        ac.add_string("rue jean jaures", 6);
        ac.add_string("rue jean zay ", 7);
        ac.add_string("place jean paul gaultier ", 8);

        ac.build();

        auto res = ac.find_complete("rue jean");
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
