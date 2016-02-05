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

#include "ptreferential.h"
#include "reflexion.h"
#include "where.h"
#include "proximity_list/proximity_list.h"
#include "type/data.h"

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
        method_clause = (word >> "." >> word >> "(" >> args_clause >> ")")
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
                LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                        "unhandled filter type: " << filter.attribute << ". The filter is ignored");
            }
        } catch (unknown_member) {
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                    "given object has no member: " << filter.attribute << ". The filter is ignored");
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
std::vector<idx_t> filtered_indexes(const T& data, const C& clause) {
    std::vector<idx_t> result;
    for (size_t i = 0; i < data.size(); ++i) {
        if (ClauseType<T, C>::is_clause_tested(GetterType<T>::get(data, i), clause)) {
            result.push_back(i);
        }
    }
    return result;
}

template<typename T>
static
typename boost::enable_if<
    typename boost::mpl::contains<
        nt::CodeContainer::SupportedTypes,
        T>::type,
    std::vector<idx_t>>::type
get_indexes_from_code(const Data& d, const std::string& key, const std::string& value) {
    std::vector<idx_t> res;
    for (const auto* obj: d.pt_data->codes.get_objs<T>(key, value)) {
        res.push_back(obj->idx);
    }
    return res;
}
template<typename T>
static
typename boost::disable_if<
    typename boost::mpl::contains<
        nt::CodeContainer::SupportedTypes,
        T>::type,
    std::vector<idx_t>>::type
get_indexes_from_code(const Data&, const std::string&, const std::string&) {
    // there is no codes for unsupporded types, thus the result is empty
    return {};
}

