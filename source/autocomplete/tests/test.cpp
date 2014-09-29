/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_autocomplete

#include "autocomplete/autocomplete.h"
#include "autocomplete/autocomplete_api.h"
#include "type/data.h"
#include <boost/test/unit_test.hpp>
#include <vector>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include<map>
#include<unordered_map>
#include "type/type.h"
#include <boost/regex.hpp>
#include "georef/adminref.h"
#include "georef/georef.h"
#include "routing/raptor.h"
#include "ed/build_helper.h"

namespace pt = boost::posix_time;
using namespace navitia::autocomplete;
using namespace navitia::georef;

BOOST_AUTO_TEST_CASE(parse_find_with_synonym_and_synonyms_test){
    int nbmax = 10;
    std::vector<std::string> admins;
    std::string admin_uri = "";

    autocomplete_map synonyms;
    synonyms["hotel de ville"]="mairie";
    synonyms["c c"]="centre commercial";
    synonyms["cc"]="centre commercial";
    synonyms["c.h.u"]="hopital";
    synonyms["c.h.r"]="hopital";
    synonyms["ld"]="Lieu-Dit";
    synonyms["de"]="";
    synonyms["la"]="";
    synonyms["les"]="";
    synonyms["des"]="";
    synonyms["d"]="";
    synonyms["l"]="";

    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["cc"]="centre commercial";
    synonyms["chu"]="hopital";
    synonyms["chr"]="hopital";
    synonyms["c.h.u"]="hopital";
    synonyms["c.h.r"]="hopital";
    synonyms["all"]="allée";
    synonyms["allee"]="allée";
    synonyms["ave"]="avenue";
    synonyms["av"]="avenue";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";
    synonyms["b"]="boulevard";
    synonyms["r"]="rue";
    synonyms["pl"]="place";

    Autocomplete<unsigned int> ac;
    ac.add_string("hotel de ville paris", 0, synonyms);
    ac.add_string("r jean jaures", 1, synonyms);
    ac.add_string("rue jeanne d'arc", 2, synonyms);
    ac.add_string("avenue jean jaures", 3, synonyms);
    ac.add_string("boulevard poniatowski", 4, synonyms);
    ac.add_string("pente de Bray", 5, synonyms);
    ac.add_string("Centre Commercial Caluire 2", 6, synonyms);
    ac.add_string("Rue René", 7, synonyms);
    ac.build();

    //Dans le dictionnaire : "hotel de ville paris" -> "mairie paris"
    //Recherche : "mai paris" -> "mai paris"
    // distance = 3 / word_weight = 0*5 = 0
    // Qualité = 100 - (3 + 0) = 97
    auto res = ac.find_complete("mai paris", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res.at(0).quality, 100);

    //Dans le dictionnaire : "hotel de ville paris" -> "mairie paris"
    //Recherche : "hotel de ville par" -> "mairie par"
    // distance = 3 / word_weight = 0*5 = 0
    // Qualité = 100 - (2 + 0) = 98
    auto res1 = ac.find_complete("hotel de ville par", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.at(0).quality, 100);

    //Dans le dictionnaire : "Centre Commercial Caluire 2" -> "centre commercial Caluire 2"
    //Recherche : "c c ca 2" -> "centre commercial Ca"
    // distance = 5 / word_weight = 0*5 = 0
    // Qualité = 100 - (5 + 0) = 95

    auto res2 = ac.find_complete("c c ca 2",synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res2.size(), 1);
    BOOST_REQUIRE_EQUAL(res2.at(0).quality, 100);

    auto res3 = ac.find_complete("cc ca 2", synonyms, nbmax,[](int){return true;});
    BOOST_REQUIRE_EQUAL(res3.size(), 1);
    BOOST_REQUIRE_EQUAL(res3.at(0).quality, 100);

    auto res4 = ac.find_complete("rue rene", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res4.size(), 1);
    BOOST_REQUIRE_EQUAL(res4.at(0).quality, 100);

    auto res5 = ac.find_complete("rue rené", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res5.size(), 1);
    BOOST_REQUIRE_EQUAL(res5.at(0).quality, 100);
}

