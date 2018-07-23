/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "ptreferential_legacy.h"
#include "ptreferential_utils.h"
#include "reflexion.h"
#include "where.h"
#include "proximity_list/proximity_list.h"
#include "type/data.h"

#include <algorithm>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_optional.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 105700
#include <boost/phoenix/object/construct.hpp>
#else
#include <boost/spirit/home/phoenix/object/construct.hpp>
#endif
#include <boost/date_time.hpp>
#include <boost/date_time/time_duration.hpp>
#include "type/pt_data.h"
#include "type/meta_data.h"
#include "routing/dataraptor.h"
#include <boost/range/adaptors.hpp>


namespace bt = boost::posix_time;

namespace navitia{ namespace ptref{
using namespace navitia::type;

const char* ptref_error::what() const noexcept {
    return this->more.c_str();
}
parsing_error::~parsing_error() noexcept {}

namespace qi = boost::spirit::qi;

/// Fonction qui va lire une chaîne de caractère et remplir un vector de Filter
template <typename Iterator>
struct select_r: qi::grammar<Iterator, std::vector<Filter>(), qi::space_type>
{
    qi::rule<Iterator, std::string(), qi::space_type> word, text; // Match a simple word, a string escaped by "" or enclosed by ()
    qi::rule<Iterator, std::string()> escaped_string, bracket_string;
    qi::rule<Iterator, Operator_e(), qi::space_type> bin_op; // Match a binary operator like <, =...
    qi::rule<Iterator, std::vector<Filter>(), qi::space_type> filter; // the complete string to parse
    qi::rule<Iterator, Filter(), qi::space_type> single_clause, having_clause, after_clause, method_clause;
    qi::rule<Iterator, std::vector<std::string>(), qi::space_type> args_clause;

    select_r() : select_r::base_type(filter) {
        // Warning, the '-' in a qi::char_ can have a particular meaning as 'a-z'
        word = qi::lexeme[+(qi::alnum|qi::char_("_:\x7c-"))];
        text = qi::lexeme[+(qi::alnum|qi::char_("_:=.<>\x7c ")|qi::char_("-"))];

        escaped_string = '"'
                        >>  *((qi::char_ - qi::char_('\\') - qi::char_('"')) | ('\\' >> qi::char_))
                        >> '"';

        bracket_string = '(' >> qi::lexeme[+(qi::alnum|qi::char_("_:=.<> \x7c")|qi::char_("-")|qi::char_(","))] >> ')';
        bin_op =  qi::string("<=")[qi::_val = LEQ]
                | qi::string(">=")[qi::_val = GEQ]
                | qi::string("<>")[qi::_val = NEQ]
                | qi::string("<") [qi::_val = LT]
                | qi::string(">") [qi::_val = GT]
                | qi::string("=") [qi::_val = EQ]
                | qi::string("DWITHIN") [qi::_val = DWITHIN];

        single_clause = (word >> "." >> word >> bin_op >> (word|escaped_string|bracket_string))
            [qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2, qi::_3, qi::_4)];
        having_clause = (word >> "HAVING" >> bracket_string)
            [qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2)];
        after_clause = ("AFTER(" >> text >> ')')[qi::_val = boost::phoenix::construct<Filter>(qi::_1)];
        args_clause = (word|escaped_string|bracket_string)[boost::phoenix::push_back(qi::_val, qi::_1)] % ',';
        // args_clause is optional.
        // Example          : R"(vehicle_journey.has_headsign("john"))", args_clause = john
        //                  : R"(vehicle_journey.has_disruption())", args_clause = ""
        method_clause = (word >> "." >> word >> "("  >> -(args_clause) >> ")" )
            [qi::_val = boost::phoenix::construct<Filter>(qi::_1, qi::_2, qi::_3)];
        filter %= (single_clause | having_clause | after_clause | method_clause)
            % (qi::lexeme["and"] | qi::lexeme["AND"]);
    }

};


