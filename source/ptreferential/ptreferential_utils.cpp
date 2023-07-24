/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "georef/georef.h"
#include "ptreferential_utils.h"
#include "ptref_graph.h"
#include "ptreferential.h"
#include "routing/dataraptor.h"
#include "type/data.h"
#include "type/message.h"
#include "type/meta_data.h"
#include "type/pt_data.h"
#include "type/static_data.h"

#include <boost/range/algorithm/find.hpp>
#include <boost/dynamic_bitset.hpp>

#include <string>

using navitia::type::Data;
using navitia::type::Indexes;
using navitia::type::make_indexes;
using navitia::type::Type_e;

namespace bt = boost::posix_time;
namespace nt = navitia::type;

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
    // Exceptional case: if from = PhysicalMode and to = Impact
    // 1. Get all vehicle_journeys impacted
    // 2. Keep only vehicle_journeys with physical_mode in the parameter
    if (from == Type_e::PhysicalMode && to == Type_e::Impact) {
        auto vj_idxs = get_indexes_by_impacts(Type_e::VehicleJourney, data, false);
        const auto vjs = data.get_data<type::VehicleJourney>(vj_idxs);
        Indexes impact_indexes, temp;
        for (type::VehicleJourney* vj : vjs) {
            if (indexes.find(vj->physical_mode->idx) != indexes.cend()) {
                // Add Impact on vehicle_journeys having PhysicalMode
                temp = vj->get(Type_e::Impact, *(data.pt_data.get()));
                impact_indexes.insert(temp.begin(), temp.end());
            }
        }
        return impact_indexes;
    }

    const std::map<Type_e, Type_e> path = find_path(to);
    boost::dynamic_bitset<> set_idx(data.get_nb_obj(from));
    for (idx_t i : indexes) {
        set_idx.set(i);
    }
    while (path.at(from) != from) {
        set_idx = data.get_target_by_source(from, path.at(from), set_idx);
        from = path.at(from);
    }
    if (from != to) {
        // there was no path to find a requested type
        return Indexes{};
    }

    Indexes result;
    // result.reserve(set_idx.count());
    idx_t idx = set_idx.find_first();
    while(idx != boost::dynamic_bitset<>::npos) {
        result.insert(idx);
        idx = set_idx.find_next(idx);
    }
    return result;

    // return Indexes{set_idx};
}

Type_e type_by_caption(const std::string& type) {
    try {
        return navitia::type::static_data::get()->typeByCaption(type);
    } catch (...) {
        throw parsing_error(parsing_error::error_type::unknown_object, "Filter Unknown object type: " + type);
    }
}

using pair_indexes = std::pair<type::Type_e, Indexes>;
struct VariantVisitor : boost::static_visitor<pair_indexes> {
    const Data& d;
    explicit VariantVisitor(const Data& d) : d(d) {}
    pair_indexes operator()(const type::disruption::UnknownPtObj /*unused*/) {
        return {type::Type_e::Unknown, Indexes{}};
    }
    pair_indexes operator()(const type::Network* /*unused*/) { return {type::Type_e::Network, Indexes{}}; }
    pair_indexes operator()(const type::StopArea* /*unused*/) { return {type::Type_e::StopArea, Indexes{}}; }
    pair_indexes operator()(const type::StopPoint* /*unused*/) { return {type::Type_e::StopPoint, Indexes{}}; }
    pair_indexes operator()(const type::disruption::LineSection /*unused*/&) {
        return {type::Type_e::Unknown, Indexes{}};
    }
    pair_indexes operator()(const type::disruption::RailSection /*unused*/&) {
        return {type::Type_e::Unknown, Indexes{}};
    }
    pair_indexes operator()(const type::Line* /*unused*/) { return {type::Type_e::Line, Indexes{}}; }
    pair_indexes operator()(const type::Route* /*unused*/) { return {type::Type_e::Route, Indexes{}}; }
    pair_indexes operator()(const type::MetaVehicleJourney* meta_vj) {
        return {type::Type_e::VehicleJourney, meta_vj->get(type::Type_e::VehicleJourney, *d.pt_data)};
    }
};

Indexes get_indexes_by_impacts(const type::Type_e& type_e, const Data& d, bool only_no_service) {
    Indexes result;
    VariantVisitor visit(d);
    const auto& impacts = d.pt_data->disruption_holder.get_weak_impacts();
    for (const auto& impact : impacts) {
        const auto imp = impact.lock();
        if (!imp) {
            continue;
        }
        if (only_no_service && imp->severity->effect != type::disruption::Effect::NO_SERVICE) {
            continue;
        }
        for (const auto& entitie : imp->informed_entities()) {
            auto pair_type_indexes = boost::apply_visitor(visit, entitie);
            if (type_e == pair_type_indexes.first) {
                result.insert(pair_type_indexes.second.begin(), pair_type_indexes.second.end());
            }
        }
    }
    return result;
}

