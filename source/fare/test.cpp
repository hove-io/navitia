#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_fares
#include <boost/test/unit_test.hpp>
#include "fare.h"
BOOST_AUTO_TEST_CASE(parse_state_test){
    State state;

    // * correspond au state vide, toujours vrai
    BOOST_CHECK(parse_state("*") == state);

    // on n'est pas case sensitive
    BOOST_CHECK(parse_state("mode=Metro").mode == "metro");

    BOOST_CHECK(parse_state("zone=1").zone == "1");

    // on ignore les espaces
    BOOST_CHECK(parse_state(" mode = Metro  ").mode == "metro");
    BOOST_CHECK(parse_state("line=L1").line == "L1");
    BOOST_CHECK(parse_state("stop_area=chatelet").stop_area == "chatelet");

    // Qu'est-ce qui se passe avec des boulets ?
    BOOST_CHECK(parse_state("mode=Metro=foo").mode == "metro=foo");

    // On ne respecte pas la grammaire => exception
    //BOOST_CHECK(parse_state("coucou=møø"));
    //BOOST_CHECK(parse_state("hello world!"));

}

BOOST_AUTO_TEST_CASE(parse_condition_test){
     BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(parse_section_key_test){
     BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(graph_construction_test){
     BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(transition_validation_test){
     BOOST_CHECK(false);
}
