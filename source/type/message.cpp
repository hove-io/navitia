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

#include "type/message.h"
#include "type/pt_data.h"
#include "utils/logger.h"

#include <boost/format.hpp>

namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia { namespace type { namespace disruption {

std::vector<ImpactedVJ>
get_impacted_vehicle_journeys(const LineSection& ls,
                              const Impact& impact,
                              const boost::gregorian::date_period& production_period,
                              type::RTLevel rt_level) {
    auto log = log4cplus::Logger::getInstance("log");
    // Get all impacted VJs and compute the corresponding base_canceled vp
    std::vector<ImpactedVJ> vj_vp_pairs;

    // Computing a validity_pattern of impact used to pre-filter concerned vjs later
    type::ValidityPattern impact_vp = impact.get_impact_vp(production_period);

    // Loop on impacted routes of the line section
    for(const auto* route: ls.routes) {
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
            auto section = vj.get_sections_stop_points(ls.start_point, ls.end_point);
            // If the vj pass by both stops both elements will be different than nullptr, otherwise
            // it's not passing by both stops and should not be impacted
            if( ! section.empty()) {
                // Once we know the line section is part of the vj we compute the vp for the adapted_vj
                LOG4CPLUS_TRACE(log, "vj " << vj.uri << " pass by both stops, might be affected.");
                nt::ValidityPattern new_vp{vj.validity_patterns[rt_level]->beginning_date};
                for(const auto& period : impact.application_periods) {
                    // get the vp of the section
                    new_vp.days |= vj.get_vp_for_section(section, rt_level, period).days;
                }
                // If there is effective days for the adapted vp we're keeping it
                if(!new_vp.days.none()){
                    LOG4CPLUS_TRACE(log, "vj " << vj.uri << " is affected, keeping it.");
                    new_vp.days >>= vj.shift;
                    vj_vp_pairs.emplace_back(&vj, new_vp, std::move(section));
                }
            }
            return true;
        });
    }
    return vj_vp_pairs;
}

struct InformedEntitiesLinker: public boost::static_visitor<> {
    const boost::shared_ptr<Impact>& impact;
    const boost::gregorian::date_period& production_period;
    type::RTLevel rt_level;
    InformedEntitiesLinker(const boost::shared_ptr<Impact>& impact,
                           const boost::gregorian::date_period& production_period,
                           RTLevel rt_level):
        impact(impact), production_period(production_period), rt_level(rt_level) {}

    template <typename NavitiaPTObject>
    void operator()(NavitiaPTObject* bo) const {
        // the the ptobject that match a navitia object, we can just add the impact to the object
        bo->add_impact(impact);
    }
    void operator()(const nt::disruption::LineSection& line_section) const {
        // for a line section it's a bit more complex, we need to register the impact
        // to all impacted stoppoints and vehiclejourneys
        auto impacted_vjs = get_impacted_vehicle_journeys(line_section, *impact, production_period, rt_level);
        std::set<type::StopPoint*> impacted_stop_points;
        std::set<type::MetaVehicleJourney*> impacted_meta_vjs;
        if (impacted_vjs.empty()) {
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "line section impact " << impact->uri
                        << " does not impact any vj, it will not be linked to anything");
        }
        for (auto& impacted_vj : impacted_vjs) {
            const auto* vj = impacted_vj.vj;
            if (impacted_meta_vjs.insert(vj->meta_vj).second) {
                // it's the first time we see this metavj, we add the impact to it
                vj->meta_vj->add_impact(impact);
            }
            for (auto* sp: impacted_vj.impacted_stops) {
                if (impacted_stop_points.insert(sp).second) {
                    // it's the first time we see this stoppoint, we add the impact to it
                    sp->add_impact(impact);
                }
            }
        }
    }
    void operator()(const nt::disruption::UnknownPtObj&) const {}
};