BOOST_AUTO_TEST_CASE(regex_tests){
    boost::regex re("\\<c c\\>");
    BOOST_CHECK(boost::regex_search("c c", re));
    BOOST_CHECK(boost::regex_search("bonjour c c", re));
    BOOST_CHECK(boost::regex_search("c c au revoir", re));
    BOOST_CHECK(!boost::regex_search("ac c", re));
    BOOST_CHECK(!boost::regex_search("c ca", re));
    BOOST_CHECK(boost::regex_search("c c'a", re));
    BOOST_CHECK(boost::regex_search("a c c a", re));
}

BOOST_AUTO_TEST_CASE(regex_strip_accents_tests){
    std::map<char, char> map_accents;

    Autocomplete<unsigned int> ac;
    std::string test("républiquê");
    BOOST_CHECK_EQUAL(ac.strip_accents(test) ,"republique");
    test = "ÂâÄäçÇÉéÈèÊêËëÖöÔôÜüÎîÏïæœ";
    BOOST_CHECK_EQUAL(ac.strip_accents(test) ,"aaaacceeeeeeeeoooouuiiiiaeoe");
}


BOOST_AUTO_TEST_CASE(regex_replace_tests){
    boost::regex re("\\<c c\\>");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("c c"), re , "centre commercial"), "centre commercial");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("cc c"), re , "centre commercial"), "cc c");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("c cv"), re , "centre commercial"), "c cv");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("bonjour c c"), re , "centre commercial"), "bonjour centre commercial");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("c c revoir"), re , "centre commercial"), "centre commercial revoir");
    BOOST_CHECK_EQUAL( boost::regex_replace(std::string("bonjour c c revoir"), re , "centre commercial"), "bonjour centre commercial revoir");
}

BOOST_AUTO_TEST_CASE(regex_toknize_tests){

    autocomplete_map synonyms;
    synonyms["hotel de ville"]="mairie";
    synonyms["c c"]="centre commercial";
    synonyms["cc"]="centre commercial";
    synonyms["c.h.u"]="hopital";
    synonyms["c.h.r"]="hopital";
    synonyms["ld"]="Lieu-Dit";
    synonyms["de"]="";
    synonyms["la"]="";
    synonyms["les"]="";
    synonyms["des"]="";
    synonyms["d"]="";
    synonyms["l"]="";
    synonyms["st"]="saint";
    synonyms["r"]="rue";

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;

    //synonyme : "cc" = "centre commercial" / synonym : de = ""
    //"cc Carré de Soie" -> "centre commercial carré de soie"
    vec = ac.tokenize("cc Carré de Soie", synonyms);
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("centre") != vec.end());
    BOOST_CHECK(vec.find("commercial") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());

    vec.clear();
    //synonyme : "c c"= "centre commercial" / synonym : de = ""
    //"c c Carré de Soie" -> "centre commercial carré de soie"
    vec = ac.tokenize("c c Carré de Soie", synonyms);
    BOOST_CHECK(vec.find("carre") != vec.end());
    BOOST_CHECK(vec.find("centre") != vec.end());
    BOOST_CHECK(vec.find("commercial") != vec.end());
    BOOST_CHECK(vec.find("soie") != vec.end());
}

BOOST_AUTO_TEST_CASE(regex_address_type_tests){

    autocomplete_map synonyms;
    synonyms["hotel de ville"]="mairie";
    synonyms["c c"]="centre commercial";
    synonyms["cc"]="centre commercial";
    synonyms["c.h.u"]="hopital";
    synonyms["c.h.r"]="hopital";
    synonyms["ld"]="Lieu-Dit";
    synonyms["av"]="avenue";
    synonyms["r"]="rue";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";
    //AddressType = {"rue", "avenue", "place", "boulevard","chemin", "impasse"}

    Autocomplete<unsigned int> ac;
    bool addtype = ac.is_address_type("r", synonyms);
    BOOST_CHECK_EQUAL(addtype, true);
}

