#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_fares
#include <boost/test/unit_test.hpp>
#include "fare/fare.h"
#include "type/data.h"
#include "ed/connectors/fare_parser.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

using namespace navitia::fare;
namespace qi = boost::spirit::qi;
namespace ph = boost::phoenix;

struct invalid_condition : public std::exception {};

void print_res(const results& res) {
    std::cout << "found : " << !res.not_found << std::endl;

    for (const auto& ticket : res.tickets) {
        std::cout << ticket.key << " : " << ticket.value << " (" << ticket.comment << ")" << std::endl;
        for (const auto& s : ticket.sections) {
            std::cout << " -- " << s.path_item_idx << std::endl;
        }
    }

}

boost::posix_time::time_duration parse_time(const std::string & time_str){
    // Règle permettant de parser une heure au format HH|MM
    qi::rule<std::string::const_iterator, int()> time_r = (qi::int_ >> '|' >> qi::int_)[qi::_val = qi::_1 * 3600 + qi::_2 * 60];
    int time;
    std::string::const_iterator begin = time_str.begin();
    std::string::const_iterator end = time_str.end();
    if(!qi::phrase_parse(begin, end, time_r, boost::spirit::ascii::space, time) || begin != end) {
        throw invalid_condition();
    }
    return boost::posix_time::seconds(time);
}

boost::gregorian::date parse_nav_date(const std::string & date_str){
     std::vector< std::string > res;
   boost::algorithm::split(res, date_str, boost::algorithm::is_any_of("|"));
   if(res.size() != 3)
       throw std::string("Date dans un format non parsable : " + date_str);
   boost::gregorian::date date;
   try{
       date = boost::gregorian::date(boost::lexical_cast<int>(res.at(0)),
                                     boost::lexical_cast<int>(res.at(1)),
                                     boost::lexical_cast<int>(res.at(2)));
   } catch (boost::bad_lexical_cast e){
       throw std::string("Conversion des chiffres dans la date impossible " + date_str);
   }
   return date;
}

const navitia::routing::Path string_to_path(const std::vector<std::string>& keys) {
    navitia::routing::Path p;
    for (const auto& key : keys) {
        std::string lower_key = boost::algorithm::to_lower_copy(key);
        std::vector<std::string> string_vec;
        boost::algorithm::split(string_vec, lower_key , boost::algorithm::is_any_of(";"));
        if (string_vec.size() != 10)
            throw std::string("Nombre incorrect d'éléments dans une section :" + boost::lexical_cast<std::string>(string_vec.size()) + " sur 10 attendus. " + key);

        std::string network = string_vec.at(0);
        std::string start_stop_area = string_vec.at(1);
        std::string dest_stop_area = string_vec.at(3);
        std::string line = string_vec.at(2);
        auto date = parse_nav_date(string_vec.at(4));
        auto start_time = parse_time(string_vec.at(5));
        auto dest_time = parse_time(string_vec.at(6));
        std::string start_zone = string_vec.at(7);
        std::string dest_zone = string_vec.at(8);
        std::string mode = string_vec.at(9);

        //construction of a mock item
        //will leak from everywhere :)
        navitia::routing::PathItem item(boost::posix_time::ptime(date, start_time), boost::posix_time::ptime(date, dest_time));
        nt::StopPoint* first_sp = new nt::StopPoint();
        first_sp->stop_area = new nt::StopArea();
        first_sp->stop_area->uri = start_stop_area;
        nt::StopPoint* last_sp = new nt::StopPoint();
        last_sp->stop_area = new nt::StopArea();
        last_sp->stop_area->uri = dest_stop_area;

        nt::StopTime* first_st = new nt::StopTime();
        first_st->vehicle_journey = new nt::VehicleJourney();
        first_st->vehicle_journey->journey_pattern = new nt::JourneyPattern();
        first_st->vehicle_journey->journey_pattern->route = new nt::Route();
        first_st->vehicle_journey->journey_pattern->route->line = new nt::Line();
        first_st->vehicle_journey->journey_pattern->route->line->uri = line;
        first_st->vehicle_journey->journey_pattern->route->line->network = new nt::Network();
        first_st->vehicle_journey->journey_pattern->route->line->network->uri = network;
        first_st->vehicle_journey->journey_pattern->physical_mode = new nt::PhysicalMode();
        first_st->vehicle_journey->journey_pattern->physical_mode->uri = mode;
        nt::StopTime* last_st = new nt::StopTime();

        item.stop_points.push_back(first_sp);
        item.stop_points.push_back(last_sp);
        item.stop_times.push_back(first_st);
        item.stop_times.push_back(last_st);

        first_sp->fare_zone = boost::lexical_cast<int>(start_zone);
        last_sp->fare_zone = boost::lexical_cast<int>(dest_zone);

        item.type = navitia::routing::ItemType::public_transport;

        p.items.push_back(item);
    }
    return p;
}

