#include "ptreferential.h"
#include "reflexion.h"
#include "where.h"
#include "proximity_list/proximity_list.h"

#include <algorithm>
#include <regex>

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
    qi::rule<Iterator, std::string(), qi::space_type> txt, txt2; // Match une string
    qi::rule<Iterator, std::string()> txt3;
    qi::rule<Iterator, float, qi::space_type> fl1; 
    qi::rule<Iterator, Operator_e(), qi::space_type> bin_op; // Match une operator binaire telle que <, =...
    qi::rule<Iterator, std::vector<Filter>(), qi::space_type> filter, pre_filter, a_filter; // La string complète a parser
    qi::rule<Iterator, Filter(), qi::space_type> filter1, filter2, filter3; // La string complète à parser

    select_r() : select_r::base_type(filter) {
        txt = qi::lexeme[+(qi::alnum|qi::char_("_:-"))];
        txt2 = qi::lexeme[+(qi::alnum|qi::char_("_:-=.<> "))];
        txt3 = '"' >> qi::lexeme[+(qi::alnum|qi::char_("_:- &"))] >> '"';
        bin_op =  qi::string("<=")[qi::_val = LEQ]
                | qi::string(">=")[qi::_val = GEQ]
                | qi::string("<>")[qi::_val = NEQ]
                | qi::string("<") [qi::_val = LT]
                | qi::string(">") [qi::_val = GT]
                | qi::string("=") [qi::_val = EQ];

        filter1 = (txt >> "." >> txt >> bin_op >> (txt|txt3))[qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2, qi::_3, qi::_4)];
        filter2 = (txt >> "HAVING" >> '(' >> txt2 >> ')')[qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2)];
        filter3 = ("AROUND"  >> qi::double_ >> ':' >> qi::double_ >> "WITHIN" >> qi::int_ ) [qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2, qi::_3)];
        pre_filter %= (filter1 | filter2) % (qi::lexeme["and"] | qi::lexeme["AND"]);
        a_filter = pre_filter >> -filter3;
        filter = (a_filter | filter3);
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
        else if(filter.attribute == "uri")
            wh = wh && WHERE(ptr_uri<T>(), filter.op, filter.value);
        else if(filter.attribute == "name")
            wh = wh && WHERE(ptr_name<T>(), filter.op, filter.value);
        }
    return wh;
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
    if(filter.op == AROUND) {
        type::GeographicalCoord coord(filter.lon, filter.lat);
        auto tmp = d.pt_data.stop_point_proximity_list.find_within(coord, filter.distance);
        for(auto idx_coord : tmp)
            indexes.push_back(idx_coord.first);
    } else if( filter.op == HAVING ) {
        indexes = make_query(nt::static_data::get()->typeByCaption(filter.object), filter.value, d);
    } else {
        indexes = filtered_indexes(data, build_clause<T>({filter}));
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
            error.more = "Filter: Unable to parse the whole string. Not parsed: >>" + unparsed + "<<";
            throw error;
        }
    } else {
        ptref_parsing_error error;
        error.type = ptref_parsing_error::global_error;
        error.more = "Filter: unable to parse";
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
            ptref_parsing_error error;
            error.type = ptref_parsing_error::error_type::unknown_object;
            error.more = "Filter Unknown object type: " + filter.object;
            throw error;
        }
    }

    std::vector<idx_t> final_indexes = data.pt_data.get_all_index(requested_type);
    std::vector<idx_t> indexes;
    for(const Filter & filter : filters){
        switch(filter.navitia_type){
        case Type_e::eLine: indexes = get_indexes<Line>(filter, requested_type, data); break;
        case Type_e::eValidityPattern: indexes = get_indexes<ValidityPattern>(filter, requested_type, data); break;
        case Type_e::eJourneyPattern: indexes = get_indexes<JourneyPattern>(filter, requested_type, data); break;
        case Type_e::eStopPoint: indexes = get_indexes<StopPoint>(filter, requested_type, data); break;
        case Type_e::eStopArea: indexes = get_indexes<StopArea>(filter, requested_type, data); break;
        case Type_e::eNetwork: indexes = get_indexes<Network>(filter, requested_type, data); break;
        case Type_e::ePhysicalMode: indexes = get_indexes<PhysicalMode>(filter, requested_type, data); break;
        case Type_e::eCommercialMode: indexes = get_indexes<CommercialMode>(filter, requested_type, data); break;
        case Type_e::eCity: indexes = get_indexes<City>(filter, requested_type, data); break;
        case Type_e::eConnection: indexes = get_indexes<Connection>(filter, requested_type, data); break;
        case Type_e::eJourneyPatternPoint: indexes = get_indexes<JourneyPatternPoint>(filter, requested_type, data); break;
        case Type_e::eCompany: indexes = get_indexes<Company>(filter, requested_type, data); break;
        case Type_e::eVehicleJourney : indexes = get_indexes<VehicleJourney>(filter, requested_type, data); break;
        case Type_e::eRoute: indexes = get_indexes<Route>(filter, requested_type, data); break;
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






}} // navitia::ptref