BOOST_AUTO_TEST_CASE(regex_synonyme_gare_sncf_tests){

    autocomplete_map synonyms;
    synonyms["gare sncf"]="gare";
    synonyms["gare snc"]="gare";
    synonyms["gare sn"]="gare";
    synonyms["gare s"]="gare";
    synonyms["de"]="";
    synonyms["la"]="";
    synonyms["les"]="";
    synonyms["des"]="";
    synonyms["d"]="";
    synonyms["l"]="";
    synonyms["st"]="saint";
    synonyms["av"]="avenue";
    synonyms["r"]="rue";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";

    Autocomplete<unsigned int> ac;
    std::set<std::string> vec;

    //synonyme : "gare sncf" = "gare"
    //"gare sncf" -> "gare"
    vec = ac.tokenize("gare sncf", synonyms);
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare snc" = "gare"
    //"gare snc" -> "gare"
    vec = ac.tokenize("gare snc", synonyms);
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare sn" = "gare"
    //"gare sn" -> "gare"
    vec = ac.tokenize("gare sn", synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare s" = "gare"
    //"gare s" -> "gare"
    vec = ac.tokenize("gare s", synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare sn nantes" -> "gare nantes"
    vec = ac.tokenize("gare sn nantes", synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK(vec.find("nantes") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare sn  nantes" -> "gare nantes"
    vec = ac.tokenize("gare sn  nantes", synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK(vec.find("nantes") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare s  nantes" -> "gare nantes"
    vec = ac.tokenize("gare  s  nantes", synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK(vec.find("nantes") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare s    nantes" -> "gare nantes"
    vec = ac.tokenize("gare  s    nantes", synonyms);
    BOOST_CHECK(vec.find("gare") != vec.end());
    BOOST_CHECK(vec.find("nantes") != vec.end());
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();
}

BOOST_AUTO_TEST_CASE(parse_find_with_name_in_vector_test){
    autocomplete_map synonyms;
    std::set<std::string> vec;
    std::string admin_uri = "";

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jean jaures", 0, synonyms);
    ac.add_string("place jean jaures", 1, synonyms);
    ac.add_string("rue jeanne d'arc", 2, synonyms);
    ac.add_string("avenue jean jaures", 3, synonyms);
    ac.add_string("boulevard poniatowski", 4, synonyms);
    ac.add_string("pente de Bray", 5, synonyms);
    ac.build();

    vec.insert("rue");
    vec.insert("jean");
    vec.insert("jaures");
    auto res = ac.find(vec);
    std::vector<int> expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("jaures");
    res = ac.find(vec);
    expected = {0, 1, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("avenue");
    res = ac.find(vec);
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("av");
    res = ac.find(vec);
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("r");
    vec.insert("jean");
    res = ac.find(vec);
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("jean");
    vec.insert("r");
    res = ac.find(vec);
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("ponia");
    res = ac.find(vec);
    expected = {4};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.insert("ru");
    vec.insert("je");
    vec.insert("jau");
    res = ac.find(vec);
    expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());    
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

BOOST_AUTO_TEST_CASE(Faute_de_frappe_One){

        autocomplete_map synonyms;
        int word_weight = 5;
        int nbmax = 10;

        Autocomplete<unsigned int> ac;

        ac.add_string("gare Château", 0, synonyms);
        ac.add_string("gare bateau", 1, synonyms);
        ac.add_string("gare de taureau", 2, synonyms);
        ac.add_string("gare tauro", 3, synonyms);
        ac.add_string("gare gateau", 4, synonyms);

        ac.build();

        auto res = ac.find_partial_with_pattern("batau", synonyms,word_weight, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res.size(), 1);
        BOOST_CHECK_EQUAL(res.at(0).idx, 1);
        BOOST_CHECK_EQUAL(res.at(0).quality, 90);

        auto res1 = ac.find_partial_with_pattern("gare patea", synonyms,word_weight, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res1.size(), 3);
        BOOST_CHECK_EQUAL(res1.at(0).idx, 4);
        BOOST_CHECK_EQUAL(res1.at(1).idx, 1);
        BOOST_CHECK_EQUAL(res1.at(2).idx, 0);
        BOOST_CHECK_EQUAL(res1.at(0).quality, 94);


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

    autocomplete_map synonyms;
    std::vector<std::string> admins;
    std::string admin_uri = "";
    int nbmax = 10;

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jeanne d'arc", 0, synonyms);
    ac.add_string("place jean jaures", 1, synonyms);
    ac.add_string("rue jean paul gaultier paris", 2, synonyms);
    ac.add_string("avenue jean jaures", 3, synonyms);
    ac.add_string("boulevard poniatowski", 4, synonyms);
    ac.add_string("pente de Bray", 5, synonyms);
    ac.add_string("rue jean jaures", 6, synonyms);
    ac.add_string("rue jean zay ", 7, synonyms);
    ac.add_string("place jean paul gaultier ", 8, synonyms);

    ac.build();

    auto res = ac.find_complete("rue jean", synonyms, nbmax,[](int){return true;});
    std::vector<int> expected = {6,7,0,2};
    BOOST_REQUIRE_EQUAL(res.size(), 4);
    BOOST_REQUIRE_EQUAL(res.at(0).quality, 100);
    BOOST_REQUIRE_EQUAL(res.at(1).quality, 100);
    BOOST_REQUIRE_EQUAL(res.at(2).quality, 100);
    BOOST_REQUIRE_EQUAL(res.at(3).quality, 100);
    BOOST_REQUIRE_EQUAL(res.at(0).idx, 2);
    BOOST_REQUIRE_EQUAL(res.at(1).idx, 7);
    BOOST_REQUIRE_EQUAL(res.at(2).idx, 0);
    BOOST_REQUIRE_EQUAL(res.at(3).idx, 6);
}

///Test pour verifier que - entres les deux mots est ignoré.
BOOST_AUTO_TEST_CASE(autocomplete_add_string_with_Line){

    autocomplete_map synonyms;
    std::vector<std::string> admins;
    std::string admin_uri = "";
    int nbmax = 10;

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jeanne d'arc", 0, synonyms);
    ac.add_string("place jean jaures", 1, synonyms);
    ac.add_string("avenue jean jaures", 3, synonyms);
    ac.add_string("rue jean-jaures", 6, synonyms);
    ac.add_string("rue jean zay ", 7, synonyms);

    ac.build();

    auto res = ac.find_complete("jean-jau", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res.size(), 3);
    BOOST_REQUIRE_EQUAL(res.at(0).idx, 1);
    BOOST_REQUIRE_EQUAL(res.at(1).idx, 3);
    BOOST_REQUIRE_EQUAL(res.at(2).idx, 6);
}

BOOST_AUTO_TEST_CASE(autocompletesynonym_and_weight_test){

        autocomplete_map synonyms;
        std::vector<std::string> admins;
        std::string admin_uri;
        int nbmax = 10;

        synonyms["de"]="";
        synonyms["la"]="";
        synonyms["les"]="";
        synonyms["des"]="";
        synonyms["d"]="";
        synonyms["l"]="";

        synonyms["st"]="saint";
        synonyms["ste"]="sainte";
        synonyms["cc"]="centre commercial";
        synonyms["chu"]="hopital";
        synonyms["chr"]="hopital";
        synonyms["c.h.u"]="hopital";
        synonyms["c.h.r"]="hopital";
        synonyms["all"]="allée";
        synonyms["allee"]="allée";
        synonyms["ave"]="avenue";
        synonyms["av"]="avenue";
        synonyms["bvd"]="boulevard";
        synonyms["bld"]="boulevard";
        synonyms["bd"]="boulevard";
        synonyms["b"]="boulevard";
        synonyms["r"]="rue";
        synonyms["pl"]="place";

        Autocomplete<unsigned int> ac;
        ac.add_string("rue jeanne d'arc", 0, synonyms);
        ac.add_string("place jean jaures", 1, synonyms);
        ac.add_string("rue jean paul gaultier paris", 2, synonyms);
        ac.add_string("avenue jean jaures", 3, synonyms);
        ac.add_string("boulevard poniatowski", 4, synonyms);
        ac.add_string("pente de Bray", 5, synonyms);
        ac.add_string("rue jean jaures", 6, synonyms);
        ac.add_string("rue jean zay ", 7, synonyms);
        ac.add_string("place jean paul gaultier ", 8, synonyms);
        ac.add_string("hopital paul gaultier", 8, synonyms);

        ac.build();

        auto res = ac.find_complete("rue jean", synonyms, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_REQUIRE_EQUAL(res.at(0).quality, 100);

        auto res1 = ac.find_complete("r jean", synonyms, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res1.size(), 4);

        BOOST_REQUIRE_EQUAL(res1.at(0).quality, 100);

        auto res2 = ac.find_complete("av jean", synonyms, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res2.size(), 1);
        //rue jean zay
        // distance = 6 / word_weight = 1*5 = 5
        // Qualité = 100 - (6 + 5) = 89
        BOOST_REQUIRE_EQUAL(res2.at(0).quality, 100);

        auto res3 = ac.find_complete("av jean", synonyms, nbmax,[](int){return true;});
        BOOST_REQUIRE_EQUAL(res3.size(), 1);
        //rue jean zay
        // distance = 6 / word_weight = 1*10 = 10
        // Qualité = 100 - (6 + 10) = 84
        BOOST_REQUIRE_EQUAL(res3.at(0).quality, 100);

        auto res4 = ac.find_complete("chu gau", synonyms, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res4.size(), 1);
        //hopital paul gaultier
        // distance = 9 / word_weight = 1*10 = 10
        // Qualité = 100 - (9 + 10) = 81
        BOOST_REQUIRE_EQUAL(res4.at(0).quality, 100);
    }

BOOST_AUTO_TEST_CASE(autocomplete_duplicate_words_and_weight_test){

    autocomplete_map synonyms;
    std::vector<std::string> admins;
    std::string admin_uri;
    int nbmax = 10;

    synonyms["de"]="";
    synonyms["la"]="";
    synonyms["le"]="";
    synonyms["les"]="";
    synonyms["des"]="";
    synonyms["d"]="";
    synonyms["l"]="";

    synonyms["st"]="saint";
    synonyms["ste"]="sainte";
    synonyms["cc"]="centre commercial";
    synonyms["chu"]="hopital";
    synonyms["chr"]="hopital";
    synonyms["c.h.u"]="hopital";
    synonyms["c.h.r"]="hopital";
    synonyms["all"]="allée";
    synonyms["allee"]="allée";
    synonyms["ave"]="avenue";
    synonyms["av"]="avenue";
    synonyms["bvd"]="boulevard";
    synonyms["bld"]="boulevard";
    synonyms["bd"]="boulevard";
    synonyms["b"]="boulevard";
    synonyms["r"]="rue";
    synonyms["pl"]="place";

    Autocomplete<unsigned int> ac;
    ac.add_string("gare de Tours Tours", 0, synonyms);
    ac.add_string("Garennes-sur-Eure", 1, synonyms);
    ac.add_string("gare d'Orléans Orléans", 2, synonyms);
    ac.add_string("gare de Bourges Bourges", 3, synonyms);
    ac.add_string("Gare SNCF Blois", 4, synonyms);
    ac.add_string("gare de Dreux Dreux", 5, synonyms);
    ac.add_string("gare de Lucé Lucé", 6, synonyms);
    ac.add_string("gare de Vierzon Vierzon", 7, synonyms);
    ac.add_string("Les Sorinières, Les Sorinières", 8, synonyms);
    ac.add_string("Les Sorinières 44840", 9, synonyms);
    ac.add_string("Rouet, Les Sorinières", 10, synonyms);
    ac.add_string("Ecoles, Les Sorinières", 11, synonyms);
    ac.add_string("Prières, Les Sorinières", 12, synonyms);
    ac.add_string("Calvaire, Les Sorinières", 13, synonyms);
    ac.add_string("Corberie, Les Sorinières", 14, synonyms);
    ac.add_string("Le Pérou, Les Sorinières", 15, synonyms);
    ac.add_string("Les Sorinières, Villebernier", 16, synonyms);
    ac.add_string("Les Faulx, Les Sorinières", 17, synonyms);
    ac.add_string("cimetière, Les Sorinières", 18, synonyms);

    ac.build();

    auto res = ac.find_complete("gare", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res.size(), 8);
    BOOST_REQUIRE_EQUAL(res.at(0).quality, 100);
    BOOST_REQUIRE_EQUAL(res.at(0).idx, 1);
    BOOST_REQUIRE_EQUAL(res.at(1).idx, 5);
    BOOST_REQUIRE_EQUAL(res.at(2).idx, 2);
    BOOST_REQUIRE_EQUAL(res.at(3).idx, 4);
    BOOST_REQUIRE_EQUAL(res.at(4).idx, 6);
    BOOST_REQUIRE_EQUAL(res.at(5).idx, 0);
    BOOST_REQUIRE_EQUAL(res.at(6).idx, 3);
    BOOST_REQUIRE_EQUAL(res.at(7).idx, 7);
    BOOST_REQUIRE_EQUAL(res.at(7).quality, 100);


    auto res1 = ac.find_complete("gare tours", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.at(0).quality, 100);

    auto res2 = ac.find_complete("gare tours tours", synonyms, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.at(0).quality, 100);

    auto res3 = ac.find_complete("les Sorinières", synonyms, nbmax,[](int){return true;});
    BOOST_REQUIRE_EQUAL(res3.size(), 10);
    BOOST_REQUIRE_EQUAL(res3.at(0).quality, 100);    
}

/*
1. We have 1 administrative_region and 11  stop_area
2. All the stop_areas are attached to the same administrative_region.
3. Call with "quimer" and count = 10
4. In the result the administrative_region is the first one
5. All the stop_areas with same quality are sorted by name (case insensitive).
6. There 10 elements in the result.
7. We don't have penalty on "admin weight" and "stoparea weight".
*/
BOOST_AUTO_TEST_CASE(autocomplete_functional_test_admin_and_SA_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("IUT", 0, 0);
    b.sa("Gare", 0, 0);
    b.sa("Resistance", 0, 0);
    b.sa("Becharles", 0, 0);
    b.sa("Yoyo", 0, 0);
    b.sa("Luther King", 0, 0);
    b.sa("Napoleon III", 0, 0);
    b.sa("MPT kerfeunteun", 0, 0);
    b.sa("Marcel Paul", 0, 0);
    b.sa("chaptal", 0, 0);
    b.sa("Zebre", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->post_code = "29000";
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 10, admins, 0, *(b.data));

    BOOST_REQUIRE_EQUAL(resp.places_size(), 10);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 100);
    BOOST_REQUIRE_EQUAL(resp.places(1).quality(), 100);
    BOOST_REQUIRE_EQUAL(resp.places(7).quality(), 100);
    BOOST_REQUIRE_EQUAL(resp.places(8).quality(), 100);
    BOOST_REQUIRE_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_REQUIRE_EQUAL(resp.places(1).uri(), "Becharles");
    BOOST_REQUIRE_EQUAL(resp.places(7).uri(), "MPT kerfeunteun");
    BOOST_REQUIRE_EQUAL(resp.places(8).uri(), "Napoleon III");
    BOOST_REQUIRE_EQUAL(resp.places(9).uri(), "Resistance");

    resp = navitia::autocomplete::autocomplete("qui", type_filter , 1, 10, admins, 0, *(b.data));
    BOOST_REQUIRE_EQUAL(resp.places_size(), 10);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_REQUIRE_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_REQUIRE_EQUAL(resp.places(1).uri(), "Becharles");
    BOOST_REQUIRE_EQUAL(resp.places(9).uri(), "Resistance");
}

/*
1. We have 1 administrative_region and 9  stop_area
2. All the stop_areas are attached to the same administrative_region.
3. Call with "quimer" and count = 5
4. In the result the administrative_region is absent
5. We don't have penalty on "admin weight" and "stoparea weight".
*/
BOOST_AUTO_TEST_CASE(autocomplete_functional_test_SA_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("IUT", 0, 0);
    b.sa("Gare SNCF", 0, 0);
    b.sa("Resistance", 0, 0);
    b.sa("Becharles", 0, 0);
    b.sa("Luther King", 0, 0);
    b.sa("Napoleon III", 0, 0);
    b.sa("MPT kerfeunteun", 0, 0);
    b.sa("Marcel Paul", 0, 0);
    b.sa("chaptal", 0, 0);
    b.sa("Tourbie", 0, 0);
    b.sa("Bourgogne", 0, 0);

    b.data->pt_data->index();
    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->post_code = "29000";
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 5, admins, 0, *(b.data));

    BOOST_REQUIRE_EQUAL(resp.places_size(), 5);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 100);
    BOOST_REQUIRE_EQUAL(resp.places(1).quality(), 100);
    BOOST_REQUIRE_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_REQUIRE_EQUAL(resp.places(4).uri(), "Gare SNCF");
}

/*
1. We have 1 administrative_region ,6 stop_area and 3 way
2. All these objects are attached to the same administrative_region.
3. Call with "quimer" and count = 10
4. In the result the administrative_region is the first one
5. All the stop_areas with same quality are sorted by name (case insensitive).
6. The addresses have different quality values due to diferent number of words.
7. There 10 elements in the result.
8. We don't have penalty on "admin weight" and "stoparea weight".
*/
BOOST_AUTO_TEST_CASE(autocomplete_functional_test_admin_SA_and_Address_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("20140614");
    b.sa("IUT", 0, 0);
    b.sa("Gare", 0, 0);
    b.sa("Becharles", 0, 0);
    b.sa("Luther King", 0, 0);
    b.sa("Napoleon III", 0, 0);
    b.sa("MPT kerfeunteun", 0, 0);
    b.data->pt_data->index();
    Way w;
    w.idx = 0;
    w.name = "rue DU TREGOR";
    w.uri = w.name;
    b.data->geo_ref->add_way(w);
    w.name = "rue VIS";
    w.uri = w.name;
    w.idx = 1;
    b.data->geo_ref->add_way(w);
    w.idx = 2;
    w.name = "quai NEUF";
    w.uri = w.name;
    b.data->geo_ref->add_way(w);

    Admin* ad = new Admin;
    ad->name = "Quimper";
    ad->uri = "Quimper";
    ad->level = 8;
    ad->post_code = "29000";
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();
    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::StopArea);
    type_filter.push_back(navitia::type::Type_e::Admin);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 10, admins, 0, *(b.data));
    //Here we want only Admin and StopArea
    BOOST_REQUIRE_EQUAL(resp.places_size(), 7);
    type_filter.push_back(navitia::type::Type_e::Address);
    resp = navitia::autocomplete::autocomplete("quimper", type_filter , 1, 10, admins, 0, *(b.data));

    BOOST_REQUIRE_EQUAL(resp.places_size(), 10);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type() , pbnavitia::ADMINISTRATIVE_REGION);
    BOOST_REQUIRE_EQUAL(resp.places(0).quality(), 100);
    BOOST_REQUIRE_EQUAL(resp.places(1).quality(), 100);

    BOOST_REQUIRE_EQUAL(resp.places(0).uri(), "Quimper");
    BOOST_REQUIRE_EQUAL(resp.places(1).uri(), "Becharles");
    BOOST_REQUIRE_EQUAL(resp.places(7).uri(), "quai NEUF");
    BOOST_REQUIRE_EQUAL(resp.places(8).uri(), "rue DU TREGOR");
    BOOST_REQUIRE_EQUAL(resp.places(9).uri(), "rue VIS");
}

/*
1. We have 1 Network (name="base_network"), 2 mode (name="Tram" et name="Metro")
2. 1 line for mode "Tram" and and 2 routes
*/
BOOST_AUTO_TEST_CASE(autocomplete_pt_object_Network_Mode_Line_Route_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("201409011T1739");
    b.generate_dummy_basis();

    //Create a new line and affect it to mode "Tram"
    navitia::type::Line* line = new navitia::type::Line();
    line->idx = b.data->pt_data->lines.size();
    line-> uri = "line 1";
    line-> name = "line 1";
    line->commercial_mode = b.data->pt_data->commercial_modes[0];
    b.data->pt_data->lines.push_back(line);

    //Add two routes in the line
    navitia::type::Route* route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::Network);
    type_filter.push_back(navitia::type::Type_e::CommercialMode);
    type_filter.push_back(navitia::type::Type_e::Line);
    type_filter.push_back(navitia::type::Type_e::Route);
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("base", type_filter , 1, 10, admins, 0, *(b.data));
    //The result contains only network
    BOOST_REQUIRE_EQUAL(resp.places_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type(), pbnavitia::NETWORK);

    // Call with "Tram" and &type[]=network&type[]=mode&type[]=line
    resp = navitia::autocomplete::autocomplete("Tram", type_filter , 1, 10, admins, 0, *(b.data));
    //In the result the first line is Mode and the second is line
    BOOST_REQUIRE_EQUAL(resp.places_size(), 4);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type(), pbnavitia::COMMERCIAL_MODE);
    BOOST_REQUIRE_EQUAL(resp.places(1).embedded_type(), pbnavitia::LINE);
    BOOST_REQUIRE_EQUAL(resp.places(2).embedded_type(), pbnavitia::ROUTE);
    BOOST_REQUIRE_EQUAL(resp.places(3).embedded_type(), pbnavitia::ROUTE);

    // Call with "line" and &type[]=network&type[]=mode&type[]=line&type[]=route
    resp = navitia::autocomplete::autocomplete("line", type_filter , 1, 10, admins, 0, *(b.data));
    //In the result the first line is line and the others are the routes
    BOOST_REQUIRE_EQUAL(resp.places_size(), 3);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type(), pbnavitia::LINE);
    BOOST_REQUIRE_EQUAL(resp.places(1).embedded_type(), pbnavitia::ROUTE);
    BOOST_REQUIRE_EQUAL(resp.places(2).embedded_type(), pbnavitia::ROUTE);
}

/*
1. We have 1 Network (name="base_network"), 2 mode (name="Tram" et name="Metro")
2. 1 line for mode Metro and and 2 routes
3. 4 stop_areas
*/
BOOST_AUTO_TEST_CASE(autocomplete_pt_object_Network_Mode_Line_Route_stop_area_test) {
    std::vector<std::string> admins;
    std::vector<navitia::type::Type_e> type_filter;
    ed::builder b("201409011T1739");
    b.generate_dummy_basis();

    //Create a new line and affect it to mode "Metro"
    navitia::type::Line* line = new navitia::type::Line();
    line->idx = b.data->pt_data->lines.size();
    line-> uri = "line 1";
    line-> name = "Chatelet - Vincennes";
    line->commercial_mode = b.data->pt_data->commercial_modes[1];
    b.data->pt_data->lines.push_back(line);

    //Add two routes in the line
    navitia::type::Route* route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    route = new navitia::type::Route();
    route->idx = b.data->pt_data->routes.size();
    route->name = line->name;
    route->uri = line->name + ":" + std::to_string(b.data->pt_data->routes.size());
    b.data->pt_data->routes.push_back(route);
    line->route_list.push_back(route);
    route->line = line;

    b.sa("base", 0, 0);
    b.sa("chatelet", 0, 0);
    b.sa("Chateau", 0, 0);
    b.sa("Mettray", 0, 0);
    Admin* ad = new Admin;
    ad->name = "paris";
    ad->uri = "paris";
    ad->level = 8;
    ad->post_code = "75000";
    ad->idx = 0;
    b.data->geo_ref->admins.push_back(ad);
    b.manage_admin();

    b.build_autocomplete();

    type_filter.push_back(navitia::type::Type_e::Network);
    type_filter.push_back(navitia::type::Type_e::CommercialMode);
    type_filter.push_back(navitia::type::Type_e::Line);
    type_filter.push_back(navitia::type::Type_e::Route);
    type_filter.push_back(navitia::type::Type_e::StopArea);
    //Call with q=base
    pbnavitia::Response resp = navitia::autocomplete::autocomplete("base", type_filter , 1, 10, admins, 0, *(b.data));
    //The result contains network and stop_area
    BOOST_REQUIRE_EQUAL(resp.places_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type(), pbnavitia::NETWORK);
    BOOST_REQUIRE_EQUAL(resp.places(1).embedded_type(), pbnavitia::STOP_AREA);

    //Call with q=met
    resp = navitia::autocomplete::autocomplete("met", type_filter , 1, 10, admins, 0, *(b.data));
    //The result contains mode, stop_area, line
    BOOST_REQUIRE_EQUAL(resp.places_size(), 5);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type(), pbnavitia::COMMERCIAL_MODE);
    BOOST_REQUIRE_EQUAL(resp.places(1).embedded_type(), pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(2).embedded_type(), pbnavitia::LINE);
    BOOST_REQUIRE_EQUAL(resp.places(3).embedded_type(), pbnavitia::ROUTE);
    BOOST_REQUIRE_EQUAL(resp.places(4).embedded_type(), pbnavitia::ROUTE);

    //Call with q=chat
    resp = navitia::autocomplete::autocomplete("chat", type_filter , 1, 10, admins, 0, *(b.data));
    //The result contains 2 stop_areas, one line and 2 routes
    BOOST_REQUIRE_EQUAL(resp.places_size(), 5);
    BOOST_REQUIRE_EQUAL(resp.places(0).embedded_type(), pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(1).embedded_type(), pbnavitia::STOP_AREA);
    BOOST_REQUIRE_EQUAL(resp.places(2).embedded_type(), pbnavitia::LINE);
    BOOST_REQUIRE_EQUAL(resp.places(3).embedded_type(), pbnavitia::ROUTE);
    BOOST_REQUIRE_EQUAL(resp.places(4).embedded_type(), pbnavitia::ROUTE);

}
