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
        google::protobuf::Message* child = NULL;
        switch(type){
            case nt::eStopArea:
                child = item->mutable_stop_area();
                fill_pb_object<nt::eStopArea>(result_item.idx, data, child, 2);
                item->set_name(data.pt_data.stop_areas[result_item.idx].name);
                item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_areas[result_item.idx]));
                item->set_quality(result_item.quality);
                break;
            case nt::eCity:
                child = item->mutable_city();
                fill_pb_object<nt::eCity>(result_item.idx, data, child);
                item->set_name(data.pt_data.cities[result_item.idx].name);
                item->set_uri(nt::EntryPoint::get_uri(data.pt_data.cities[result_item.idx]));
                item->set_quality(result_item.quality);
                break;
            case nt::eStopPoint:
                child = item->mutable_stop_point();
                fill_pb_object<nt::eStopPoint>(result_item.idx, data, child, 2);
                item->set_name(data.pt_data.stop_points[result_item.idx].name);
                item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_points[result_item.idx]));
                item->set_quality(result_item.quality);
                break;
        case nt::eWay:
            child = item->mutable_way();
            fill_pb_object<nt::eWay>(result_item.idx, data, child, 2);
            item->set_name(data.street_network.ways[result_item.idx].name);
//            item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_points[result_item.idx]));
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
        case nt::eStopArea:
            result = d.pt_data.stop_area_first_letter.find_complete(name);
            break;
        case nt::eStopPoint:
            result = d.pt_data.stop_point_first_letter.find_complete(name);
            break;
        case nt::eCity:
            result = d.pt_data.city_first_letter.find_complete(name);
            break;
        case nt::eWay:
            result = d.street_network.fl.find_complete(name);
            break;
        default: break;
        }
        create_pb(result, type, d, *pb);
    }
    return pb_response;
}

}} //namespace navitia::firstletter
