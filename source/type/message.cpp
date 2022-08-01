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

#include "type/message.h"

#include "type/pt_data.h"
#include "type/company.h"
#include "type/network.h"
#include "type/base_pt_objects.h"
#include "type/meta_vehicle_journey.h"
#include "type/serialization.h"
#include "utils/logger.h"

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/format.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/set.hpp>
#include <boost/range/algorithm.hpp>

namespace nt = navitia::type;

namespace navitia {
namespace type {
namespace disruption {

template <class Archive>
void Property::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& key& type& value;
}
SERIALIZABLE(Property)

template <class Archive>
void Cause::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& uri& wording& created_at& updated_at& category;
}
SERIALIZABLE(Cause)

template <class Archive>
void Severity::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& uri& wording& created_at& updated_at& color& priority& effect;
}
SERIALIZABLE(Severity)

template <class archive>
void LineSection::serialize(archive& ar, const unsigned int /*unused*/) {
    ar& line& start_point& end_point& routes;
}
SERIALIZABLE(LineSection)

template <class archive>
void RailSection::serialize(archive& ar, const unsigned int /*unused*/) {
    ar& line& start& end& blockeds& impacted_stop_areas& routes;
}
SERIALIZABLE(RailSection)

template <class Archive>
void StopTimeUpdate::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& stop_time& cause& departure_status& arrival_status;
}
SERIALIZABLE(StopTimeUpdate)

template <class Archive>
void Message::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& text& created_at& updated_at& channel_id& channel_name& channel_content_type& channel_types;
}
SERIALIZABLE(Message)

namespace detail {
template <class Archive>
void AuxInfoForMetaVJ::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& stop_times;
}
SERIALIZABLE(AuxInfoForMetaVJ)
}  // namespace detail

template <class Archive>
void TimeSlot::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& begin& end;
}
SERIALIZABLE(TimeSlot)

template <class Archive>
void ApplicationPattern::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& week_pattern& application_period& time_slots;
}
SERIALIZABLE(ApplicationPattern)

template <class Archive>
void Impact::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& uri& company_id& physical_mode_id& headsign& created_at& updated_at& application_periods& severity&
        _informed_entities& messages& disruption& aux_info& application_patterns;
}
SERIALIZABLE(Impact)

template <class Archive>
void Tag::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& uri& name& created_at& updated_at;
}
SERIALIZABLE(Tag)

template <class Archive>
void Disruption::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& uri& reference& rt_level& publication_period& created_at& updated_at& cause& impacts& localization& tags& note&
        contributor& properties;
}
SERIALIZABLE(Disruption)

template <class Archive>
void DisruptionHolder::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& disruptions_by_uri& causes& severities& tags& weak_impacts;
}
SERIALIZABLE(DisruptionHolder)

std::vector<ImpactedVJ> get_impacted_vehicle_journeys(const LineSection& ls,
                                                      const Impact& impact,
                                                      const boost::gregorian::date_period& production_period,
                                                      type::RTLevel rt_level) {
    log4cplus::Logger log = log4cplus::Logger::getInstance("log");
    // Get all impacted VJs and compute the corresponding base_canceled vp
    std::vector<ImpactedVJ> vj_vp_pairs;

    // Computing a validity_pattern of impact used to pre-filter concerned vjs later
    type::ValidityPattern impact_vp = impact.get_impact_vp(production_period);

    // Loop on impacted routes of the line section
    for (const auto* route : ls.routes) {
        // Loop on each vj
        route->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
            /*
             * Pre-filtering by validity pattern, which allows us to check if the vj is impacted quickly
             *
             * Since the validity pattern runs only by day not by hour, we'll compute in detail to
             * check if the vj is really impacted or not.
             *
             * */
            if ((vj.validity_patterns[rt_level]->days & impact_vp.days).none()) {
                return true;
            }

            // Filtering each journey to see if it's impacted by the section.
            auto section = vj.get_sections_ranks(ls.start_point, ls.end_point);
            // If the vj pass by both stops both elements will be different than nullptr, otherwise
            // it's not passing by both stops and should not be impacted
            if (!section.empty()) {
                // Once we know the line section is part of the vj we compute the vp for the adapted_vj
                nt::ValidityPattern new_vp{vj.validity_patterns[rt_level]->beginning_date};
                for (const auto& period : impact.application_periods) {
                    // get the vp of the section
                    new_vp.days |= vj.get_vp_for_section(section, rt_level, period).days;
                }
                // If there is effective days for the adapted vp we're keeping it
                if (!new_vp.days.none()) {
                    LOG4CPLUS_TRACE(log, "vj " << vj.uri << " is affected, keeping it.");
                    new_vp.days >>= vj.shift;
                    vj_vp_pairs.emplace_back(vj.uri, new_vp, std::move(section));
                }
            }
            return true;
        });
    }
    return vj_vp_pairs;
}

