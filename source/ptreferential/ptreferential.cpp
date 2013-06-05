#include "ptreferential.h"
#include "type/data.h"
#include "reflexion.h"
#include "where.h"
#include "proximity_list/proximity_list.h"
#include "georef/pois.h"

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


namespace navitia{ namespace ptref{
using namespace navitia::type;

namespace qi = boost::spirit::qi;

/// Fonction qui va lire une chaîne de caractère et remplir un vector de Filter
        template <typename Iterator>
        struct select_r
            : qi::grammar<Iterator, std::vector<Filter>(), qi::space_type>
{
    qi::rule<Iterator, std::string(), qi::space_type> word, text; // Match un mot simple, une chaine échappée par des "" et entourée de ()
    qi::rule<Iterator, std::string()> escaped_string, bracket_string;
    qi::rule<Iterator, Operator_e(), qi::space_type> bin_op; // Match une operator binaire telle que <, =...
    qi::rule<Iterator, std::vector<Filter>(), qi::space_type> filter; // La string complète a parser
    qi::rule<Iterator, Filter(), qi::space_type> single_clause, having_clause; // La string complète à parser

    select_r() : select_r::base_type(filter) {
        // Attention, le - dans un qi::char_ *peut* avoir une signification particulière telle que a-z
        word = qi::lexeme[+(qi::alnum|qi::char_("_:\x7c-"))];
        text = qi::lexeme[+(qi::alnum|qi::char_("_:-=.<> \x7c"))];
        escaped_string = '"' >> qi::lexeme[+(qi::alnum|qi::char_("_: &.\x7c-"))] >> '"';
        bracket_string = '(' >> qi::lexeme[+(qi::alnum|qi::char_("_: &,.-"))] >> ')';
        bin_op =  qi::string("<=")[qi::_val = LEQ]
                | qi::string(">=")[qi::_val = GEQ]
                | qi::string("<>")[qi::_val = NEQ]
                | qi::string("<") [qi::_val = LT]
                | qi::string(">") [qi::_val = GT]
                | qi::string("=") [qi::_val = EQ]
                | qi::string("DWITHIN") [qi::_val = DWITHIN];

        single_clause = (word >> "." >> word >> bin_op >> (word|escaped_string|bracket_string))[qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2, qi::_3, qi::_4)];
        having_clause = (word >> "HAVING" >> '(' >> text >> ')')[qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2)];
        filter %= (single_clause | having_clause) % (qi::lexeme["and"] | qi::lexeme["AND"]);
    }

};


// vérification d'une clause WHERE
template<class T>
WhereWrapper<T> build_clause(std::vector<Filter> filters) {
    WhereWrapper<T> wh(new BaseWhere<T>());
    for(const Filter & filter : filters) {
        if(filter.attribute == "uri") {
            wh = wh && WHERE(ptr_uri<T>(), filter.op, filter.value);
        } else if(filter.attribute == "name") {
            wh = wh && WHERE(ptr_name<T>(), filter.op, filter.value);
        }
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
    auto & data = d.get_data<T>();
    std::vector<idx_t> indexes;
    if(filter.op == DWITHIN) {
        std::vector<std::string> splited;
        boost::algorithm::split(splited, filter.value, boost::algorithm::is_any_of(","));
        GeographicalCoord coord;
        float distance;
        if(splited.size() == 3) {
            try {
                std::string slon = boost::trim_copy(splited[0]);
                std::string slat = boost::trim_copy(splited[1]);
                std::string sdist = boost::trim_copy(splited[2]);
                coord = type::GeographicalCoord(boost::lexical_cast<double>(slon), boost::lexical_cast<double>(slat) );
                distance = boost::lexical_cast<float>(sdist);
            } catch (...) {
                throw parsing_error(parsing_error::partial_error, "Unable to parse the DWITHIN parameter " + filter.value);
            }
            std::vector<std::pair<idx_t, GeographicalCoord> > tmp;
            switch(filter.navitia_type){
            case Type_e::StopPoint: tmp = d.pt_data.stop_point_proximity_list.find_within(coord, distance); break;
            case Type_e::StopArea: tmp = d.pt_data.stop_area_proximity_list.find_within(coord, distance);break;
            case Type_e::POI: tmp = d.geo_ref.poi_proximity_list.find_within(coord, distance);break;
            default: throw ptref_error("The requested object can not be used a DWITHIN clause");
            }
            for(auto pair : tmp) {
                indexes.push_back(pair.first);
            }
        }
    }

    else if( filter.op == HAVING ) {
        indexes = make_query(nt::static_data::get()->typeByCaption(filter.object), filter.value, d);
    } else {
        indexes = filtered_indexes(data, build_clause<T>({filter}));
    }

    Type_e current = filter.navitia_type;
    std::map<Type_e, Type_e> path = find_path(requested_type);
    while(path[current] != current){
        indexes = d.get_target_by_source(current, path[current], indexes);
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
            throw parsing_error(parsing_error::partial_error, "Filter: Unable to parse the whole string. Not parsed: >>" + unparsed + "<<");
        }
    } else {
        throw parsing_error(parsing_error::global_error, "Filter: unable to parse " + request);
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
            throw parsing_error(parsing_error::error_type::unknown_object, "Filter Unknown object type: " + filter.object);
        }
    }

    std::vector<idx_t> final_indexes = data.get_all_index(requested_type);
    std::vector<idx_t> indexes;
    for(const Filter & filter : filters){
        switch(filter.navitia_type){
#define GET_INDEXES(type_name, collection_name) case Type_e::type_name: indexes = get_indexes<type_name>(filter, requested_type, data); break;
        ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
            case Type_e::POI: indexes = get_indexes<georef::POI>(filter, requested_type, data); break;
            case Type_e::POIType: indexes = get_indexes<georef::POIType>(filter, requested_type, data); break;
        default:
            throw parsing_error(parsing_error::partial_error,"Filter: Unable to find the requested type. Not parsed: >>" + nt::static_data::get()->captionByType(filter.navitia_type) + "<<");
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