Indexes get_impacts_by_tags(const std::vector<std::string>& tag_name, const Data& d) {
    Indexes result;
    const auto& w_impacts = d.pt_data->disruption_holder.get_weak_impacts();

    for (size_t i = 0; i < w_impacts.size(); i++) {
        auto impact = w_impacts[i].lock();
        if (impact && impact->disruption) {
            for (const auto& tag : impact->disruption->tags) {
                if (tag && boost::range::find(tag_name, tag->name) != tag_name.end()) {
                    result.insert(i);
                }
            }
        }
    }

    return result;
}
static bool vj_active_at(const type::VehicleJourney* vj,
                         const bt::ptime current_datetime,
                         const type::RTLevel rt_level,
                         const type::Data& data) {
    if (vj->stop_time_list.empty()) {
        return false;
    }

    const auto* vp = vj->validity_patterns[rt_level];
    if (vp == nullptr) {
        return false;
    }

    if (data.meta->production_date.contains(current_datetime.date()) && vp->check(current_datetime.date())) {
        bt::time_period period = vj->execution_period(current_datetime.date());
        if (period.contains(current_datetime)) {
            return true;
        }
    }
    const auto yesterday_datetime = current_datetime - boost::gregorian::days(1);
    if (data.meta->production_date.contains(yesterday_datetime.date()) && vp->check(yesterday_datetime.date())) {
        bt::time_period period = vj->execution_period(yesterday_datetime.date());
        if (period.contains(current_datetime)) {
            return true;
        }
    }
    return false;
}

static bool keep_vj(const type::VehicleJourney* vj, const bt::time_period& period, const type::RTLevel rt_level) {
    if (vj->stop_time_list.empty()) {
        return false;  // no stop time, so it cannot be valid
    }

    const auto& first_departure_dt = vj->earliest_time();
    for (boost::gregorian::day_iterator it(period.begin().date()); it <= period.last().date(); ++it) {
        if ((vj->validity_patterns[rt_level] != nullptr) && !vj->validity_patterns[rt_level]->check(*it)) {
            continue;
        }
        bt::ptime vj_dt = bt::ptime(*it, bt::seconds(first_departure_dt));
        if (period.contains(vj_dt)) {
            return true;
        }
    }

    return false;
}

static Indexes filter_vj_on_period(const Indexes& indexes,
                                   const bt::time_period& period,
                                   const type::RTLevel rt_level,
                                   const type::Data& data) {
    Indexes res;
    bt::time_period production_period = {bt::ptime(data.meta->production_date.begin()),
                                         bt::ptime(data.meta->production_date.end())};
    const auto period_to_check = production_period.intersection(period);

    for (const idx_t idx : indexes) {
        const auto* vj = data.pt_data->vehicle_journeys[idx];
        if (!keep_vj(vj, period_to_check, rt_level)) {
            continue;
        }
        res.insert(idx);
    }
    return res;
}