// WHERE condition check
template<class T>
WhereWrapper<T> build_clause(std::vector<Filter> filters) {
    WhereWrapper<T> wh(new BaseWhere<T>());
    for(const Filter & filter : filters) {
        try {
            if(filter.attribute == "uri") {
                wh = wh && WHERE(ptr_uri<T>(), filter.op, filter.value);
            } else if(filter.attribute == "name") {
                wh = wh && WHERE(ptr_name<T>(), filter.op, filter.value);
            } else if(filter.attribute == "code") {
                wh = wh && WHERE(ptr_code<T>(), filter.op, filter.value);
            } else {
                throw parsing_error(parsing_error::partial_error, "unhandled filter type: " + filter.attribute);
            }
        } catch (unknown_member) {
            throw parsing_error(parsing_error::partial_error, "given object has no member: " + filter.attribute);
        }
    }
    return wh;
}



template<typename T, typename C>
struct ClauseType {
    static bool is_clause_tested(typename T::const_reference e, const C & clause){
        return clause(*e);
    }
};

using vec_impact_wptr = std::vector<boost::weak_ptr<navitia::type::disruption::Impact>>;

// Partial specialization for boost::weak_ptr<navitia::type::disruption::Impact>
template<typename C>
struct ClauseType<vec_impact_wptr, C> {
    static bool is_clause_tested(vec_impact_wptr::const_reference e, const C & clause){
        auto impact_sptr = e.lock();
        if (impact_sptr) {
            return clause(*impact_sptr);
        }
        return false;
    }
};

template<typename C>
struct GetterType {
    static typename C::const_reference get(const C& c, size_t i) {
        return c[i];
    }
};

template<typename T>
struct GetterType<ObjFactory<T>> {
    static const T* get(const ObjFactory<T>& c, size_t i) {
        return c[Idx<T>(i)];
    }
};

template<typename T, typename C>
Indexes filtered_indexes(const T& data, const C& clause) {
    Indexes result;
    for (size_t i = 0; i < data.size(); ++i) {
        if (ClauseType<T, C>::is_clause_tested(GetterType<T>::get(data, i), clause)) {
            result.insert(i);
        }
    }
    return result;
}

template<typename T>
Indexes get_indexes(Filter filter,  Type_e requested_type, const Data & d) {
    Indexes indexes;
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
            indexes = get_within(filter.navitia_type, coord, distance, d);
        }
    }

    else if( filter.op == HAVING ) {
        indexes = make_query(type_by_caption(filter.object), filter.value, d);
    } else if(filter.op == AFTER) {
        //this does only work with jpp
        if (filter.object == "journey_pattern_point") {
            // Getting the jpps from the request
            const auto& first_jpps = make_query(type_by_caption(filter.object), filter.value, d);

            // We get the jpps after the ones of the request
            for (const auto& first_jpp: first_jpps) {
                const auto& jpp = d.dataRaptor->jp_container.get(routing::JppIdx(first_jpp));
                const auto& jp = d.dataRaptor->jp_container.get(jpp.jp_idx);
                for (const auto& other_jpp_idx : jp.jpps) {
                    const auto& other_jpp = d.dataRaptor->jp_container.get(other_jpp_idx);
                    if (other_jpp.order > jpp.order) {
                        indexes.insert(other_jpp_idx.val); // TODO bulk insert ? (I don't think it's usefull)
                    }
                }
            }
        }
    } else if(! filter.method.empty()) {
        if (filter.object == "vehicle_journey"
            && filter.method == "has_headsign"
            && filter.args.size() == 1) {
            for (const VehicleJourney* vj: d.pt_data->headsign_handler.get_vj_from_headsign(filter.args.at(0))) {
                indexes.insert(vj->idx); //TODO bulk insert ?
            }
        } else if ((filter.object == "vehicle_journey")
                   && (filter.method == "has_disruption")
                   && (filter.args.size() == 0)) {
                // For traffic_report api, get indexes of VehicleJourney impacted only.
            indexes = get_indexes_by_impacts(type::Type_e::VehicleJourney, d);
        } else if (filter.method == "has_code" && filter.args.size() == 2) {
            indexes = get_indexes_from_code(filter.navitia_type, filter.args.at(0), filter.args.at(1), d);
        } else {
            throw parsing_error(parsing_error::partial_error,
                                "Unknown method " + filter.object + ":" + filter.method);
        }
    } else if (filter.attribute == "uri" && filter.op == EQ) {
        // for filtering with uri we can look up in the maps
        indexes = get_indexes_from_id(filter.navitia_type, filter.value, d);
    } else {
        const auto& data = d.get_data<T>();
        indexes = filtered_indexes(data, build_clause<T>({filter}));
    }
    return get_corresponding(indexes, filter.navitia_type, requested_type, d);
}