Fare load_fare_from_ed(ed::Data ed_data) {
    Fare fare;
    //for od and price, easy
    fare.od_tickets = ed_data.od_tickets;
    for (const auto& f: ed_data.fare_map) {
        fare.fare_map.insert(f);
    }
//    fare.fare_map = ed_data.fare_map;

    //for transition we have to build the graph
    std::map<State, Fare::vertex_t> state_map;
    State begin; // Start is an empty node (and the node is already is the fare graph, since it has been added in the constructor with the default ticket)
    state_map[begin] = fare.begin_v;

    for (auto tuple : ed_data.transitions) {
        const State& start = std::get<0>(tuple);
        const State& end = std::get<1>(tuple);

        const Transition& transition = std::get<2>(tuple);

        Fare::vertex_t start_v, end_v;
        if(state_map.find(start) == state_map.end()){
            start_v = boost::add_vertex(start, fare.g);
            state_map[start] = start_v;
        }
        else start_v = state_map[start];

        if(state_map.find(end) == state_map.end()) {
            end_v = boost::add_vertex(end, fare.g);
            state_map[end] = end_v;
        }
        else end_v = state_map[end];

        boost::add_edge(start_v, end_v, transition, fare.g);
    }
//    Transition default_transition;
//    Ticket default_ticket = make_default_ticket();
//    default_transition.ticket_key = default_ticket.key;
//    boost::add_edge(fare.begin_v, fare.begin_v, default_transition, fare.g);
//    DateTicket dticket;
//    dticket.add(boost::gregorian::date(boost::gregorian::neg_infin), boost::gregorian::date(boost::gregorian::pos_infin), default_ticket);
//    fare.fare_map.insert({default_ticket.key, dticket});

    return fare;
}

