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

#include "disruption.h"
#include "ptreferential/ptreferential.h"
#include "type/data.h"

namespace navitia { namespace disruption {

static int min_priority(const DisruptionSet& disruptions){
    int min = std::numeric_limits<int>::max();
    for(const auto& wp: disruptions){
        auto i = wp.lock();
        if(!i || !i->severity) continue;
        if(i->severity->priority < min){
            min = i->severity->priority;
        }
    }
    return min;
}


bool Comp::operator()(const boost::weak_ptr<type::new_disruption::Impact>& lhs,
                      const boost::weak_ptr<type::new_disruption::Impact>& rhs){
    auto i1 = lhs.lock();
    auto i2 = rhs.lock();
    if(!i1){
        return true;
    }
    if(!i2){
        return false;
    }
    return *i1 < *i2;
}

Disrupt& Disruption::find_or_create(const type::Network* network){
    auto find_predicate = [&](const Disrupt& network_disrupt) {
        return network == network_disrupt.network;
    };
    auto it = std::find_if(this->disrupts.begin(),
                           this->disrupts.end(),
                           find_predicate);
    if(it == this->disrupts.end()){
        Disrupt dist;
        dist.network = network;
        dist.idx = this->disrupts.size();
        this->disrupts.push_back(dist);
        return disrupts.back();
    }
    return *it;
}

void Disruption::add_stop_areas(const std::vector<type::idx_t>& network_idx,
                      const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data& d,
                      const boost::posix_time::ptime now){

    for(auto idx : network_idx){
        const auto* network = d.pt_data->networks[idx];
        std::string new_filter = "network.uri=" + network->uri;
        if(!filter.empty()){
            new_filter  += " and " + filter;
        }
        std::vector<type::idx_t> line_list;

       try{
            line_list = ptref::make_query(type::Type_e::StopArea, new_filter,
                    forbidden_uris, d);
        } catch(const ptref::parsing_error &parse_error) {
            LOG4CPLUS_WARN(logger, "Disruption::add_stop_areas : Unable to parse filter "
                                + parse_error.more);
        } catch(const ptref::ptref_error &ptref_error){
            LOG4CPLUS_WARN(logger, "Disruption::add_stop_areas : ptref : "  + ptref_error.more);
        }
        for(auto stop_area_idx: line_list){
            const auto* stop_area = d.pt_data->stop_areas[stop_area_idx];
            auto v = stop_area->get_publishable_messages(now);
            for(const auto* stop_point: stop_area->stop_point_list){
                auto vsp = stop_point->get_publishable_messages(now);
                v.insert(v.end(), vsp.begin(), vsp.end());
            }
            if (!v.empty()){
                Disrupt& dist = this->find_or_create(network);
                auto find_predicate = [&](const std::pair<const type::StopArea*, DisruptionSet>& item) {
                    return item.first == stop_area;
                };
                auto it = std::find_if(dist.stop_areas.begin(),
                                       dist.stop_areas.end(),
                                       find_predicate);
                if(it == dist.stop_areas.end()){
                    dist.stop_areas.push_back(std::make_pair(stop_area, DisruptionSet(v.begin(), v.end())));
                }else{
                    it->second.insert(v.begin(), v.end());
                }
            }
        }
    }
}

void Disruption::add_networks(const std::vector<type::idx_t>& network_idx,
                      const type::Data &d,
                      const boost::posix_time::ptime now){

    for(auto idx : network_idx){
        const auto* network = d.pt_data->networks[idx];
        if (network->has_publishable_message(now)){
            auto& res = this->find_or_create(network);
            auto v = network->get_publishable_messages(now);
            res.network_disruptions.insert(v.begin(), v.end());
        }
    }
}

void Disruption::add_lines(const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data& d,
                      const boost::posix_time::ptime now){

    std::vector<type::idx_t> line_list;
    try{
        line_list  = ptref::make_query(type::Type_e::Line, filter, forbidden_uris, d);
    } catch(const ptref::parsing_error &parse_error) {
        LOG4CPLUS_WARN(logger, "Disruption::add_lines : Unable to parse filter " + parse_error.more);
    } catch(const ptref::ptref_error &ptref_error){
        LOG4CPLUS_WARN(logger, "Disruption::add_lines : ptref : "  + ptref_error.more);
    }
    for(auto idx : line_list){
        const auto* line = d.pt_data->lines[idx];
        auto v = line->get_publishable_messages(now);
        for(const auto* route: line->route_list){
            auto vr = route->get_publishable_messages(now);
            v.insert(v.end(), vr.begin(), vr.end());
        }
        if (!v.empty()){
            Disrupt& dist = this->find_or_create(line->network);
            auto find_predicate = [&](const std::pair<const type::Line*, DisruptionSet>& item) {
                return line == item.first;
            };
            auto it = std::find_if(dist.lines.begin(),
                                   dist.lines.end(),
                                   find_predicate);
            if(it == dist.lines.end()){
                dist.lines.push_back(std::make_pair(line, DisruptionSet(v.begin(), v.end())));
            }else{
                it->second.insert(v.begin(), v.end());
            }
        }
    }
}

void Disruption::sort_disruptions(){

    auto sort_disruption = [&](const Disrupt& d1, const Disrupt& d2){
            return d1.network->idx < d2.network->idx;
    };

    auto sort_lines = [&](const std::pair<const type::Line*, DisruptionSet>& l1,
                          const std::pair<const type::Line*, DisruptionSet>& l2) {
        int p1 = min_priority(l1.second);
        int p2 = min_priority(l2.second);
        if(p1 != p2){
            return p1 < p2;
        }else if(l1.first->code != l2.first->code){
            return l1.first->code < l2.first->code;
        }else{
            return l1.first->name < l2.first->name;
        }
    };

    std::sort(this->disrupts.begin(), this->disrupts.end(), sort_disruption);
    for(auto& disrupt : this->disrupts){
        std::sort(disrupt.lines.begin(), disrupt.lines.end(), sort_lines);
    }

}

void Disruption::disruptions_list(const std::string& filter,
                        const std::vector<std::string>& forbidden_uris,
                        const type::Data& d,
                        const boost::posix_time::ptime now){

    std::vector<type::idx_t> network_idx = ptref::make_query(type::Type_e::Network, filter,
                                                             forbidden_uris, d);
    add_networks(network_idx, d, now);
    add_lines(filter, forbidden_uris, d, now);
    add_stop_areas(network_idx, filter, forbidden_uris, d, now);
    sort_disruptions();
}
const std::vector<Disrupt>& Disruption::get_disrupts() const{
    return this->disrupts;
}

size_t Disruption::get_disrupts_size(){
    return this->disrupts.size();
}

}}
