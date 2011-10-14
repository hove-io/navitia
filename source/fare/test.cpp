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
    BOOST_CHECK(cond.comparaison == EQ);

    BOOST_CHECK(parse_condition("coucou <= moo").comparaison == LTE);
    BOOST_CHECK(parse_condition("coucou >= moo").comparaison == GTE);
    BOOST_CHECK(parse_condition("coucou != moo").comparaison == NEQ);
    BOOST_CHECK(parse_condition("coucou < moo").comparaison == LT);
    BOOST_CHECK(parse_condition("coucou > moo").comparaison == GT);

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
    Fare f;
    f.init("/home/tristram/fare/idf.fares", "/home/tristram/fare/prix.csv");
    f.load_od_stif("/home/tristram/fare/tarifs_od.csv");
    std::vector<Ticket> res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value , 170);

    // Correspondance métro sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|20;02|30;1;1;metro");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);

    // Correspondance RER sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|35;02|40;1;1;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);

    // Correspondance BUS, donc nouveau billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|35;02|40;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 2);

    // Correspondance Tramwayway-bus autant qu'on veut
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|50;03|30;1;1;Tramway");
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;03|30;04|20;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 2);
    // On a dépassé les 90 minutes de validité du ticket t+, il faut en racheter un
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 3);

    // On teste un peu les OD vers paris
    keys.clear();
    keys.push_back("ratp;8711388;8775890;FILGATO-2;2011|06|01;04|40;04|50;4;1;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,370);

    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|06|01;04|40;04|50;1;1;metro");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,370);

    // Et le Tramway bien évidemment payant !
    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|06|01;04|40;04|50;1;1;Tramway");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 2);

    // On teste le tarif à une autre date
    keys.clear();
    keys.push_back("ratp;mantes;FILNav31;FILGATO-2;2010|12|01;04|40;04|50;4;1;metro");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.at(0).value,160);

    // On teste le noctilien
    keys.clear();
    keys.push_back("Noctilien;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;1;1;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res[0].value , 170);

    keys.clear();
    keys.push_back("Noctilien;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;1;3;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res[0].value , 340);  // C'est un trajet qui coûte 2 tickets


    keys.clear();
    keys.push_back("Noctilien;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;3;1;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res[0].value , 340);  // C'est un trajet qui coûte 2 tickets
    // Prendre le bus après le noctilien coûte
    keys.push_back("ratp;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|30;04|40;1;3;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 2);

    // On rajoute un bus après : il faut reprendre un billet
    keys.push_back("ratp;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;04|40;04|50;1;3;bus");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 2);

    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775499;2010|12|01;04|40;04|50;4;1;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,215);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 1);

    // Cas avec deux RER
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2010|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2010|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,215);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 2);

    // Cas avec un RER, un changement en métro
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2010|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2010|12|01;04|40;04|50;1;1;metro");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,295);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 2);


    // Cas avec deux RER, un changement en métro au milieu
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2010|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2010|12|01;04|40;04|50;1;1;metro");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2010|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,215);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 3);


    // Cas avec un RER, un changement en bus => faut payer
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2010|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;bus");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2010|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 3);
    BOOST_CHECK_EQUAL(res.at(0).value,295);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 1);
    BOOST_CHECK_EQUAL(res.at(1).value,170);
    BOOST_CHECK_EQUAL(res.at(1).sections.size() , 1);
    BOOST_CHECK_EQUAL(res.at(2).value,655);
    BOOST_CHECK_EQUAL(res.at(2).sections.size() , 1);

    keys.clear();
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,655);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 2);

    keys.clear();
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;blibal;2011|12|01;04|40;04|50;1;3;rapidtransit");
    keys.push_back("ratp;bliabal;FILGATO-2;8775499;2011|12|01;04|40;04|50;3;5;rapidtransit");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).value,655);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 3);

    keys.clear();
    keys.push_back("5604:127;11:120;050050023:23;8727622;2011|05|31;09|28;09|39;4;4;Bus");
    keys.push_back(";8727622;800:D;8775860;2011|05|31;09|47;10|09;4;1;RapidTransit");
    keys.push_back(";8775860;100110007:7;R_0007;2011|05|31;10|20;10|21;1;1;Metro");
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size() , 2);
    BOOST_CHECK_EQUAL(res.at(0).value,170);
    BOOST_CHECK_EQUAL(res.at(1).value , 370);
    BOOST_CHECK_EQUAL(res.at(0).sections.size() , 1);
    BOOST_CHECK_EQUAL(res.at(0).sections.at(0).section , keys.at(0));
    BOOST_CHECK_EQUAL(res.at(1).sections.size(), 2);
    BOOST_CHECK_EQUAL(res.at(1).sections.at(0).section, keys.at(1));
    BOOST_CHECK_EQUAL(res.at(1).sections.at(1).section, keys.at(2));

    // On prend le RER intramuros
    keys.clear();
    keys.push_back(";8727141;RER B;8770870;2011|05|31;09|28;09|39;1;1;RapidTransit"); // Aulnay -> CDG "intramuros"
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res.at(0).value, 170);

    // 13/10/2011 : youpppiiii on peut prendre "parfois" le tramway avec un ticket O/D
    keys.clear();
    keys.push_back(";8711388;T4;8727141;2011|05|31;09|28;09|39;4;4;Tramway"); // L'abbaye -> Aulnay
    keys.push_back(";8727141;RER B;8770870;2011|05|31;09|28;09|39;4;4;RapidTransit"); // Aulnay -> CDG
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size(), 1);
    BOOST_CHECK_EQUAL(res.at(0).value, 280);

    keys.clear();
    keys.push_back(";bled_paumé;bus_magique;8711388;2011|05|31;09|28;09|39;4;4;Bus"); // Bled Paumé -> L'Abbaye
    keys.push_back(";8711388;T4;8727141;2011|05|31;09|28;09|39;4;4;Tramway"); // L'abbaye -> Aulnay
    keys.push_back(";8727141;RER B;8770870;2011|05|31;09|28;09|39;4;4;RapidTransit"); // Aulnay -> CDG
    res = f.compute(keys);
    BOOST_CHECK_EQUAL(res.size(), 2);
    BOOST_CHECK_EQUAL(res.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.at(1).value, 215);

}