BOOST_AUTO_TEST_CASE(test_computation) {
    std::vector<std::string> keys;

    ed::Data ed_data;


    ed::connectors::fare_parser parser(ed_data, std::string(FIXTURES_DIR) + "/fare/idf.fares",
                                       std::string(FIXTURES_DIR) + "/fare/prix.csv",
                                       std::string(FIXTURES_DIR) + "/fare/tarifs_od.csv");

    parser.load();

    Fare f = load_fare_from_ed(parser.data);

    // Un trajet simple
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|06;02|10;1;1;metro");
    navitia::fare::results res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value , 170);

    // Correspondance métro sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|20;02|30;1;1;metro");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);

    // Correspondance RER sans billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|35;02|40;1;1;rapidtransit");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);

    // Correspondance BUS, donc nouveau billet
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|35;02|40;1;1;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    // Correspondance Tramwayway-bus autant qu'on veut
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|50;03|30;1;1;tramway");
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;03|30;04|20;1;1;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);
    // On a dépassé les 90 minutes de validité du ticket t+, il faut en racheter un
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;1;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 3);

    // On teste un peu les OD vers paris
    keys.clear();
    keys.push_back("ratp;8711388;8775890;FILGATO-2;2011|07|01;04|40;04|50;4;1;rapidtransit");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,395);//code 140

    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|07|01;04|40;04|50;1;1;metro");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,395);

    // Et le Tramway bien évidemment payant !
    // Le métro doit être gratuit après
    keys.push_back("ratp;paris;FILNav31;FILGATO-2;2011|07|01;04|40;04|50;1;1;tramway");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    // On teste le tarif à une autre date
    keys.clear();
    keys.push_back("ratp;mantes;FILNav31;FILGATO-2;2011|12|01;04|40;04|50;4;1;metro");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,170);

    // On teste le noctilien
    keys.clear();
    keys.push_back("56;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;1;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets[0].value , 170);

    keys.clear();
    keys.push_back("56;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;3;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets[0].value , 340);  // C'est un trajet qui coûte 2 tickets


    keys.clear();
    keys.push_back("56;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;3;1;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets[0].value , 340);  // C'est un trajet qui coûte 2 tickets
    // Prendre le bus après le noctilien coûte
    keys.push_back("ratp;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|30;04|40;1;3;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    // On rajoute un bus après : il faut reprendre un billet
    keys.push_back("ratp;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;04|40;04|50;1;3;bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);

    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775499;2011|12|01;04|40;04|50;4;1;rapidtransit");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,960); // 230 + 120 + 610 [codes 30;100;44]
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 1);

    // Cas avec deux RER
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,960);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 2);

    // Cas avec un RER, un changement en métro
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;metro");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 320); // code 130
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 2);


    // Cas avec deux RER, un changement en métro au milieu
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;metro");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,960);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 3);


    // Cas avec un RER, un changement en bus => faut payer
    keys.clear();
    keys.push_back("ratp;8739300;FILGATO-2;8775890;2011|12|01;04|40;04|50;4;1;rapidtransit");
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;bus");
    keys.push_back("ratp;8775890;FILGATO-2;8775499;2011|12|01;04|40;04|50;1;5;rapidtransit");
    res = f.compute_fare(string_to_path(keys));
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
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 700);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 2);

    keys.clear();
    keys.push_back("ratp;nation;montparnasse;FILGATO-2;2011|12|01;04|40;04|50;1;1;rapidtransit");
    keys.push_back("ratp;8775890;FILGATO-2;blibal;2011|12|01;04|40;04|50;1;3;rapidtransit");
    keys.push_back("ratp;bliabal;FILGATO-2;8775499;2011|12|01;04|40;04|50;3;5;rapidtransit");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,700);
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 3);

    keys.clear();
    keys.push_back("5604:127;11:120;050050023:23;8727622;2011|07|31;09|28;09|39;4;4;Bus");
    keys.push_back(";8727622;800:D;8775890;2011|07|31;09|47;10|09;4;1;RapidTransit");
    keys.push_back(";8775860;100110007:7;R_0007;2011|07|31;10|20;10|21;1;1;Metro");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size() , 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value,170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value , 395); // code 140
    BOOST_CHECK_EQUAL(res.tickets.at(0).sections.size() , 1);
    BOOST_CHECK_EQUAL(res.tickets.at(1).sections.size(), 2);

    // On prend le RER intramuros
    keys.clear();
    keys.push_back(";8727141;RER B;8770870;2011|07|31;09|28;09|39;1;1;RapidTransit"); // Aulnay -> CDG "intramuros"
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // 13/10/2011 : youpppiiii on peut prendre "parfois" le tramway avec un ticket O/D
    keys.clear();
    keys.push_back(";8711388;800:T4;8727141;2011|07|31;09|28;09|39;4;4;tramway"); // L'abbaye -> Aulnay
    keys.push_back(";8727141;RER B;8770870;2011|07|31;09|28;09|39;4;4;RapidTransit"); // Aulnay -> CDG
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 545); // code 13 et 84 => 305 + 240

    keys.clear();
    keys.push_back(";bled_paumé;bus_magique;8711388;2011|07|31;09|28;09|39;4;4;Bus"); // Bled Paumé -> L'Abbaye
    keys.push_back(";8711388;800:T4;8727141;2011|07|31;09|40;09|50;4;4;tramway"); // L'abbaye -> Aulnay
    keys.push_back(";8727141;RER B;8770870;2011|07|31;09|28;09|39;4;4;RapidTransit"); // Aulnay -> CDG
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 470); // code 12+84 => 230 + 240

    // On teste les lignes à tarif exclusif
    keys.clear();
    keys.push_back(";paris;098098001:1;areoport;2011|07|31;09|28;09|39;4;4;Bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 1150); // Kof ! c'est cher la navette AF

    keys.clear();
    keys.push_back(";bled_paumé;bus_magique;8711388;2011|07|31;09|28;09|39;4;4;Bus"); // Bled Paumé -> L'Abbaye
    keys.push_back(";paris;098098001:1;areoport;2011|07|31;09|28;09|39;4;4;Bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 1150); // Kof ! c'est cher la navette AF}

    // Metro-RER-Bus
    keys.clear();
    keys.push_back("439;59465;100110008:8;8739303;2011|12|01;16|07;16|34;1;1;Metro");
    keys.push_back("436;8739303;800:C;8739315;2011|12|01;16|41;17|12;1;4;RapidTransit");
    keys.push_back("285;8739315;056356006:H;2:212;2011|12|01;17|17;17|19;4;4;Bus");
    res = f.compute_fare(string_to_path(keys));
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
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 3);
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
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 170);

    // Orlybus
    keys.clear();
    keys.push_back("442;8775863;100100283:ORLYBUS;59675;2011|12|05;05|35;05|56;1;4;Bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 690);

    // UN trajet qui coûtait 0 pour des raison obscures
    keys.clear();
    keys.push_back("440;59108;100112013:T3;59624;2011|12|05;11|43;11|45;1;1;Tramway");
    keys.push_back("442;59624;100100028:28;59349;2011|12|05;11|56;12|09;1;1;Bus");
    keys.push_back("442;59349;100100092:92;59:181763;2011|12|05;12|12;12|27;1;1;Bus");
    res = f.compute_fare(string_to_path(keys));
    BOOST_CHECK_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // Ticket mantis sword 36393
    // Ressort deux ticket t+ au lieu d'un seul
    keys.clear();
    keys.push_back("436;8768600;810:A;8775800;2011|12|13;09|49;09|57;1;1;RapidTransit");
    keys.push_back("439;8775800;100110006:6;59455;2011|12|13;10|04;10|12;1;1;Metro");
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // Remonté par le stif, deux tickets au lieu d'un
    keys.clear();
    keys.push_back("439;59500;100110009:9;59489;2011|12|13;10|17;10|23;1;1;Metro");
    keys.push_back("437;8738400;800:J;8738107;2011|12|13;10|31;10|39;1;3;LocalTrain");
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 245);

    keys.clear();
    keys.push_back("437;8738287;800:L;8738221;2011|12|13;19|36;19|52;4;3;LocalTrain");
    keys.push_back("439;59594;100110001:1;59250;2011|12|13;20|04;20|09;2;2;Metro");
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 395);

    // Remonté par le stif, deux tickets au lieu d'un
    keys.clear();
    keys.push_back("439;59591;100110001:1;59592;2012|01|03;11|13;11|17;1;1;Metro");
    keys.push_back("439;59592;100110013:13;8739100;2012|01|03;11|23;11|30;1;1;Metro");
    keys.push_back("437;8739100;800:N;8739156;2012|01|03;11|35;11|42;1;2;LocalTrain");
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 250);

    // Mantis sword 39517
    keys.clear();
    keys.push_back("440;8711389;800:T4;8743179;2012|04|23;14|28;14|29;4;4;Tramway");
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // on rajoute un bout de RER, on doit basculer vers un ticket OD
    keys.push_back("436;8727141;810:B;8727148;2012|06|26;19|41;19|50;4;4;RapidTransit");
    res = f.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 265);

    // Mantis sword 46044
    keys.clear();
    keys.push_back("440;59062;100112013:T3;8775864;2012|12|05;17|12;17|18;1;1;Tramway");
    keys.push_back("436;8775864;810:B;8775499;2012|12|05;17|25;17|34;1;3;RapidTransit");
    res = f.compute_fare(string_to_path(keys));
    print_res(res);
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);

    // tests with unknown section in the middle
    keys.clear();
    keys.push_back("439;59591;100110001:1;59592;2012|01|03;11|13;11|17;1;1;Metro");
    keys.push_back("439;59592;100110013:13;8739100;2012|01|03;11|23;11|30;1;1;Metro");
    keys.push_back("437;8739100;800:N;8739156;2012|01|03;11|35;11|42;1;2;LocalTrain");
    keys.push_back("bob;morane;contre;tout;2011|07|01;02|06;02|10;1;1;chacal"); //unkown fare section
    res = f.compute_fare(string_to_path(keys));
    print_res(res);
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 250);
    BOOST_CHECK_EQUAL(res.tickets.at(1).key, make_default_ticket().key);

    keys.clear();
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|07|01;02|06;02|10;1;1;metro");
    keys.push_back("bob;morane;contre;tout;2011|07|01;02|06;02|10;1;1;chacal"); //unkown fare section
    keys.push_back("439;59592;100110013:13;8739100;2012|01|03;11|23;11|30;1;1;Metro");
    keys.push_back("437;8739100;800:N;8739156;2012|01|03;11|35;11|42;1;2;LocalTrain");
    res = f.compute_fare(string_to_path(keys));
    print_res(res);
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 3);
    BOOST_CHECK_EQUAL(res.tickets.at(0).value, 170);
    BOOST_CHECK_EQUAL(res.tickets.at(1).key, make_default_ticket().key);
    BOOST_CHECK_EQUAL(res.tickets.at(2).value, 250);//we have to take another ticket since we don't have any information on the bob-morane section

    // tests with unknown section in the beginning
    keys.clear();
    keys.push_back("bob;morane;contre;tout;2011|07|01;02|06;02|10;1;1;chacal"); //unkown fare section
    keys.push_back("439;59591;100110001:1;59592;2012|01|03;11|13;11|17;1;1;Metro");
    keys.push_back("439;59592;100110013:13;8739100;2012|01|03;11|23;11|30;1;1;Metro");
    keys.push_back("437;8739100;800:N;8739156;2012|01|03;11|35;11|42;1;2;LocalTrain");
    res = f.compute_fare(string_to_path(keys));
    print_res(res);
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 2);
    BOOST_CHECK_EQUAL(res.tickets.at(0).key, make_default_ticket().key);
    BOOST_CHECK_EQUAL(res.tickets.at(1).value, 250);

    // tests with unknown section in the beginning
    keys.clear();
    keys.push_back("bob;morane;contre;tout;2011|07|01;02|06;02|10;1;1;chacal"); //unkown fare section
    keys.push_back("bob;morane;contre;tout;2011|07|01;02|06;02|10;1;1;chacal"); //unkown fare section
    keys.push_back("bob;morane;contre;tout;2011|07|01;02|06;02|10;1;1;chacal"); //unkown fare section
    res = f.compute_fare(string_to_path(keys));
    print_res(res);
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 3);
    BOOST_CHECK_EQUAL(res.tickets.at(0).key, make_default_ticket().key);
    BOOST_CHECK_EQUAL(res.tickets.at(1).key, make_default_ticket().key);
    BOOST_CHECK_EQUAL(res.tickets.at(2).key, make_default_ticket().key);
}

