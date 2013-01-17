#include "ptreferential.h"
#include "reflexion.h"
#include "where.h"
#include "type/pb_converter.h"

#include <algorithm>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/phoenix/object/construct.hpp>


//TODO: change!
using namespace navitia::type;
using namespace navitia::ptref;

namespace qi = boost::spirit::qi;


namespace navitia{ namespace ptref{
using namespace navitia::type;

namespace qi = boost::spirit::qi;

/// Fonction qui va lire une chaîne de caractère et remplir un vector de Filter
        template <typename Iterator>
        struct select_r
            : qi::grammar<Iterator, std::vector<Filter>(), qi::space_type>
{
    qi::rule<Iterator, std::string(), qi::space_type> txt, txt2, txt3; // Match une string
    qi::rule<Iterator, Operator_e(), qi::space_type> bin_op; // Match une operator binaire telle que <, =...
    qi::rule<Iterator, std::vector<Filter>(), qi::space_type> filter; // La string complète à parser
    qi::rule<Iterator, Filter(), qi::space_type> filter1, filter2; // La string complète à parser

    select_r() : select_r::base_type(filter) {
        txt = qi::lexeme[+(qi::alnum|qi::char_("_:-"))];
        txt2 = qi::lexeme[+(qi::alnum|qi::char_("_:-=.<> "))];
        txt3 = '"' >> qi::lexeme[+(qi::alnum|qi::char_("_:- "))] >> '"';
        bin_op =  qi::string("<=")[qi::_val = LEQ]
                | qi::string(">=")[qi::_val = GEQ]
                | qi::string("<>")[qi::_val = NEQ]
                | qi::string("<") [qi::_val = LT]
                | qi::string(">") [qi::_val = GT]
                | qi::string("=") [qi::_val = EQ];

        filter1 = (txt >> "." >> txt >> bin_op >> (txt|txt3))[qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2, qi::_3, qi::_4)];
        filter2 = (txt >> "HAVING" >> '(' >> txt2 >> ')')[qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2)];
        filter %= (filter1 | filter2) % (qi::lexeme["and"] | qi::lexeme["AND"]) ;
    }

};



// vérification d'une clause WHERE
template<class T>
WhereWrapper<T> build_clause(std::vector<Filter> filters) {
    WhereWrapper<T> wh(new BaseWhere<T>());
    for(const Filter & filter : filters) {
        if(filter.attribute == "id")
            wh = wh && WHERE(ptr_id<T>(), filter.op, filter.value);
        else if(filter.attribute == "idx")
            wh = wh && WHERE(ptr_idx<T>(), filter.op, filter.value);
        else if(filter.attribute == "external_code")
            wh = wh && WHERE(ptr_external_code<T>(), filter.op, filter.value);
        else if(filter.attribute == "name")
            wh = wh && WHERE(ptr_name<T>(), filter.op, filter.value);
        }
    return wh;
}



pbnavitia::Response extract_data(const Data & data, Type_e requested_type, std::vector<idx_t> & rows, const int depth) {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::PTREFERENTIAL);
    pbnavitia::PTReferential * pb_response = result.mutable_ptref();

    for(idx_t idx : rows){
        switch(requested_type){
        case Type_e::eLine: fill_pb_object(idx, data, pb_response->add_line(), depth); break;
        case Type_e::eRoute: fill_pb_object(idx, data, pb_response->add_route(), depth); break;
        case Type_e::eStopPoint: fill_pb_object(idx, data, pb_response->add_stop_point(), depth); break;
        case Type_e::eStopArea: fill_pb_object(idx, data, pb_response->add_stop_area(), depth); break;
        case Type_e::eNetwork: fill_pb_object(idx, data, pb_response->add_network(), depth); break;
        case Type_e::eMode: fill_pb_object(idx, data, pb_response->add_commercial_mode(), depth); break;
        case Type_e::eModeType: fill_pb_object(idx, data, pb_response->add_physical_mode(), depth); break;
        case Type_e::eCity: fill_pb_object(idx, data, pb_response->add_city(), depth); break;
        case Type_e::eConnection: fill_pb_object(idx, data, pb_response->add_connection(), depth); break;
        case Type_e::eRoutePoint: fill_pb_object(idx, data, pb_response->add_route_point(), depth); break;
        case Type_e::eCompany: fill_pb_object(idx, data, pb_response->add_company(), depth); break;
        default: break;
        }
    }
    return result;
}


template<typename T, typename C>
std::vector<idx_t> filtered_indexes(const std::vector<T> & data, const C & clause) {
    std::vector<idx_t> result;
    for(size_t i = 0; i < data.size(); ++i){
        if(clause(data[i]))
            result.push_back(i);
    }
    return result;
}

