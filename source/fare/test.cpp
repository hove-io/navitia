#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_fares
#include <boost/test/unit_test.hpp>
#include "fare.h"
BOOST_AUTO_TEST_CASE(parse_state_test){
    State state;

    // * correspond au state vide, toujours vrai
    BOOST_CHECK(parse_state("*") == state);
    BOOST_CHECK(parse_state("") == state);

    // on n'est pas case sensitive
    BOOST_CHECK(parse_state("mode=Metro").mode == "metro");

    BOOST_CHECK(parse_state("zone=1").zone == "1");

    // on ignore les espaces
    BOOST_CHECK(parse_state(" mode = Metro  ").mode == "metro");
    BOOST_CHECK(parse_state("line=L1").line == "l1");
    BOOST_CHECK(parse_state("stop_area=chatelet").stop_area == "chatelet");

    // Qu'est-ce qui se passe avec des boulets ?
    BOOST_CHECK_THROW(parse_state("mode=Metro=foo"), invalid_condition);

    // On ne respecte pas la grammaire => exception
    BOOST_CHECK_THROW(parse_state("coucou=moo"), invalid_key);


    // On essaye de parser des choses plus compliquées
    State state2 = parse_state("mode=metro&stop_area=chatelet");
    BOOST_CHECK(state2.mode == "metro");
    BOOST_CHECK(state2.stop_area == "chatelet");

    // Si un attribut est en double
    BOOST_CHECK_THROW(parse_state("mode=foo&mode=bar"), invalid_key);

    // On ne veut rien d'autre que de l'égalité
    BOOST_CHECK_THROW(parse_state("mode < bli"), invalid_key);
}


BOOST_AUTO_TEST_CASE(parse_condition_test){
    BOOST_CHECK_THROW(parse_condition("moo"), invalid_condition);

    Condition cond = parse_condition(" key = value ");
    BOOST_CHECK(cond.value == "value");
    BOOST_CHECK(cond.key == "key");
    BOOST_CHECK(cond.comparaison == EQ);

    BOOST_CHECK(parse_condition("coucou <= moo").comparaison == LTE);
    BOOST_CHECK(parse_condition("coucou >= moo").comparaison == GTE);
    BOOST_CHECK(parse_condition("coucou != moo").comparaison == NEQ);
    BOOST_CHECK(parse_condition("coucou < moo").comparaison == LT);
    BOOST_CHECK(parse_condition("coucou > moo").comparaison = GT);

    BOOST_CHECK(parse_conditions("coucoun<= bli& foo =azw &abc>=123").size() == 3);
}

BOOST_AUTO_TEST_CASE(parse_file){
     BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE(time_parse){
     BOOST_CHECK(parse_time("12|30") == 12*3600 + 30*60);
     BOOST_CHECK(parse_time("01|03") == 1*3600 + 3*60);
}

BOOST_AUTO_TEST_CASE(transition_validation_test){
     BOOST_CHECK(false);
}
