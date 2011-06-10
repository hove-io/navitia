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

BOOST_AUTO_TEST_CASE(time_parse){
     BOOST_CHECK(parse_time("12|30") == 12*3600 + 30*60);
     BOOST_CHECK(parse_time("01|03") == 1*3600 + 3*60);
}

BOOST_AUTO_TEST_CASE(test_computation) {
    std::vector<std::string> keys;

    // Un trajet simple
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|06;02|10;1;1;metro");
    Fare f("/home/tristram/idf.fares", "/home/tristram/prix.csv");
    std::vector<ticket_t> res = f.compute(keys);
    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(res[0].second == 170);

    // Correspondance métro sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|20;02|30;1;1;metro");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 1);

    // Correspondance RER sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|35;02|40;1;1;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 1);

    // Correspondance BUS, donc nouveau billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|35;02|40;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 2);

    // Correspondance tramway-bus autant qu'on veut
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|50;03|30;1;1;tram");
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;03|30;04|20;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 2);
    // On a dépassé les 90 minutes de validité du ticket t+, il faut en racheter un
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 3);

    // On teste le noctilien
    keys.clear();
    keys.push_back("Noctilien;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(res[0].second == 170);

    keys.clear();
    keys.push_back("noctilien;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;1;3;bus");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(res[0].second == 340); // C'est un trajet qui coûte 2 tickets

    // On rajoute un bus après : il faut reprendre un billet
    keys.push_back("ratp;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|40;04|50;1;3;bus");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 2);

    // On teste un peu les OD
    keys.clear();
    keys.push_back("ratp;mantes;FILNav31;FILGATO-2;2011|06|01;04|40;04|50;4;1;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(res[0].second=450);

    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|06|01;04|40;04|50;1;1;metro");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 1);
    BOOST_CHECK(res[0].second=450);

    // Et le tram bien évidemment payant !
    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|06|01;04|40;04|50;1;1;tram");
    res = f.compute(keys);
    BOOST_CHECK(res.size() == 2);

    // On teste le tarif à une autre date
    keys.clear();
    keys.push_back("ratp;mantes;FILNav31;FILGATO-2;2010|12|01;04|40;04|50;4;1;metro");
    res = f.compute(keys);
    BOOST_CHECK(res[0].second=160);
}