std::vector<ImpactedVJ> get_impacted_vehicle_journeys(const RailSection& rs,
                                                      const Impact& impact,
                                                      const boost::gregorian::date_period& production_period,
                                                      type::RTLevel rt_level) {
    log4cplus::Logger log = log4cplus::Logger::getInstance("log");
    std::vector<ImpactedVJ> vj_vp_pairs;

    if (rs.routes.empty() && rs.line == nullptr) {
        LOG4CPLUS_ERROR(log, "rail section: routes or line have to be filled");
        return vj_vp_pairs;
    }

    std::vector<navitia::type::Route*> routes;
    if (rs.routes.empty()) {
        routes = rs.line->route_list;
    } else {
        routes = rs.routes;
    }

    // Computing a validity_pattern of impact used to pre-filter concerned vjs later
    type::ValidityPattern impact_vp = impact.get_impact_vp(production_period);

    auto apply_impacts_on_vj = [&](const nt::VehicleJourney& vj) {
        /*
         * Pre-filtering by validity pattern, which allows us to check if the vj is impacted quickly
         *
         * Since the validity pattern runs only by day not by hour, we'll compute in detail to
         * check if the vj is really impacted or not.
         *
         * */
        if ((vj.validity_patterns[rt_level]->days & impact_vp.days).none()) {
            return true;
        }

        // Filtering each journey to see if it's impacted by the section.

        if (rs.impacts(&vj)) {
            auto section = vj.get_sections_ranks(rs.start, rs.end);

            // For NO_SERVICE we should impact all stop_points from rs.start_point to
            // last stop_point of the vj
            if (impact.severity->effect == nt::disruption::Effect::NO_SERVICE) {
                section = vj.get_no_service_sections_ranks(rs.start);
            }

            // Once we know the line section is part of the vj we compute the vp for the adapted_vj
            nt::ValidityPattern new_vp{vj.validity_patterns[rt_level]->beginning_date};
            for (const auto& period : impact.application_periods) {
                // get the vp of the section
                auto section_validity_pattern = vj.get_vp_for_section(section, rt_level, period);
                new_vp.days |= section_validity_pattern.days;
            }

            // If there is effective days for the adapted vp we're keeping it
            if (!new_vp.days.none()) {
                new_vp.days >>= vj.shift;
                vj_vp_pairs.emplace_back(vj.uri, new_vp, std::move(section));
            }
        }
        return true;
    };

    for (const auto* route : routes) {
        // Loop on each vj
        route->for_each_vehicle_journey(apply_impacts_on_vj);
    }
    return vj_vp_pairs;
}

struct InformedEntitiesLinker : public boost::static_visitor<> {
    const boost::shared_ptr<Impact>& impact;
    const boost::gregorian::date_period& production_period;
    type::RTLevel rt_level;
    nt::PT_Data& pt_data;
    log4cplus::Logger log = log4cplus::Logger::getInstance("log");

    InformedEntitiesLinker(const boost::shared_ptr<Impact>& impact,
                           const boost::gregorian::date_period& production_period,
                           RTLevel rt_level,
                           nt::PT_Data& pt_data)
        : impact(impact), production_period(production_period), rt_level(rt_level), pt_data(pt_data) {}