static Indexes filter_impact_on_period(const Indexes& indexes, const bt::time_period& period, const type::Data& data) {
    Indexes res;
    for (const idx_t idx : indexes) {
        auto impact = data.pt_data->disruption_holder.get_weak_impacts()[idx].lock();

        if (!impact) {
            continue;
        }

        // to keep an impact, we want the intersection between its application periods
        // and the period to be non empy
        for (const auto& application_period : impact->application_periods) {
            if (application_period.intersection(period).is_null()) {
                continue;
            }
            res.insert(idx);
            break;
        }
    }
    return res;
}
type::Indexes filter_vj_active_at(const Indexes& indexes,
                                  const bt::ptime current_datetime,
                                  const type::RTLevel rt_level,
                                  const type::Data& data) {
    Indexes res;
    for (const idx_t idx : indexes) {
        const auto* vj = data.pt_data->vehicle_journeys[idx];
        if (!vj_active_at(vj, current_datetime, rt_level, data)) {
            continue;
        }
        res.insert(idx);
    }
    return res;
}
Indexes filter_on_period(const Indexes& indexes,
                         const navitia::type::Type_e requested_type,
                         const boost::optional<bt::ptime>& since,
                         const boost::optional<bt::ptime>& until,
                         const type::RTLevel rt_level,
                         const type::Data& data) {
    if (since && until && until < since) {
        throw ptref_error("invalid filtering period");
    }
    auto start = bt::ptime(bt::neg_infin);
    auto end = bt::ptime(bt::pos_infin);
    if (since && *since > start) {
        start = *since;
    }
    // we want end to be in the period, so we add one seconds
    if (until && *until < end) {
        end = *until + bt::seconds(1);
    }
    bt::time_period period{start, end};

    switch (requested_type) {
        case nt::Type_e::VehicleJourney:
            return filter_vj_on_period(indexes, period, rt_level, data);
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
    for (const auto& p : tmp) {
        indexes.insert(p.first);
    }
    return indexes;
}

template <typename T>
static
    typename boost::enable_if<typename boost::mpl::contains<nt::CodeContainer::SupportedTypes, T>::type, Indexes>::type
    get_indexes_from_code(const std::string& key, const std::string& value, const Data& data) {
    Indexes res;
    for (const auto* obj : data.pt_data->codes.get_objs<T>(key, value)) {
        res.insert(obj->idx);
    }
    return res;
}
template <typename T>
static
    typename boost::disable_if<typename boost::mpl::contains<nt::CodeContainer::SupportedTypes, T>::type, Indexes>::type
    get_indexes_from_code(const std::string& /*unused*/, const std::string& /*unused*/, const Data& /*unused*/) {
    // there is no codes for unsupporded types, thus the result is empty
    return Indexes{};
}

type::Indexes get_indexes_from_code(const type::Type_e type,
                                    const std::string& key,
                                    const std::string& value,
                                    const type::Data& data) {
    switch (type) {
#define GET_INDEXES(type_name, collection_name) \
    case Type_e::type_name:                     \
        return get_indexes_from_code<type::type_name>(key, value, data);
        ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
#undef GET_INDEXES
        default:
            return Indexes();  // no code supported, empty result
    }
}

template <typename T>
static
    typename boost::enable_if<typename boost::mpl::contains<nt::CodeContainer::SupportedTypes, T>::type, Indexes>::type
    get_indexes_from_code_type(const std::vector<std::string>& keys, const Data& data) {
    Indexes indexes;
    auto collection = data.pt_data->collection<T>();
    for (const auto* obj : collection) {
        auto codes = data.pt_data->codes.get_codes<T>(obj);
        for (const auto& key : keys) {
            if (codes.find(key) != codes.end()) {
                indexes.insert(obj->idx);
                break;
            }
        }
    }

    return indexes;
}
template <typename T>
static
    typename boost::disable_if<typename boost::mpl::contains<nt::CodeContainer::SupportedTypes, T>::type, Indexes>::type
    get_indexes_from_code_type(const std::vector<std::string>& /*unused*/, const Data& /*unused*/) {
    // there is no code for unsupported types, thus the result is empty
    return Indexes{};
}
type::Indexes get_indexes_from_code_type(const type::Type_e type,
                                         const std::vector<std::string>& keys,
                                         const type::Data& data) {
    switch (type) {
#define GET_INDEXES(type_name, collection_name) \
    case Type_e::type_name:                     \
        return get_indexes_from_code_type<type::type_name>(keys, data);
        ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
#undef GET_INDEXES
        default:
            return Indexes{};  // no code supported, empty result
    }
}

type::Indexes get_indexes_from_route_direction_type(const std::vector<std::string>& keys, const type::Data& data) {
    Indexes indexes;
    for (const auto* route : data.pt_data->routes) {
        if (contains(keys, route->direction_type)) {
            indexes.insert(route->idx);
        }
    }

    return indexes;
}

type::Indexes get_indexes_from_id(const type::Type_e type, const std::string& id, const type::Data& data) {
    switch (type) {
#define GET_INDEXES(type_name, collection_name)                                    \
    case Type_e::type_name:                                                        \
        if (auto elt = find_or_default(id, data.pt_data->collection_name##_map)) { \
            return make_indexes({elt->idx});                                       \
        }                                                                          \
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
                    if (impact->uri == id) {
                        indexes.insert(i);
                    }
                }
            }
            return indexes;
        }
        default:
            return Indexes();
    }
}

template <typename T>
static Indexes get_indexes_from_name(const std::string& name, const std::vector<T*>& objs) {
    Indexes indexes;
    for (const auto* obj : objs) {
        if (obj->name != name) {
            continue;
        }
        indexes.insert(obj->idx);
    }
    return indexes;
}
static Indexes get_indexes_from_name(const std::string& /*unused*/,
                                     const std::vector<type::ValidityPattern*>& /*unused*/) {
    return Indexes();
}

type::Indexes get_indexes_from_name(const type::Type_e type, const std::string& name, const type::Data& data) {
    switch (type) {
#define GET_INDEXES(type_name, collection_name) \
    case Type_e::type_name:                     \
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

}  // namespace ptref
}  // namespace navitia
