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

namespace navitia { namespace routing {

std::string PathItem::print() const {
    std::stringstream ss;

    if(stop_points.size() < 2) {
        ss << "Section avec moins de 2 stop points, hrmmmm \n";
        return ss.str();
    }

    const navitia::type::StopArea* start = stop_points.front()->stop_area;
    const navitia::type::StopArea* dest = stop_points.back()->stop_area;
    ss << "Section de type ";
    if(type == public_transport)
        ss << "transport en commun";
    else if(type == walking)
        ss << "marche";
    else if(type == stay_in)
        ss << "prolongement de service";
    else if(type ==guarantee)
        ss << "corresopondance garantie";
    ss << "\n";


    if(type == public_transport && ! stop_times.empty()){
        const navitia::type::StopTime* st = stop_times.front();
        const navitia::type::VehicleJourney* vj = st->vehicle_journey;
        const navitia::type::JourneyPattern* journey_pattern = vj->journey_pattern;
        const navitia::type::Route* route = journey_pattern->route;
        const navitia::type::Line* line = route->line;
        ss << "Ligne : " << line->name  << " (" << line->uri << " " << line->idx << "), "
           << "Route : " << route->name  << " (" << route->uri << " " << route->idx << "), "
           << "JourneyPattern : " << journey_pattern->name << " (" << journey_pattern->uri << " " << journey_pattern->idx << "), "
           << "Vehicle journey " << vj->idx << "\n";
    }
    ss << "Départ de " << start->name << "(" << start->uri << " " << start->idx << ") à " << departure << "\n";
    for(auto sp : stop_points){
        ss << "    " << sp->name << " (" << sp->uri << " " << sp->idx << ")" << "\n";
    }
    ss << "Arrivée à " << dest->name << "(" << dest->uri << " " << dest->idx << ") à " << arrival << "\n";
    return ss.str();
}


}}