    template <typename NavitiaPTObject>
    void operator()(NavitiaPTObject* bo) const {
        // the ptobject that match a navitia object, we can just add the impact to the object
        bo->add_impact(impact);
    }
    void operator()(const nt::disruption::LineSection& line_section) const {
        // for a line section it's a bit more complex, we need to register the impact
        // to all impacted stoppoints and vehiclejourneys
        auto impacted_vjs = get_impacted_vehicle_journeys(line_section, *impact, production_period, rt_level);
        std::set<type::StopPoint*> impacted_stop_points;
        std::set<type::MetaVehicleJourney*> impacted_meta_vjs;
        if (impacted_vjs.empty()) {
            LOG4CPLUS_INFO(log, "line section impact " << impact->uri
                                                       << " does not impact any vj, it will not be linked to anything");
        }
        for (auto& impacted_vj : impacted_vjs) {
            const std::string& vj_uri = impacted_vj.vj_uri;
            LOG4CPLUS_TRACE(log, "Impacted vj : " << vj_uri);
            auto vj_iterator = pt_data.vehicle_journeys_map.find(vj_uri);
            if (vj_iterator == pt_data.vehicle_journeys_map.end()) {
                LOG4CPLUS_TRACE(log, "impacted vj : " << vj_uri << " not found in data. I ignore it.");
                continue;
            }
            nt::VehicleJourney* vj = vj_iterator->second;

            if (impacted_meta_vjs.insert(vj->meta_vj).second) {
                // it's the first time we see this metavj, we add the impact to it
                vj->meta_vj->add_impact(impact);
            }
            // Get base vj to impact all stop_points
            auto base_vj = vj->get_corresponding_base();
            for (const auto& st : base_vj->stop_time_list) {
                // if the stop_point is impacted and it's the first time we see it
                if (impacted_vj.impacted_ranks.count(st.order()) && impacted_stop_points.insert(st.stop_point).second) {
                    // it's the first time we see this stoppoint, we add the impact to it
                    st.stop_point->add_impact(impact);
                }
            }
        }
    }
    void operator()(const nt::disruption::RailSection& rail_section) const {
        LOG4CPLUS_INFO(log, "New rail section for impact " << impact->uri);
        auto impacted_vjs = get_impacted_vehicle_journeys(rail_section, *impact, production_period, rt_level);
        std::set<type::StopPoint*> impacted_stop_points;
        std::set<type::MetaVehicleJourney*> impacted_meta_vjs;
        if (impacted_vjs.empty()) {
            LOG4CPLUS_INFO(log, "rail section impact " << impact->uri
                                                       << " does not impact any vj, it will not be linked to anything");
        }
        for (auto& impacted_vj : impacted_vjs) {
            const std::string& vj_uri = impacted_vj.vj_uri;
            LOG4CPLUS_TRACE(log, "Impacted vj : " << vj_uri);
            auto vj_iterator = pt_data.vehicle_journeys_map.find(vj_uri);
            if (vj_iterator == pt_data.vehicle_journeys_map.end()) {
                LOG4CPLUS_TRACE(log, "impacted vj : " << vj_uri << " not found in data. I ignore it.");
                continue;
            }
            nt::VehicleJourney* vj = vj_iterator->second;

            if (impacted_meta_vjs.insert(vj->meta_vj).second) {
                // it's the first time we see this metavj, we add the impact to it
                vj->meta_vj->add_impact(impact);
            }
            // Get base vj to impact all stop_points
            auto base_vj = vj->get_corresponding_base();
            for (const auto& st : base_vj->stop_time_list) {
                // if the stop_point is impacted and it's the first time we see it
                if (impacted_vj.impacted_ranks.count(st.order()) && impacted_stop_points.insert(st.stop_point).second) {
                    // it's the first time we see this stoppoint, we add the impact to it
                    st.stop_point->add_impact(impact);
                }
            }
        }
    }
    void operator()(const nt::disruption::UnknownPtObj& /*unused*/) const {}
};

void Impact::link_informed_entity(PtObj ptobj,
                                  boost::shared_ptr<Impact>& impact,
                                  const boost::gregorian::date_period& production_period,
                                  type::RTLevel rt_level,
                                  nt::PT_Data& pt_data) {
    InformedEntitiesLinker v(impact, production_period, rt_level, pt_data);
    boost::apply_visitor(v, ptobj);

    impact->_informed_entities.push_back(std::move(ptobj));
}

bool Impact::is_valid(const boost::posix_time::ptime& publication_date,
                      const boost::posix_time::time_period& active_period) const {
    if (publication_date.is_not_a_date_time() && active_period.is_null()) {
        return false;
    }

    // we check if we want to publish the impact
    if (!disruption->is_publishable(publication_date)) {
        return false;
    }

    // if we have a active_period we check if the impact applies on this period
    if (active_period.is_null()) {
        return true;
    }

    for (const auto& period : application_periods) {
        if (!period.intersection(active_period).is_null()) {
            return true;
        }
    }
    return false;
}

ActiveStatus Impact::get_active_status(const boost::posix_time::ptime& publication_date) const {
    bool is_future = false;
    for (const auto& period : application_periods) {
        if (period.contains(publication_date)) {
            return ActiveStatus::active;
        }
        if (!period.is_null() && period.begin() >= publication_date) {
            is_future = true;
        }
    }

    if (is_future) {
        return ActiveStatus::future;
    }
    return ActiveStatus::past;
}

