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
#include "type/type.h"

namespace pt = boost::posix_time;
using namespace navitia::autocomplete;

BOOST_AUTO_TEST_CASE(parse_state_test){
    /// Liste des alias (Pas serialisé)
    std::map<std::string, std::string> alias;

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jean jaures", 0, navitia::type::Type_e::eStopArea, alias);
    ac.add_string("place jean jaures", 1, navitia::type::Type_e::eStopArea, alias);
    ac.add_string("rue jeanne d'arc", 2, navitia::type::Type_e::eStopPoint, alias);
    ac.add_string("avenue jean jaures", 3, navitia::type::Type_e::eStopPoint, alias);
    ac.add_string("boulevard poniatowski", 4, navitia::type::Type_e::eStopPoint, alias);
    ac.add_string("pente de Bray", 5, navitia::type::Type_e::eStopPoint, alias);
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
    std::string str = ac.alias_word("st", map);
    BOOST_CHECK_EQUAL("saint",str);

    map["de"]="";
    str = ac.alias_word("de", map);
    BOOST_CHECK_EQUAL("", str);
}

/*
    > Le fonctionnement partiel :> On prends tous les autocomplete s'il y au moins un match
      et trie la liste des Autocomplete par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des matchs dans Autocomplete.

    Exemple:
    vector[] = {0,1,2,3,4,5};
    Recherche : "rue jean";
    Vector      nb_found    Base Quality    Distance    Penalty Quality
        0           1       33              15-7 = 8    2       33-8-2 = 23
        1           2       50              13-7 = 6    2       50-6-2 = 42
        2           1       33              16-7 = 9    2       33-9-2 = 22
        5           2       66              13-7 = 6    2       66-6-2 = 58

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
        /// Liste des alias (Pas serialisé)
        std::map<std::string, std::string> alias;
        int word_weight = 5;


        Autocomplete<unsigned int> ac;


        ac.add_string("place jean jaures", 0, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jeanne d'arc", 1, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("avenue jean jaures", 2, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("boulevard poniatowski", 3, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("pente de Bray", 4, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jean jaures", 5, navitia::type::Type_e::eStopArea, alias);

        ac.build();

        auto res = ac.find_partial("rue jean", alias, word_weight);
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_CHECK_EQUAL(res.at(0).idx, 5);
        BOOST_CHECK_EQUAL(res.at(1).idx, 1);
        //Ils ont la même qualité, l'ordre est donc indéterminé
        BOOST_CHECK(res.at(2).idx == 0 || res.at(2).idx == 2);
        BOOST_CHECK(res.at(3).idx == 0 || res.at(3).idx == 2);

        BOOST_CHECK_EQUAL(res.at(0).quality, 87);//66
        BOOST_CHECK_EQUAL(res.at(1).quality, 82);//50
        BOOST_CHECK_EQUAL(res.at(2).quality, 80);//33
        BOOST_CHECK_EQUAL(res.at(3).quality, 79);//33
    }

/*
    > Le fonctionnement normal :> On prends que les autocomplete si tous les mots dans la recherche existent
      et trie la liste des Autocomplete par la qualité.
    > La qualité est calculé avec le nombre des mots dans la recherche et le nombre des mots dans autocomplete remonté.
sort
    Exemple:
    vector[] = {0,1,2,3,4,5,6,7,8};
    Recherche : "rue jean";
    Vector      nb_found    AC_wordcount    Base Quality  Distance    Penalty Quality
        0           2           4               50        13-7=6        2       50-6-2 = 42
        2           2           5               40        24-7=17       2       40-17-2= 21
        6           2           3               66        13-7=6        2       66-6-2 = 58
        7           2           3               66        10-7=3        2       66-3-2 = 61

    > Le résultat est trié par qualité.
    Result [] = {6,7,0,2}
*/

