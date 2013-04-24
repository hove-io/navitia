#include "proximitylist_api.h"
#include "type/pb_converter.h"

namespace navitia { namespace proximitylist {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
void create_pb(const std::vector<std::pair<type::idx_t, type::GeographicalCoord> >& result,
          const nt::Type_e type, uint32_t depth, const nt::Data& data,
          pbnavitia::Response& pb_response,type::GeographicalCoord coord){
    for(auto result_item : result){
        pbnavitia::Place* place = pb_response.add_places();
        //on récupére la date pour les impacts
        auto current_date = boost::posix_time::second_clock::local_time();
        switch(type){
        case nt::Type_e::StopArea:
            fill_pb_object(result_item.first, data, place->mutable_stop_area(), depth, current_date);
            place->set_name(data.pt_data.stop_areas[result_item.first].name);
            place->set_uri(data.pt_data.stop_areas[result_item.first].uri);
            place->set_distance(coord.distance_to(result_item.second));
            break;
        case nt::Type_e::StopPoint:
            fill_pb_object(result_item.first, data, place->mutable_stop_point(), depth, current_date);
            place->set_name(data.pt_data.stop_points[result_item.first].name);
            place->set_uri(data.pt_data.stop_points[result_item.first].uri);
            place->set_distance(coord.distance_to(result_item.second));
            break;

        default:
            break;
        }
    }
}


pbnavitia::Response find(type::GeographicalCoord coord, double distance,
                         const std::vector<nt::Type_e> & filter,
                         uint32_t depth,
                         const type::Data & data) {
    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::places_nearby);

    std::vector<std::pair<type::idx_t, type::GeographicalCoord> > result;
    for(nt::Type_e type : filter){
        switch(type){
        case nt::Type_e::StopArea:
            result = data.pt_data.stop_area_proximity_list.find_within(coord, distance);
            break;
        case nt::Type_e::StopPoint:
            result = data.pt_data.stop_point_proximity_list.find_within(coord, distance);
            break;
        default: break;
        }
        create_pb(result, type, depth, data, response, coord);
    }
    std::sort(response.mutable_places()->begin(), response.mutable_places()->end(),
              [](pbnavitia::Place a, pbnavitia::Place b){return a.distance() < b.distance();});
    return response;
}
}} // namespace navitia::proximitylist
