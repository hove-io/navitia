#include "proximitylist_api.h"
#include "type/pb_converter.h"

namespace navitia { namespace proximitylist {
/**
 * se charge de remplir l'objet protocolbuffer firstletter passé en paramètre
 *
 */
void create_pb(const std::vector<std::pair<type::idx_t, type::GeographicalCoord> >& result, const nt::Type_e type, const nt::Data& data, pbnavitia::ProximityList& pb_pl,type::GeographicalCoord coord){
    for(auto result_item : result){
        pbnavitia::ProximityListItem* item = pb_pl.add_items();
        google::protobuf::Message* child = NULL;
        switch(type){
            case nt::Type_e::eStopArea:
                child = item->mutable_stop_area();
                fill_pb_object<nt::Type_e::eStopArea>(result_item.first, data, child, 2);
                item->set_name(data.pt_data.stop_areas[result_item.first].name);
                item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_areas[result_item.first]));
                item->set_distance(coord.distance_to(result_item.second));
                break;
            case nt::Type_e::eCity:
                child = item->mutable_city();
                fill_pb_object<nt::Type_e::eCity>(result_item.first, data, child);
                item->set_name(data.pt_data.cities[result_item.first].name);
                item->set_uri(nt::EntryPoint::get_uri(data.pt_data.cities[result_item.first]));
                item->set_distance(coord.distance_to(result_item.second));
                break;
            case nt::Type_e::eStopPoint:
                child = item->mutable_stop_point();
                fill_pb_object<nt::Type_e::eStopPoint>(result_item.first, data, child, 2);
                item->set_name(data.pt_data.stop_points[result_item.first].name);
                item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_points[result_item.first]));
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