template<typename T>
std::vector<idx_t> get_indexes(Filter filter,  Type_e requested_type, const Data & d) {
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
            case Type_e::StopPoint: tmp = d.pt_data->stop_point_proximity_list.find_within(coord, distance); break;
            case Type_e::StopArea: tmp = d.pt_data->stop_area_proximity_list.find_within(coord, distance);break;
            case Type_e::POI: tmp = d.geo_ref->poi_proximity_list.find_within(coord, distance);break;
            default: throw ptref_error("The requested object can not be used a DWITHIN clause");
            }
            for(auto pair : tmp) {
                indexes.push_back(pair.first);
            }
        }
    }

    else if( filter.op == HAVING ) {
        indexes = make_query(nt::static_data::get()->typeByCaption(filter.object), filter.value, d);
    } else if(filter.op == AFTER) {
        // Getting the jpps from the request
        const auto& first_jpps = make_query(nt::static_data::get()->typeByCaption(filter.object), filter.value, d);

        // We get the jpps after the ones of the request
        for (const auto& first_jpp: first_jpps) {
            const auto& jpp = d.dataRaptor->jp_container.get(routing::JppIdx(first_jpp));
            const auto& jp = d.dataRaptor->jp_container.get(jpp.jp_idx);
            for (const auto& other_jpp_idx : jp.jpps) {
                const auto& other_jpp = d.dataRaptor->jp_container.get(other_jpp_idx);
                if (other_jpp.order > jpp.order) {
                    indexes.push_back(other_jpp_idx.val);
                }
            }
        }
    } else if(! filter.method.empty()) {
        if (filter.object == "vehicle_journey"
            && filter.method == "has_headsign"
            && filter.args.size() == 1) {
            for (const VehicleJourney* vj: d.pt_data->headsign_handler.get_vj_from_headsign(filter.args.at(0))) {
                indexes.push_back(vj->idx);
            }
        } else if (filter.method == "has_code" && filter.args.size() == 2) {
            indexes = get_indexes_from_code<T>(d, filter.args.at(0), filter.args.at(1));
        } else {
            throw parsing_error(parsing_error::partial_error,
                                "Unknown method " + filter.object + ":" + filter.method);
        }
    } else if (filter.object == "journey_pattern" && filter.op == EQ &&
               in(filter.attribute, {"uri", "name"})) {
        if (const auto jp_idx = d.dataRaptor->jp_container.get_jp_from_id(filter.value)) {
            indexes.push_back(jp_idx->val);
        }
    } else if (filter.object == "journey_pattern_point" && filter.op == EQ &&
               in(filter.attribute, {"uri", "name"})) {
        if (const auto jpp_idx = d.dataRaptor->jp_container.get_jpp_from_id(filter.value)) {
            indexes.push_back(jpp_idx->val);
        }
    } else {
        const auto& data = d.get_data<T>();
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

static std::vector<type::idx_t>::iterator
sort_and_ge_new_end(std::vector<type::idx_t>& list_idx){
    std::sort(list_idx.begin(), list_idx.end());
    return std::unique(list_idx.begin(), list_idx.end());
}

std::vector<idx_t> get_difference(std::vector<type::idx_t>& list_idx1,
                                 std::vector<type::idx_t>& list_idx2){
   std::vector<type::idx_t>::iterator new_end_1 = sort_and_ge_new_end(list_idx1);
   std::vector<type::idx_t>::iterator new_end_2 = sort_and_ge_new_end(list_idx2);

   std::vector<idx_t> tmp_indexes;
   std::back_insert_iterator< std::vector<idx_t> > it(tmp_indexes);
   std::set_difference(list_idx1.begin(), new_end_1,
           list_idx2.begin(), new_end_2, it);
   return tmp_indexes;
}

std::vector<type::idx_t> get_intersection(std::vector<type::idx_t>& list_idx1,
                                      std::vector<type::idx_t>& list_idx2){
   std::vector<type::idx_t>::iterator new_end_1 = sort_and_ge_new_end(list_idx1);
   std::vector<type::idx_t>::iterator new_end_2 = sort_and_ge_new_end(list_idx2);

   std::vector<idx_t> tmp_indexes;
   std::back_insert_iterator< std::vector<idx_t> > it(tmp_indexes);
   std::set_intersection(list_idx1.begin(), new_end_1, list_idx2.begin(), new_end_2, it);
   return tmp_indexes;
}

std::vector<idx_t> manage_odt_level(const std::vector<type::idx_t>& final_indexes,
                                          const navitia::type::Type_e requested_type,
                                          const navitia::type::OdtLevel_e odt_level,
                                          const type::Data & data){
    if((!final_indexes.empty()) && (requested_type == navitia::type::Type_e::Line)
            && (odt_level != navitia::type::OdtLevel_e::all)){
        std::vector<idx_t> odt_level_idx;
        for(const idx_t idx : final_indexes){
            const navitia::type::Line* line = data.pt_data->lines[idx];
            navitia::type::hasOdtProperties odt_property = line->get_odt_properties();
            switch(odt_level){
                case navitia::type::OdtLevel_e::scheduled:
                    if (odt_property.is_scheduled()){
                        odt_level_idx.push_back(idx);
                    };
                    break;
                case navitia::type::OdtLevel_e::with_stops:
                    if (odt_property.is_with_stops()){
                        odt_level_idx.push_back(idx);
                    };
                    break;
                case navitia::type::OdtLevel_e::zonal:
                    if (odt_property.is_zonal()){
                        odt_level_idx.push_back(idx);
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

static bool keep_vj(const nt::VehicleJourney* vj,
                    const bt::time_period& period) {
    if (vj->stop_time_list.empty()) {
        return false; //no stop time, so it cannot be valid
    }
    const auto& first_departure_dt = vj->stop_time_list.front().departure_time;

    for (boost::gregorian::day_iterator it(period.begin().date()); it <= period.last().date(); ++it) {
        if (! vj->base_validity_pattern()->check(*it)) { continue; }
        bt::ptime vj_dt = bt::ptime(*it, bt::seconds(first_departure_dt));
        if (period.contains(vj_dt)) { return true; }
    }

    return false;
}

static std::vector<idx_t>
filter_vj_on_period(const std::vector<type::idx_t>& indexes,
                    const  bt::time_period& period,
                    const type::Data& data) {

    std::vector<idx_t> res;
    for (const idx_t idx: indexes) {
        const auto* vj = data.pt_data->vehicle_journeys[idx];
        if (! keep_vj(vj, period)) { continue; }
        res.push_back(idx);
    }
    return res;
}

static std::vector<idx_t>
filter_impact_on_period(const std::vector<type::idx_t>& indexes,
                    const  bt::time_period& period,
                    const type::Data& data) {

    std::vector<idx_t> res;
    for (const idx_t idx: indexes) {
        auto impact = data.pt_data->disruption_holder.get_weak_impacts()[idx].lock();

        // to keep an impact, we want the intersection between its application periods
        // and the period to be non empy
        for (const auto& application_period: impact->application_periods) {
            if (application_period.intersection(period).is_null()) { continue; }
            res.push_back(idx);
            break;
        }
    }
    return res;
}

static std::vector<idx_t>
filter_on_period(const std::vector<type::idx_t>& indexes,
                 const navitia::type::Type_e requested_type,
                 const boost::optional<boost::posix_time::ptime>& since,
                 const boost::optional<boost::posix_time::ptime>& until,
                 const type::Data& data) {

    // we create the right period using since, until and the production period
    if (since && until && until < since) {
        throw ptref_error("invalid filtering period");
    }
    auto start = bt::ptime(data.meta->production_date.begin());
    auto end = bt::ptime(data.meta->production_date.end());

    if (since) {
        if (data.meta->production_date.is_before(since->date())) {
            throw ptref_error("invalid filtering period, not in production period");
        }
        if (since->date() >= data.meta->production_date.begin()) {
            start = *since;
        }
    }
    if (until) {
        if (data.meta->production_date.is_after(until->date())) {
            throw ptref_error("invalid filtering period, not in production period");
        }
        if (until->date() <= data.meta->production_date.last()) {
            end = *until;
        }
    }
    // we want end to be in the period, so we add one seconds
    bt::time_period period {start, end + bt::seconds(1)};

    switch (requested_type) {
    case nt::Type_e::VehicleJourney:
        return filter_vj_on_period(indexes, period, data);
    case nt::Type_e::Impact:
        return filter_impact_on_period(indexes, period, data);
    default:
        throw parsing_error(parsing_error::error_type::global_error,
                            "cannot filter on validity period for this type");
    }
}

std::vector<idx_t> make_query(const Type_e requested_type,
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

    type::static_data * static_data = type::static_data::get();
    for(Filter & filter : filters){
        try {
            filter.navitia_type = static_data->typeByCaption(filter.object);
        } catch(...) {
            throw parsing_error(parsing_error::error_type::unknown_object,
                    "Filter Unknown object type: " + filter.object);
        }
    }
    std::vector<idx_t> final_indexes = data.get_all_index(requested_type);
    // When we have no objets asked(like for example companies)
    if(final_indexes.empty()){
        throw ptref_error("Filters: No requested object in the database");
    }

    std::vector<idx_t> indexes;
    for(const Filter & filter : filters){
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
        final_indexes = get_intersection(final_indexes, indexes);
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
        std::vector<idx_t> forbidden_idx;
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
    final_indexes = manage_odt_level(final_indexes, requested_type, odt_level, data);

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

std::vector<type::idx_t> make_query(const type::Type_e requested_type,
                                    const std::string& request,
                                    const std::vector<std::string>& forbidden_uris,
                                    const type::Data &data) {
    return make_query(requested_type,
                      request,
                      forbidden_uris,
                      type::OdtLevel_e::all,
                      boost::none,
                      boost::none,
                      data);
}

std::vector<type::idx_t> make_query(const type::Type_e requested_type,
                                    const std::string& request,
                                    const type::Data &data) {
    const std::vector<std::string> forbidden_uris;
    return make_query(requested_type, request, forbidden_uris, data);
}

}} // navitia::ptref
