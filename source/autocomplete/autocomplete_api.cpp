#include "autocomplete_api.h"
#include "type/pb_converter.h"
#include <boost/foreach.hpp>

namespace navitia { namespace autocomplete {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
void create_pb(const std::vector<Autocomplete<nt::idx_t>::fl_quality>& result,
               const nt::Type_e type, uint32_t depth, const nt::Data& data,
               pbnavitia::Autocomplete& pb_fl){
    BOOST_FOREACH(auto result_item, result){
        pbnavitia::AutocompleteItem* item = pb_fl.add_items();
        pbnavitia::PlaceMark* place_mark = item->mutable_object();
        switch(type){
        case nt::Type_e::StopArea:
            place_mark->set_type(pbnavitia::STOP_AREA);
            fill_pb_object(result_item.idx, data, place_mark->mutable_stop_area(), depth);
            item->set_name(data.pt_data.stop_areas[result_item.idx].name);
            item->set_uri(data.pt_data.stop_areas[result_item.idx].uri);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::City:
            place_mark->set_type(pbnavitia::CITY);
            fill_pb_object(result_item.idx, data, place_mark->mutable_city(), depth);
            item->set_name(data.pt_data.cities[result_item.idx].name);
            item->set_uri(data.pt_data.cities[result_item.idx].uri);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::StopPoint:
            place_mark->set_type(pbnavitia::STOP_POINT);
            fill_pb_object(result_item.idx, data, place_mark->mutable_stop_point(), depth);
            item->set_name(data.pt_data.stop_points[result_item.idx].name);
            item->set_uri(data.pt_data.stop_points[result_item.idx].uri);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::Address:
            place_mark->set_type(pbnavitia::ADDRESS);
            //fill_pb_object(result_item.idx, data, place_mark->mutable_way(), 2);
            fill_pb_object(result_item.idx, data, place_mark->mutable_address(), result_item.house_number,result_item.coord, depth);
            item->set_name(data.geo_ref.ways[result_item.idx].name);
            item->set_uri(data.geo_ref.ways[result_item.idx].uri+":"+boost::lexical_cast<std::string>(result_item.house_number));
            item->set_quality(result_item.quality);
            break;

        default:
            break;
        }
    }
}

pbnavitia::Response autocomplete(const std::string &name,
                                 const std::vector<nt::Type_e> &filter,
                                 uint32_t depth,
                                 const navitia::type::Data &d){

    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::AUTOCOMPLETE);

    std::vector<Autocomplete<nt::idx_t>::fl_quality> result;
    pbnavitia::Autocomplete* pb = pb_response.mutable_autocomplete();
    BOOST_FOREACH(nt::Type_e type, filter){
        switch(type){
        case nt::Type_e::StopArea:
            result = d.pt_data.stop_area_autocomplete.find_complete(name);
            break;
        case nt::Type_e::StopPoint:
            result = d.pt_data.stop_point_autocomplete.find_complete(name);
            break;
        case nt::Type_e::City:
            result = d.pt_data.city_autocomplete.find_complete(name);
            break;
        case nt::Type_e::Address:
            //result = d.geo_ref.fl.find_complete(name);
            result = d.geo_ref.find_ways(name);
            break;
        default: break;
        }
        create_pb(result, type, depth, d, *pb);
    }
    return pb_response;
}

}} //namespace navitia::autocomplete
