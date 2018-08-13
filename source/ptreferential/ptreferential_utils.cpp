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

#include "ptreferential_utils.h"
#include "ptreferential.h"
#include "ptref_graph.h"
#include "type/message.h"
#include "type/data.h"
#include "type/pt_data.h"
#include "type/meta_data.h"
#include "routing/dataraptor.h"

#include <boost/range/algorithm/find.hpp>
#include <string>

using navitia::type::Indexes;
using navitia::type::Type_e;
using navitia::type::Data;
using navitia::type::make_indexes;

namespace bt = boost::posix_time;

namespace navitia {
namespace ptref {

Indexes get_difference(const Indexes& idxs1, const Indexes& idxs2) {
    Indexes tmp_indexes;
    std::insert_iterator<Indexes> it(tmp_indexes, std::begin(tmp_indexes));
    std::set_difference(std::begin(idxs1), std::end(idxs1), std::begin(idxs2), std::end(idxs2), it);
    return tmp_indexes;
}

Indexes get_intersection(const Indexes& idxs1, const Indexes& idxs2) {
    Indexes tmp_indexes;
    std::insert_iterator<Indexes> it(tmp_indexes, std::begin(tmp_indexes));
    std::set_intersection(std::begin(idxs1), std::end(idxs1), std::begin(idxs2), std::end(idxs2), it);
    return tmp_indexes;
}

Indexes get_corresponding(Indexes indexes, Type_e from, const Type_e to, const Data& data) {
    const std::map<Type_e, Type_e> path = find_path(to);
    while (path.at(from) != from) {
        indexes = data.get_target_by_source(from, path.at(from), indexes);
        from = path.at(from);
    }
    if (from != to) {
        // there was no path to find a requested type
        return Indexes{};
    }
    return indexes;
}

Type_e type_by_caption(const std::string& type) {
    type::static_data* static_data = type::static_data::get();
    try {
        return static_data->typeByCaption(type);
    } catch (...) {
        throw parsing_error(parsing_error::error_type::unknown_object,
                            "Filter Unknown object type: " + type);
    }
}

typedef std::pair<type::Type_e, Indexes> pair_indexes;
struct VariantVisitor: boost::static_visitor<pair_indexes>{
    const Data& d;
    VariantVisitor(const Data & d): d(d){}
    pair_indexes operator()(const type::disruption::UnknownPtObj){
        return {type::Type_e::Unknown, Indexes{}};
    }
    pair_indexes operator()(const type::Network*){
        return {type::Type_e::Network, Indexes{}};
    }
    pair_indexes operator()(const type::StopArea*){
        return {type::Type_e::StopArea, Indexes{}};
    }
    pair_indexes operator()(const type::StopPoint*){
        return {type::Type_e::StopPoint, Indexes{}};
    }
    pair_indexes operator()(const type::disruption::LineSection){
        return {type::Type_e::Unknown, Indexes{}};
    }
    pair_indexes operator()(const type::Line*){
        return {type::Type_e::Line, Indexes{}};
    }
    pair_indexes operator()(const type::Route*){
        return {type::Type_e::Route, Indexes{}};
    }
    pair_indexes operator()(const type::MetaVehicleJourney* meta_vj){
        return {type::Type_e::VehicleJourney, meta_vj->get(type::Type_e::VehicleJourney, *d.pt_data)};
    }
};

Indexes get_indexes_by_impacts(const type::Type_e& type_e, const Data& d) {
    Indexes result;
    VariantVisitor visit(d);
    const auto& impacts = d.pt_data->disruption_holder.get_weak_impacts();
    for(const auto& impact: impacts){
        const auto imp = impact.lock();
        if (!imp) {
            continue;
        }
        if (imp->severity->effect != type::disruption::Effect::NO_SERVICE) {
            continue;
        }
        for(const auto& entitie: imp->informed_entities()){
            auto pair_type_indexes = boost::apply_visitor(visit, entitie);
            if(type_e == pair_type_indexes.first){
                result.insert(pair_type_indexes.second.begin(), pair_type_indexes.second.end());
            }
        }
    }
    return result;
}

Indexes get_impacts_by_tags(const std::vector<std::string> & tag_uris,
                            const Data& d) {
    Indexes result;
    const auto& w_impacts = d.pt_data->disruption_holder.get_weak_impacts();

    for(size_t i=0; i<w_impacts.size(); i++) {
        auto impact = w_impacts[i].lock();
        if(impact && impact->disruption) {
            for(const auto& tag : impact->disruption->tags) {
                if(tag && boost::range::find(tag_uris, tag->uri) != tag_uris.end()) {
                    result.insert(i);
                }
            }
        }
    }

    return result;
}


static bool keep_vj(const type::VehicleJourney* vj,
                    const bt::time_period& period) {
    if (vj->stop_time_list.empty()) {
        return false; //no stop time, so it cannot be valid
    }

    const auto& first_departure_dt = vj->earliest_time();
    for (boost::gregorian::day_iterator it(period.begin().date()); it <= period.last().date(); ++it) {
        if (! vj->base_validity_pattern()->check(*it)) { continue; }
        bt::ptime vj_dt = bt::ptime(*it, bt::seconds(first_departure_dt));
        if (period.contains(vj_dt)) { return true; }
    }

    return false;
}

static Indexes
filter_vj_on_period(const Indexes& indexes,
                    const  bt::time_period& period,
                    const type::Data& data) {

    Indexes res;
    for (const idx_t idx: indexes) {
        const auto* vj = data.pt_data->vehicle_journeys[idx];
        if (! keep_vj(vj, period)) { continue; }
        res.insert(idx);
    }
    return res;
}

static Indexes
filter_impact_on_period(const Indexes& indexes,
                    const  bt::time_period& period,
                    const type::Data& data) {

    Indexes res;
    for (const idx_t idx: indexes) {
        auto impact = data.pt_data->disruption_holder.get_weak_impacts()[idx].lock();

        if (! impact) { continue; }

        // to keep an impact, we want the intersection between its application periods
        // and the period to be non empy
        for (const auto& application_period: impact->application_periods) {
            if (application_period.intersection(period).is_null()) { continue; }
            res.insert(idx);
            break;
        }
    }
    return res;
}

Indexes
filter_on_period(const Indexes& indexes,
                 const navitia::type::Type_e requested_type,
                 const boost::optional<bt::ptime>& since,
                 const boost::optional<bt::ptime>& until,
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
        throw parsing_error(parsing_error::error_type::partial_error,
                            "cannot filter on validity period for this type");
    }
}

type::Indexes get_within(const type::Type_e type,
                         const type::GeographicalCoord& coord,
                         const double distance,
                         const type::Data& data) {
    std::vector<std::pair<idx_t, type::GeographicalCoord> > tmp;
    switch (type) {
    case Type_e::StopPoint:
        tmp = data.pt_data->stop_point_proximity_list.find_within(coord, distance);
        break;
    case Type_e::StopArea:
        tmp = data.pt_data->stop_area_proximity_list.find_within(coord, distance);
        break;
    case Type_e::POI:
        tmp = data.geo_ref->poi_proximity_list.find_within(coord, distance);
        break;
    default:
        throw parsing_error(parsing_error::error_type::partial_error,
                            "The requested object doesn't implement within");
    }
    Indexes indexes;
    for (const auto& p: tmp) { indexes.insert(p.first); }
    return indexes;
}

template<typename T>
static
typename boost::enable_if<
    typename boost::mpl::contains<
        nt::CodeContainer::SupportedTypes,
        T>::type,
    Indexes>::type
get_indexes_from_code(const std::string& key, const std::string& value, const Data& data) {
    Indexes res;
    for (const auto* obj: data.pt_data->codes.get_objs<T>(key, value)) {
        res.insert(obj->idx);
    }
    return res;
}
template<typename T>
static
typename boost::disable_if<
    typename boost::mpl::contains<
        nt::CodeContainer::SupportedTypes,
        T>::type,
    Indexes>::type
get_indexes_from_code(const std::string&, const std::string&, const Data&) {
    // there is no codes for unsupporded types, thus the result is empty
    return Indexes{};
}

type::Indexes get_indexes_from_code(const type::Type_e type,
                                    const std::string& key,
                                    const std::string& value,
                                    const type::Data& data) {
    switch (type) {
#define GET_INDEXES(type_name, collection_name) \
        case Type_e::type_name: return get_indexes_from_code<type::type_name>(key, value, data);
        ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
#undef GET_INDEXES
    default: return Indexes();// no code supported, empty result
    }
}

type::Indexes get_indexes_from_id(const type::Type_e type,
                                  const std::string& id,
                                  const type::Data& data) {
    switch (type) {
#define GET_INDEXES(type_name, collection_name) \
        case Type_e::type_name: \
            if (auto elt = find_or_default(id, data.pt_data->collection_name##_map)) { \
                return make_indexes({elt->idx}); \
            } \
            return Indexes();
        ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
#undef GET_INDEXES
    case Type_e::JourneyPattern:
        if (const auto jp_idx = data.dataRaptor->jp_container.get_jp_from_id(id)) {
            return make_indexes({jp_idx->val});
        }
        return Indexes();
    case Type_e::JourneyPatternPoint:
        if (const auto jpp_idx = data.dataRaptor->jp_container.get_jpp_from_id(id)) {
            return make_indexes({jpp_idx->val});
        }
        return Indexes();
    case Type_e::POI:
        if (auto elt = find_or_default(id, data.geo_ref->poi_map)) {
            return make_indexes({elt->idx});
        }
        return Indexes();
    case Type_e::POIType:
        if (auto elt = find_or_default(id, data.geo_ref->poitype_map)) {
            return make_indexes({elt->idx});
        }
        return Indexes();
    case Type_e::MetaVehicleJourney:
        if (auto elt = find_or_default(id, data.pt_data->meta_vjs)) {
            return make_indexes({elt->idx});
        }
        return Indexes();
    case Type_e::Impact: {
        Indexes indexes;
        const auto& impacts = data.pt_data->disruption_holder.get_weak_impacts();
        for (size_t i = 0; i < impacts.size(); ++i) {
            if (auto impact = impacts[i].lock()) {
                if (impact->uri == id) { indexes.insert(i); }
            }
        }
        return indexes;
    }
    default: return Indexes();
    }
}

template<typename T>
static Indexes get_indexes_from_name(const std::string& name, const std::vector<T*>& objs) {
    Indexes indexes;
    for (const auto* obj: objs) {
        if (obj->name != name) { continue; }
        indexes.insert(obj->idx);
    }
    return indexes;
}
static Indexes get_indexes_from_name(const std::string&, const std::vector<type::ValidityPattern*>&) {
    return Indexes();
}

type::Indexes get_indexes_from_name(const type::Type_e type,
                                    const std::string& name,
                                    const type::Data& data) {
    switch (type) {
#define GET_INDEXES(type_name, collection_name) \
        case Type_e::type_name: \
            return get_indexes_from_name(name, data.pt_data->collection_name);
        ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
#undef GET_INDEXES
    case Type_e::POI:
        return get_indexes_from_name(name, data.geo_ref->pois);
    case Type_e::POIType:
        return get_indexes_from_name(name, data.geo_ref->poitypes);
    default:
        return Indexes();
    }
}

}} // navitia::ptref