//TODO tout decouper + rajout fixture

BOOST_AUTO_TEST_CASE(unkown_fare) {
    std::vector<std::string> keys;

    Fare fare;
    boost::gregorian::date start_date(boost::gregorian::from_undelimited_string("20110101"));
    boost::gregorian::date end_date(boost::gregorian::from_undelimited_string("20350101"));
    fare.fare_map["price1"].add(start_date, end_date, Ticket("price1", "Ticket vj 1", 100, "125"));
    fare.fare_map["price2"].add(start_date, end_date, Ticket("price2", "Ticket vj 2", 200, "175"));

    Transition default_transition;
    Ticket default_ticket = make_default_ticket();
    default_ticket.value = 999999;
    default_transition.ticket_key = default_ticket.key;
    auto begin_v = boost::add_vertex(State(), fare.g);
    boost::add_edge(begin_v, begin_v, default_transition, fare.g);
    DateTicket dticket;
    dticket.add(boost::gregorian::date(boost::gregorian::neg_infin), boost::gregorian::date(boost::gregorian::pos_infin), default_ticket);
    fare.fare_map.insert({default_ticket.key, dticket});

    Transition transition;
    transition.start_conditions = {};
    transition.end_conditions = {};
    transition.ticket_key = "price1";
    transition.global_condition = Transition::GlobalCondition::with_changes;
    State start;
    State end;
    start.mode = "metro";
    end.mode = "metro";
    auto start_v = boost::add_vertex(start, fare.g);
    auto end_v = boost::add_vertex(end, fare.g);
    boost::add_edge(start_v, end_v, transition, fare.g);

    // Un trajet simple
    keys.push_back("bob;morane;contre;tout;2011|07|01;02|06;02|10;1;1;chacal");
    results res = fare.compute_fare(string_to_path(keys));
    BOOST_REQUIRE_EQUAL(res.tickets.size(), 1);
    BOOST_CHECK_EQUAL(res.tickets.at(0).key, make_default_ticket().key);
}
