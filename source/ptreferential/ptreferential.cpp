#include "ptreferential.h"
#include "reflexion.h"
#include "where.h"
#include "type/pb_converter.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

//TODO: change!
using namespace navitia::type;
using namespace navitia::ptref;

namespace qi = boost::spirit::qi;

// Un filter est du type stop_area.external_code = "kikoolol"
struct Filter {
    navitia::type::Type_e navitia_type; //< Le type parsé
    std::string object; //< L'objet sous forme de chaîne de caractère ("stop_area")
    std::string attribute; //< L'attribu ("external code")
    Operator_e op; //< la comparaison ("=")
    std::string value; //< la valeur comparée ("kikoolol")
};

// Wrapper permettant de remplir directement de type avec boost::spirit
BOOST_FUSION_ADAPT_STRUCT(
    Filter,
    (std::string, object)
    (std::string, attribute)
    (Operator_e, op)
    (std::string, value)
)


/// Fonction qui va lire une chaîne de caractère et remplir un vector de Filter
        template <typename Iterator>
        struct select_r
            : qi::grammar<Iterator, std::vector<Filter>(), qi::space_type>
{
    qi::rule<Iterator, std::string(), qi::space_type> txt; // Match une string
    qi::rule<Iterator, Operator_e(), qi::space_type> bin_op; // Match une operator binaire telle que <, =...
    qi::rule<Iterator, std::vector<Filter>(), qi::space_type> filter; // La string complète à parser

    select_r() : select_r::base_type(filter) {
        txt %= qi::lexeme[+(qi::alnum|'_'|'|'|':'|'-')]; // Match du texte

        bin_op =  qi::string("<=")[qi::_val = LEQ]
                | qi::string(">=")[qi::_val = GEQ]
                | qi::string("<>")[qi::_val = NEQ]
                | qi::string("<") [qi::_val = LT]
                | qi::string(">") [qi::_val = GT]
                | qi::string("=") [qi::_val = EQ];

        filter %= (txt >> "." >> txt >> bin_op >> txt) % qi::lexeme["and"];
    }

};

namespace navitia{ namespace ptref{

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



pbnavitia::Response extract_data(Data & data, Type_e requested_type, std::vector<idx_t> & rows) {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::PTREFERENTIAL);
    pbnavitia::PTReferential * pb_response = result.mutable_ptref();

    for(idx_t idx : rows){
        switch(requested_type){
        case Type_e::eLine: fill_pb_object(idx, data, pb_response->add_line(), 1); break;
        case Type_e::eRoute: fill_pb_object(idx, data, pb_response->add_route(), 1); break;
        case Type_e::eStopPoint: fill_pb_object(idx, data, pb_response->add_stop_point(), 1); break;
        case Type_e::eStopArea: fill_pb_object(idx, data, pb_response->add_stop_area(), 1); break;
        case Type_e::eNetwork: fill_pb_object(idx, data, pb_response->add_network(), 1); break;
        case Type_e::eMode: fill_pb_object(idx, data, pb_response->add_commercial_mode(), 1); break;
        case Type_e::eModeType: fill_pb_object(idx, data, pb_response->add_physical_mode(), 1); break;
        case Type_e::eCity: fill_pb_object(idx, data, pb_response->add_city(), 1); break;
        case Type_e::eConnection: fill_pb_object(idx, data, pb_response->add_connection(), 1); break;
        case Type_e::eRoutePoint: fill_pb_object(idx, data, pb_response->add_route_point(), 1); break;
        case Type_e::eCompany: fill_pb_object(idx, data, pb_response->add_company(), 1); break;
        default: break;
        }
    }
    return result;
}


template<typename T, typename C>
std::vector<idx_t> filtered_indexes(const std::vector<T> & data, const C & clause){
    std::vector<idx_t> result;
    for(size_t i = 0; i < data.size(); ++i){
        if(clause(data[i]))
            result.push_back(i);
    }
    return result;
}

template<typename T>
std::vector<idx_t> get_indexes(Filter filter,  Type_e requested_type, Data & d) {
    auto & data = d.pt_data.get_data<T>();

    std::vector<idx_t> indexes = filtered_indexes(data, build_clause<T>({filter})); // filtered.get_offsets();

    Type_e current = filter.navitia_type;
    std::map<Type_e, Type_e> path = find_path(requested_type);
    while(path[current] != current){
        indexes = d.pt_data.get_target_by_source(current, path[current], indexes);
        current = path[current];
    }
    return indexes;
}


pbnavitia::Response query(Type_e requested_type, std::string request, Data & data){
    std::string::iterator begin = request.begin();
    pbnavitia::Response pb_response;
    std::vector<Filter> filters;
    select_r<std::string::iterator> s;

    if(!request.empty()){
        if (qi::phrase_parse(begin, request.end(), s, qi::space, filters)) {
            if(begin != request.end()) {
                std::string unparsed(begin, request.end());
                pb_response.set_error("PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + unparsed + "<<");
                return pb_response;
            }
        } else {
            pb_response.set_error("PTReferential : Impossible de parser la requête");
            return pb_response;
        }
    }

    type::static_data * static_data = type::static_data::get();
    for(Filter & filter : filters){
        try {
            filter.navitia_type = static_data->typeByCaption(filter.object);
        } catch(...) {
            pb_response.set_error("Objet NAViTiA inconnu : " + filter.object);
            return pb_response;
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
        std::vector<idx_t> tmp_indexes;
        std::back_insert_iterator< std::vector<idx_t> > it(tmp_indexes);
        std::set_intersection(final_indexes.begin(), final_indexes.end(), indexes.begin(), indexes.end(), it);
        final_indexes = tmp_indexes;
    }

    return extract_data(data, requested_type, final_indexes);
}


}} // navitia::ptref
