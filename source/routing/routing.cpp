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

#include "routing.h"
#include "type/data.h"

namespace navitia { namespace routing {

std::string PathItem::print() const {
    std::stringstream ss;

    const navitia::type::StopArea* start = stop_points.front()->stop_area;
    const navitia::type::StopArea* dest = stop_points.back()->stop_area;
    switch (type) {
    case ItemType::public_transport: ss << "public transport"; break;
    case ItemType::walking: ss << "walking"; break;
    case ItemType::stay_in: ss << "stay in"; break;
    case ItemType::waiting: ss << "waiting"; break;
    case ItemType::boarding: ss << "boarding"; break;
    case ItemType::alighting: ss << "alighting"; break;
    default: ss << "unknown"; break;
    }
    ss << " section\n";

    if(type == ItemType::public_transport && ! stop_times.empty()){
        const navitia::type::StopTime* st = stop_times.front();
        const navitia::type::VehicleJourney* vj = st->vehicle_journey;
        const navitia::type::Route* route = vj->route;
        const navitia::type::Line* line = route->line;
        ss << "Line : " << line->name  << " (" << line->uri << " " << line->idx << "), "
           << "Route : " << route->name  << " (" << route->uri << " " << route->idx << "), "
           << "Vehicle journey " << vj->idx << "\n";
    }
    ss << "From " << start->name << "(" << start->uri << " " << start->idx << ") at " << departure << "\n";
    for(auto sp : stop_points){
        ss << "    " << sp->name << " (" << sp->uri << " " << sp->idx << ")" << "\n";
    }
    ss << "To " << dest->name << "(" << dest->uri << " " << dest->idx << ") at " << arrival << "\n";
    return ss.str();
}

bool is_same_stop_point(const type::EntryPoint& point, const type::StopPoint& stop_point) {
    return point.type == type::Type_e::StopPoint && point.uri == stop_point.uri;
}

bool use_crow_fly(const type::EntryPoint& point, const type::StopPoint& stop_point,
                  const georef::Path& street_network_path, const type::Data& data) {
    if (is_same_stop_point(point, stop_point)) {
        return false;
    }
    if (street_network_path.path_items.empty()) {
        return true;
    }
    if(point.type == type::Type_e::StopArea){
        //if we have a stop area in the request,
        //we only do a crowfly section if the used stop point belongs to this stop area
        return point.uri == stop_point.stop_area->uri;
    }else if(point.type == type::Type_e::Admin){
        //we handle the main_stop_area of an admin here
        //we want a crowfly for all main_stop_areas of an admin,
        //even if the stop_area is not in the admin
        auto admin = data.geo_ref->admins[data.geo_ref->admin_map[point.uri]];
        auto it = find_if(begin(admin->main_stop_areas), end(admin->main_stop_areas),
                [stop_point](const type::StopArea* stop_area){return stop_area == stop_point.stop_area;});
        return it != end(admin->main_stop_areas);
    }else{
        //if the request is on any other type we don't want a crowfly section
        return false;
    }
}

}}//namespace