void Impact::link_informed_entity(PtObj ptobj, boost::shared_ptr<Impact>& impact,
                          const boost::gregorian::date_period& production_period, type::RTLevel rt_level) {
    InformedEntitiesLinker v(impact, production_period, rt_level);
    boost::apply_visitor(v, ptobj);

    impact->_informed_entities.push_back(std::move(ptobj));
}

bool Impact::is_valid(const boost::posix_time::ptime& publication_date, const boost::posix_time::time_period& active_period) const {

    if(publication_date.is_not_a_date_time() && active_period.is_null()){
        return false;
    }

    // we check if we want to publish the impact
    if (! disruption->is_publishable(publication_date)) {
        return false;
    }

    //if we have a active_period, we check if the impact applies on this period
    if (active_period.is_null()) {
        return true;
    }

    for (const auto& period: application_periods) {
        if (! period.intersection(active_period).is_null()) {
            return true;
        }
    }
    return false;
}

bool Impact::is_relevant(const std::vector<const StopTime*>& stop_times) const {
    // No delay on the section
    if (severity->effect == nt::disruption::Effect::SIGNIFICANT_DELAYS && ! aux_info.stop_times.empty()) {
        // We don't handle removed or added stop, but we already match
        // on SIGNIFICANT_DELAYS, thus we should not have that here
        // (removed and added stop should be a DETOUR)
        const auto nb_aux = aux_info.stop_times.size();
        for (const auto& st: stop_times) {
            const auto base_st = st->get_base_stop_time();
            if (base_st == nullptr) { return true; }
            const auto idx = base_st->order();
            if (idx >= nb_aux) { return true; }
            const auto& amended_st = aux_info.stop_times.at(idx).stop_time;
            if (base_st->arrival_time != amended_st.arrival_time) { return true; }
            if (base_st->departure_time != amended_st.departure_time) { return true; }
        }
        return false;
    }

    // line section not relevant
    auto line_section_impacted_obj_it = boost::find_if(informed_entities(), [](const PtObj& ptobj) {
            return boost::get<LineSection>(&ptobj) != nullptr;
        });
    if (line_section_impacted_obj_it != informed_entities().end()) {
        // note in this we take the premise that an impact
        // cannot impact a line section AND a vj
        for (const auto& st: stop_times) {
            // if one stop point of the stoptimes is impacted by the same impact
            // it means the section is impacted
            for (const auto& sp_message: st->stop_point->get_impacts()) {
                if (sp_message.get() == this) {
                    return true;
                }
            }
        }
        return false;
    }

    // else, no reason to not be interested by it
    return true;
}

bool Impact::is_only_line_section() const {
    if (_informed_entities.empty()) { return false; }
    for (const auto& entity: informed_entities()) {
        if (! boost::get<nt::disruption::LineSection>(&entity)) {
            return false;
        }
    }
    return true;
}

const type::ValidityPattern Impact::get_impact_vp(const boost::gregorian::date_period& production_date) const {
    type::ValidityPattern impact_vp{production_date.begin()}; // bitset are all initialised to 0
    for (const auto& period: this->application_periods) {
        // For each period of the impact loop from the previous day (for pass midnight services) to the last day
        // If the day is in the production_date add it to the vp
        boost::gregorian::day_iterator it(period.begin().date() - boost::gregorian::days(1));
        for (; it <= period.end().date() ; ++it) {
            if (! production_date.contains(*it)) { continue; }
            auto day = (*it - production_date.begin()).days();
            impact_vp.add(day);
        }
    }
    return impact_vp;
}

bool Disruption::is_publishable(const boost::posix_time::ptime& current_time) const{
    if(current_time.is_not_a_date_time()){
        return false;
    }

    if (this->publication_period.contains(current_time)) {
        return true;
    }
    return false;
}

void Disruption::add_impact(const boost::shared_ptr<Impact>& impact, DisruptionHolder& holder){
    impact->disruption = this;
    impacts.push_back(impact);
    // we register the impact in it's factory
    holder.add_weak_impact(impact);
}

