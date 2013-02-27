#include "type/data.h"

using namespace navitia::type;

template<typename T> bool valid_uri(const T & a){
    if(a.uri.empty()){
        std::cout << "    Code externe non renseigné : " << a.idx << std::endl;
        return false;
    } else {
        return true;
    }
}

template<typename T> bool valid_idx(const T & a, idx_t idx){
    if(idx != a.idx){
        std::cout << "    idx invalide. On a : " << a.idx << ", attendu : " << idx << std::endl;
        return false;
    } else {
        return true;
    }
}

template<typename T> int general_check(const std::vector<T> & elements){
    static_data * s = static_data::get();
    std::cout << "Vérification générale des " << s->captionByType(T::type) << std::endl;
    int error_count = 0;
    for(size_t i = 0; i < elements.size(); ++i) {
        const T & el = elements[i];
        if(!valid_uri(el)) ++error_count;
        if(!valid_idx(el, i)) ++error_count;
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}

template<typename A, typename B> int check_relations(const std::vector<A> &as, idx_t A::*idx, const std::vector<B> &bs){
    static_data * s = static_data::get();
    int error_count = 0;
    std::cout << "Vérification des relations de " << s->captionByType(A::type) << " vers " << s->captionByType(B::type) << std::endl;
    for(const A & a : as){
        if(a.*idx != invalid_idx && a.*idx > bs.size()){
            std::cout << "    idx invalide : " << a.*idx << " pour le " << s->captionByType(A::type) << "(" << a.idx << ")" << std::endl;
            error_count++;
        }
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}

template<typename A, typename B> int check_relations(const std::vector<A> &as, std::vector<idx_t> A::*idx_list, const std::vector<B> &bs){
    static_data * s = static_data::get();
    int error_count = 0;
    std::cout << "Vérification des relations de " << s->captionByType(A::type) << " vers N-" << s->captionByType(B::type) << std::endl;
    for(const A & a : as){
        for(idx_t idx : a.*idx_list){
            if(idx > bs.size()){
                std::cout << "    idx invalide : " << idx << " pour le " << s->captionByType(A::type) << "(" << a.idx << ")" << std::endl;
                error_count++;
            }
        }
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}


template<typename A, typename B> int check_relations(const std::vector<A> &as, std::vector<idx_t> A::*idx_list, const std::vector<B> &bs, idx_t B::*to_a_idx){
    static_data * s = static_data::get();
    int error_count = 0;
    std::cout << "Vérification des relations de " << s->captionByType(A::type) << " vers N-" << s->captionByType(B::type) << " et retour" << std::endl;
    for(const A & a : as){
        for(idx_t idx : a.*idx_list){
            if(bs[idx].*to_a_idx != a.idx) {
                std::cout << "    idx invalide : " << idx << " pour le " << s->captionByType(A::type) << "(" << a.idx << ")" << std::endl;
                error_count++;
            }
        }
    }
    std::cout << "    Nombre d'erreurs : " << error_count << std::endl;
    return error_count;
}
int main(int argc, char** argv) {
    if(argc != 2){
        std::cout << "Utilisation : " << argv[0] << " fichier_navitia.lz4" << std::endl;
    }

    Data d;
    std::cout << "Chargement des données : " << argv[1] << std::endl;
    d.load(argv[1]);
    int error_count = 0;

    error_count += general_check(d.pt_data.stop_areas);
    error_count += check_relations(d.pt_data.stop_areas, &StopArea::city_idx, d.pt_data.cities);
    error_count += check_relations(d.pt_data.stop_areas, &StopArea::stop_point_list, d.pt_data.stop_points);
    error_count += check_relations(d.pt_data.stop_areas, &StopArea::stop_point_list, d.pt_data.stop_points, &StopPoint::stop_area_idx);

    error_count += general_check(d.pt_data.stop_points);
    error_count += check_relations(d.pt_data.stop_points, &StopPoint::stop_area_idx, d.pt_data.stop_areas);
    error_count += check_relations(d.pt_data.stop_points, &StopPoint::city_idx, d.pt_data.cities);
    error_count += check_relations(d.pt_data.stop_points, &StopPoint::physical_mode_idx, d.pt_data.physical_modes);
    error_count += check_relations(d.pt_data.stop_points, &StopPoint::network_idx, d.pt_data.networks);
    error_count += check_relations(d.pt_data.stop_points, &StopPoint::journey_pattern_point_list, d.pt_data.journey_pattern_points);
    error_count += check_relations(d.pt_data.stop_points, &StopPoint::journey_pattern_point_list, d.pt_data.journey_pattern_points, &JourneyPatternPoint::stop_point_idx);

    error_count += general_check(d.pt_data.lines);
    error_count += check_relations(d.pt_data.lines, &Line::company_list, d.pt_data.companies);

    error_count += general_check(d.pt_data.journey_patterns);
    error_count += check_relations(d.pt_data.journey_patterns, &JourneyPattern::route_idx, d.pt_data.routes);
    error_count += check_relations(d.pt_data.routes, &Route::line_idx, d.pt_data.lines);
    error_count += check_relations(d.pt_data.journey_patterns, &JourneyPattern::journey_pattern_point_list, d.pt_data.journey_pattern_points);
    error_count += check_relations(d.pt_data.journey_patterns, &JourneyPattern::vehicle_journey_list, d.pt_data.vehicle_journeys);
    error_count += check_relations(d.pt_data.journey_patterns, &JourneyPattern::vehicle_journey_list, d.pt_data.vehicle_journeys, &VehicleJourney::journey_pattern_idx);

    error_count += general_check(d.pt_data.journey_pattern_points);
    error_count += check_relations(d.pt_data.journey_pattern_points, &JourneyPatternPoint::stop_point_idx, d.pt_data.stop_points);

    error_count += general_check(d.pt_data.vehicle_journeys);
    error_count += check_relations(d.pt_data.vehicle_journeys, &VehicleJourney::journey_pattern_idx, d.pt_data.journey_patterns);
    error_count += check_relations(d.pt_data.vehicle_journeys, &VehicleJourney::vehicle_idx, d.pt_data.vehicles);
    for(const VehicleJourney &vj: d.pt_data.vehicle_journeys){
        for(idx_t idx : vj.stop_time_list){
            if(idx > d.pt_data.stop_times.size()){
                std::cout << "    idx invalide : " << idx << " pour le vehicle journey(" << vj.idx << ")" << std::endl;
                error_count++;
            }
        }
        for(size_t i = 1; i < vj.stop_time_list.size(); ++i){
            StopTime st1 = d.pt_data.stop_times[vj.stop_time_list[i-1]];
            StopTime st2 = d.pt_data.stop_times[vj.stop_time_list[i]];
            JourneyPatternPoint rp1 = d.pt_data.journey_pattern_points[st1.journey_pattern_point_idx];
            JourneyPatternPoint rp2 = d.pt_data.journey_pattern_points[st2.journey_pattern_point_idx];
            if(rp1.order + 1!=  rp2.order){
                std::cout << "Problème de tri des stop_time du vj " << vj.idx << std::endl;
                error_count++;
            }
        }
    }

    error_count += general_check(d.pt_data.cities);
    error_count += general_check(d.pt_data.companies);
    error_count += general_check(d.pt_data.countries);
    error_count += general_check(d.pt_data.departments);
    error_count += general_check(d.pt_data.physical_modes);
    error_count += general_check(d.pt_data.commercial_modes);
    error_count += general_check(d.pt_data.vehicles);

    std::cout << "Vérification des stop_times" << std::endl;
    for(size_t i = 0; i < d.pt_data.stop_times.size(); ++i){
        if(!valid_idx(d.pt_data.stop_times[i], i)) ++error_count;
    }


    return 0;
}