bool Impact::is_relevant(const std::vector<const StopTime*>& stop_times) const {
    // No delay on the section
    if ((severity->effect == nt::disruption::Effect::SIGNIFICANT_DELAYS
         || severity->effect == nt::disruption::Effect::UNKNOWN_EFFECT)
        && !aux_info.stop_times.empty()) {
        // We don't handle removed or added stop, but we already match
        // on SIGNIFICANT_DELAYS, thus we should not have that here
        // (removed and added stop should be a DETOUR)
        const auto nb_aux = aux_info.stop_times.size();
        for (const auto& st : stop_times) {
            const auto base_st = st->get_base_stop_time();
            if (base_st == nullptr) {
                return true;
            }
            const auto idx = base_st->order();
            if (idx.val >= nb_aux) {
                return true;
            }
            const auto& amended_st = aux_info.get_stop_time_update(idx).stop_time;
            if (base_st->arrival_time != amended_st.arrival_time) {
                return true;
            }
            if (base_st->departure_time != amended_st.departure_time) {
                return true;
            }
        }
        return false;
    }

    // line section not relevant
    auto line_section_impacted_obj_it = boost::find_if(
        informed_entities(), [](const PtObj& ptobj) { return boost::get<LineSection>(&ptobj) != nullptr; });
    auto is_line_section = (line_section_impacted_obj_it != informed_entities().end());

    // rail section not relevant
    auto rail_section_impacted_obj_it = boost::find_if(
        informed_entities(), [](const PtObj& ptobj) { return boost::get<RailSection>(&ptobj) != nullptr; });
    auto is_rail_section = (rail_section_impacted_obj_it != informed_entities().end());

    if (is_line_section) {
        // note in this we take the premise that an impact
        // cannot impact a line section AND a vj

        // if the origin or the destination is impacted by the same impact
        // it means the section is impacted
        for (const auto& st : {stop_times.front(), stop_times.back()}) {
            for (const auto& sp_message : st->stop_point->get_impacts()) {
                if (sp_message.get() == this) {
                    return true;
                }
            }
        }
        return false;
    } else if (is_rail_section && this->severity->effect == nt::disruption::Effect::REDUCED_SERVICE) {
        const auto& informed_entity = *rail_section_impacted_obj_it;
        const RailSection* rail_section = boost::get<RailSection>(&informed_entity);

        const auto& first_stop_time = stop_times.front();
        if (first_stop_time->stop_point->stop_area != rail_section->start
            && first_stop_time->stop_point->stop_area != rail_section->end) {
            for (const auto& sp_message : first_stop_time->stop_point->get_impacts()) {
                if (sp_message.get() == this) {
                    return true;
                }
            }
        }

        const auto& last_stop_time = stop_times.back();
        if (last_stop_time->stop_point->stop_area != rail_section->start
            && last_stop_time->stop_point->stop_area != rail_section->end) {
            for (const auto& sp_message : last_stop_time->stop_point->get_impacts()) {
                if (sp_message.get() == this) {
                    return true;
                }
            }
        }
        return false;
    } else if (is_rail_section && this->severity->effect == nt::disruption::Effect::NO_SERVICE) {
        for (auto informed_entity : informed_entities()) {
            const RailSection* rail_section = boost::get<RailSection>(&informed_entity);
            if (rail_section == nullptr) {
                continue;
            }

            // From the vehicle journey of first date_time retrieve all stop_time ranks
            auto first_st = stop_times.front();
            auto vj = first_st->vehicle_journey;
            auto section = vj->get_no_service_sections_ranks(rail_section->start);

            for (const auto& st : stop_times) {
                if (st->stop_point->stop_area == rail_section->start) {
                    continue;
                }

                const auto base_st = st->get_base_stop_time();
                if (!base_st) {
                    continue;
                }
                if (section.count(base_st->order()) > 0) {
                    return true;
                }
            }
            return false;
        }

    } else {
        // else, no reason to not be interested by it
        return true;
    }
    return false;
}

bool Impact::is_only_line_section() const {
    if (_informed_entities.empty()) {
        return false;
    }
    return boost::algorithm::all_of(
        informed_entities(), [](const PtObj& entity) { return boost::get<nt::disruption::LineSection>(&entity); });
}

bool Impact::is_line_section_of(const Line& line) const {
    return boost::algorithm::any_of(informed_entities(), [&](const PtObj& entity) {
        const auto* line_section = boost::get<nt::disruption::LineSection>(&entity);
        return line_section && line_section->line && line_section->line->idx == line.idx;
    });
}

bool Impact::is_only_rail_section() const {
    if (_informed_entities.empty()) {
        return false;
    }
    return boost::algorithm::all_of(
        informed_entities(), [](const PtObj& entity) { return boost::get<nt::disruption::RailSection>(&entity); });
}

bool Impact::is_rail_section_of(const Line& line) const {
    return boost::algorithm::any_of(informed_entities(), [&](const PtObj& entity) {
        const auto* rail_section = boost::get<nt::disruption::RailSection>(&entity);
        return rail_section && rail_section->line && rail_section->line->idx == line.idx;
    });
}

bool TimeSlot::operator<(const TimeSlot& other) const {
    return std::tie(this->begin, this->end) < std::tie(other.begin, other.end);
}

