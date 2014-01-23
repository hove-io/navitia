#ifndef WIN32
#define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE test_fares
#include <boost/test/unit_test.hpp>
#include "fare/fare.h"
#include "ed/connectors/fare_parser.h"

using namespace navitia::fare;
BOOST_AUTO_TEST_CASE(test_computation) {
    std::vector<std::string> keys;

    ed::connectors::fare_parser parser(std::string(FIXTURES_DIR) + "/fare/idf.fares",
                                       std::string(FIXTURES_DIR) + "/fare/prix.csv",
                                       std::string(FIXTURES_DIR) + "/fare/tarifs_od.csv");
    parser.use_stif_format = true;

    parser.load();

    const Fare& f = parser.data;

    // Un trajet simple
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|06;02|10;1;1;metro");
    auto res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value , 170);

    // Correspondance métro sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|20;02|30;1;1;metro");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);

    // Correspondance RER sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|35;02|40;1;1;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);

    // Correspondance BUS, donc nouveau billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|35;02|40;1;1;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    // Correspondance Tramwayway-bus autant qu'on veut
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|50;03|30;1;1;tramway");
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;03|30;04|20;1;1;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);
    // On a dépassé les 90 minutes de validité du ticket t+, il faut en racheter un
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;1;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 3);

    // On teste un peu les OD vers paris
    keys.clear();
    keys.push_back("ratp;8711388;8775890;FILGATO-2;2011|07|01;04|40;04|50;4;1;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,395);//code 140

    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|07|01;04|40;04|50;1;1;metro");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,395);

    // Et le Tramway bien évidemment payant !
    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|07|01;04|40;04|50;1;1;tramway");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    // On teste le tarif à une autre date
    keys.clear();
    keys.push_back("ratp;mantes;FILNav31;FILGATO-2;2011|12|01;04|40;04|50;4;1;metro");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,170);

    // On teste le noctilien
    keys.clear();
    keys.push_back("56;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;1;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets[0].value , 170);

    keys.clear();
    keys.push_back("56;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;3;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets[0].value , 340);  // C'est un trajet qui coûte 2 tickets


    keys.clear();
    keys.push_back("56;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;3;1;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets[0].value , 340);  // C'est un trajet qui coûte 2 tickets
    // Prendre le bus après le noctilien coûte
    keys.push_back("ratp;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;3;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    // On rajoute un bus après : il faut reprendre un billet
    keys.push_back("ratp;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|40;04|50;1;3;bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775499;2011|12|01;04|40;04|50;4;1;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,960); // 230 + 120 + 610 [codes 30;100;44]
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 1);

    // Cas avec deux RER
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,960);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 2);

    // Cas avec un RER, un changement en métro
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;metro");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 320); // code 130
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 2);


    // Cas avec deux RER, un changement en métro au milieu
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;metro");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,960);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 3);


    // Cas avec un RER, un changement en bus => faut payer
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;bus");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 3);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,320); // code 140
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value,170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).sections.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(2).value,700); // code 144
    BOOST_CHECK_EQUAL(res.tickets.at(2).sections.size() , 1);

    keys.clear();
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 700);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 2);

    keys.clear();
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;blibal;2011|12|01;04|40;04|50;1;3;rapidtransit");
    keys.push_back("ratp;bliabal;FILGATO-2;8775499;2011|12|01;04|40;04|50;3;5;rapidtransit");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,700);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 3);

    keys.clear();
    keys.push_back("5604:127;11:120;050050023:23;8727622;2011|07|31;09|28;09|39;4;4;Bus");
    keys.push_back(";8727622;800:D;8775890;2011|07|31;09|47;10|09;4;1;RapidTransit");
    keys.push_back(";8775860;100110007:7;R_0007;2011|07|31;10|20;10|21;1;1;Metro");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value , 395); // code 140
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.at(0).section, keys.at(0));
    BOOST_CHECK_EQUAL(res.tickets.at(1).sections.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(1).sections.at(0).section, keys.at(1));
    BOOST_CHECK_EQUAL(res.tickets.at(1).sections.at(1).section, keys.at(2));

    // On prend le RER intramuros
    keys.clear();
    keys.push_back(";8727141;RER B;8770870;2011|07|31;09|28;09|39;1;1;RapidTransit"); // Aulnay -> CDG "intramuros"
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // 13/10/2011 : youpppiiii on peut prendre "parfois" le tramway avec un ticket O/D
    keys.clear();
    keys.push_back(";8711388;800:T4;8727141;2011|07|31;09|28;09|39;4;4;tramway"); // L'abbaye -> Aulnay
    keys.push_back(";8727141;RER B;8770870;2011|07|31;09|28;09|39;4;4;RapidTransit"); // Aulnay -> CDG
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 545); // code 13 et 84 => 305 + 240

    keys.clear();
    keys.push_back(";bled_paumé;bus_magique;8711388;2011|07|31;09|28;09|39;4;4;Bus"); // Bled Paumé -> L'Abbaye
    keys.push_back(";8711388;800:T4;8727141;2011|07|31;09|40;09|50;4;4;tramway"); // L'abbaye -> Aulnay
    keys.push_back(";8727141;RER B;8770870;2011|07|31;09|28;09|39;4;4;RapidTransit"); // Aulnay -> CDG
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 470); // code 12+84 => 230 + 240

    // On teste les lignes à tarif exclusif
    keys.clear();
    keys.push_back(";paris;098098001:1;areoport;2011|07|31;09|28;09|39;4;4;Bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 1150); // Kof ! c'est cher la navette AF

    keys.clear();
    keys.push_back(";bled_paumé;bus_magique;8711388;2011|07|31;09|28;09|39;4;4;Bus"); // Bled Paumé -> L'Abbaye
    keys.push_back(";paris;098098001:1;areoport;2011|07|31;09|28;09|39;4;4;Bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 1150); // Kof ! c'est cher la navette AF}

    // Metro-RER-Bus
    keys.clear();
    keys.push_back("439;59465;100110008:8;8739303;2011|12|01;16|07;16|34;1;1;Metro");
    keys.push_back("436;8739303;800:C;8739315;2011|12|01;16|41;17|12;1;4;RapidTransit");
    keys.push_back("285;8739315;056356006:H;2:212;2011|12|01;17|17;17|19;4;4;Bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 320);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 170);

    // Jeux de tests rajoutés par le STIF

    // Essais avec noctilien
    keys.clear();
    keys.push_back("56;59557;100987785:N12;59452;2011|12|02;03|20;03|42;1;1;Bus");
    keys.push_back("56;59452;100987762:N62;59:182428;2011|12|02;03|48;04|22;1;3;Bus");
    keys.push_back("442;41:6487;100100195:195;59:153463;2011|12|02;05|14;05|20;3;3;Bus");
    keys.push_back("442;59:153463;100100194:194;59:1055459;2011|12|02;05|28;05|31;3;3;Bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 3);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 340);
    BOOST_CHECK_EQUAL(res.tickets.at(2).value, 170);

    // Plus de 90min depuis le dernier compostage
    keys.clear();
    keys.push_back("442;59362;100100068:68;59624;2011|12|05;10|52;11|08;1;2;Bus");
    keys.push_back("442;59624;100100295:295;24:14919;2011|12|05;11|15;11|56;2;3;Bus");
    keys.push_back("101;24:14919;039039307:307;24:14929;2011|12|05;12|01;12|15;3;4;Bus");
    keys.push_back("285;8739300;056356102:BAK;8739315;2011|12|05;12|21;12|26;4;4;Bus");
    keys.push_back("405;8739315;027027011:11;50:8168;2011|12|05;12|30;12|33;4;4;Bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 170);

    // Orlybus
    keys.clear();
    keys.push_back("442;8775863;100100283:ORLYBUS;59675;2011|12|05;05|35;05|56;1;4;Bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 690);

    // UN trajet qui coûtait 0 pour des raison obscures
    keys.clear();
    keys.push_back("440;59108;100112013:T3;59624;2011|12|05;11|43;11|45;1;1;Tramway");
    keys.push_back("442;59624;100100028:28;59349;2011|12|05;11|56;12|09;1;1;Bus");
    keys.push_back("442;59349;100100092:92;59:181763;2011|12|05;12|12;12|27;1;1;Bus");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // Ticket mantis sword 36393
    // Ressort deux ticket t+ au lieu d'un seul
    keys.clear();
    keys.push_back("436;8768600;810:A;8775800;2011|12|13;09|49;09|57;1;1;RapidTransit");
    keys.push_back("439;8775800;100110006:6;59455;2011|12|13;10|04;10|12;1;1;Metro");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // Remonté par le stif, deux tickets au lieu d'un
    keys.clear();
    keys.push_back("439;59500;100110009:9;59489;2011|12|13;10|17;10|23;1;1;Metro");
    keys.push_back("437;8738400;800:J;8738107;2011|12|13;10|31;10|39;1;3;LocalTrain");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 245);

    keys.clear();
    keys.push_back("437;8738287;800:L;8738221;2011|12|13;19|36;19|52;4;3;LocalTrain");
    keys.push_back("439;59594;100110001:1;59250;2011|12|13;20|04;20|09;2;2;Metro");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 395);

    // Remonté par le stif, deux tickets au lieu d'un
    keys.clear();
    keys.push_back("439;59591;100110001:1;59592;2012|01|03;11|13;11|17;1;1;Metro");
    keys.push_back("439;59592;100110013:13;8739100;2012|01|03;11|23;11|30;1;1;Metro");
    keys.push_back("437;8739100;800:N;8739156;2012|01|03;11|35;11|42;1;2;LocalTrain");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 250);

    // Mantis sword 39517
    keys.clear();
    keys.push_back("440;8711389;800:T4;8743179;2012|04|23;14|28;14|29;4;4;Tramway");
    res = f.compute_fare(keys);
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // on rajoute un bout de RER, on doit basculer vers un ticket OD
    keys.push_back("436;8727141;810:B;8727148;2012|06|26;19|41;19|50;4;4;RapidTransit");
    res = f.compute_fare(keys);
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 265);

    // Mantis sword 46044
    keys.clear();
    keys.push_back("440;59062;100112013:T3;8775864;2012|12|05;17|12;17|18;1;1;Tramway");
    keys.push_back("436;8775864;810:B;8775499;2012|12|05;17|25;17|34;1;3;RapidTransit");
    res = f.compute_fare(keys);
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
}

