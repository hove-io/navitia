#include "proximitylist_api.h"
#include "type/pb_converter.h"
#include "utils/paginate.h"

namespace navitia { namespace proximitylist {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
typedef std::tuple<nt::idx_t, nt::GeographicalCoord, nt::Type_e> t_result;
typedef std::pair<nt::idx_t, nt::GeographicalCoord> idx_coord;
void create_pb(const std::vector<t_result>& result, uint32_t depth, const nt::Data& data,
               pbnavitia::Response& pb_response, type::GeographicalCoord coord){

    for(auto result_item : result){
        pbnavitia::Place* place = pb_response.add_places_nearby();
        //on récupére la date pour les impacts
        auto current_date = boost::posix_time::second_clock::local_time();
        auto idx = std::get<0>(result_item);
        auto coord_item = std::get<1>(result_item);
        auto type = std::get<2>(result_item);
        switch(type){
        case nt::Type_e::StopArea:
            fill_pb_object(data.pt_data.stop_areas[idx], data, place->mutable_stop_area(), depth,
                           current_date);
            place->set_name(data.pt_data.stop_areas[idx]->name);
            place->set_uri(data.pt_data.stop_areas[idx]->uri);
            place->set_distance(coord.distance_to(coord_item));
            place->set_embedded_type(pbnavitia::STOP_AREA);
            break;
        case nt::Type_e::StopPoint:
            fill_pb_object(data.pt_data.stop_points[idx], data, place->mutable_stop_point(), depth,
                           current_date);
            place->set_name(data.pt_data.stop_points[idx]->name);
            place->set_uri(data.pt_data.stop_points[idx]->uri);
            place->set_distance(coord.distance_to(coord_item));
            place->set_embedded_type(pbnavitia::STOP_POINT);
            break;
        case nt::Type_e::POI:
            fill_pb_object(data.geo_ref.pois[idx], data, place->mutable_poi(), depth,
                           current_date);
            place->set_name(data.geo_ref.pois[idx]->name);
            place->set_uri(data.geo_ref.pois[idx]->uri);
            place->set_distance(coord.distance_to(coord_item));
            place->set_embedded_type(pbnavitia::POI);
            break;
        default:
            break;
        }
    }
}


pbnavitia::Response find(const type::GeographicalCoord& coord, const double distance,
                         const std::vector<nt::Type_e>& filter,
                         const uint32_t depth, const uint32_t count, const uint32_t start_page,
                         const type::Data & data) {
    pbnavitia::Response response;
    int total_result = 0;
    std::vector<t_result > result;
    auto end_pagination = (start_page+1)*count;
    for(nt::Type_e type : filter){
        std::vector<idx_coord> list;
        switch(type){
        case nt::Type_e::StopArea:
            list = data.pt_data.stop_area_proximity_list.find_within(coord, distance);
            break;
        case nt::Type_e::StopPoint:
            list = data.pt_data.stop_point_proximity_list.find_within(coord, distance);
            break;
        case nt::Type_e::POI:
            list = data.geo_ref.poi_proximity_list.find_within(coord, distance);
            break;
        default: break;
        }
        if(!list.empty()) {
            total_result += list.size();
            auto nb_sort = (list.size() < end_pagination) ? list.size():end_pagination;
            std::partial_sort(list.begin(), list.begin() + nb_sort, list.end(),
                [&](idx_coord a, idx_coord b)
                {return coord.distance_to(a.second)< coord.distance_to(b.second);});
            list.resize(nb_sort);
            for(auto idx_coord : list) {
                result.push_back(t_result(idx_coord.first, idx_coord.second, type));
            }
        }

    }
    auto nb_sort = (result.size() < end_pagination) ? result.size():end_pagination;
    std::partial_sort(result.begin(), result.begin() + nb_sort, result.end(),
        [&](t_result t1, t_result t2)
        {   auto a = std::get<1>(t1); auto b = std::get<1>(t2);
            return coord.distance_to(a)< coord.distance_to(b);});
    result = paginate(result, count, start_page);
    create_pb(result, depth, data, response, coord);
    auto pagination = response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(result.size());
    return response;
}
}} // namespace navitia::proximitylist