bool TimeSlot::operator==(const TimeSlot& other) const {
    return std::tie(this->begin, this->end) == std::tie(other.begin, other.end);
}

void ApplicationPattern::add_time_slot(uint32_t begin, uint32_t end) {
    this->time_slots.insert(TimeSlot(begin, end));
}

bool ApplicationPattern::operator<(const ApplicationPattern& other) const {
    auto start_this = navitia::to_int_date(this->application_period.begin());
    auto end_this = navitia::to_int_date(this->application_period.end());
    auto start_other = navitia::to_int_date(other.application_period.begin());
    auto end_other = navitia::to_int_date(other.application_period.end());

    if (std::tie(start_this, end_this) != std::tie(start_other, end_other)) {
        return std::tie(start_this, end_this) < std::tie(start_other, end_other);
    }

    for (std::size_t n = 0; n < std::min(this->time_slots.size(), other.time_slots.size()); n++) {
        const auto this_ts = *next(this->time_slots.begin(), n);
        const auto other_ts = *next(other.time_slots.begin(), n);
        if (!(this_ts == other_ts)) {
            return this_ts < other_ts;
        }
    }
    if (this->time_slots.size() != other.time_slots.size()) {
        return this->time_slots.size() < other.time_slots.size();
    }

    return this->week_pattern.to_ulong() < other.week_pattern.to_ulong();
}

template <class Cont>
Indexes make_indexes(const Cont& objs) {
    using ObjPtrType = typename Cont::value_type;
    static_assert(std::is_pointer<ObjPtrType>::value, "objs should be a container of pointers");

    using ObjType = typename std::remove_pointer<ObjPtrType>::type;
    static_assert(std::is_base_of<Header, ObjType>::value,
                  "objs be a container of pointers that inherit from navitia::type::Header");

    Indexes indexes;
    for (const auto o : objs) {
        indexes.insert(o->idx);
    }
    return indexes;
}

template <>
Indexes make_indexes(const idx_t& idx) {
    Indexes indexes;
    indexes.insert(idx);
    return indexes;
}

std::set<StopPoint*> LineSection::get_stop_points_section() const {
    std::set<StopPoint*> res;
    for (const auto* route : routes) {
        route->for_each_vehicle_journey([&](const VehicleJourney& vj) {
            auto ranks = vj.get_sections_ranks(start_point, end_point);
            if (ranks.empty()) {
                return true;
            }
            for (const auto& rank : ranks) {
                res.insert(vj.get_stop_time(rank).stop_point);
            }

            return false;
        });
    }
    return res;
}

