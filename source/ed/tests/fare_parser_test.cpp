#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_fare
#include <boost/test/unit_test.hpp>
#include "ed/connectors/fare_parser.h"
#include "ed/connectors/fare_utils.h"
#include "utils/logger.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

using namespace navitia::fare;
using namespace ed::connectors;

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
    //parse_state("stop_area=chatelet").stop_area;
    BOOST_CHECK(parse_state("stoparea=chatelet").stop_area == "chatelet");

    // Qu'est-ce qui se passe avec des boulets ?
    BOOST_CHECK_THROW(parse_state("mode=Metro=foo"), std::exception);

    // On ne respecte pas la grammaire => exception
    BOOST_CHECK_THROW(parse_state("coucou=moo"), invalid_key);


    // On essaye de parser des choses plus compliquées
    State state2 = parse_state("mode=metro&stoparea=chatelet");
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
    BOOST_CHECK(cond.comparaison == Comp_e::EQ);

    BOOST_CHECK(parse_condition("coucou <= moo").comparaison == Comp_e::LTE);
    BOOST_CHECK(parse_condition("coucou >= moo").comparaison == Comp_e::GTE);
    BOOST_CHECK(parse_condition("coucou != moo").comparaison == Comp_e::NEQ);
    BOOST_CHECK(parse_condition("coucou < moo").comparaison == Comp_e::LT);
    BOOST_CHECK(parse_condition("coucou > moo").comparaison == Comp_e::GT);

    BOOST_CHECK(parse_conditions("coucoun<= bli& foo =azw &abc>=123").size() == 3);
}
