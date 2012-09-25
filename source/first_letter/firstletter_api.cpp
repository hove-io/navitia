#include "firstletter_api.h"
#include "type/pb_converter.h"
#include <boost/foreach.hpp>

namespace navitia { namespace firstletter {
/**
 * se charge de remplir l'objet protocolbuffer firstletter passé en paramètre
 *
 */
void create_pb(const std::vector<FirstLetter<nt::idx_t>::fl_quality>& result, const nt::Type_e type, const nt::Data& data, pbnavitia::FirstLetter& pb_fl){
    BOOST_FOREACH(auto result_item, result){
        pbnavitia::FirstLetterItem* item = pb_fl.add_items();
        pbnavitia::PlaceMark* place_mark = item->mutable_object();
        switch(type){
        case nt::Type_e::eStopArea:
            place_mark->set_type(pbnavitia::STOPAREA);
            fill_pb_object<nt::Type_e::eStopArea>(result_item.idx, data, place_mark->mutable_stop_area(), 2);
            item->set_name(data.pt_data.stop_areas[result_item.idx].name);
            item->set_uri(data.pt_data.stop_areas[result_item.idx].external_code);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::eCity:
            place_mark->set_type(pbnavitia::CITY);
            fill_pb_object<nt::Type_e::eCity>(result_item.idx, data, place_mark->mutable_city());
            item->set_name(data.pt_data.cities[result_item.idx].name);
            item->set_uri(data.pt_data.cities[result_item.idx].external_code);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::eStopPoint:
            place_mark->set_type(pbnavitia::STOPPOINT);
            fill_pb_object<nt::Type_e::eStopPoint>(result_item.idx, data, place_mark->mutable_stop_point(), 2);
            item->set_name(data.pt_data.stop_points[result_item.idx].name);
            item->set_uri(data.pt_data.stop_points[result_item.idx].external_code);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::eWay:
            place_mark->set_type(pbnavitia::WAY);
            fill_pb_object<nt::Type_e::eWay>(result_item.idx, data, place_mark->mutable_way(), 2);
            item->set_name(data.street_network.ways[result_item.idx].name);
            item->set_quality(result_item.quality);
            break;

        default:
            break;
        }
    }
}

pbnavitia::Response firstletter(const std::string &name, const std::vector<nt::Type_e> &filter, const navitia::type::Data &d){
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::FIRSTLETTER);

    std::vector<FirstLetter<nt::idx_t>::fl_quality> result;
    pbnavitia::FirstLetter* pb = pb_response.mutable_firstletter();
    BOOST_FOREACH(nt::Type_e type, filter){
        switch(type){
        case nt::Type_e::eStopArea:
            result = d.pt_data.stop_area_first_letter.find_complete(name);
            break;
        case nt::Type_e::eStopPoint:
            result = d.pt_data.stop_point_first_letter.find_complete(name);
            break;
        case nt::Type_e::eCity:
            result = d.pt_data.city_first_letter.find_complete(name);
            break;
        case nt::Type_e::eWay:
            result = d.street_network.fl.find_complete(name);
            break;
        default: break;
        }
        create_pb(result, type, d, *pb);
    }
    return pb_response;
}

}} //namespace navitia::firstletter