boost::optional<RailSection> try_make_rail_section(
    const navitia::type::PT_Data& pt_data,
    const std::string& start_uri,
    const std::vector<std::pair<std::string, uint32_t>>& blockeds_uri_order,
    const std::string& end_uri,
    const boost::optional<std::string>& line_uri,  // may be null
    const std::vector<std::string>& routes_uris    // may be empty
) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    const std::unordered_map<std::string, StopArea*>& stop_areas_map = pt_data.stop_areas_map;

    // find start stop area if it exists
    auto find_start = stop_areas_map.find(start_uri);
    if (find_start == stop_areas_map.end()) {
        LOG4CPLUS_WARN(logger, "Rail section with unknown start stop area: " << start_uri);
        return boost::none;
    }
    StopArea* start = stop_areas_map.at(start_uri);

    // find end_stop_area if it exists
    auto find_end = stop_areas_map.find(end_uri);
    if (find_end == stop_areas_map.end()) {
        LOG4CPLUS_WARN(logger, "Rail section with unknown end stop area: " << end_uri);
        return boost::none;
    }
    StopArea* end = stop_areas_map.at(end_uri);

    // sort blockeds by their order
    std::vector<std::pair<std::string, uint32_t>> blockeds_sorted = blockeds_uri_order;
    auto sort_predicate = [](const std::pair<std::string, uint32_t>& sa1, const std::pair<std::string, uint32_t>& sa2) {
        return sa1.second < sa2.second;
    };
    std::sort(blockeds_sorted.begin(), blockeds_sorted.end(), sort_predicate);

    // find blocked_stop_areas
    std::vector<StopArea*> blockeds{};
    for (const auto& pair : blockeds_sorted) {
        const std::string& blocked_uri = pair.first;
        auto find_blocked = stop_areas_map.find(blocked_uri);
        if (find_blocked == stop_areas_map.end()) {
            LOG4CPLUS_WARN(logger, "Rail section with unknown blocked stop area : " << blocked_uri);
            return boost::none;
        }
        StopArea* blocked_stop_area = stop_areas_map.at(blocked_uri);
        blockeds.push_back(blocked_stop_area);
    }

    if (line_uri == nullptr && routes_uris.empty()) {
        LOG4CPLUS_WARN(logger, "Rail section with no line and empty routes.");
        return boost::none;
    }

    // find routes
    const std::unordered_map<std::string, Route*>& routes_map = pt_data.routes_map;
    std::vector<Route*> routes{};
    for (const auto& route_uri : routes_uris) {
        auto find_route = routes_map.find(route_uri);
        if (find_route == routes_map.end()) {
            LOG4CPLUS_WARN(logger, "Rail section with unknown route : " << route_uri);
            return boost::none;
        }
        Route* route = routes_map.at(route_uri);
        routes.push_back(route);
    }

    // find lines
    Line* line = nullptr;
    if (line_uri) {
        const std::unordered_map<std::string, Line*>& lines_map = pt_data.lines_map;
        auto find_line = lines_map.find(*line_uri);
        if (find_line == lines_map.end()) {
            LOG4CPLUS_WARN(logger, "Rail section with unknown line : " << *line_uri);
            return boost::none;
        }
        line = lines_map.at(*line_uri);

        // if no routes are given in the input, we take all routes from the line
        if (routes.empty()) {
            routes = line->route_list;
        }
        // some routes were given, let's check that they belong to "line"
        else {
            for (const Route* route : routes) {
                if (route->line == nullptr) {
                    LOG4CPLUS_WARN(logger, "Rail section has line: '"
                                               << *line_uri << "' but also a route with no line: " << route->uri);
                    return boost::none;
                }
                if (route->line->idx != line->idx) {
                    LOG4CPLUS_WARN(logger, "Rail section has line: '"
                                               << *line_uri << "' but also a route: '" << route->uri
                                               << "' that belongs to a different line: " << route->line->uri);
                    return boost::none;
                }
            }
        }
    }

    // fill impacted stop_areas from start, blockeds and end
    std::vector<StopArea*> impacted_stop_areas{};
    if (blockeds.empty()) {
        impacted_stop_areas.push_back(start);
        if (end->idx != start->idx) {
            impacted_stop_areas.push_back(end);
        }

    } else {
        if (blockeds.front()->idx != start->idx) {
            impacted_stop_areas.push_back(start);
        }

        for (StopArea* blocked : blockeds) {
            impacted_stop_areas.push_back(blocked);
        }

        if (blockeds.back()->idx != end->idx) {
            impacted_stop_areas.push_back(end);
        }
    }

    RailSection result(start, end, blockeds, impacted_stop_areas, line, routes);
    return result;
}

bool RailSection::is_blocked_start_point() const {
    if (this->blockeds.empty()) {
        return false;
    }
    return this->blockeds.front()->idx == this->start->idx;
}

bool RailSection::is_start_stop(const std::string& uri) const {
    return this->start->uri == uri;
}

bool RailSection::is_blocked_end_point() const {
    if (this->blockeds.empty()) {
        return false;
    }
    return this->blockeds.back()->idx == this->end->idx;
}

bool RailSection::is_end_stop(const std::string& uri) const {
    return this->end->uri == uri;
}

bool RailSection::impacts(const VehicleJourney* vehicle_journey) const {
    if (vehicle_journey == nullptr) {
        return false;
    }
    // we work on the base_vj (if it exists) to determine if this rail section impacts vehicle_journey
    const VehicleJourney* base_vj = vehicle_journey->get_corresponding_base();
    const VehicleJourney* vj = base_vj ? base_vj : vehicle_journey;

    const std::vector<StopTime>& stop_times = vj->stop_time_list;

    if (stop_times.empty()) {
        return false;
    }

    // the RailSection impacts the vj if :
    // the vj contains all impacted_stop_areas in their exact order

    // let's first check if we can find the first impacted_stop_area in the stop_time list
    const StopArea* first_stop_area = impacted_stop_areas.front();
    auto find_first_stop_area = std::find_if(stop_times.begin(), stop_times.end(), [&](const StopTime& stop_time) {
        return stop_time.is_in_stop_area(first_stop_area);
    });
    bool first_stop_area_found_in_stop_times = find_first_stop_area != stop_times.end();

    // the first impacted stop_area appears in the stop_time list,
    // let's keep iterating the stop_times to see if it contains all impacted_stop_areas consecutively
    if (first_stop_area_found_in_stop_times) {
        auto impacted_stop_area_iter = impacted_stop_areas.begin();
        auto stop_times_iter = find_first_stop_area;

        // let's iterate on both stop_times and impacted stop_areas
        while (impacted_stop_area_iter != impacted_stop_areas.end() && stop_times_iter != stop_times.end()) {
            const StopArea* stop_area = *impacted_stop_area_iter;
            const StopTime& stop_time = *stop_times_iter;
            if (!stop_time.is_in_stop_area(stop_area)) {
                // found a stop_time which is not in the sequence of impacted_stop_areas
                return false;
            }
            impacted_stop_area_iter++;
            stop_times_iter++;
        }
        if (impacted_stop_area_iter == impacted_stop_areas.end()) {
            // here it means that found all impacted_stop_areas in their
            //  exact order
            return true;
        } else {
            // here it means that the vj ends in an impacted stop_area, and visit the
            //  previous impacted_stop_areas in their exact order
            return false;
        }
    }

    return false;
}