std::vector<Filter> parse(std::string request){
    std::string::iterator begin = request.begin();
    std::vector<Filter> filters;
    // the spirit parser does not depend of the data, it can be static
    static const select_r<std::string::iterator> s;
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

Indexes manage_odt_level(const Indexes& final_indexes,
                         const navitia::type::Type_e requested_type,
                         const navitia::type::OdtLevel_e odt_level,
                         const type::Data & data) {
    if ((!final_indexes.empty()) && (requested_type == navitia::type::Type_e::Line)) {
        Indexes odt_level_idx;
        for(const idx_t idx : final_indexes){
            const navitia::type::Line* line = data.pt_data->lines[idx];
            navitia::type::hasOdtProperties odt_property = line->get_odt_properties();
            switch(odt_level){
                case navitia::type::OdtLevel_e::scheduled:
                    if (odt_property.is_scheduled()){
                        odt_level_idx.insert(idx);
                    };
                    break;
                case navitia::type::OdtLevel_e::with_stops:
                    if (odt_property.is_with_stops()){
                        odt_level_idx.insert(idx);
                    };
                    break;
                case navitia::type::OdtLevel_e::zonal:
                    if (odt_property.is_zonal()){
                        odt_level_idx.insert(idx);
                    };
                    break;
                case navitia::type::OdtLevel_e::all:
                    break;
            }
        }
        return odt_level_idx;
    }
    return final_indexes;
}

Indexes make_query_legacy(const Type_e requested_type,
                          const std::string& request,
                          const std::vector<std::string>& forbidden_uris,
                          const type::OdtLevel_e odt_level,
                          const boost::optional<boost::posix_time::ptime>& since,
                          const boost::optional<boost::posix_time::ptime>& until,
                          const Data& data) {
    std::vector<Filter> filters;

    if(!request.empty()){
        filters = parse(request);
    }

    type::static_data* static_data = type::static_data::get();
    for(Filter & filter : filters){
        filter.navitia_type = type_by_caption(filter.object);
    }

    Indexes final_indexes;
    if (! data.get_nb_obj(requested_type)) {
        throw ptref_error("Filters: No requested object in the database");
    }

    if (filters.empty()) {
        final_indexes = data.get_all_index(requested_type);
    } else {
        Indexes indexes;
        bool first_time = true;
        for (const Filter& filter : filters) {
            switch(filter.navitia_type){
    #define GET_INDEXES(type_name, collection_name)\
            case Type_e::type_name:\
                indexes = get_indexes<type_name>(filter, requested_type, data);\
                break;
            ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
    #undef GET_INDEXES
            case Type_e::JourneyPattern:
                indexes = get_indexes<routing::JourneyPattern>(filter, requested_type, data);
                break;
            case Type_e::JourneyPatternPoint:
                indexes = get_indexes<routing::JourneyPatternPoint>(filter, requested_type, data);
                break;
            case Type_e::POI:
                indexes = get_indexes<georef::POI>(filter, requested_type, data);
                break;
            case Type_e::POIType:
                indexes = get_indexes<georef::POIType>(filter, requested_type, data);
                break;
            case Type_e::Connection:
                indexes = get_indexes<type::StopPointConnection>(filter, requested_type, data);
                break;
            case Type_e::MetaVehicleJourney:
                indexes = get_indexes<type::MetaVehicleJourney>(filter, requested_type, data);
                break;
            case Type_e::Impact:
                indexes = get_indexes<type::disruption::Impact>(filter, requested_type, data);
                break;
            default:
                throw parsing_error(parsing_error::partial_error,
                        "Filter: Unable to find the requested type. Not parsed: >>"
                        + nt::static_data::get()->captionByType(filter.navitia_type) + "<<");
            }
            if (first_time) {
                final_indexes = indexes;
            } else {
                final_indexes = get_intersection(final_indexes, indexes);
            }
            first_time = false;
        }
    }
    //We now filter with forbidden uris
    for(const auto forbidden_uri : forbidden_uris) {
        const auto type_ = data.get_type_of_id(forbidden_uri);
        //We don't use unknown forbidden type object as a filter.
        if (type_==navitia::type::Type_e::Unknown)
            continue;
        std::string caption_type;
        try {
            caption_type = static_data->captionByType(type_);
        } catch(std::out_of_range&) {
            throw parsing_error(parsing_error::error_type::unknown_object,
                    "Filter Unknown object type: " + forbidden_uri);
        }

        Filter filter_forbidden(caption_type, "uri", Operator_e::EQ, forbidden_uri);
        filter_forbidden.navitia_type = type_;
        Indexes forbidden_idx;
        switch(type_){
#define GET_INDEXES_FORBID(type_name, collection_name)\
        case Type_e::type_name:\
            forbidden_idx = get_indexes<type_name>(filter_forbidden, requested_type, data);\
            break;
            ITERATE_NAVITIA_PT_TYPES(GET_INDEXES_FORBID)
#undef GET_INDEXES_FORBID
        case Type_e::JourneyPattern:
            forbidden_idx = get_indexes<routing::JourneyPattern>(filter_forbidden, requested_type, data);
            break;
        case Type_e::JourneyPatternPoint:
            forbidden_idx = get_indexes<routing::JourneyPatternPoint>(filter_forbidden, requested_type, data);
            break;
        case Type_e::POI:
            forbidden_idx = get_indexes<georef::POI>(filter_forbidden, requested_type, data);
            break;
        case Type_e::POIType:
            forbidden_idx = get_indexes<georef::POIType>(filter_forbidden, requested_type, data);
            break;
        case Type_e::Connection:
            forbidden_idx = get_indexes<type::StopPointConnection>(filter_forbidden, requested_type, data);
            break;
        default:
            throw parsing_error(parsing_error::partial_error,
                                "Filter: Unable to find the requested type. Not parsed: >>"
                                + nt::static_data::get()->captionByType(filter_forbidden.navitia_type)
                                + "<<");
        }
        final_indexes = get_difference(final_indexes, forbidden_idx);
    }
    // Manage OdtLevel
    if (odt_level != navitia::type::OdtLevel_e::all) {
        final_indexes = manage_odt_level(final_indexes, requested_type, odt_level, data);
    }

    // filter on validity periods
    if (since || until) {
        final_indexes = filter_on_period(final_indexes, requested_type, since, until, data);
    }

    // When the filters have emptied the results
    if(final_indexes.empty()){
        throw ptref_error("Filters: Unable to find object");
    }
    auto sort_networks = [&](type::idx_t n1_, type::idx_t n2_) {
        const Network & n1 = *(data.pt_data->networks[n1_]);
        const Network & n2 = *(data.pt_data->networks[n2_]);
        return n1 < n2;
    };

    auto sort_lines = [&](type::idx_t l1_, type::idx_t l2_) {
        const Line & l1 = *(data.pt_data->lines[l1_]);
        const Line & l2 = *(data.pt_data->lines[l2_]);
        return l1 < l2;
    };

    switch(requested_type){
        case Type_e::Network:
            std::sort(final_indexes.begin(), final_indexes.end(), sort_networks);
            break;
        case Type_e::Line:
            std::sort(final_indexes.begin(), final_indexes.end(), sort_lines);
            break;
        default:
            break;
    }

    return final_indexes;
}

}} // navitia::ptref
