#include "proximitylist_api.h"
#include "type/pb_converter.h"

namespace navitia { namespace proximitylist {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
void create_pb(const std::vector<std::pair<type::idx_t, type::GeographicalCoord> >& result, const nt::Type_e type, const nt::Data& data, pbnavitia::ProximityList& pb_pl,type::GeographicalCoord coord){
    for(auto result_item : result){
        pbnavitia::ProximityListItem* item = pb_pl.add_items();
        pbnavitia::PlaceMark* place_mark = item->mutable_object();
        switch(type){
        case nt::Type_e::eStopArea:
            place_mark->set_type(pbnavitia::STOPAREA);
            fill_pb_object(result_item.first, data, place_mark->mutable_stop_area(), 2);
            item->set_name(data.pt_data.stop_areas[result_item.first].name);
            item->set_uri(data.pt_data.stop_areas[result_item.first].uri);
            item->set_distance(coord.distance_to(result_item.second));
            break;
        case nt::Type_e::eCity:
            place_mark->set_type(pbnavitia::CITY);
            fill_pb_object(result_item.first, data, place_mark->mutable_city());
            item->set_name(data.pt_data.cities[result_item.first].name);
            item->set_uri(data.pt_data.cities[result_item.first].uri);
            item->set_distance(coord.distance_to(result_item.second));
            break;
        case nt::Type_e::eStopPoint:
            place_mark->set_type(pbnavitia::STOPPOINT);
            fill_pb_object(result_item.first, data, place_mark->mutable_stop_point(), 2);
            item->set_name(data.pt_data.stop_points[result_item.first].name);
            item->set_uri(data.pt_data.stop_points[result_item.first].uri);
            item->set_distance(coord.distance_to(result_item.second));
            break;

        default:
            break;
        }
    }
}


pbnavitia::Response find(type::GeographicalCoord coord, double distance, const std::vector<nt::Type_e> & filter, const type::Data & data) {
    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::PROXIMITYLIST);

    std::vector<std::pair<type::idx_t, type::GeographicalCoord> > result;
    pbnavitia::ProximityList* pb = response.mutable_proximitylist();
    for(nt::Type_e type : filter){
        switch(type){
        case nt::Type_e::eStopArea:
            result = data.pt_data.stop_area_proximity_list.find_within(coord, distance);
            break;
        case nt::Type_e::eStopPoint:
            result = data.pt_data.stop_point_proximity_list.find_within(coord, distance);
            break;
        case nt::Type_e::eCity:
            result = data.pt_data.city_proximity_list.find_within(coord, distance);
            break;
        default: break;
        }
        create_pb(result, type, data, *pb, coord);
    }
    return response;
}
}} // namespace navitia::proximitylist