std::set<StopPoint*> get_stop_points_section(const RailSection& rs, const Effect effect) {
    std::set<StopPoint*> res;
    std::vector<navitia::type::Route*> routes;
    if (rs.routes.empty()) {
        routes = rs.line->route_list;
    } else {
        routes = rs.routes;
    }

    for (const auto* route : routes) {
        route->for_each_vehicle_journey([&](const VehicleJourney& vj) {
            if (rs.impacts(&vj)) {
                auto section = vj.get_sections_ranks(rs.start, rs.end);
                if (effect == nt::disruption::Effect::NO_SERVICE) {
                    section = vj.get_no_service_sections_ranks(rs.start);
                }
                if (section.empty()) {
                    return true;
                }
                for (auto stop_time : vj.stop_time_list) {
                    const StopTime* base_stop_time = stop_time.get_base_stop_time();
                    if (base_stop_time == nullptr) {
                        continue;
                    }
                    if (section.count(base_stop_time->order()) > 0) {
                        res.insert(stop_time.stop_point);
                    }
                }
            }
            return false;
        });
    }
    return res;
}

using pair_indexes = std::pair<Type_e, Indexes>;
struct ImpactVisitor : boost::static_visitor<pair_indexes> {
    Type_e target = Type_e::Unknown;
    PT_Data& pt_data;
    Effect effect;

    ImpactVisitor(Type_e target, PT_Data& pt_data, Effect effect) : target(target), pt_data(pt_data), effect(effect) {}

    pair_indexes operator()(const disruption::UnknownPtObj /*unused*/) { return {Type_e::Unknown, Indexes{}}; }
    pair_indexes operator()(const Network* n) { return {Type_e::Network, make_indexes(n->idx)}; }
    pair_indexes operator()(const StopArea* sa) { return {Type_e::StopArea, make_indexes(sa->idx)}; }
    pair_indexes operator()(const StopPoint* sp) { return {Type_e::StopPoint, make_indexes(sp->idx)}; }
    pair_indexes operator()(const LineSection& ls) {
        switch (target) {
            case Type_e::Line:
                return {target, make_indexes(ls.line->idx)};
            case Type_e::Network:
                return {target, make_indexes(ls.line->network->idx)};
            case Type_e::Route:
                return {target, make_indexes(ls.routes)};
            case Type_e::StopPoint: {
                const auto& sps = ls.get_stop_points_section();
                return {target, make_indexes(sps)};
            }
            default:
                return {Type_e::Unknown, Indexes{}};
        }
    }
    pair_indexes operator()(const RailSection& rs) {
        switch (target) {
            case Type_e::Line:
                return {target, make_indexes(rs.line->idx)};
            case Type_e::Network:
                return {target, make_indexes(rs.line->network->idx)};
            case Type_e::Route:
                return {target, make_indexes(rs.line->route_list)};
            case Type_e::StopPoint: {
                const auto& sps = get_stop_points_section(rs, effect);
                return {target, make_indexes(sps)};
            }
            default:
                return {Type_e::Unknown, Indexes{}};
        }
    }
    pair_indexes operator()(const Line* l) { return {Type_e::Line, make_indexes(l->idx)}; }
    pair_indexes operator()(const Route* r) { return {Type_e::Route, make_indexes(r->idx)}; }
    pair_indexes operator()(const MetaVehicleJourney* mvj) { return {Type_e::ValidityPattern, make_indexes(mvj->idx)}; }
};

Indexes Impact::get(Type_e target, PT_Data& pt_data) const {
    Indexes result;
    ImpactVisitor visitor(target, pt_data, severity->effect);

    for (const auto& entitie : informed_entities()) {
        auto pair_type_indexes = boost::apply_visitor(visitor, entitie);
        if (target == pair_type_indexes.first) {
            result.insert(pair_type_indexes.second.begin(), pair_type_indexes.second.end());
        }
    }

    return result;
}

