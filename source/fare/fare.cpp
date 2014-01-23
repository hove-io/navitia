#include "fare.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

namespace greg = boost::gregorian;
namespace qi = boost::spirit::qi;
namespace ph = boost::phoenix;


namespace navitia { namespace fare {

Label next_label(Label label, Ticket ticket, const SectionKey & section) {
    // On note des informations sur le dernier mode utilisé
    label.line = section.line;
    label.mode = section.mode;
    label.network = section.network;


    if(ticket.type == Ticket::ODFare){
        if(label.stop_area == "" || label.current_type != Ticket::ODFare){ // C'est un nouveau ticket de type OD
            label.stop_area = section.start_stop_area;
            label.zone = section.start_zone;
            label.nb_changes = 0;
            label.start_time = section.start_time;

            ticket.sections.push_back(section);
            label.tickets.push_back(ticket);
        }else{ // On a un ancien ticket
            label.tickets.back().sections.push_back(section);
            label.nb_changes++;
        }

    }else{
        // Si c'est un ticket vide, c'est juste un changement
        // On incrémente le nombre de changements et la durée effectuée avec le même ticket
        if(ticket.caption == "" && ticket.value == 0){
            label.nb_changes++;
        } else {
            // On a acheté un nouveau billet
            // On note le coût global du trajet, remet à 0 la durée/changements
            label.cost += ticket.value;
            label.tickets.push_back(ticket);
            label.nb_changes = 0;
            label.start_time = section.start_time;
            label.stop_area = section.start_stop_area;
        }
        if(label.tickets.size() == 0)
            throw("Problème interne");
        label.tickets.back().sections.push_back(section);
    }
    label.current_type = ticket.type;
    return label;
}

bool valid(const State & state, SectionKey section){
    if((state.mode != "" && !boost::iequals(state.mode, section.mode)) ||
       (state.network != "" && !boost::iequals(state.network, section.network)) ||
       (state.line != "" && !boost::iequals(state.line, section.line)) )
        return false;
    return true;
}

bool valid(const State & state, Label label){
    if((state.mode != "" && !boost::iequals(state.mode, label.mode)) ||
       (state.network != "" && !boost::iequals(state.network, label.network)) ||
       (state.line != "" && !boost::iequals(state.line, label.line))  ||
       (state.ticket != "" && !boost::iequals(state.ticket, label.tickets.back().caption)) )
        return false;
    return true;
}

results Fare::compute_fare(const std::vector<std::string>& path) const {
    results res;
	int nb_nodes = boost::num_vertices(g);
    std::vector< std::vector<Label> > labels(nb_nodes);
    // Étiquette de départ
    labels[0].push_back(Label());
    BOOST_FOREACH(const std::string & section_key, path){
        LOG4CPLUS_INFO(logger, "Nouvelle section à étudier : " << section_key);
        std::vector< std::vector<Label> > new_labels(nb_nodes);
        SectionKey section(section_key);
        try {
            BOOST_FOREACH(edge_t e, boost::edges(g)){
                vertex_t u = boost::source(e,g);
                vertex_t v = boost::target(e,g);
                if(valid(g[v], section)){
                    BOOST_FOREACH(Label label, labels[u]){
                        Ticket ticket;
                        Transition transition = g[e];
                        if (valid(g[u], label) &&  transition.valid(section_key, label)){
                            LOG4CPLUS_INFO(logger, "\tTransition valide : " << transition.csv_string);
                            if(transition.ticket_key != "") {
                                bool ticket_found = false; //TODO refactor this, optional is way better
                                auto it = fare_map.find(transition.ticket_key);
                                try {
                                    if (it != fare_map.end()) {
                                        ticket = it->second.get_fare(section.date);
                                        ticket_found = true;
                                    }
                                }
                                catch(no_ticket) { //the ticket_found bool is still false
                                }
                                if (! ticket_found) {
                                    ticket.type = Ticket::None;
                                    ticket.key = transition.ticket_key;
                                    ticket.value = -100; // no price
                                    ticket.comment = "no price found";
                                }
                            }
                            if(transition.global_condition == "exclusive")
                                throw ticket;
                            else if(transition.global_condition == "with_changes"){
                                ticket.type = Ticket::ODFare;
                            }
                            Label next = next_label(label, ticket, section);

                            // On résoud le ticket OD : cas où on ne se servira plus de ce ticket
                            if(label.current_type == Ticket::ODFare || ticket.type == Ticket::ODFare){
                                try {
                                    Ticket ticket_od;
                                    ticket_od = get_od(next, section).get_fare(section.date);
                                    if(label.tickets.size() > 0 && label.current_type == Ticket::ODFare)
                                        ticket_od.sections = label.tickets.back().sections;
                                        
                                    ticket_od.sections.push_back(section);
                                    Label n = next;
                                    n.cost += ticket_od.value;
                                    n.tickets.back() = ticket_od;
                                    n.current_type = Ticket::FlatFare;

                                    new_labels.at(0).push_back(n);
                                    LOG4CPLUS_INFO(logger, "\t\tOn résoud un ticket OD : " << ticket_od.value);
                                } catch (no_ticket) {
                                    LOG4CPLUS_INFO(logger, "\t\tOn a pas réussi à résoudre le ticket OD; SA=" << next.stop_area << " zone=" << next.zone
                                             << ", section start_zone=" << section.start_zone << ", dest_zone=" << section.start_zone
                                             << " start_sa=" << section.start_stop_area << " dest_sa=" << section.dest_stop_area
                                             << " mode=" << section.mode);
                                }

                            } else {
                                new_labels.at(0).push_back(next);
                            }
                            new_labels[v].push_back(next);
                        }
                    }
                }
            }
        }
        // On est tombé sur un segment exclusif : on est obligé d'utilisé ce ticket
        catch(Ticket ticket) {
            LOG4CPLUS_INFO(logger, "\t On tombe sur un segment exclusif");
            new_labels.clear();
            new_labels.resize(nb_nodes);
            BOOST_FOREACH(Label label, labels.at(0)){
                new_labels.at(0).push_back(next_label(label, ticket, section));
            }
        }
        labels = new_labels;
    }

    // On recherche le label de moindre coût
    // Si on a deux fois le même coût, on prend celui qui nécessite le moins de billets
    size_t best_num_tickets = std::numeric_limits<size_t>::max();
    int best_cost = std::numeric_limits<int>::max();
    BOOST_FOREACH(Label label, labels.at(0)){
        LOG4CPLUS_INFO(logger, "prix : " << label.cost);
        if(label.cost < best_cost || (label.cost == best_cost && label.tickets.size() < best_num_tickets)){
            res.tickets = label.tickets;
            best_cost = label.cost;
            best_num_tickets = label.tickets.size();
        }
    }

    LOG4CPLUS_INFO(logger, "nombre de résultats : " << res.tickets.size());
    return res;
}


