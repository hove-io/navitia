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
/*
    if(str == "exclusive"){
        cond.comparaison = Exclusive;
        return cond;
    }
    else if(str == "with_changes"){
        cond.comparaison = WithChanges;
        return cond;
    }
*/
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

Fare::Fare(const std::string & filename, const std::string & prices_filename){
     CsvReader reader(filename);
     std::vector<std::string> row;

     // Associe un état à un nœud du graph
     std::map<State, vertex_t> state_map;
     State begin; // Le début est un nœud vide
     vertex_t begin_v = boost::add_vertex(begin, g);
     state_map[begin] = begin_v;
     reader.next(); //en-tête

     for(row=reader.next(); row != reader.end(); row = reader.next()) {
         State start = parse_state(row[0]);
         State end = parse_state(row[1]);

         Transition transition;
         transition.start_conditions = parse_conditions(row[2]);
         transition.end_conditions = parse_conditions(row[3]);
         transition.global_condition = boost::algorithm::trim_copy(row[4]);
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
     }
     load_fares(prices_filename);
}

Label next_label(Label label, ticket_t ticket, const SectionKey & section, const std::string & dest_stop_area){
    label.cost += ticket.second;
    label.nb_changes++;
    label.duration += section.duration();
    label.line = section.line;
    label.mode = section.mode;
    label.network = section.network;
    // On ne note de nouveau billet que s'il vaut quelque chose ou qu'il a un intitulé
    // Et on remet à 0 la durée et le nombre de changements
    if(ticket.first != "" && ticket.second > 0){
        label.tickets.push_back(ticket);
        label.nb_changes = 0;
        label.duration = section.duration();
        label.stop_area = section.start_stop_area;
        label.dest_stop_area = dest_stop_area;
    }
    return label;
}

 std::vector<ticket_t> Fare::compute(const std::vector<std::string> & section_keys){
     int nb_nodes = boost::num_vertices(g);
     std::vector< std::vector<Label> > labels(nb_nodes);
     // Étiquette de départ
     labels[0].push_back(Label());
     BOOST_FOREACH(const std::string & section_key, section_keys ){
         std::vector< std::vector<Label> > new_labels(nb_nodes);
         SectionKey section(section_key);
         try {
             BOOST_FOREACH(edge_t e, boost::edges(g)){
                 Transition transition = g[e];
                 ticket_t ticket;
                 if(transition.ticket_key != "")
                     ticket = fare_map[transition.ticket_key].get_fare(section.date);
                 vertex_t u = boost::source(e,g);
                 vertex_t v = boost::target(e,g);
                 if(valid_dest(g[v], section)){
                     BOOST_FOREACH(Label label, labels[u]){
                         if (valid_start(g[u], label) &&  transition.valid(section_key, label)){
                             if(transition.is_exclusive()) throw ticket;


                             if(label.dest_stop_area == ""){
                                 new_labels[0].push_back(next_label(label, ticket, section, ""));// On peut toujours partir du cas "aucun billet"
                                 new_labels[v].push_back(next_label(label, ticket, section, ""));
                             } else if(label.dest_stop_area == section.dest_stop_area){
                                 new_labels[0].push_back(next_label(label, ticket_t(), section, label.dest_stop_area));
                                 new_labels[v].push_back(next_label(label, ticket_t(), section, label.dest_stop_area));
                             } else
                             //    new_labels[v].push_back(next_label(label, ticket_t(), section, transition.dest_stop_area()));
                                     new_labels[v].push_back(next_label(label, ticket_t(), section, label.dest_stop_area));

                        }
                     }
                 }
             }
         }
         // On est tombé sur un segment exclusif : on est obligé d'utilisé ce ticket
         catch(ticket_t ticket) {
             new_labels.clear();
             new_labels.resize(nb_nodes);
             BOOST_FOREACH(Label label, labels[0]){
                 new_labels[0].push_back(next_label(label, ticket, section, ""));
             }
         }
         labels = new_labels;
     }

     std::vector<ticket_t> result;

     // On recherche le label de moindre coût
     // Si on a deux fois le même coût, on prend celui qui nécessite le moind de billets
     int best_changes = std::numeric_limits<int>::max();
     int best_cost = std::numeric_limits<int>::max();
     BOOST_FOREACH(Label label, labels[0]){
         if(label.cost < best_cost || (label.cost == best_cost && label.nb_changes < best_changes)){
             result = label.tickets;
             best_cost = label.cost;
             best_changes = label.nb_changes;
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
         fare_map[row[0]].add(row[1], row[2],
                              ticket_t(row[4], boost::lexical_cast<int>(row[3])) );
     }
 }

 void DateTicket::add(std::string begin_date, std::string end_date, ticket_t ticket){
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

    return boost::gregorian::date(boost::lexical_cast<int>(res[0]),
                                  boost::lexical_cast<int>(res[1]),
                                  boost::lexical_cast<int>(res[2]));
}

SectionKey::SectionKey(const std::string & key) {
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, key, boost::algorithm::is_any_of(";"));
    assert(string_vec.size() == 10);
    network = string_vec[0];
    start_stop_area = string_vec[1];
    dest_stop_area = string_vec[2];
    line = string_vec[3];
    date = parse_nav_date(string_vec[4]);
    start_time = parse_time(string_vec[5]);
    dest_time = parse_time(string_vec[6]);
    start_zone = string_vec[7];
    dest_zone = string_vec[8];
    mode = string_vec[9];
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


bool valid_dest(const State & state, SectionKey section_key){
    if((state.mode != "" && state.mode != section_key.mode) ||
       (state.network != "" && state.network != section_key.network) ||
       (state.line != "" && state.line != section_key.line) )
        return false;
    return true;
}


bool valid_start(const State & state, const Label & label){
    if((state.mode != "" && state.mode != label.mode) ||
       (state.network != "" && state.network != label.network) ||
       (state.line != "" && state.line != label.line) )
        return false;
    return true;
}

int SectionKey::duration() const {
    if (start_time < dest_time)
        return dest_time-start_time;
    else // Passe-minuit
        return (dest_time + 24*3600) - start_time;
}

ticket_t DateTicket::get_fare(boost::gregorian::date date){
    BOOST_FOREACH(date_ticket_t dticket, tickets){
        if(dticket.first.contains(date))
            return dticket.second;
    }
    throw no_ticket();
}

bool Transition::valid(const SectionKey & section, const Label & label) const
{
    bool result = true;
    BOOST_FOREACH(Condition cond, this->start_conditions)
    {
        if(cond.key == "zone" && cond.value != section.start_zone&& global_condition != "with_changes")
            result = false;
        else if(cond.key == "stop_area" && cond.value != section.start_stop_area
                && !(global_condition == "with_changes" && label.stop_area == cond.value) )
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
        if(cond.key == "zone" && cond.value != section.dest_zone&& global_condition != "with_changes")
            result = false;
        else if(cond.key == "stop_area" && cond.value != section.dest_stop_area && global_condition != "with_changes")
            result = false;
        else if(cond.key == "duration") {
            // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
            int duration = boost::lexical_cast<int>(cond.value) * 60 + section.duration();
            result = compare(label.duration, duration, cond.comparaison);
        }
    }
    return result;
}

bool Transition::is_exclusive() const {
  /*  BOOST_FOREACH(Condition cond, this->start_conditions)
            if(cond.comparaison == Exclusive) return true;
    BOOST_FOREACH(Condition cond, this->end_conditions)
            if(cond.comparaison == Exclusive) return true;
    return false;*/
    return global_condition == "exclusive";
}

std::string Transition::dest_stop_area() const {
    BOOST_FOREACH(Condition cond, end_conditions)
            if(cond.key == "stop_area")
            return cond.value;
    return "";
}
