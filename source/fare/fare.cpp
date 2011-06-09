#include "fare.h"
#include "csv.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/lexical_cast.hpp>

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
    if(str == "exclusive"){
        cond.comparaison = Exclusive;
        return cond;
    }


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
    if(!qi::phrase_parse(begin, end, condition_r, boost::spirit::ascii::space, cond) || begin != end)
    {
        throw invalid_condition();
    }
    return cond;
}

Fare::Fare(const std::string & filename){
     CsvReader reader(filename);
     std::vector<std::string> row;

     // Associe un état à un nœud du graph
     std::map<State, vertex_t> state_map;
     State begin; // Le début est un nœud vide
     vertex_t begin_v = boost::add_vertex(begin, g);
     state_map[begin] = begin_v;

     for(row=reader.next(); row != reader.end(); row = reader.next()) {
         if(row.size() != 5)
             std::cout << row.size() << std::endl;
         State start = parse_state(row[0]);
         State end = parse_state(row[1]);

         Transition transition;
         transition.cond = parse_condition(row[2]);
         try{
             transition.ticket = std::make_pair(boost::algorithm::trim_copy(row[3]), boost::lexical_cast<int>(boost::algorithm::trim_copy(row[4])));
         } catch (boost::bad_lexical_cast) {
             transition.ticket = std::make_pair(boost::algorithm::trim_copy(row[3]), 0);
         }

         vertex_t start_v, end_v;
         if(state_map.find(start) == state_map.end()){
             start_v = boost::add_vertex(start, g);
             state_map[start] = start_v;
         }
         else
             start_v = state_map[start];

         if(state_map.find(end) == state_map.end()) {
             end_v = boost::add_vertex(end, g);
             state_map[end] = end_v;
         }
         else
             end_v = state_map[end];

         bool b;
         edge_t edge;
         boost::tie(edge, b) = boost::add_edge(start_v, end_v, transition, g);
         assert(b);// On accepte les arcs parallèles, donc ça devrait tjs être vrai
     }
}


 std::vector<ticket_t> Fare::compute(const std::vector<std::string> & section_keys){
     int nb_nodes = boost::num_vertices(g);
     std::vector< std::vector<Label_ptr> > labels(nb_nodes);
     // Étiquette de départ
     labels[0].push_back(Label_ptr(new Label()));
     BOOST_FOREACH(const std::string & section_key, section_keys ){
         std::vector< std::vector<Label_ptr> > new_labels(nb_nodes);
         SectionKey section(section_key);
         try {
             BOOST_FOREACH(edge_t e, boost::edges(g)){
                 Transition transition = g[e];
                 vertex_t u = boost::source(e,g);
                 vertex_t v = boost::target(e,g);
                 if(valid_start(g[u], section) && valid_dest(g[v], section)){
                     // Ah! c'est un segment exclusif, on est obligé de prendre ce billet et rien d'autre
                     if(transition.cond.comparaison == Exclusive)
                         throw transition.ticket;
                     BOOST_FOREACH(Label_ptr label, labels[u]){
                         if (valid_transition(transition, label)){
                             Label_ptr new_label(new Label(*label));
                             new_label->cost += transition.ticket.second;
                             new_label->nb_changes++;
                             new_label->duration += section.duration();
                             // On ne note de nouveau billet que s'il vaut quelque chose ou qu'il a un intitulé
                             // Et on remet à 0 la durée et le nombre de changements
                             if( (transition.ticket.first != "") || (transition.ticket.second != 0)){
                                 new_label->tickets.push_back(transition.ticket);
                                 new_label->nb_changes = 0;
                                 new_label->duration = section.duration();
                             }
                             new_label->pred = label;
                             new_labels[v].push_back(new_label);
                             new_labels[0].push_back(new_label);// On peut toujours partir du cas "aucun billet"
                         }
                     }
                 }
             }
         }
         // On est tombé sur un segment exclusif : on est obligé d'utilisé ce ticket
         catch(ticket_t ticket) {
             new_labels.clear();
             new_labels.resize(nb_nodes);
             BOOST_FOREACH(Label_ptr label, labels[0]){
                 Label_ptr new_label(new Label(*label));
                 new_label->cost += ticket.second;

                 new_label->tickets.push_back(ticket);
                 new_label->nb_changes = 0;
                 new_label->duration = section.duration();

                 new_label->pred = label;
                 new_labels[0].push_back(new_label);
             }
         }

         labels = new_labels;
     }

     std::vector<ticket_t> result;

     // On recherche le label de moindre coût
     // Si on a deux fois le même coût, on prend celui qui nécessite le moind de billets
     int best_changes = std::numeric_limits<int>::max();
     int best_cost = std::numeric_limits<int>::max();
     BOOST_FOREACH(vertex_t u, boost::vertices(g)){
         BOOST_FOREACH(Label_ptr label, labels[u]){
             if(label->cost < best_cost || (label->cost == best_cost && label->nb_changes < best_changes)){
                 result = label->tickets;
                 best_cost = label->cost;
                 best_changes = label->nb_changes;
             }
         }
     }

     std::cout << "nombre de résultats : " << result.size() << std::endl;
     return result;
 }

int parse_time(const std::string & time_str){
    // Règle permettant de parser une heure au format HH|MM
    qi::rule<std::string::const_iterator, int()> time_r = (qi::int_ >> '|' >> qi::int_)[qi::_val = qi::_1 * 3600 + qi::_2 * 60];
    int time;
    std::string::const_iterator begin = time_str.begin();
    std::string::const_iterator end = time_str.end();
    if(!qi::phrase_parse(begin, end, time_r, boost::spirit::ascii::space, time) || begin != end)
    {
        throw invalid_condition();
    }
    return time;
}

SectionKey::SectionKey(const std::string & key) {
    std::vector<std::string> string_vec;
    boost::algorithm::split(string_vec, key, boost::algorithm::is_any_of(";"));
    assert(string_vec.size() == 10);
    network = string_vec[0];
    start_stop_area = string_vec[1];
    dest_stop_area = string_vec[2];
    line = string_vec[3];
    // On zappe la date
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

bool valid_transition(const Transition & transition, Label_ptr label){
    if(transition.cond.comparaison == True)
        return true;
    bool result = true;
    if(transition.cond.key == "duration") {
        // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
        int duration = boost::lexical_cast<int>(transition.cond.value) * 60;
        result = compare(label->duration, duration, transition.cond.comparaison);
    }
    else if(transition.cond.key == "nb_changes") {
        int nb_changes = boost::lexical_cast<int>(transition.cond.value);
        result = compare(label->nb_changes, nb_changes, transition.cond.comparaison);
    }
    return result;
}

bool valid_dest(const State & state, SectionKey section_key){
    if((state.mode != "" && state.mode != section_key.mode) ||
       (state.network != "" && state.network != section_key.network) ||
       (state.line != "" && state.line != section_key.line) ||
       (state.zone != "" && state.zone != section_key.dest_zone) ||
       (state.stop_area != "" && state.stop_area != section_key.dest_stop_area))
        return false;
    return true;
}


bool valid_start(const State & state, SectionKey section_key){
    if((state.zone != "" && state.zone != section_key.start_zone) ||
       (state.stop_area != "" && state.stop_area != section_key.start_stop_area))
        return false;
    return true;
}

int SectionKey::duration() const {
    if (start_time < dest_time)
        return dest_time-start_time;
    else // Passe-minuit
        return (dest_time + 24*3600) - start_time;
}