BOOST_AUTO_TEST_CASE(autocomplete_find_quality_test){
        /// Liste des alias (Pas serialisé)
        std::map<std::string, std::string> alias;
        int word_weight = 5;

        Autocomplete<unsigned int> ac;
        ac.add_string("rue jeanne d'arc", 0, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("place jean jaures", 1, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jean paul gaultier paris", 2, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("avenue jean jaures", 3, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("boulevard poniatowski", 4, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("pente de Bray", 5, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jean jaures", 6, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jean zay ", 7, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("place jean paul gaultier ", 8, navitia::type::Type_e::eStopArea, alias);

        ac.build();

        auto res = ac.find_complete("rue jean", alias, word_weight);
        std::vector<int> expected = {6,7,0,2};
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_REQUIRE_EQUAL(res.at(0).quality, 90);//66
        BOOST_REQUIRE_EQUAL(res.at(1).quality, 87);//66
        BOOST_REQUIRE_EQUAL(res.at(2).quality, 82);//50
        BOOST_REQUIRE_EQUAL(res.at(3).quality, 66);//40
        BOOST_REQUIRE_EQUAL(res.at(0).idx, 7);//6
        BOOST_REQUIRE_EQUAL(res.at(1).idx, 6);//7
        BOOST_REQUIRE_EQUAL(res.at(2).idx, 0);
        BOOST_REQUIRE_EQUAL(res.at(3).idx, 2);
    }


BOOST_AUTO_TEST_CASE(autocomplete_alias_and_weight_test){
        /// Liste des alias (Pas serialisé)
        std::map<std::string, std::string> alias;
        int word_weight = 5;

        alias["de"]="";
        alias["la"]="";
        alias["les"]="";
        alias["des"]="";
        alias["d"]="";
        alias["l"]="";

        alias["st"]="saint";
        alias["ste"]="sainte";
        alias["cc"]="centre commercial";
        alias["chu"]="hopital";
        alias["chr"]="hopital";
        alias["c.h.u"]="hopital";
        alias["c.h.r"]="hopital";
        alias["all"]="allée";
        alias["allee"]="allée";
        alias["ave"]="avenue";
        alias["av"]="avenue";
        alias["bvd"]="boulevard";
        alias["bld"]="boulevard";
        alias["bd"]="boulevard";
        alias["b"]="boulevard";
        alias["r"]="rue";
        alias["pl"]="place";

        Autocomplete<unsigned int> ac;
        ac.add_string("rue jeanne d'arc", 0, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("place jean jaures", 1, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jean paul gaultier paris", 2, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("avenue jean jaures", 3, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("boulevard poniatowski", 4, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("pente de Bray", 5, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jean jaures", 6, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("rue jean zay ", 7, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("place jean paul gaultier ", 8, navitia::type::Type_e::eStopArea, alias);
        ac.add_string("hopital paul gaultier", 8, navitia::type::Type_e::ePOI, alias);

        ac.build();

        auto res = ac.find_complete("rue jean", alias, word_weight);
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_REQUIRE_EQUAL(res.at(0).quality, 90);

        auto res1 = ac.find_complete("r jean", alias, word_weight);
        BOOST_REQUIRE_EQUAL(res1.size(), 4);

        BOOST_REQUIRE_EQUAL(res1.at(0).quality, 90);

        auto res2 = ac.find_complete("av jean", alias, word_weight);
        BOOST_REQUIRE_EQUAL(res2.size(), 1);
        //rue jean zay
        // distance = 6 / pénalité = 2 / word_weight = 1*5 = 5
        // Qualité = 100 - (6 + 2 + 5) = 87
        BOOST_REQUIRE_EQUAL(res2.at(0).quality, 87);

        word_weight = 10;
        auto res3 = ac.find_complete("av jean", alias, word_weight);
        BOOST_REQUIRE_EQUAL(res3.size(), 1);
        //rue jean zay
        // distance = 6 / pénalité = 2 / word_weight = 1*10 = 10
        // Qualité = 100 - (6 + 2 + 10) = 82
        BOOST_REQUIRE_EQUAL(res3.at(0).quality, 82);

        word_weight = 10;
        auto res4 = ac.find_complete("chu gau", alias, word_weight);
        BOOST_REQUIRE_EQUAL(res4.size(), 1);
        //hopital paul gaultier
        // distance = 9 / pénalité = 6 / word_weight = 1*10 = 10
        // Qualité = 100 - (9 + 6 + 10) = 75
        BOOST_REQUIRE_EQUAL(res4.at(0).quality, 75);

        //auto res5 = ac.find_complete("c.h.u. gau", alias, word_weight);
        //BOOST_REQUIRE_EQUAL(res5.size(), 1);
        //hopital paul gaultier
        // distance = 9 / pénalité = 6 / word_weight = 1*10 = 10
        // Qualité = 100 - (9 + 6 + 10) = 75
        //BOOST_REQUIRE_EQUAL(res5.at(0).quality, 75);
    }
