#include "fare.h"
#include "csv.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/lexical_cast.hpp>

namespace greg = boost::gregorian;
namespace qi = boost::spirit::qi;
namespace ph = boost::phoenix;

std::vector<Condition> parse_conditions(const std::string & conditions){
    std::vector<Condition> ret;
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, conditions, boost::algorithm::is_any_of("&"));
    BOOST_FOREACH(const std::string & cond_str, string_vec){
        ret.push_back(parse_condition(cond_str));
    }
    return ret;
}

State parse_state(const std::string & state_str){
    State state;
    if(state_str == "" || state_str == "*")
        return state;
    BOOST_FOREACH(Condition cond, parse_conditions(state_str)){
        if(cond.comparaison != EQ) throw invalid_key();
        if(cond.key == "line"){
            if(state.line != "") throw invalid_key();
            state.line = cond.value;
        }
        else if(cond.key == "zone"){
            if(state.zone != "") throw invalid_key();
            state.zone = cond.value;
        }
        else if(cond.key == "mode"){
            if(state.mode != "") throw invalid_key();
            state.mode = cond.value;
        }
        else if(cond.key == "stop_area"){
            if(state.stop_area != "") throw invalid_key();
            state.stop_area = cond.value;
        }
        else if(cond.key == "network"){
            if(state.network != "") throw invalid_key();
            state.network = cond.value;
        }
        else{
            throw invalid_key();
        }
    }

    return state;
}

Condition parse_condition(const std::string & condition_str) {
    std::string str = boost::algorithm::to_lower_copy(condition_str);
    boost::algorithm::replace_all(str, " ", "");
    Condition cond;

    if(str.empty())
        return cond;

    // Match du texte
    qi::rule<std::string::iterator, std::string()> txt =  qi::lexeme[+(qi::alnum|'_')];

    // Tous les opérateurs que l'on veut matcher
    qi::rule<std::string::iterator, Comp_e()> operator_r = qi::string("<=")[qi::_val = LTE]
                                                         | qi::string(">=")[qi::_val = GTE]
                                                         | qi::string("!=")[qi::_val = NEQ]
                                                         | qi::string("<") [qi::_val = LT]
                                                         | qi::string(">") [qi::_val = GT]
                                                         | qi::string("=")[qi::_val = EQ];

    // Une condition est de la forme "txt op txt"
    qi::rule<std::string::iterator, Condition()> condition_r = txt >> operator_r >> txt ;

    std::string::iterator begin = str.begin();
    std::string::iterator end = str.end();

    // Si on n'arrive pas à tout parser
    if(!qi::phrase_parse(begin, end, condition_r, boost::spirit::ascii::space, cond) || begin != end) {
        throw invalid_condition();
    }
    return cond;
}

void Fare::init(const std::string & filename, const std::string & prices_filename){
     CsvReader reader(filename);
     std::vector<std::string> row;

     // Associe un état à un nœud du graph
     std::map<State, vertex_t> state_map;
     State begin; // Le début est un nœud vide
     vertex_t begin_v = boost::add_vertex(begin, g);
     state_map[begin] = begin_v;
     reader.next(); //en-tête

     for(row=reader.next(); row != reader.end(); row = reader.next()) {
         bool symetric = false;

         State start = parse_state(row.at(0));
         State end = parse_state(row.at(1));

         Transition transition;
         transition.start_conditions = parse_conditions(row.at(2));
         transition.end_conditions = parse_conditions(row.at(3));
         std::vector<std::string> global_conditions;
         std::string str_condition = boost::algorithm::trim_copy(row.at(4));
         boost::algorithm::split(global_conditions, str_condition, boost::algorithm::is_any_of("&"));
         BOOST_FOREACH(std::string cond, global_conditions){
            if(cond == "symetric"){
                symetric = true;
            }else{
                transition.global_condition = cond;
            }
         }
         transition.ticket_key = boost::algorithm::trim_copy(row[5]);

         vertex_t start_v, end_v;
         if(state_map.find(start) == state_map.end()){
             start_v = boost::add_vertex(start, g);
             state_map[start] = start_v;
         }
         else start_v = state_map[start];

         if(state_map.find(end) == state_map.end()) {
             end_v = boost::add_vertex(end, g);
             state_map[end] = end_v;
         }
         else end_v = state_map[end];

         boost::add_edge(start_v, end_v, transition, g);
         if(symetric){
            Transition sym_transition = transition;
            sym_transition.start_conditions = transition.end_conditions;
            sym_transition.end_conditions = transition.start_conditions;
            boost::add_edge(start_v, end_v, sym_transition, g);
         }
     }
     load_fares(prices_filename);
}