 void DateTicket::add(std::string begin_date, std::string end_date, Ticket ticket){
     greg::date begin(greg::from_undelimited_string(begin_date));
     greg::date end(greg::from_undelimited_string(end_date));
     tickets.push_back(date_ticket_t(greg::date_period(begin, end), ticket));
 }

 struct invalid_condition : public std::exception {};

 int parse_time(const std::string & time_str){
     // Règle permettant de parser une heure au format HH|MM
     qi::rule<std::string::const_iterator, int()> time_r = (qi::int_ >> '|' >> qi::int_)[qi::_val = qi::_1 * 3600 + qi::_2 * 60];
     int time;
     std::string::const_iterator begin = time_str.begin();
     std::string::const_iterator end = time_str.end();
     if(!qi::phrase_parse(begin, end, time_r, boost::spirit::ascii::space, time) || begin != end) {
         throw invalid_condition();
     }
     return time;
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

SectionKey::SectionKey(const std::string & key) : section(key) {
    std::string lower_key = boost::algorithm::to_lower_copy(key);
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, lower_key , boost::algorithm::is_any_of(";"));
    if (string_vec.size() != 10)
        throw std::string("Nombre incorrect d'éléments dans une section :" + boost::lexical_cast<std::string>(string_vec.size()) + " sur 10 attendus. " + key);
    network = string_vec.at(0);
    start_stop_area = string_vec.at(1);
    dest_stop_area = string_vec.at(3);
    line = string_vec.at(2);
    date = parse_nav_date(string_vec.at(4));//TODO virer
    start_time = parse_time(string_vec.at(5));
    dest_time = parse_time(string_vec.at(6));
    start_zone = string_vec.at(7);
    dest_zone = string_vec.at(8);
    mode = string_vec.at(9);
}

template<class T> bool compare(T a, T b, Comp_e comp){
    switch(comp) {
    case Comp_e::LT: return a < b; break;
    case Comp_e::GT: return a > b; break;
    case Comp_e::GTE: return a >= b; break;
    case Comp_e::LTE: return a <= b; break;
    case Comp_e::EQ: return a == b; break;
    case Comp_e::NEQ: return a != b; break;
    default: return true;
    }
}

int SectionKey::duration_at_begin(int ticket_start_time) const {
    if (ticket_start_time < start_time)
        return start_time - ticket_start_time;
    else // Passe-minuit
        return (start_time + 24*3600) - ticket_start_time;
}

int SectionKey::duration_at_end(int ticket_start_time) const {
    if (ticket_start_time < dest_time)
        return dest_time - ticket_start_time;
    else // Passe-minuit
        return (dest_time + 24*3600) - ticket_start_time;
}

Ticket DateTicket::get_fare(boost::gregorian::date date) const {
    BOOST_FOREACH(date_ticket_t dticket, tickets){
        if(dticket.first.contains(date))
            return dticket.second;
    }

    throw no_ticket();
}

DateTicket DateTicket::operator +(const DateTicket & other) const{
    DateTicket new_ticket = *this;
    if(this->tickets.size() != other.tickets.size())
        std::cerr << "Tickets n'ayant pas le même nombre de dates"  << std::endl;

    for(size_t i = 0; i < std::min(this->tickets.size(), other.tickets.size()); ++i) {
        if(this->tickets[i].first != other.tickets[i].first)
            std::cerr << "Le ticket n° " << i << " n'a pas les même dates; " << this->tickets[i].first << " versus " << other.tickets[i].first << std::endl;
        new_ticket.tickets[i].second.value = this->tickets[i].second.value + other.tickets[i].second.value;
    }
    return new_ticket;
}

bool Transition::valid(const SectionKey & section, const Label & label) const
{
    bool result = true;
    if(label.current_type == Ticket::ODFare && this->global_condition!="with_changes")
        result = false;
    BOOST_FOREACH(Condition cond, this->start_conditions)
    {
        if(cond.key == "zone" && cond.value != section.start_zone)
            result = false;
        else if(cond.key == "stoparea" && cond.value != section.start_stop_area)
            result = false;
        else if(cond.key == "duration") {
            // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
            int duration = boost::lexical_cast<int>(cond.value) * 60;
            int ticket_duration = section.duration_at_begin(label.start_time);
            result &= compare(ticket_duration, duration, cond.comparaison);
        }
        else if(cond.key == "nb_changes") {
            int nb_changes = boost::lexical_cast<int>(cond.value);
            result &= compare(label.nb_changes, nb_changes, cond.comparaison);
        }
        else if(cond.key == "ticket" && label.tickets.size() > 0) {
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), label.tickets.back().key << " " << cond.value);
            result &= compare(label.tickets.back().key, cond.value, cond.comparaison);
        }
    }
    BOOST_FOREACH(Condition cond, this->end_conditions)
    {
        if(cond.key == "zone" && cond.value != section.dest_zone)
            result = false;
        else if(cond.key == "stoparea" && cond.value != section.dest_stop_area)
            result = false;
        else if(cond.key == "duration") {
            // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
            int duration = boost::lexical_cast<int>(cond.value) * 60;
            int ticket_duration = section.duration_at_end(label.start_time);
            result &= compare(ticket_duration, duration, cond.comparaison);
        }
    }
    return result;
}