const type::ValidityPattern Impact::get_impact_vp(const boost::gregorian::date_period& production_date) const {
    type::ValidityPattern impact_vp{production_date.begin()};  // bitset are all initialised to 0
    for (const auto& period : this->application_periods) {
        // For each period of the impact loop from the previous day (for pass midnight services) to the last day
        // If the day is in the production_date add it to the vp
        boost::gregorian::day_iterator it(period.begin().date() - boost::gregorian::days(1));
        for (; it <= period.end().date(); ++it) {
            if (!production_date.contains(*it)) {
                continue;
            }
            auto day = (*it - production_date.begin()).days();
            impact_vp.add(day);
        }
    }
    return impact_vp;
}

bool Disruption::is_publishable(const boost::posix_time::ptime& current_time) const {
    if (current_time.is_not_a_date_time()) {
        return false;
    }

    return this->publication_period.contains(current_time);
}

void Disruption::add_impact(const boost::shared_ptr<Impact>& impact, DisruptionHolder& holder) {
    impact->disruption = this;
    impacts.push_back(impact);
    // we register the impact in it's factory
    holder.add_weak_impact(impact);
}

namespace {
template <typename T>
PtObj transform_pt_object(const std::string& uri, T* o) {
    if (o != nullptr) {
        return o;
    }
    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "Impossible to find pt object " << uri);
    return UnknownPtObj();
}
template <typename T>
PtObj transform_pt_object(const std::string& uri, const std::unordered_map<std::string, T*>& map) {
    return transform_pt_object(uri, find_or_default(uri, map));
}
template <typename T>
PtObj transform_pt_object(const std::string& uri, ObjFactory<T>& factory) {
    return transform_pt_object(uri, factory.get_mut(uri));
}
}  // namespace

PtObj make_pt_obj(Type_e type, const std::string& uri, PT_Data& pt_data) {
    switch (type) {
        case Type_e::Network:
            return transform_pt_object(uri, pt_data.networks_map);
        case Type_e::StopArea:
            return transform_pt_object(uri, pt_data.stop_areas_map);
        case Type_e::StopPoint:
            return transform_pt_object(uri, pt_data.stop_points_map);
        case Type_e::Line:
            return transform_pt_object(uri, pt_data.lines_map);
        case Type_e::Route:
            return transform_pt_object(uri, pt_data.routes_map);
        case Type_e::MetaVehicleJourney:
            return transform_pt_object(uri, pt_data.meta_vjs);
        default:
            return UnknownPtObj();
    }
}

bool Impact::operator<(const Impact& other) {
    if (this->severity->priority != other.severity->priority) {
        return this->severity->priority < other.severity->priority;
    }
    if (this->created_at != other.created_at) {
        return this->created_at < other.created_at;
    }
    return this->uri < other.uri;
}

Disruption& DisruptionHolder::make_disruption(const std::string& uri, type::RTLevel lvl) {
    auto it = disruptions_by_uri.find(uri);
    if (it != std::end(disruptions_by_uri)) {
        // we cannot just replace the old one, the model needs to be updated accordingly, so we stop
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "disruption " << uri << " already exists, delete it first");
        throw navitia::exception("disruption already exists");
    }
    auto disruption = std::make_unique<Disruption>(uri, lvl);
    return *(disruptions_by_uri[uri] = std::move(disruption));
}

/*
 * remove the disruption (if it exists) from the collection
 * transfer the ownership of the disruption
 *
 * If it does not exist, return a nullptr
 */
std::unique_ptr<Disruption> DisruptionHolder::pop_disruption(const std::string& uri) {
    const auto it = disruptions_by_uri.find(uri);
    if (it == std::end(disruptions_by_uri)) {
        return {nullptr};
    }
    auto res = std::move(it->second);
    disruptions_by_uri.erase(it);
    return res;
}

const Disruption* DisruptionHolder::get_disruption(const std::string& uri) const {
    const auto it = disruptions_by_uri.find(uri);
    if (it == std::end(disruptions_by_uri)) {
        return nullptr;
    }
    return it->second.get();
}

void DisruptionHolder::add_weak_impact(const boost::weak_ptr<Impact>& weak_impact) {
    weak_impacts.push_back(weak_impact);
}

void DisruptionHolder::clean_weak_impacts() {
    clean_up_weak_ptr(weak_impacts);
}

void DisruptionHolder::forget_vj(const VehicleJourney* vj) {
    for (const auto& impact : vj->meta_vj->get_impacts()) {
        for (auto& stu : impact->aux_info.stop_times) {
            if (stu.stop_time.vehicle_journey == vj) {
                stu.stop_time.vehicle_journey = nullptr;
            }
        }
    }
}

}  // namespace disruption
}  // namespace type
}  // namespace navitia