namespace {
template<typename T>
PtObj transform_pt_object(const std::string& uri, T* o) {
    if (o != nullptr) {
        return o;
    } else {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "Impossible to find pt object " << uri);
        return UnknownPtObj();
    }
}
template<typename T>
PtObj transform_pt_object(const std::string& uri,
                          const std::unordered_map<std::string, T*>& map) {
    return transform_pt_object(uri, find_or_default(uri, map));
}
template<typename T>
PtObj transform_pt_object(const std::string& uri, ObjFactory<T>& factory) {
    return transform_pt_object(uri, factory.get_mut(uri));
}
}

PtObj make_pt_obj(Type_e type,
                  const std::string& uri,
                  PT_Data& pt_data) {
    switch (type) {
    case Type_e::Network: return transform_pt_object(uri, pt_data.networks_map);
    case Type_e::StopArea: return transform_pt_object(uri, pt_data.stop_areas_map);
    case Type_e::StopPoint: return transform_pt_object(uri, pt_data.stop_points_map);
    case Type_e::Line: return transform_pt_object(uri, pt_data.lines_map);
    case Type_e::Route: return transform_pt_object(uri, pt_data.routes_map);
    case Type_e::MetaVehicleJourney: return transform_pt_object(uri, pt_data.meta_vjs);
    default: return UnknownPtObj();
    }
}

bool Impact::operator<(const Impact& other){
    if(this->severity->priority != other.severity->priority){
        return this->severity->priority < other.severity->priority;
    }if(this->created_at != other.created_at){
        return this->created_at < other.created_at;
    }else{
        return this->uri < other.uri;
    }
}

Disruption& DisruptionHolder::make_disruption(const std::string& uri, type::RTLevel lvl) {
    auto it = disruptions_by_uri.find(uri);
    if (it != std::end(disruptions_by_uri)) {
        // we cannot just replace the old one, the model needs to be updated accordingly, so we stop
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"), "disruption " << uri
                       << " already exists, delete it first");
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

void DisruptionHolder::add_weak_impact(boost::weak_ptr<Impact> weak_impact) {
    weak_impacts.push_back(weak_impact);
}

void DisruptionHolder::clean_weak_impacts(){
    clean_up_weak_ptr(weak_impacts);
}

void DisruptionHolder::forget_vj(const VehicleJourney* vj) {
    for (const auto& impact: vj->meta_vj->get_impacts()) {
        for (auto& stu: impact->aux_info.stop_times) {
            if (stu.stop_time.vehicle_journey == vj) {
                stu.stop_time.vehicle_journey = nullptr;
            }
        }
    }
}

namespace detail {

const StopTime* AuxInfoForMetaVJ::get_base_stop_time(const StopTimeUpdate& stu) const {
    //TODO check effect when available, we can do this only for UPDATED
    auto* vj = stu.stop_time.vehicle_journey;
    auto log = log4cplus::Logger::getInstance("log");
    if (! vj) {
        LOG4CPLUS_WARN(log, "impossible to find corresponding base stoptime "
                            "since the stoptime update is not yet associated to a vj");
        return nullptr;
    }

    const auto* base_vj = vj->get_corresponding_base();

    if (! base_vj) {
        return nullptr;
    }

    size_t idx = 0;
    for (const auto& stop_update: stop_times) {
        // TODO check stop_update effect not to consider added stops
        if (&stop_update != &stu) {
            idx ++;
            continue;
        }
        if (idx >= base_vj->stop_time_list.size()) {
            LOG4CPLUS_WARN(log, "The stoptime update list of " << base_vj->meta_vj->uri <<
                                " is not consistent with the list of stoptimes");
            return nullptr;
        }
        const auto& base_st = base_vj->stop_time_list[idx];
        if (stu.stop_time.stop_point != base_st.stop_point) {
            LOG4CPLUS_WARN(log, "The stoptime update list of " << base_vj->meta_vj->uri <<
                                " is not consistent with the list of stoptimes, stoppoint are different");
            return nullptr;
        }
        return &base_st;
    }

    return nullptr;
}
}

}}}//namespace
