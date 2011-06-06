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
         State start = parse_state(row[0]);
         State end = parse_state(row[1]);

         Transition transition;
         transition.cond = parse_condition(row[2]);
         transition.ticket = std::make_pair(row[3], boost::lexical_cast<float>(row[4]));

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
     std::vector< std::vector<ticket_t> > tickets(boost::num_vertices(g));

     BOOST_FOREACH(const std::string & section_key, section_keys ){
         std::vector< std::vector<ticket_t> > tickets(boost::num_vertices(g));
         SectionKey sc(section_key);
         BOOST_FOREACH(vertex_t v, boost::vertices(g)) {
             BOOST_FOREACH(edge_t e, boost::out_edges(v, g)){
                 BOOST_FOREACH(ticket_t ticket, tickets[v]){

                 }
             }
         }
     }

     std::vector<ticket_t> result;
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
    assert(string_vec.size() == 6);
    network = string_vec[0];
    start_stop_area = string_vec[1];
    dest_stop_area = string_vec[2];
    line = string_vec[3];
    // On zappe la date
    start_time = parse_time(string_vec[5]);
    dest_time = parse_time(string_vec[6]);
    start_zone = string_vec[7];
    dest_zone = string_vec[8];
}

bool valid_transition(const Transition & transition, boost::shared_ptr<Label> label){
    bool result = true;
    if(transition.cond.key == "duration") {
        float duration = boost::lexical_cast<float>(transition.cond.value);
        if(transition.cond.comparaison == LT && label->duration >= duration)
            result = false;
        else if(transition.cond.comparaison == LTE && label->duration > duration)
            result = false;
        else if(transition.cond.comparaison == GT && label->duration <= duration)
            result = false;
        else if(transition.cond.comparaison == GTE && label->duration < duration)
            result = false;
        else if(transition.cond.comparaison == EQ && label->duration != duration)
            result = false;
    }
    else if(transition.cond.key == "nb_changes") {
        int nb_changes = boost::lexical_cast<int>(transition.cond.value);
        if(transition.cond.comparaison == LT && label->nb_changes >= nb_changes)
            result = false;
        else if(transition.cond.comparaison == LTE && label->nb_changes > nb_changes)
            result = false;
        else if(transition.cond.comparaison == GT && label->nb_changes <= nb_changes)
            result = false;
        else if(transition.cond.comparaison == GTE && label->nb_changes < nb_changes)
            result = false;
        else if(transition.cond.comparaison == EQ && label->nb_changes != nb_changes)
            result = false;
    }
    return result;
}
