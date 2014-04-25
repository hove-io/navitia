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

#include "type/data.h"
#include "utils/init.h"
#include "type/pt_data.h"

using namespace navitia::type;

template<typename T> bool valid_uri(const T* a){
    static_data * s = static_data::get();
    if(s->captionByType(T::type) == "validity_pattern")
        return true;
    if(a->uri.empty()){
        std::cout << "    Code externe non renseigné : " << a->idx << std::endl;
        return false;
    } else {
        return true;
    }
}

template<typename T> bool valid_idx(const T* a, idx_t idx){
    if(idx != a->idx){
        std::cout << "    idx invalide. On a : " << a->idx << ", attendu : " << idx << std::endl;
        return false;
    } else {
        return true;
    }
}

template<typename T> int general_check(const std::vector<T*> & elements){
    static_data * s = static_data::get();
    std::cout << "Vérification générale des " << s->captionByType(T::type) << std::endl;
    int error_count = 0;
    for(size_t i = 0; i < elements.size(); ++i) {
        const T* el = elements[i];
        if(!valid_uri(el)) ++error_count;
        if(!valid_idx(el, i)) ++error_count;
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}

template<typename A, typename B> int check_relations(const std::vector<A*> &as, B* A::*b, const std::vector<B*> &bs){
    static_data * s = static_data::get();
    int error_count = 0;
    std::cout << "Vérification des relations de " << s->captionByType(A::type) << " vers " << s->captionByType(B::type) << std::endl;
    for(const A* a : as){
        if(a->*b != nullptr && (a->*b)->idx > bs.size()){
            std::cout << "    idx invalide : " << (a->*b)->idx << " pour le " << s->captionByType(A::type) << "(" << a->idx << ")" << std::endl;
            error_count++;
        }
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}

template<typename A, typename B> int check_relations(const std::vector<A*> &as, std::vector<B*> A::*b_list, const std::vector<B*> ){
    static_data * s = static_data::get();
    int error_count = 0;
    std::cout << "Vérification des relations de " << s->captionByType(A::type) << " vers N-" << s->captionByType(B::type) << std::endl;
    for(const A* a : as){
        for(const B* b: a->*b_list){
            if(b == nullptr){
                std::cout << "    idx invalide : " << b->idx << " pour le " << s->captionByType(A::type) << "(" << a->idx << ")" << std::endl;
                error_count++;
            }
        }
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}


template<typename A, typename B> int check_relations(const std::vector<A*> &as, std::vector<B*> A::*b_list, const std::vector<B*> &, A* B::*to_a_ptr){
    static_data * s = static_data::get();
    int error_count = 0;
    std::cout << "Vérification des relations de " << s->captionByType(A::type) << " vers N-" << s->captionByType(B::type) << " et retour" << std::endl;
    for(const A* a : as){
        for(const B* b : a->*b_list){
            if(b->*to_a_ptr != a) {
                std::cout << "    idx invalide : " << b->idx << " pour le " << s->captionByType(A::type) << "(" << a->idx << ")" << std::endl;
                error_count++;
            }
        }
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}
int main(int argc, char** argv) {
    navitia::init_app();
    if(argc != 2){
        std::cout << "Utilisation : " << argv[0] << " fichier_navitia.lz4" << std::endl;
    }

    Data d;
    std::cout << "Chargement des données : " << argv[1] << std::endl;
    d.load(argv[1]);
    int error_count = 0;

#define GENERAL_CHECK(type_name, collection_name) error_count += general_check(d.pt_data->collection_name);
    ITERATE_NAVITIA_PT_TYPES(GENERAL_CHECK)

//    error_count += check_relations(d.pt_data->stop_areas, &StopArea::admin_list, d.geo_ref.admins);
    error_count += check_relations(d.pt_data->stop_areas, &StopArea::stop_point_list, d.pt_data->stop_points);
    error_count += check_relations(d.pt_data->stop_areas, &StopArea::stop_point_list, d.pt_data->stop_points, &StopPoint::stop_area);

    error_count += check_relations(d.pt_data->stop_points, &StopPoint::stop_area, d.pt_data->stop_areas);
//    error_count += check_relations(d.pt_data->stop_points, &StopPoint::admin_list, d.geo_ref.admins);
    error_count += check_relations(d.pt_data->stop_points, &StopPoint::network, d.pt_data->networks);
    error_count += check_relations(d.pt_data->stop_points, &StopPoint::journey_pattern_point_list, d.pt_data->journey_pattern_points);
    error_count += check_relations(d.pt_data->stop_points, &StopPoint::journey_pattern_point_list, d.pt_data->journey_pattern_points, &JourneyPatternPoint::stop_point);

    error_count += check_relations(d.pt_data->lines, &Line::company_list, d.pt_data->companies);

    error_count += check_relations(d.pt_data->journey_patterns, &JourneyPattern::route, d.pt_data->routes);
    error_count += check_relations(d.pt_data->routes, &Route::line, d.pt_data->lines);
    error_count += check_relations(d.pt_data->journey_patterns, &JourneyPattern::journey_pattern_point_list, d.pt_data->journey_pattern_points);
    error_count += check_relations(d.pt_data->journey_patterns, &JourneyPattern::vehicle_journey_list, d.pt_data->vehicle_journeys);
    error_count += check_relations(d.pt_data->journey_patterns, &JourneyPattern::vehicle_journey_list, d.pt_data->vehicle_journeys, &VehicleJourney::journey_pattern);

    error_count += check_relations(d.pt_data->journey_pattern_points, &JourneyPatternPoint::stop_point, d.pt_data->stop_points);

    error_count += check_relations(d.pt_data->vehicle_journeys, &VehicleJourney::journey_pattern, d.pt_data->journey_patterns);
    for(const VehicleJourney* vj: d.pt_data->vehicle_journeys){
        for(const StopTime* stop_time : vj->stop_time_list){
            if(stop_time == nullptr){
                std::cout << "    stop_time vaut nullptr pour le vehicle journey(" << vj->idx << ")" << std::endl;
                error_count++;
            }
        }
        for(size_t i = 1; i < vj->stop_time_list.size(); ++i){
            const StopTime* st1 = vj->stop_time_list[i-1];
            const StopTime* st2 = vj->stop_time_list[i];
            const JourneyPatternPoint* jpp1 = st1->journey_pattern_point;
            const JourneyPatternPoint* jpp2 = st2->journey_pattern_point;
            if(jpp1->order + 1!=  jpp2->order){
                std::cout << "Problème de tri des stop_time du vj " << vj->idx << std::endl;
                error_count++;

                for(auto sterr : vj->stop_time_list) {
                    std::cout << "Order : " << sterr->journey_pattern_point->order << std::endl;
                }
            }
        }
    }


    for(const JourneyPattern * journeypattern : d.pt_data->journey_patterns) {
        for(size_t i = 1; i < journeypattern->journey_pattern_point_list.size(); ++i){
            const JourneyPatternPoint* jpp1 = journeypattern->journey_pattern_point_list[i-1];
            const JourneyPatternPoint* jpp2 = journeypattern->journey_pattern_point_list[i];
            if(jpp1->order + 1!=  jpp2->order){
                std::cout << "Problème de tri des journey pattern point du journey pattern" << journeypattern->idx << std::endl;
                error_count++;

                for(auto sterr : journeypattern->journey_pattern_point_list) {
                    std::cout << "Order : " << sterr->order << std::endl;
                }
            }
        }
    }

    idx_t prev_journey_pattern_idx = invalid_idx;

    for(const JourneyPatternPoint* jpp: d.pt_data->journey_pattern_points) {
        if(prev_journey_pattern_idx == invalid_idx || prev_journey_pattern_idx != jpp->journey_pattern->idx) {
            prev_journey_pattern_idx = jpp->journey_pattern->idx;
        } else {
            if(jpp->order+1 != static_cast<int>(prev_journey_pattern_idx)) {
                std::cout << "Probleme de tri du tableau des journey pattern points " << jpp->idx << std::endl;
            }
        }
    }



    return 0;
}