Label next_label(Label label, Ticket ticket, const SectionKey & section){
    // On note des informations sur le dernier mode utilisé
    label.line = section.line;
    label.mode = section.mode;
    label.network = section.network;
    label.current_type = ticket.type;

    // Si c'est un ticket vide, c'est juste un changement
    // On incrémente le nombre de changements et la durée effectuée avec le même ticket
    if(ticket.caption == "" && ticket.value == 0){
        label.nb_changes++;
        label.duration += section.duration();
        if(ticket.type == Ticket::ODFare){
            label.stop_area = section.start_stop_area;
            label.zone = section.start_zone;
        }
    } else {
        // On a acheté un nouveau billet
        // On note le coût global du trajet, remet à 0 la durée/changements
        label.cost += ticket.value;
        label.tickets.push_back(ticket);
        label.nb_changes = 0;
        label.duration = section.duration();
        label.stop_area = section.start_stop_area;
    }
    if(!label.tickets.empty()){
        label.tickets.back().sections.push_back(section);
    }
    return label;
}

template<class T>
bool valid(const State & state, T t){
    if((state.mode != "" && !boost::iequals(state.mode, t.mode)) ||
       (state.network != "" && !boost::iequals(state.network, t.network)) ||
       (state.line != "" && !boost::iequals(state.line, t.line)) )
        return false;
    return true;
}

std::vector<Ticket> Fare::compute(const std::vector<std::string> & section_keys){
    int nb_nodes = boost::num_vertices(g);
    std::vector< std::vector<Label> > labels(nb_nodes);
    // Étiquette de départ
    labels[0].push_back(Label());
    BOOST_FOREACH(const std::string & section_key, section_keys){
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
                            if(transition.ticket_key != ""){
                                ticket = fare_map[transition.ticket_key].get_fare(section.date);
                            }
                            if(transition.global_condition == "exclusive")
                                throw ticket;
                            else if(transition.global_condition == "with_changes"){
                                ticket.type = Ticket::ODFare;
                            }
                            Label next = next_label(label, ticket, section);
                            if(label.current_type == Ticket::ODFare || ticket.type == Ticket::ODFare){
                                try {
                                    Ticket ticket_od;
                                    ticket_od.type = Ticket::ODFare;
                                    if(transition.global_condition == "with_changes")
                                        ticket_od = get_od(next, section).get_fare(section.date);
                                    else
                                        ticket_od = get_od(label, section).get_fare(section.date);
                                    new_labels.at(0).push_back(next_label(label,ticket_od , section));
                                } catch (no_ticket) {}

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
            new_labels.clear();
            new_labels.resize(nb_nodes);
            BOOST_FOREACH(Label label, labels.at(0)){
                new_labels.at(0).push_back(next_label(label, ticket, section));
            }
        }
        labels = new_labels;
    }

    std::vector<Ticket> result;

    // On recherche le label de moindre coût
    // Si on a deux fois le même coût, on prend celui qui nécessite le moins de billets
    size_t best_num_tickets = std::numeric_limits<size_t>::max();
    int best_cost = std::numeric_limits<int>::max();
    BOOST_FOREACH(Label label, labels.at(0)){
        if(label.cost < best_cost || (label.cost == best_cost && label.tickets.size() < best_num_tickets)){
            result = label.tickets;
            best_cost = label.cost;
            best_num_tickets = label.tickets.size();
        }
    }

    std::cout << "nombre de résultats : " << result.size() << std::endl;
    return result;
}

 void Fare::load_fares(const std::string & filename){
     CsvReader reader(filename);
     std::vector<std::string> row;
     for(row=reader.next(); row != reader.end(); row = reader.next()) {
         // La structure du csv est : clef;date_debut;date_fin;prix;libellé
         fare_map[row.at(0)].add(row.at(1), row.at(2),
                              Ticket(row.at(4), boost::lexical_cast<int>(row.at(3))) );
     }
 }

 void DateTicket::add(std::string begin_date, std::string end_date, Ticket ticket){
     greg::date begin(greg::from_undelimited_string(begin_date));
     greg::date end(greg::from_undelimited_string(end_date));
     tickets.push_back(date_ticket_t(greg::date_period(begin, end), ticket));
 }

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

    return boost::gregorian::date(boost::lexical_cast<int>(res.at(0)),
                                  boost::lexical_cast<int>(res.at(1)),
                                  boost::lexical_cast<int>(res.at(2)));
}