template<typename T>
std::vector<idx_t> get_indexes(Filter filter,  Type_e requested_type, const Data & d) {
    auto & data = d.pt_data.get_data<T>();
    std::vector<idx_t> indexes;
    if(filter.op != HAVING) {
        indexes = filtered_indexes(data, build_clause<T>({filter})); // filtered.get_offsets();
    } else {
        indexes = make_query(nt::static_data::get()->typeByCaption(filter.object), filter.value, d);
    }

    Type_e current = filter.navitia_type;
    std::map<Type_e, Type_e> path = find_path(requested_type);
    while(path[current] != current){
        indexes = d.pt_data.get_target_by_source(current, path[current], indexes);
        current = path[current];
    }

    return indexes;
}


std::vector<Filter> parse(std::string request){
    std::string::iterator begin = request.begin();
    std::vector<Filter> filters;
    select_r<std::string::iterator> s;
    if (qi::phrase_parse(begin, request.end(), s, qi::space, filters)) {
        if(begin != request.end()) {
            std::string unparsed(begin, request.end());
            ptref_parsing_error error;
            error.type = ptref_parsing_error::partial_error;
            error.more = unparsed;
            throw error;
        }
    } else {
        ptref_parsing_error error;
        error.type = ptref_parsing_error::global_error;
        throw error;
    }
    return filters;
}


std::vector<idx_t> make_query(Type_e requested_type, std::string request, const Data & data) {
    std::vector<Filter> filters;

    if(!request.empty()){
	filters = parse(request);
    }

    type::static_data * static_data = type::static_data::get();
    for(Filter & filter : filters){
        try {
            filter.navitia_type = static_data->typeByCaption(filter.object);
        } catch(...) {
            //ptref_unknown_object error;
            ptref_parsing_error error;
            error.more = filter.object;
            throw error;
        }
    }

    std::vector<idx_t> final_indexes = data.pt_data.get_all_index(requested_type);
    std::vector<idx_t> indexes;
    for(const Filter & filter : filters){
        switch(filter.navitia_type){
        case Type_e::eLine: indexes = get_indexes<Line>(filter, requested_type, data); break;
        case Type_e::eValidityPattern: indexes = get_indexes<ValidityPattern>(filter, requested_type, data); break;
        case Type_e::eRoute: indexes = get_indexes<Route>(filter, requested_type, data); break;
        case Type_e::eStopPoint: indexes = get_indexes<StopPoint>(filter, requested_type, data); break;
        case Type_e::eStopArea: indexes = get_indexes<StopArea>(filter, requested_type, data); break;
        case Type_e::eNetwork: indexes = get_indexes<Network>(filter, requested_type, data); break;
        case Type_e::eMode: indexes = get_indexes<Mode>(filter, requested_type, data); break;
        case Type_e::eModeType: indexes = get_indexes<ModeType>(filter, requested_type, data); break;
        case Type_e::eCity: indexes = get_indexes<City>(filter, requested_type, data); break;
        case Type_e::eConnection: indexes = get_indexes<Connection>(filter, requested_type, data); break;
        case Type_e::eRoutePoint: indexes = get_indexes<RoutePoint>(filter, requested_type, data); break;
        case Type_e::eCompany: indexes = get_indexes<Company>(filter, requested_type, data); break;
        default: break;
        }
        // Attention ! les structures doivent être triées !
        std::sort(indexes.begin(), indexes.end());
        std::vector<idx_t> tmp_indexes;
        std::back_insert_iterator< std::vector<idx_t> > it(tmp_indexes);
        std::set_intersection(final_indexes.begin(), final_indexes.end(), indexes.begin(), indexes.end(), it);
        final_indexes = tmp_indexes;
    }
    return final_indexes;
}


pbnavitia::Response query_pb(Type_e requested_type, std::string request, const int depth, const Data &data){
    std::vector<idx_t> final_indexes;
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PTREFERENTIAL);
    try {
        final_indexes = make_query(requested_type, request, data);
    } catch(ptref_parsing_error parse_error) {
        switch(parse_error.type){
        case ptref_parsing_error::error_type::partial_error:
            pb_response.set_error("PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + parse_error.more + "<<");
            break;
        case ptref_parsing_error::error_type::unknown_object:
            pb_response.set_error("Objet NAViTiA inconnu : " + parse_error.more);
            break;
        case ptref_parsing_error::error_type::global_error:
            pb_response.set_error("PTReferential : Impossible de parser la requête");
            break;
        }

        return pb_response;
    }

    final_indexes = make_query(requested_type, request, data);
    return extract_data(data, requested_type, final_indexes, depth);
}




}} // navitia::ptref
