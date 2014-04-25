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

#include "autocomplete.h"
#include "type/pt_data.h"
namespace navitia { namespace autocomplete {

void compute_score_poi(type::PT_Data&, georef::GeoRef &georef) {
    for (auto it = georef.fl_poi.word_quality_list.begin(); it != georef.fl_poi.word_quality_list.end(); ++it){
        for (navitia::georef::Admin* admin : georef.pois[it->first]->admin_list){
            if(admin->level == 8){
                (it->second).score = georef.fl_admin.word_quality_list.at(admin->idx).score;
            }
        }
    }
}


void compute_score_way(type::PT_Data&, georef::GeoRef &georef) {
    for (auto it = georef.fl_way.word_quality_list.begin(); it != georef.fl_way.word_quality_list.end(); ++it){
        for (navitia::georef::Admin* admin : georef.ways[it->first]->admin_list){
            if (admin->level == 8){
                //georef.fl_way.ac_list.at(it->first).score = georef.fl_admin.ac_list.at(admin->idx).score;
                (it->second).score = georef.fl_admin.word_quality_list.at(admin->idx).score;
            }
        }
    }
}


void compute_score_stop_area(type::PT_Data &pt_data, georef::GeoRef &georef) {
    for(auto it = pt_data.stop_area_autocomplete.word_quality_list.begin(); it != pt_data.stop_area_autocomplete.word_quality_list.end(); ++it){
        for (navitia::georef::Admin* admin : pt_data.stop_areas[it->first]->admin_list){
            if (admin->level == 8){
                (it->second).score = georef.fl_admin.word_quality_list.at(admin->idx).score;
            }
        }
    }
}


void compute_score_stop_point(type::PT_Data &pt_data, georef::GeoRef &georef) {
    //Récupérer le score de son admin du niveau 8 dans le autocomplete: georef.fl_admin
    for (auto it = pt_data.stop_point_autocomplete.word_quality_list.begin(); it != pt_data.stop_point_autocomplete.word_quality_list.end(); ++it){
        for(navitia::georef::Admin* admin : pt_data.stop_points[it->first]->admin_list){
            if (admin->level == 8){
                (it->second).score = georef.fl_admin.word_quality_list.at(admin->idx).score;
            }
        }
    }
}


void compute_score_admin(type::PT_Data &pt_data, georef::GeoRef &georef) {
    int max_score = 0;
    //Pour chaque stop_point incrémenter le score de son admin de niveau 8 par 1.
    for (navitia::type::StopPoint* sp : pt_data.stop_points){
        for (navitia::georef::Admin * admin : sp->admin_list){
            if (admin->level == 8){
                georef.fl_admin.word_quality_list.at(admin->idx).score++;
            }
        }
    }
    //Calculer le max_score des admins
    for (auto it = georef.fl_admin.word_quality_list.begin(); it != georef.fl_admin.word_quality_list.end(); ++it){
        max_score = ((it->second).score > max_score)?(it->second).score:max_score;
    }

    //Ajuster le score de chaque admin an utilisant le max_score.
    for (auto it = georef.fl_admin.word_quality_list.begin(); it != georef.fl_admin.word_quality_list.end(); ++it){
        (it->second).score = max_score == 0 ? 0 : ((it->second).score * 100)/max_score;
    }
}


void compute_score_line(type::PT_Data&, georef::GeoRef&) {
}

template<>
void Autocomplete<type::idx_t>::compute_score(type::PT_Data &pt_data, georef::GeoRef &georef,
                   const type::Type_e type) {
    switch(type){
        case type::Type_e::StopArea:
            compute_score_stop_area(pt_data, georef);
            break;
        case type::Type_e::StopPoint:
            compute_score_stop_point(pt_data, georef);
            break;
        case type::Type_e::Line:
            compute_score_line(pt_data, georef);
            break;
        case type::Type_e::Admin:
            compute_score_admin(pt_data, georef);
            break;
        case type::Type_e::Way:
            compute_score_way(pt_data, georef);
            break;
        case type::Type_e::POI:
            compute_score_poi(pt_data, georef);
            break;
        default:
            break;
    }
}


}}
