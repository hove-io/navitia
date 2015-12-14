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
PtObj transform_pt_object(const std::string& uri, T* o, const boost::shared_ptr<Impact>& impact) {
    if (o != nullptr) {
        if (impact){
            o->add_impact(impact);
        }
        return o;
    } else {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "Impossible to find pt object " << uri);
        return UnknownPtObj();
    }
}
template<typename T>
PtObj transform_pt_object(const std::string& uri,
                          const std::unordered_map<std::string, T*>& map,
                          const boost::shared_ptr<Impact>& impact) {
    return transform_pt_object(uri, find_or_default(uri, map), impact);
}
template<typename T>
PtObj transform_pt_object(const std::string& uri,
                          ObjFactory<T>& factory,
                          const boost::shared_ptr<Impact>& impact) {
    return transform_pt_object(uri, factory.get_mut(uri), impact);
}
}

PtObj make_pt_obj(Type_e type,
                  const std::string& uri,
                  PT_Data& pt_data,
                  const boost::shared_ptr<Impact>& impact) {
    switch (type) {
    case Type_e::Network: return transform_pt_object(uri, pt_data.networks_map, impact);
    case Type_e::StopArea: return transform_pt_object(uri, pt_data.stop_areas_map, impact);
    case Type_e::StopPoint: return transform_pt_object(uri, pt_data.stop_points_map, impact);
    case Type_e::Line: return transform_pt_object(uri, pt_data.lines_map, impact);
    case Type_e::Route: return transform_pt_object(uri, pt_data.routes_map, impact);
    case Type_e::MetaVehicleJourney: return transform_pt_object(uri, pt_data.meta_vjs, impact);
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
 * If it does not exists return a nullptr
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

void DisruptionHolder::add_weak_impact(boost::weak_ptr<Impact> weak_impact) {
    weak_impacts.push_back(weak_impact);
}

void DisruptionHolder::clean_weak_impacts(){
    clean_up_weak_ptr(weak_impacts);
}

const StopTime* StopTimeUpdate::get_base_stop_time() const {
    return nullptr;
//    //TODO check effect when available, we can do this only for UPDATED
//    auto* vj = stop_time.vehicle_journey;
//    if (! vj) {
//        //TODO log
//        return nullptr;
//    }

//    const auto* base_vj = vj->meta_vj->get_base_vj_circulating_at_date();

//    if (! base_vj) {
//        return nullptr;
//    }

}

}}}//namespace