DateTicket Fare::get_od(Label label, SectionKey section) const {
    OD_key sa(OD_key::StopArea, label.stop_area);
    OD_key sb(OD_key::Mode, label.mode);
    OD_key sc(OD_key::Zone, label.zone);

    OD_key da(OD_key::StopArea, section.dest_stop_area);
    OD_key db(OD_key::Mode, section.mode);
    OD_key dc(OD_key::Zone, section.dest_zone);


    auto start_map = od_tickets.find(sa);
    if(start_map == od_tickets.end())
        start_map = od_tickets.find(sb);
    if(start_map == od_tickets.end())
        start_map = od_tickets.find(sc);
    if(start_map == od_tickets.end())
        throw no_ticket();

    auto end = start_map->second.find(da);
    if(end == start_map->second.end())
        end = start_map->second.find(db);
    if(end == start_map->second.end())
        end = start_map->second.find(dc);
    if(end == start_map->second.end())
        throw no_ticket();

    // On crée un nouveau ticket en sommant toutes les composantes élémentaires
    // Un ticket OD stif est en effet la somme de plusieurs tickets
    DateTicket ticket;
    auto it = fare_map.find(end->second.at(0));
    if (it != fare_map.end()) {
        ticket = it->second;
    }
    for(size_t i = 1; i < end->second.size(); ++i){
        it = fare_map.find(end->second.at(i));
        if (it != fare_map.end()) {
            ticket = ticket + it->second;
        } else {
            auto new_ticket = DateTicket();
            ticket = ticket + new_ticket;
        }
    }
    return ticket;
}

}
}