SectionKey::SectionKey(const std::string & key) : section(key) {
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, key, boost::algorithm::is_any_of(";"));
    assert(string_vec.size() == 10);
    network = string_vec.at(0);
    start_stop_area = string_vec.at(1);
    dest_stop_area = string_vec.at(2);
    line = string_vec.at(3);
    date = parse_nav_date(string_vec.at(4));
    start_time = parse_time(string_vec.at(5));
    dest_time = parse_time(string_vec.at(6));
    start_zone = string_vec.at(7);
    dest_zone = string_vec.at(8);
    mode = string_vec.at(9);
}

template<class T> bool compare(T a, T b, Comp_e comp){
    switch(comp) {
    case LT: return a < b; break;
    case GT: return a > b; break;
    case GTE: return a >= b; break;
    case LTE: return a <= b; break;
    case EQ: return a == b; break;
    case NEQ: return a != b; break;
    default: return true;
    }
}

int SectionKey::duration() const {
    if (start_time < dest_time)
        return dest_time-start_time;
    else // Passe-minuit
        return (dest_time + 24*3600) - start_time;
}

Ticket DateTicket::get_fare(boost::gregorian::date date){
    BOOST_FOREACH(date_ticket_t dticket, tickets){
        if(dticket.first.contains(date))
            return dticket.second;
    }
    std::cout << "No ticket..." << this->tickets.size() <<std::endl;
    throw no_ticket();
}

bool Transition::valid(const SectionKey & section, const Label & label) const
{
    bool result = true;
    BOOST_FOREACH(Condition cond, this->start_conditions)
    {
        if(cond.key == "zone" && cond.value != section.start_zone)
            result = false;
        else if(cond.key == "stop_area" && cond.value != section.start_stop_area)
            result = false;
        else if(cond.key == "duration") {
            // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
            int duration = boost::lexical_cast<int>(cond.value) * 60;
            result = compare(label.duration, duration, cond.comparaison);
        }
        else if(cond.key == "nb_changes") {
            int nb_changes = boost::lexical_cast<int>(cond.value);
            result = compare(label.nb_changes, nb_changes, cond.comparaison);
        }
    }
    BOOST_FOREACH(Condition cond, this->end_conditions)
    {
        if(cond.key == "zone" && cond.value != section.dest_zone)
            result = false;
        else if(cond.key == "stop_area" && cond.value != section.dest_stop_area)
            result = false;
        else if(cond.key == "duration") {
            // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
            int duration = boost::lexical_cast<int>(cond.value) * 60 + section.duration();
            result = compare(label.duration, duration, cond.comparaison);
        }
    }
    return result;
}

void Fare::load_od_stif(const std::string & filename){
    CsvReader reader(filename);
    std::vector<std::string> row;
    reader.next(); //en-tête

    for(row=reader.next(); row != reader.end(); row = reader.next()) {
        std::string start_saec = boost::algorithm::trim_copy(row[0]);
        std::string dest_saec = boost::algorithm::trim_copy(row[2]);
        std::string price_key = boost::algorithm::trim_copy(row[4]);

        OD_key start, dest;
        if(start_saec != "8775890")
            start = OD_key(OD_key::StopArea, start_saec);
        else
            start = OD_key(OD_key::Zone, "1");

        if(dest_saec != "8775890")
            dest = OD_key(OD_key::StopArea, dest_saec);
        else
            dest = OD_key(OD_key::Zone, "1");

        od_tickets[start][dest] = price_key;

    }
}

DateTicket Fare::get_od(Label label, SectionKey section){
    OD_key sa(OD_key::StopArea, label.stop_area);
    OD_key sb(OD_key::Zone, label.zone);
    OD_key da(OD_key::StopArea, section.dest_stop_area);
    OD_key db(OD_key::Zone, section.dest_zone);

    std::map< OD_key, std::map<OD_key, std::string> >::iterator start_map;
    start_map = od_tickets.find(sa);
    if(start_map == od_tickets.end())
        start_map = od_tickets.find(sb);
    if(start_map == od_tickets.end())
        throw no_ticket();

    std::map<OD_key, std::string>::iterator end;
    end = start_map->second.find(da);
    if(end == start_map->second.end())
        end = start_map->second.find(db);
    if(end == start_map->second.end())
        throw no_ticket();

    return fare_map[end->second];
}
