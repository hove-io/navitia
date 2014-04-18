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
#include <boost/regex.hpp>

namespace pt = boost::posix_time;
using namespace navitia::autocomplete;

BOOST_AUTO_TEST_CASE(parse_find_with_synonym_and_synonyms_test){
    int word_weight = 5;
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
    auto res = ac.find_complete("mai paris", synonyms,word_weight, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res.size(), 1);
    BOOST_REQUIRE_EQUAL(res.at(0).quality, 97);

    //Dans le dictionnaire : "hotel de ville paris" -> "mairie paris"
    //Recherche : "hotel de ville par" -> "mairie par"
    // distance = 3 / word_weight = 0*5 = 0
    // Qualité = 100 - (2 + 0) = 98
    auto res1 = ac.find_complete("hotel de ville par", synonyms,word_weight, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
    BOOST_REQUIRE_EQUAL(res1.at(0).quality, 98);

    //Dans le dictionnaire : "Centre Commercial Caluire 2" -> "centre commercial Caluire 2"
    //Recherche : "c c ca 2" -> "centre commercial Ca"
    // distance = 5 / word_weight = 0*5 = 0
    // Qualité = 100 - (5 + 0) = 95

    auto res2 = ac.find_complete("c c ca 2",synonyms,word_weight, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res2.size(), 1);
    BOOST_REQUIRE_EQUAL(res2.at(0).quality, 95);

    auto res3 = ac.find_complete("cc ca 2", synonyms,word_weight, nbmax,[](int){return true;});
    BOOST_REQUIRE_EQUAL(res3.size(), 1);
    BOOST_REQUIRE_EQUAL(res3.at(0).quality, 95);

    auto res4 = ac.find_complete("rue rene", synonyms,word_weight, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res4.size(), 1);
    BOOST_REQUIRE_EQUAL(res4.at(0).quality, 100);

    auto res5 = ac.find_complete("rue rené", synonyms,word_weight, nbmax, [](int){return true;});
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
    std::vector<std::string> vec;

    //synonyme : "cc" = "centre commercial" / synonym : de = ""
    //"cc Carré de Soie" -> "centre commercial carré de soie"
    vec = ac.tokenize("cc Carré de Soie", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "centre");
    BOOST_CHECK_EQUAL(vec[1], "commercial");
    BOOST_CHECK_EQUAL(vec[2], "carre");
    BOOST_CHECK_EQUAL(vec[3], "soie");

    vec.clear();
    //synonyme : "c c"= "centre commercial" / synonym : de = ""
    //"c c Carré de Soie" -> "centre commercial carré de soie"
    vec = ac.tokenize("c c Carré de Soie", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "centre");
    BOOST_CHECK_EQUAL(vec[1], "commercial");
    BOOST_CHECK_EQUAL(vec[2], "carre");
    BOOST_CHECK_EQUAL(vec[3], "soie");
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
    std::vector<std::string> vec;

    //synonyme : "gare sncf" = "gare"
    //"gare sncf" -> "gare"
    vec = ac.tokenize("gare sncf", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare snc" = "gare"
    //"gare snc" -> "gare"
    vec = ac.tokenize("gare snc", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare sn" = "gare"
    //"gare sn" -> "gare"
    vec = ac.tokenize("gare sn", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare s" = "gare"
    //"gare s" -> "gare"
    vec = ac.tokenize("gare s", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec.size(),1);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare sn nantes" -> "gare nantes"
    vec = ac.tokenize("gare sn nantes", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec[1], "nantes");
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare sn  nantes" -> "gare nantes"
    vec = ac.tokenize("gare sn  nantes", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec[1], "nantes");
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare s  nantes" -> "gare nantes"
    vec = ac.tokenize("gare  s  nantes", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec[1], "nantes");
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();

    //synonyme : "gare sn nantes" = "gare nantes"
    //"gare s    nantes" -> "gare nantes"
    vec = ac.tokenize("gare  s    nantes", synonyms);
    BOOST_CHECK_EQUAL(vec[0], "gare");
    BOOST_CHECK_EQUAL(vec[1], "nantes");
    BOOST_CHECK_EQUAL(vec.size(),2);
    vec.clear();
}

BOOST_AUTO_TEST_CASE(parse_find_with_name_in_vector_test){
    autocomplete_map synonyms;
    std::vector<std::string> vec;
    std::string admin_uri = "";

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jean jaures", 0, synonyms);
    ac.add_string("place jean jaures", 1, synonyms);
    ac.add_string("rue jeanne d'arc", 2, synonyms);
    ac.add_string("avenue jean jaures", 3, synonyms);
    ac.add_string("boulevard poniatowski", 4, synonyms);
    ac.add_string("pente de Bray", 5, synonyms);
    ac.build();

    vec.push_back("rue");
    vec.push_back("jean");
    vec.push_back("jaures");
    auto res = ac.find(vec);
    std::vector<int> expected = {0};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.push_back("jaures");
    res = ac.find(vec);
    expected = {0, 1, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.push_back("avenue");
    res = ac.find(vec);
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.push_back("av");
    res = ac.find(vec);
    expected = {3};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.push_back("r");
    vec.push_back("jean");
    res = ac.find(vec);
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.push_back("jean");
    vec.push_back("r");
    res = ac.find(vec);
    expected = {0, 2};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.push_back("ponia");
    res = ac.find(vec);
    expected = {4};
    BOOST_CHECK_EQUAL_COLLECTIONS(res.begin(), res.end(), expected.begin(), expected.end());

    vec.clear();
    vec.push_back("ru");
    vec.push_back("je");
    vec.push_back("jau");
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
    int word_weight = 5;
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

    auto res = ac.find_complete("rue jean", synonyms, word_weight, nbmax,[](int){return true;});
    std::vector<int> expected = {6,7,0,2};
    BOOST_REQUIRE_EQUAL(res.size(), 4);
    BOOST_REQUIRE_EQUAL(res.at(0).quality, 92);
    BOOST_REQUIRE_EQUAL(res.at(1).quality, 89);
    BOOST_REQUIRE_EQUAL(res.at(2).quality, 84);
    BOOST_REQUIRE_EQUAL(res.at(3).quality, 68);
    BOOST_REQUIRE_EQUAL(res.at(0).idx, 7);
    BOOST_REQUIRE_EQUAL(res.at(1).idx, 6);
    BOOST_REQUIRE_EQUAL(res.at(2).idx, 0);
    BOOST_REQUIRE_EQUAL(res.at(3).idx, 2);
}

///Test pour verifier que - entres les deux mots est ignoré.
BOOST_AUTO_TEST_CASE(autocomplete_add_string_with_Line){

    autocomplete_map synonyms;
    std::vector<std::string> admins;
    std::string admin_uri = "";
    int word_weight = 5;
    int nbmax = 10;

    Autocomplete<unsigned int> ac;
    ac.add_string("rue jeanne d'arc", 0, synonyms);
    ac.add_string("place jean jaures", 1, synonyms);
    ac.add_string("avenue jean jaures", 3, synonyms);
    ac.add_string("rue jean-jaures", 6, synonyms);
    ac.add_string("rue jean zay ", 7, synonyms);

    ac.build();

    auto res = ac.find_complete("jean-jau", synonyms, word_weight, nbmax, [](int){return true;});
    BOOST_REQUIRE_EQUAL(res.size(), 3);
    BOOST_REQUIRE_EQUAL(res.at(0).idx, 6);
    BOOST_REQUIRE_EQUAL(res.at(1).idx, 1);
    BOOST_REQUIRE_EQUAL(res.at(2).idx, 3);
}

BOOST_AUTO_TEST_CASE(autocompletesynonym_and_weight_test){

        autocomplete_map synonyms;
        std::vector<std::string> admins;
        std::string admin_uri;
        int word_weight = 5;
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

        auto res = ac.find_complete("rue jean", synonyms, word_weight, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res.size(), 4);
        BOOST_REQUIRE_EQUAL(res.at(0).quality, 92);

        auto res1 = ac.find_complete("r jean", synonyms, word_weight, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res1.size(), 4);

        BOOST_REQUIRE_EQUAL(res1.at(0).quality, 92);

        auto res2 = ac.find_complete("av jean", synonyms, word_weight, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res2.size(), 1);
        //rue jean zay
        // distance = 6 / word_weight = 1*5 = 5
        // Qualité = 100 - (6 + 5) = 89
        BOOST_REQUIRE_EQUAL(res2.at(0).quality, 89);

        word_weight = 10;
        auto res3 = ac.find_complete("av jean", synonyms, word_weight, nbmax,[](int){return true;});
        BOOST_REQUIRE_EQUAL(res3.size(), 1);
        //rue jean zay
        // distance = 6 / word_weight = 1*10 = 10
        // Qualité = 100 - (6 + 10) = 84
        BOOST_REQUIRE_EQUAL(res3.at(0).quality, 84);

        word_weight = 10;
        auto res4 = ac.find_complete("chu gau", synonyms, word_weight, nbmax, [](int){return true;});
        BOOST_REQUIRE_EQUAL(res4.size(), 1);
        //hopital paul gaultier
        // distance = 9 / word_weight = 1*10 = 10
        // Qualité = 100 - (9 + 10) = 81
        BOOST_REQUIRE_EQUAL(res4.at(0).quality, 81);
    }
